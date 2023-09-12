/**
 * @file: web.cpp
 * @date: 2022-11-14
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
#include <client/web.hpp>

/**
 * debugHttp2 Функция обратного вызова при получении отладочной информации
 * @param format формат вывода отладочной информации
 * @param args   список аргументов отладочной информации
 */
void awh::client::WEB::debugHttp2(const char * format, va_list args) noexcept {
	// Буфер данных для логирования
	string buffer(1024, '\0');
	// Выполняем перебор всех аргументов
	while(true){
		// Создаем список аргументов
		va_list args2;
		// Копируем список аргументов
		va_copy(args2, args);
		// Выполняем запись в буфер данных
		size_t res = vsnprintf(buffer.data(), buffer.size(), format, args2);
		// Если результат получен
		if((res >= 0) && (res < buffer.size())){
			// Завершаем список аргументов
			va_end(args);
			// Завершаем список локальных аргументов
			va_end(args2);
			// Если результат не получен
			if(res == 0)
				// Выполняем сброс результата
				buffer.clear();
			// Выводим результат
			else buffer.assign(buffer.begin(), buffer.begin() + res);
			// Выходим из цикла
			break;
		}
		// Размер буфера данных
		size_t size = 0;
		// Если данные не получены, увеличиваем буфер в два раза
		if(res < 0)
			// Увеличиваем размер буфера в два раза
			size = (buffer.size() * 2);
		// Увеличиваем размер буфера на один байт
		else size = (res + 1);
		// Очищаем буфер данных
		buffer.clear();
		// Выделяем память для буфера
		buffer.resize(size);
		// Завершаем список локальных аргументов
		va_end(args2);
	}
	// Выводим отладочную информацию
	cout << " \x1B[36m\x1B[1mDebug:\x1B[0m " << buffer << endl;
}
/**
 * onFrameHttp2 Функция обратного вызова при получении фрейма заголовков HTTP/2 с сервера
 * @param session объект сессии HTTP/2
 * @param frame   объект фрейма заголовков HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::WEB::onFrameHttp2(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) session;
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Если сессия клиента совпадает с сессией полученных даных
	if(web->_http2.id == frame->hd.stream_id){
		// Выполняем определение типа фрейма
		switch(frame->hd.type){
			// Если мы получили входящие данные тела ответа
			case NGHTTP2_DATA: {
				// Определяем протокол клиента
				switch(static_cast <uint8_t> (web->_agent)){
					// Если агент является клиентом HTTP
					case static_cast <uint8_t> (agent_t::HTTP): {
						// Если мы получили флаг завершения потока
						if(frame->hd.flags & NGHTTP2_FLAG_END_STREAM){
							// Выполняем коммит полученного результата
							web->_web.http.commit();
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Если тело ответа существует
									if(!web->_web.http.body().empty())
										// Выводим сообщение о выводе чанка тела
										cout << web->_fmk->format("<body %u>", web->_web.http.body().size()) << endl << endl;
									// Иначе устанавливаем перенос строки
									else cout << endl;
								}
							#endif
							// Выполняем препарирование полученных данных
							switch(static_cast <uint8_t> (web->prepare())){
								// Если необходимо выполнить пропуск обработки данных
								case static_cast <uint8_t> (status_t::SKIP):
									// Завершаем работу
									return 0;
							}
							// Если функция обратного вызова установлена, выводим сообщение
							if(web->_web.callback.is("entity"))
								// Выполняем функцию обратного вызова дисконнекта
								web->_web.callback.bind <const u_int, const string, const vector <char>> ("entity");
							// Если подключение выполнено и список запросов не пустой
							if((web->_aid > 0) && !web->_web.requests.empty())
								// Выполняем запрос на удалённый сервер
								web->submit(web->_web.requests.front());
						}
					} break;
					// Если агент является клиентом WebSocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET): {
						// Если рукопожатие не выполнено
						if(!reinterpret_cast <http_t *> (&web->_ws.http)->isHandshake()){
							// Если мы получили флаг завершения потока
							if(frame->hd.flags & NGHTTP2_FLAG_END_STREAM){
								// Получаем объект HTTP-ответа
								http_t * http = reinterpret_cast <http_t *> (&web->_ws.http);
								// Выполняем коммит полученного результата
								http->commit();
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									{
										// Если тело ответа существует
										if(!http->body().empty())
											// Выводим сообщение о выводе чанка тела
											cout << web->_fmk->format("<body %u>", http->body().size()) << endl << endl;
										// Иначе устанавливаем перенос строки
										else cout << endl;
									}
								#endif
								// Выполняем анализ результата авторизации
								switch(static_cast <uint8_t> (http->getAuth())){
									// Если нужно попытаться ещё раз
									case static_cast <uint8_t> (awh::http_t::stath_t::RETRY): {
										// Если попытки повторить переадресацию закончились
										if((web->_stopped = (web->_attempt >= web->_attempts)))
											// Завершаем работу
											const_cast <client::core_t *> (web->_core)->close(web->_aid);
									} break;
									// Если запрос неудачный
									case static_cast <uint8_t> (awh::http_t::stath_t::FAULT):
										// Завершаем работу
										const_cast <client::core_t *> (web->_core)->close(web->_aid);
									break;
								}
								// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
								if(web->_callback.is("entity")){
									// Получаем параметры запроса
									const auto & query = http->query();
									// Выводим функцию обратного вызова
									web->_callback.call <const u_int, const string &, const vector <char> &> ("entity", query.code, query.message, http->body());
								}
							}
						// Если рукопожатие выполнено
						} else if(web->_ws.allow.receive) {
							// Если мы получили неустановленный флаг или флаг завершения потока
							if((frame->hd.flags & NGHTTP2_FLAG_NONE) || (frame->hd.flags & NGHTTP2_FLAG_END_STREAM)){
								// Выполняем препарирование полученных данных
								switch(static_cast <uint8_t> (web->prepare())){
									// Если необходимо выполнить остановку обработки
									case static_cast <uint8_t> (status_t::STOP):
										// Выходим из функции
										return 0;
									// Если необходимо выполнить переход к следующему этапу обработки
									case static_cast <uint8_t> (status_t::NEXT):
										// Выполняем отправку сообщения об ошибке
										web->sendError(web->_ws.mess);
									break;
								}
							}
						}
					} break;
				}
			} break;
			// Если мы получили входящие данные заголовков ответа
			case NGHTTP2_HEADERS: {
				// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
				if(frame->hd.flags & NGHTTP2_FLAG_END_HEADERS){
					// Определяем протокол клиента
					switch(static_cast <uint8_t> (web->_agent)){
						// Если агент является клиентом HTTP
						case static_cast <uint8_t> (agent_t::HTTP): {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Получаем данные ответа
									const auto & response = web->_web.http.response(true);
									// Если параметры ответа получены
									if(!response.empty())
										// Выводим параметры ответа
										cout << string(response.begin(), response.end()) << endl;
								}
							#endif
							// Получаем параметры запроса
							const auto & query = web->_web.http.query();
							// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
							if(web->_callback.is("response"))
								// Выводим функцию обратного вызова
								web->_callback.call <const u_int, const string &> ("response", query.code, query.message);
							// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
							if(web->_callback.is("headers"))
								// Выводим функцию обратного вызова
								web->_callback.call <const u_int, const string &, const unordered_multimap <string, string> &> ("headers", query.code, query.message, web->_web.http.headers());
						} break;
						// Если агент является клиентом WebSocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET): {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Получаем данные ответа
									const auto & response = reinterpret_cast <http_t *> (&web->_ws.http)->response(true);
									// Если параметры ответа получены
									if(!response.empty())
										// Выводим параметры ответа
										cout << string(response.begin(), response.end()) << endl;
								}
							#endif
							// Получаем параметры запроса
							const auto & query = web->_ws.http.query();
							// Получаем объект биндинга ядра TCP/IP
							client::core_t * core = const_cast <client::core_t *> (web->_core);
							// Выполняем препарирование полученных данных
							switch(static_cast <uint8_t> (web->prepare())){
								// Если необходимо выполнить остановку обработки
								case static_cast <uint8_t> (status_t::STOP): {
									// Выполняем сброс количества попыток
									web->_attempt = 0;
									// Завершаем работу
									core->close(web->_aid);
								}
								// Если необходимо выполнить переход к следующему этапу обработки
								case static_cast <uint8_t> (status_t::NEXT): {
									// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
									if(web->_callback.is("response"))
										// Выводим функцию обратного вызова
										web->_callback.call <const u_int, const string &> ("response", query.code, query.message);
									// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
									if(web->_callback.is("headers"))
										// Выводим функцию обратного вызова
										web->_callback.call <const u_int, const string &, const unordered_multimap <string, string> &> ("headers", query.code, query.message, web->_ws.http.headers());
									// Очищаем буфер собранных данных
									web->_ws.buffer.payload.clear();
									// Завершаем работу
									return 0;
								}
								// Если необходимо выполнить пропуск обработки данных
								case static_cast <uint8_t> (status_t::SKIP): {
									// Если соединение является постоянным
									if(web->_ws.http.isAlive())
										// Выполняем открытие текущего подключения
										web->actionConnect();
									// Если нам необходимо отключиться
									else core->close(web->_aid);
									// Завершаем работу
									return 0;
								}
							}
							// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
							if(web->_callback.is("response"))
								// Выводим функцию обратного вызова
								web->_callback.call <const u_int, const string &> ("response", query.code, query.message);
							// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
							if(web->_callback.is("headers"))
								// Выводим функцию обратного вызова
								web->_callback.call <const u_int, const string &, const unordered_multimap <string, string> &> ("headers", query.code, query.message, web->_web.http.headers());
						} break;
					}
				}
			} break;
		}
	}
	// Выводим результат
	return 0;
}
/**
 * onCloseHttp2 Метод закрытия подключения с сервером HTTP/2
 * @param session объект сессии HTTP/2
 * @param sid     идентификатор сессии HTTP/2
 * @param error   флаг ошибки HTTP/2 если присутствует
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученного события
 */
int awh::client::WEB::onCloseHttp2(nghttp2_session * session, const int32_t sid, const uint32_t error, void * ctx) noexcept {
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Если идентификатор сессии клиента совпадает
	if(web->_http2.id == sid){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим заголовок ответа
			cout << "\x1B[33m\x1B[1m^^^^^^^^^ CLOSE SESSION HTTP2 ^^^^^^^^^\x1B[0m" << endl;
			// Определяем тип получаемой ошибки
			switch(error){
				// Если ошибка не получена
				case 0x0:
					// Выводим информацию о закрытии сессии
					cout << web->_fmk->format("Stream %d closed", sid) << endl << endl;
				break;
				// Если получена ошибка протокола
				case 0x1:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "PROTOCOL_ERROR") << endl << endl;
				break;
				// Если получена ошибка реализации
				case 0x2:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "INTERNAL_ERROR") << endl << endl;
				break;
				// Если получена ошибка превышения предела управления потоком
				case 0x3:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "FLOW_CONTROL_ERROR") << endl << endl;
				break;
				// Если установка не подтверждённа
				case 0x4:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "SETTINGS_TIMEOUT") << endl << endl;
				break;
				// Если получен кадр для завершения потока
				case 0x5:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "STREAM_CLOSED") << endl << endl;
				break;
				// Если размер кадра некорректен
				case 0x6:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "FRAME_SIZE_ERROR") << endl << endl;
				break;
				// Если поток не обработан
				case 0x7:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "REFUSED_STREAM") << endl << endl;
				break;
				// Если поток аннулирован
				case 0x8:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "CANCEL") << endl << endl;
				break;
				// Если состояние компрессии не обновлено
				case 0x9:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "COMPRESSION_ERROR") << endl << endl;
				break;
				// Если получена ошибка TCP-соединения для метода CONNECT
				case 0xa:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "CONNECT_ERROR") << endl << endl;
				break;
				// Если превышена емкость для обработки
				case 0xb:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "ENHANCE_YOUR_CALM") << endl << endl;
				break;
				// Если согласованные параметры TLS не приемлемы
				case 0xc:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "INADEQUATE_SECURITY") << endl << endl;
				break;
				// Если для запроса используется HTTP/1.1
				case 0xd:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "HTTP_1_1_REQUIRED") << endl << endl;
				break;
			}
		#endif
		// Отключаем флаг HTTP/2 так-как сессия уже закрыта
		web->_http2.mode = false;
		// Если сессия HTTP/2 закрыта не удачно
		if(nghttp2_session_terminate_session(session, NGHTTP2_NO_ERROR) != 0)
			// Выводим сообщение об ошибке
			return NGHTTP2_ERR_CALLBACK_FAILURE;
	}
	// Выводим результат
	return 0;
}
/**
 * onChunkHttp2 Функция обратного вызова при получении чанка с сервера HTTP/2
 * @param session объект сессии HTTP/2
 * @param flags   флаги события для сессии HTTP/2
 * @param sid     идентификатор сессии HTTP/2
 * @param buffer  буфер данных который содержит полученный чанк
 * @param size    размер полученного буфера данных чанка
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::WEB::onChunkHttp2(nghttp2_session * session, const uint8_t flags, const int32_t sid, const uint8_t * buffer, const size_t size, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Если идентификатор сессии клиента совпадает
	if(web->_http2.id == sid){
		// Если функция обратного вызова на перехват входящих чанков установлена
		if(web->_callback.is("chunking"))
			// Выводим функцию обратного вызова
			web->_callback.call <const vector <char> &, const awh::http_t *> ("chunking", vector <char> (buffer, buffer + size), &web->_web.http);
		// Если функция перехвата полученных чанков не установлена
		else {
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (web->_agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP):
					// Добавляем полученный чанк в тело данных
					web->_web.http.body(vector <char> (buffer, buffer + size));
				break;
				// Если агент является клиентом WebSocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET): {
					// Если подключение закрыто
					if(web->_ws.close){
						// Принудительно выполняем отключение лкиента
						reinterpret_cast <client::core_t *> (const_cast <core_t *> (web->_core))->close(web->_aid);
						// Выходим из функции
						return 0;
					}
					// Если рукопожатие не выполнено
					if(!reinterpret_cast <http_t *> (&web->_ws.http)->isHandshake())
						// Добавляем полученный чанк в тело данных
						web->_ws.http.body(vector <char> (buffer, buffer + size));
					// Если рукопожатие выполнено
					else if(web->_ws.allow.receive)
						// Добавляем полученные данные в буфер
						web->_ws.buffer.payload.insert(web->_ws.buffer.payload.end(), buffer, buffer + size);
				} break;
			}
		}
	}
	// Выводим результат
	return 0;
}
/**
 * onBeginHeadersHttp2 Функция начала получения фрейма заголовков HTTP/2
 * @param session объект сессии HTTP/2
 * @param frame   объект фрейма заголовков HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::WEB::onBeginHeadersHttp2(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) session;
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Выполняем определение типа фрейма
	switch(frame->hd.type){
		// Если мы получили входящие данные заголовков ответа
		case NGHTTP2_HEADERS:{
			// Если сессия клиента совпадает с сессией полученных даных
			if((frame->headers.cat == NGHTTP2_HCAT_RESPONSE) && (web->_http2.id == frame->hd.stream_id)){
				// Определяем протокол клиента
				switch(static_cast <uint8_t> (web->_agent)){
					// Если агент является клиентом HTTP
					case static_cast <uint8_t> (agent_t::HTTP):
						// Выполняем очистку параметров HTTP запроса
						web->_web.http.clear();
					break;
					// Если агент является клиентом WebSocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем очистку параметров HTTP запроса
						web->_ws.http.clear();
					break;
				}
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим информацию об ошибке
					cout << web->_fmk->format("Stream ID=%d", frame->hd.stream_id) << endl << endl;
				#endif
			}
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * onHeaderHttp2 Функция обратного вызова при получении заголовка HTTP/2
 * @param session объект сессии HTTP/2
 * @param frame   объект фрейма заголовков HTTP/2
 * @param key     данные ключа заголовка
 * @param keySize размер ключа заголовка
 * @param val     данные значения заголовка
 * @param valSize размер значения заголовка
 * @param flags   флаги события для сессии HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::WEB::onHeaderHttp2(nghttp2_session * session, const nghttp2_frame * frame, const uint8_t * key, const size_t keySize, const uint8_t * val, const size_t valSize, const uint8_t flags, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Выполняем определение типа фрейма
	switch(frame->hd.type){
		// Если мы получили входящие данные заголовков ответа
		case NGHTTP2_HEADERS:{
			// Если сессия клиента совпадает с сессией полученных даных
			if((frame->headers.cat == NGHTTP2_HCAT_RESPONSE) && (web->_http2.id == frame->hd.stream_id)){
				// Полкючаем ключ заголовка
				const string keyHeader((const char *) key, keySize);
				// Получаем значение заголовка
				const string valHeader((const char *) val, valSize);
				// Определяем протокол клиента
				switch(static_cast <uint8_t> (web->_agent)){
					// Если агент является клиентом HTTP
					case static_cast <uint8_t> (agent_t::HTTP):
						// Устанавливаем полученные заголовки
						web->_web.http.header2(keyHeader, valHeader);
					break;
					// Если агент является клиентом WebSocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Устанавливаем полученные заголовки
						web->_ws.http.header2(keyHeader, valHeader);
					break;
				}
				// Если функция обратного вызова на полученного заголовка с сервера установлена
				if(web->_callback.is("header"))
					// Выводим функцию обратного вызова
					web->_callback.call <const string &, const string &> ("header", keyHeader, valHeader);
			}
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * sendHttp2 Функция обратного вызова при подготовки данных для отправки на сервер
 * @param session объект сессии HTTP/2
 * @param buffer  буфер данных которые следует отправить
 * @param size    размер буфера данных для отправки
 * @param flags   флаги события для сессии HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        количество отправленных байт
 */
ssize_t awh::client::WEB::sendHttp2(nghttp2_session * session, const uint8_t * buffer, const size_t size, const int flags, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Выполняем отправку заголовков запроса на сервер
	const_cast <client::core_t *> (web->_core)->write((const char *) buffer, size, web->_aid);
	// Возвращаем количество отправленных байт
	return static_cast <ssize_t> (size);
}
/**
 * readHttp2 Функция чтения подготовленных данных для формирования буфера данных который необходимо отправить на HTTP/2 сервер
 * @param session объект сессии HTTP/2
 * @param sid     идентификатор сессии HTTP/2
 * @param buffer  буфер данных которые следует отправить
 * @param size    размер буфера данных для отправки
 * @param flags   флаги события для сессии HTTP/2
 * @param source  объект промежуточных данных локального подключения
 * @param ctx     передаваемый промежуточный контекст
 * @return        количество отправленных байт
 */
ssize_t awh::client::WEB::readHttp2(nghttp2_session * session, const int32_t sid, uint8_t * buffer, const size_t size, uint32_t * flags, nghttp2_data_source * source, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) sid;
	(void) ctx;
	(void) session;
	// Результат работы функции
	ssize_t result = -1;
	/**
	 * Методы только для OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Выполняем чтение данных из сокета в буфер данных
		while(((result = _read(source->fd, buffer, size)) == -1) && (errno == EINTR));
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Выполняем чтение данных из сокета в буфер данных
		while(((result = ::read(source->fd, buffer, size)) == -1) && (errno == EINTR));
	#endif
	// Если данные не прочитанны из сокета
	if(result < 0)
		// Выводим сообщение об ошибке
		return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
	// Если все данные прочитаны полностью
	else if(result == 0) {
		/**
		 * Методы только для OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем закрытие подключения
			_close(source->fd);
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			// Выполняем закрытие подключения
			::close(source->fd);
		#endif
		// Устанавливаем флаг, завершения чтения данных
		(* flags) |= NGHTTP2_DATA_FLAG_EOF;
	}
	// Выводим количество прочитанных байт
	return result;
}
/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::client::WEB::chunking(const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()){
		// Выполняем добавление полученного чанка в тело ответа
		const_cast <awh::http_t *> (http)->body(chunk);
		// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
		if(this->_callback.is("chunks"))
			// Выводим функцию обратного вызова
			this->_callback.call <const vector <char> &> ("chunks", chunk);
	}
}
/**
 * ping2 Метод выполнения пинга сервера
 * @return результат работы пинга
 */
bool awh::client::WEB::ping2() noexcept {
	// Если протокол подключения установлен как HTTP/2
	if(this->_http2.mode && (this->_http2.ctx != nullptr)){
		// Результат выполнения поерации
		int rv = -1;
		// Выполняем пинг удалённого сервера
		if((rv = nghttp2_submit_ping(this->_http2.ctx, 0, nullptr)) != 0){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Выходим из функции
			return false;
		}
		// Фиксируем отправленный результат
		if((rv = nghttp2_session_send(this->_http2.ctx)) != 0){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Выходим из функции
			return false;
		}
		// Выводим результат
		return true;
	}
	// Выводим результат
	return false;
}
/**
 * ping Метод проверки доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WEB::ping(const string & message) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_ws.allow.send){
		// Если рукопожатие выполнено
		if(this->_ws.http.isHandshake() && (this->_aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->_ws.frame.methods.ping(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				const_cast <client::core_t *> (this->_core)->write(buffer.data(), buffer.size(), this->_aid);
		}
	}
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WEB::pong(const string & message) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_ws.allow.send){
		// Если рукопожатие выполнено
		if(this->_ws.http.isHandshake() && (this->_aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->_ws.frame.methods.pong(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				const_cast <client::core_t *> (this->_core)->write(buffer.data(), buffer.size(), this->_aid);
		}
	}
}
/** 
 * submit Метод выполнения удалённого запроса на сервер
 * @param request объект запроса на удалённый сервер
 */
void awh::client::WEB::submit(const req_t & request) noexcept {
	// Если подключение выполнено
	if(this->_aid > 0){
		// Выполняем сброс состояния HTTP парсера
		this->_web.http.reset();
		// Выполняем очистку параметров HTTP запроса
		this->_web.http.clear();
		// Выполняем очистку функций обратного вызова
		this->_web.callback.clear();
		// Устанавливаем метод компрессии
		this->_web.http.compress(this->_compress);
		// Если список заголовков получен
		if(!request.headers.empty())
			// Устанавливаем заголовоки запроса
			this->_web.http.headers(request.headers);
		// Если тело запроса существует
		if(!request.entity.empty())
			// Устанавливаем тело запроса
			this->_web.http.body(request.entity);
		// Получаем URL ссылку для выполнения запроса
		this->_web.uri.append(this->_scheme.url, request.query);
		// Получаем объект биндинга ядра TCP/IP
		client::core_t * core = const_cast <client::core_t *> (this->_core);
		// Если активирован режим работы с HTTP/2 протоколом
		if(this->_http2.mode){
			// Список заголовков для запроса
			vector <nghttp2_nv> nva;
			// Выполняем установку параметры ответа сервера
			this->_web.http.query(awh::web_t::query_t(2.0f, request.method));
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим заголовок запроса
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
				// Получаем бинарные данные WEB запроса
				const auto & buffer = this->_web.http.request(this->_scheme.url, request.method);
				// Выводим параметры запроса
				cout << string(buffer.begin(), buffer.end()) << endl;
			#endif
			// Выполняем запрос на получение заголовков
			const auto & headers = this->_web.http.request2(this->_scheme.url, request.method);
			// Выполняем перебор всех заголовков HTTP/2 запроса
			for(auto & header : headers){
				// Выполняем добавление метода запроса
				nva.push_back({
					(uint8_t *) header.first.c_str(),
					(uint8_t *) header.second.c_str(),
					header.first.size(),
					header.second.size(),
					NGHTTP2_NV_FLAG_NONE
				});
			}
			// Если тело запроса существует
			if(!request.entity.empty()){
				// Список файловых дескрипторов
				int fds[2];
				// Тело WEB сообщения
				vector <char> entity;
				/**
				 * Методы только для OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
					const int rv = _pipe(fds, 4096, O_BINARY);
				/**
				 * Для всех остальных операционных систем
				 */
				#else
					// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
					const int rv = ::pipe(fds);
				#endif
				// Выполняем подписку на основной канал передачи данных
				if(rv != 0){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					// Выполняем закрытие подключения
					core->close(this->_aid);
					// Выходим из функции
					return;
				}
				// Получаем данные тела запроса
				while(!(entity = this->_web.http.payload()).empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка тела
						cout << this->_fmk->format("<chunk %u>", entity.size()) << endl << endl;
					#endif
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Если данные небыли записаны в сокет
						if(static_cast <int> (_write(fds[1], entity.data(), entity.size())) != static_cast <int> (entity.size())){
							// Выполняем закрытие сокета для чтения
							_close(fds[0]);
							// Выполняем закрытие сокета для записи
							_close(fds[1]);
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выполняем закрытие подключения
							core->close(this->_aid);
							// Выходим из функции
							return;
						}
					/**
					 * Для всех остальных операционных систем
					 */
					#else
						// Если данные небыли записаны в сокет
						if(static_cast <int> (::write(fds[1], entity.data(), entity.size())) != static_cast <int> (entity.size())){
							// Выполняем закрытие сокета для чтения
							::close(fds[0]);
							// Выполняем закрытие сокета для записи
							::close(fds[1]);
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выполняем закрытие подключения
							core->close(this->_aid);
							// Выходим из функции
							return;
						}
					#endif
				}
				/**
				 * Методы только для OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем закрытие подключения
					_close(fds[1]);
				/**
				 * Для всех остальных операционных систем
				 */
				#else
					// Выполняем закрытие подключения
					::close(fds[1]);
				#endif
				// Создаём объект передачи данных тела полезной нагрузки
				nghttp2_data_provider data;
				// Зануляем передаваемый контекст
				data.source.ptr = nullptr;
				// Устанавливаем файловый дескриптор
				data.source.fd = fds[0];
				// Устанавливаем функцию обратного вызова
				data.read_callback = &web_t::readHttp2;
				// Выполняем запрос на удалённый сервер
				this->_http2.id = nghttp2_submit_request(this->_http2.ctx, nullptr, nva.data(), nva.size(), &data, this);
			// Если тело запроса не существует, выполняем установленный запрос
			} else this->_http2.id = nghttp2_submit_request(this->_http2.ctx, nullptr, nva.data(), nva.size(), nullptr, this);
			// Если запрос не получилось отправить
			if(this->_http2.id < 0){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(this->_http2.id));
				// Выполняем закрытие подключения
				core->close(this->_aid);
				// Выходим из функции
				return;
			}{
				// Результат фиксации сессии
				int rv = -1;
				// Фиксируем отправленный результат
				if((rv = nghttp2_session_send(this->_http2.ctx)) != 0){
					// Выводим сообщение об полученной ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
					// Выходим из функции
					return;
				}
			}
		// Если активирован режим работы с HTTP/1.1 протоколом
		} else {
			// Получаем бинарные данные WEB запроса
			const auto & buffer = this->_web.http.request(this->_scheme.url, request.method);
			// Если бинарные данные запроса получены
			if(!buffer.empty()){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок запроса
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры запроса
					cout << string(buffer.begin(), buffer.end()) << endl << endl;
				#endif
				// Тело WEB сообщения
				vector <char> entity;
				// Выполняем отправку заголовков запроса на сервер
				core->write(buffer.data(), buffer.size(), this->_aid);
				// Получаем данные тела запроса
				while(!(entity = this->_web.http.payload()).empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка тела
						cout << this->_fmk->format("<chunk %u>", entity.size()) << endl;
					#endif
					// Выполняем отправку тела запроса на сервер
					core->write(entity.data(), entity.size(), this->_aid);
				}
			}	
		}
	}
}
/**
 * openCallback Метод обратного вызова при запуске работы
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WEB::openCallback(const size_t sid, awh::core_t * core) noexcept {
	// Если дисконнекта ещё не произошло
	if(this->_action == action_t::NONE){
		// Устанавливаем экшен выполнения
		this->_action = action_t::OPEN;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::client::WEB::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Если система была остановлена
		if(status == awh::core_t::status_t::STOP){
			// Если контекст сессии HTTP/2 создан
			if(this->_http2.mode && (this->_http2.ctx != nullptr))
				// Выполняем удаление сессии
				nghttp2_session_del(this->_http2.ctx);
			// Деактивируем флаг работы с протоколом HTTP/2
			this->_http2.mode = false;
		}
		// Если функция получения событий запуска и остановки сетевого ядра установлена
		if(this->_callback.is("events"))
			// Выводим функцию обратного вызова
			this->_callback.call <const awh::core_t::status_t, awh::core_t *> ("events", status, core);
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WEB::persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Определяем протокол клиента
		switch(static_cast <uint8_t> (this->_agent)){
			// Если агент является клиентом HTTP
			case static_cast <uint8_t> (agent_t::HTTP): {
				// Если сервер уже отключился
				if(!this->ping2())
					// Завершаем работу
					reinterpret_cast <client::core_t *> (core)->close(aid);
			} break;
			// Если агент является клиентом WebSocket
			case static_cast <uint8_t> (agent_t::WEBSOCKET): {
				// Получаем текущий штамп времени
				const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
				// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
				if(this->_ws.close || ((stamp - this->_ws.point) >= (PERSIST_INTERVAL * 5)))
					// Завершаем работу
					reinterpret_cast <client::core_t *> (core)->close(aid);
				// Отправляем запрос адъютанту
				else this->ping(to_string(aid));
			} break;
		}
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WEB::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Устанавливаем экшен выполнения
		this->_action = action_t::CONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WEB::disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((sid > 0) && (core != nullptr)){
		// Устанавливаем экшен выполнения
		this->_action = action_t::DISCONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::WEB::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Если протокол подключения является HTTP/2
		if(core->proto(aid) == engine_t::proto_t::HTTP2){
			// Если агент является клиентом WebSocket
			if(this->_agent == agent_t::WEBSOCKET){
				// Если подключение закрыто
				if(this->_ws.close){
					// Принудительно выполняем отключение лкиента
					reinterpret_cast <client::core_t *> (core)->close(aid);
					// Выходим из функции
					return;
				}
				// Если получение данных не разрешено
				if(!this->_ws.allow.receive)
					// Выходим из функции
					return;
			}
			// Выполняем извлечение полученного чанка данных из сокета
			ssize_t bytes = nghttp2_session_mem_recv(this->_http2.ctx, (const uint8_t *) buffer, size);
			// Если данные не прочитаны, выводим ошибку и выходим
			if(bytes < 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
				// Выходим из функции
				return;
			}
			// Фиксируем полученный результат
			if((bytes = nghttp2_session_send(this->_http2.ctx)) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
				// Выходим из функции
				return;
			}
		// Если активирован режим работы с HTTP/1.1 протоколом
		} else {
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (this->_agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP): {
					// Если дисконнекта ещё не произошло
					if((this->_action == action_t::NONE) || (this->_action == action_t::READ)){
						// Устанавливаем экшен выполнения
						this->_action = action_t::READ;
						// Добавляем полученные данные в буфер
						this->_web.buffer.insert(this->_web.buffer.end(), buffer, buffer + size);
						// Выполняем запуск обработчика событий
						this->handler();
					}
				} break;
				// Если агент является клиентом WebSocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET): {
					// Если подключение закрыто
					if(this->_ws.close){
						// Принудительно выполняем отключение лкиента
						reinterpret_cast <client::core_t *> (core)->close(aid);
						// Выходим из функции
						return;
					}
					// Если разрешено получение данных
					if(this->_ws.allow.receive){
						// Устанавливаем экшен выполнения
						this->_action = action_t::READ;
						// Добавляем полученные данные в буфер
						this->_ws.buffer.payload.insert(this->_ws.buffer.payload.end(), buffer, buffer + size);
						// Выполняем запуск обработчика событий
						this->handler();
					}
				} break;
			}
		}
	}
}
/**
 * writeCallback Метод обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::WEB::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Если агент является клиентом WebSocket
		if(this->_agent == agent_t::WEBSOCKET){
			// Если необходимо выполнить закрыть подключение
			if(!this->_ws.close && this->_stopped){
				// Устанавливаем флаг закрытия подключения
				this->_ws.close = !this->_ws.close;
				// Принудительно выполняем отключение лкиента
				const_cast <client::core_t *> (this->_core)->close(aid);
			}
		}
	}
}
/**
 * enableTLSCallback Метод активации зашифрованного канала TLS
 * @param url  адрес сервера для которого выполняется активация зашифрованного канала TLS
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат активации зашифрованного канала TLS
 */
bool awh::client::WEB::enableTLSCallback(const uri_t::url_t & url, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Блокируем переменные которые не используем
	(void) aid;
	(void) sid;
	(void) core;
	// Определяем протокол клиента
	switch(static_cast <uint8_t> (this->_agent)){
		// Если агент является клиентом HTTP
		case static_cast <uint8_t> (agent_t::HTTP):
			// Выводим результат активации
			return (!url.empty() && this->_fmk->compare(url.schema, "https"));
		// Если агент является клиентом WebSocket
		case static_cast <uint8_t> (agent_t::WEBSOCKET):
			// Выводим результат активации
			return (!url.empty() && this->_fmk->compare(url.schema, "wss"));
	}
	// Сообщаем, что подключение производится в незащищённом режиме
	return false;
}
/**
 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WEB::proxyConnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Устанавливаем экшен выполнения
		this->_action = action_t::PROXY_CONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * proxyReadCallback Метод обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::WEB::proxyReadCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Если дисконнекта ещё не произошло
		if((this->_action == action_t::NONE) || (this->_action == action_t::PROXY_READ)){
			// Устанавливаем экшен выполнения
			this->_action = action_t::PROXY_READ;
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (this->_agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP):
					// Добавляем полученные данные в буфер
					this->_web.buffer.insert(this->_web.buffer.end(), buffer, buffer + size);
				break;
				// Если агент является клиентом WebSocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET):
					// Добавляем полученные данные в буфер
					this->_ws.buffer.payload.insert(this->_ws.buffer.payload.end(), buffer, buffer + size);
				break;
			}
			// Выполняем запуск обработчика событий
			this->handler();
		}
	}
}
/**
 * handler Метод управления входящими методами
 */
void awh::client::WEB::handler() noexcept {
	// Если управляющий блокировщик не заблокирован
	if(!this->_locker.mode){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_locker.mtx);
		// Флаг разрешающий циклический перебор экшенов
		bool loop = true;
		// Выполняем блокировку обработчика
		this->_locker.mode = true;
		// Выполняем обработку всех экшенов
		while(loop && (this->_action != action_t::NONE)){
			// Определяем обрабатываемый экшен
			switch(static_cast <uint8_t> (this->_action)){
				// Если необходимо запустить экшен открытия подключения
				case static_cast <uint8_t> (action_t::OPEN): this->actionOpen(); break;
				// Если необходимо запустить экшен обработки данных поступающих с сервера
				case static_cast <uint8_t> (action_t::READ): this->actionRead(); break;
				// Если необходимо запустить экшен обработки подключения к серверу
				case static_cast <uint8_t> (action_t::CONNECT): this->actionConnect(); break;
				// Если необходимо запустить экшен обработки отключения от сервера
				case static_cast <uint8_t> (action_t::DISCONNECT): this->actionDisconnect(); break;
				// Если необходимо запустить экшен обработки данных поступающих с прокси-сервера
				case static_cast <uint8_t> (action_t::PROXY_READ): this->actionProxyRead(); break;
				// Если необходимо запустить экшен обработки подключения к прокси-серверу
				case static_cast <uint8_t> (action_t::PROXY_CONNECT): this->actionProxyConnect(); break;
				// Если сработал неизвестный экшен, выходим
				default: loop = false;
			}
		}
		// Выполняем разблокировку обработчика
		this->_locker.mode = false;
	}
}
/**
 * actionOpen Метод обработки экшена открытия подключения
 */
void awh::client::WEB::actionOpen() noexcept {
	// Выполняем подключение
	this->open();
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::OPEN)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
}
/**
 * actionRead Метод обработки экшена чтения с сервера
 */
void awh::client::WEB::actionRead() noexcept {
	// Определяем протокол клиента
	switch(static_cast <uint8_t> (this->_agent)){
		// Если агент является клиентом HTTP
		case static_cast <uint8_t> (agent_t::HTTP): {
			// Флаг удачного получения данных
			bool receive = false;
			// Флаг завершения работы
			bool completed = false;
			// Выполняем обработку полученных данных
			while(!this->_active){
				// Выполняем парсинг полученных данных
				size_t bytes = this->_web.http.parse(this->_web.buffer.data(), this->_web.buffer.size());
				// Если все данные получены
				if((completed = this->_web.http.isEnd())){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						{
							// Получаем данные ответа
							const auto & response = this->_web.http.response(true);
							// Если параметры ответа получены
							if(!response.empty()){
								// Выводим заголовок ответа
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры ответа
								cout << string(response.begin(), response.end()) << endl;
								// Если тело ответа существует
								if(!this->_web.http.body().empty())
									// Выводим сообщение о выводе чанка тела
									cout << this->_fmk->format("<body %u>", this->_web.http.body().size()) << endl << endl;
								// Иначе устанавливаем перенос строки
								else cout << endl;
							}
						}
					#endif
					// Выполняем препарирование полученных данных
					switch(static_cast <uint8_t> (this->prepare())){
						// Если необходимо выполнить остановку обработки
						case static_cast <uint8_t> (status_t::STOP):
							// Выполняем завершение работы
							goto Stop;
						// Если необходимо выполнить переход к следующему этапу обработки
						case static_cast <uint8_t> (status_t::NEXT):
							// Выполняем переход к следующему этапу обработки
							goto Next;
						// Если необходимо выполнить пропуск обработки данных
						case static_cast <uint8_t> (status_t::SKIP):
							// Завершаем работу
							return;
					}
				}
				// Устанавливаем метку продолжения обработки пайплайна
				Next:
				// Если парсер обработал какое-то количество байт
				if((receive = ((bytes > 0) && !this->_web.buffer.empty()))){
					// Если размер буфера больше количества удаляемых байт
					if((receive = (this->_web.buffer.size() >= bytes)))
						// Удаляем количество обработанных байт
						this->_web.buffer.assign(this->_web.buffer.begin() + bytes, this->_web.buffer.end());
						// vector <decltype(this->_web.buffer)::value_type> (this->_web.buffer.begin() + bytes, this->_web.buffer.end()).swap(this->_web.buffer);
				}
				// Если данные мы все получили, выходим
				if(!receive || this->_web.buffer.empty()) break;
			}
			// Устанавливаем метку завершения работы
			Stop:
			// Если экшен соответствует, выполняем его сброс
			if(this->_action == action_t::READ)
				// Выполняем сброс экшена
				this->_action = action_t::NONE;
			// Если получение данных выполнено
			if(completed){
				// Если функция обратного вызова установлена, выводим сообщение
				if(this->_web.callback.is("entity"))
					// Выполняем функцию обратного вызова дисконнекта
					this->_web.callback.bind <const u_int, const string, const vector <char>> ("entity");
				// Если подключение выполнено и список запросов не пустой
				if((this->_aid > 0) && !this->_web.requests.empty())
					// Выполняем запрос на удалённый сервер
					this->submit(this->_web.requests.front());
			}
		} break;
		// Если агент является клиентом WebSocket
		case static_cast <uint8_t> (agent_t::WEBSOCKET): {
			// Если рукопожатие не выполнено
			if(!reinterpret_cast <http_t *> (&this->_ws.http)->isHandshake()){
				// Выполняем парсинг полученных данных
				const size_t bytes = this->_ws.http.parse(this->_ws.buffer.payload.data(), this->_ws.buffer.payload.size());
				// Если все данные получены
				if(this->_ws.http.isEnd()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Получаем данные ответа
						const auto & response = reinterpret_cast <http_t *> (&this->_ws.http)->response(true);
						// Если параметры ответа получены
						if(!response.empty()){
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры ответа
							cout << string(response.begin(), response.end()) << endl;
							// Если тело ответа существует
							if(!this->_ws.http.body().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->_fmk->format("<body %u>", this->_ws.http.body().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					#endif
					// Выполняем препарирование полученных данных
					switch(static_cast <uint8_t> (this->prepare())){
						// Если необходимо выполнить остановку обработки
						case static_cast <uint8_t> (status_t::STOP): {
							// Если экшен соответствует, выполняем его сброс
							if(this->_action == action_t::READ)
								// Выполняем сброс экшена
								this->_action = action_t::NONE;
							// Выполняем сброс количества попыток
							this->_attempt = 0;
							// Завершаем работу
							const_cast <client::core_t *> (this->_core)->close(this->_aid);
						}
						// Если необходимо выполнить переход к следующему этапу обработки
						case static_cast <uint8_t> (status_t::NEXT): {
							// Есла данных передано больше чем обработано
							if(this->_ws.buffer.payload.size() > bytes)
								// Удаляем количество обработанных байт
								this->_ws.buffer.payload.assign(this->_ws.buffer.payload.begin() + bytes, this->_ws.buffer.payload.end());
								// vector <decltype(this->_ws.buffer.payload)::value_type> (this->_ws.buffer.payload.begin() + bytes, this->_ws.buffer.payload.end()).swap(this->_ws.buffer.payload);
							// Если данных в буфере больше нет
							else {
								// Очищаем буфер собранных данных
								this->_ws.buffer.payload.clear();
								// Если экшен соответствует, выполняем его сброс
								if(this->_action == action_t::READ)
									// Выполняем сброс экшена
									this->_action = action_t::NONE;
							}
							// Завершаем работу
							return;
						}
						// Если необходимо выполнить пропуск обработки данных
						case static_cast <uint8_t> (status_t::SKIP): {
							// Если соединение является постоянным
							if(this->_ws.http.isAlive())
								// Устанавливаем новый экшен выполнения
								this->_action = action_t::CONNECT;
							// Завершаем работу
							else {
								// Если экшен соответствует, выполняем его сброс
								if(this->_action == action_t::READ)
									// Выполняем сброс экшена
									this->_action = action_t::NONE;
								// Завершаем работу
								const_cast <client::core_t *> (this->_core)->close(this->_aid);
							}
							// Завершаем работу
							return;
						}
					}
				// Если экшен соответствует, выполняем его сброс
				} else if(this->_action == action_t::READ)
					// Выполняем сброс экшена
					this->_action = action_t::NONE;
				// Завершаем работу
				return;
			// Если рукопожатие выполнено
			} else if(this->_ws.allow.receive) {
				// Выполняем препарирование полученных данных
				switch(static_cast <uint8_t> (this->prepare())){
					// Если необходимо выполнить остановку обработки
					case static_cast <uint8_t> (status_t::STOP): {
						// Если экшен соответствует, выполняем его сброс
						if(this->_action == action_t::READ)
							// Выполняем сброс экшена
							this->_action = action_t::NONE;
						// Выходим из функции
						return;
					}
					// Если необходимо выполнить переход к следующему этапу обработки
					case static_cast <uint8_t> (status_t::NEXT):
						// Выполняем реконнект
						goto Reconnect;
				}
			}
			// Устанавливаем метку реконнекта
			Reconnect:
			// Выполняем отправку сообщения об ошибке
			this->sendError(this->_ws.mess);
			// Если экшен соответствует, выполняем его сброс
			if(this->_action == action_t::READ)
				// Выполняем сброс экшена
				this->_action = action_t::NONE;
		} break;
	}
}
/**
 * actionConnect Метод обработки экшена подключения к серверу
 */
void awh::client::WEB::actionConnect() noexcept {
	// Если агент является клиентом WebSocket
	if(this->_agent == agent_t::WEBSOCKET){
		// Выполняем сброс параметров запроса
		this->flush();
		// Выполняем сброс состояния HTTP парсера
		this->_ws.http.reset();
		// Выполняем очистку параметров HTTP запроса
		this->_ws.http.clear();
		// Устанавливаем метод сжатия
		this->_ws.http.compress(this->_compress);
		// Разрешаем перехватывать контекст для клиента
		this->_ws.http.clientTakeover(this->_ws.client.takeOver);
		// Разрешаем перехватывать контекст для сервера
		this->_ws.http.serverTakeover(this->_ws.server.takeOver);
		// Разрешаем перехватывать контекст компрессии
		this->_ws.hash.takeoverCompress(this->_ws.client.takeOver);
		// Разрешаем перехватывать контекст декомпрессии
		this->_ws.hash.takeoverDecompress(this->_ws.server.takeOver);
	}
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->_core);
	// Если протокол подключения является HTTP/2
	if(!this->_http2.mode && (this->_core->proto(this->_aid) == engine_t::proto_t::HTTP2)){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выполняем установку функции для вывода отладочной информации
			nghttp2_set_debug_vprintf_callback(&web_t::debugHttp2);
		#endif
		// Создаём объект функций обратного вызова
		nghttp2_session_callbacks * callbacks;
		// Выполняем инициализацию сессию функций обратного вызова
		nghttp2_session_callbacks_new(&callbacks);
		// Выполняем установку функции обратного вызова при подготовки данных для отправки на сервер
		nghttp2_session_callbacks_set_send_callback(callbacks, &web_t::sendHttp2);
		// Выполняем установку функции обратного вызова при получении заголовка HTTP/2
		nghttp2_session_callbacks_set_on_header_callback(callbacks, &web_t::onHeaderHttp2);
		// Выполняем установку функции обратного вызова при получении фрейма заголовков HTTP/2 с сервера
		nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, &web_t::onFrameHttp2);
		// Выполняем установку функции обратного вызова закрытия подключения с сервером HTTP/2
		nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, &web_t::onCloseHttp2);
		// Выполняем установку функции обратного вызова при получении чанка с сервера HTTP/2
		nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, &web_t::onChunkHttp2);
		// Выполняем установку функции обратного вызова начала получения фрейма заголовков HTTP/2
		nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, &web_t::onBeginHeadersHttp2);
		// Выполняем подключение котнекста сессии HTTP/2
		nghttp2_session_client_new(&this->_http2.ctx, callbacks, this);
		// Выполняем удаление объекта функций обратного вызова
		nghttp2_session_callbacks_del(callbacks);
		// Создаём параметры сессии подключения с HTTP/2 сервером
		const vector <nghttp2_settings_entry> iv = {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 128}};
		// Клиентская 24-байтовая магическая строка будет отправлена библиотекой nghttp2
		const int rv = nghttp2_submit_settings(this->_http2.ctx, NGHTTP2_FLAG_NONE, iv.data(), iv.size());
		// Если настройки для сессии установить не удалось
		if(rv != 0){
			// Выполняем закрытие подключения
			core->close(this->_aid);
			// Выводим сообщение об ошибке
			this->_log->print("Could not submit SETTINGS: %s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Если сессия HTTP/2 создана удачно
			if(this->_http2.ctx != nullptr)
				// Выполняем удаление сессии
				nghttp2_session_del(this->_http2.ctx);
			// Выходим из функции
			return;
		}
		/*
		// Если список источников установлен
		if(!this->_origins.empty())
			// Выполняем отправку списка источников
			this->sendOrigin(this->_origins);
		*/
		// Выполняем активацию работы с протоколом HTTP/2
		this->_http2.mode = !this->_http2.mode;
	}
	// Определяем протокол клиента
	switch(static_cast <uint8_t> (this->_agent)){
		// Если агент является клиентом HTTP
		case static_cast <uint8_t> (agent_t::HTTP): {
			// Если экшен соответствует, выполняем его сброс
			if(this->_action == action_t::CONNECT)
				// Выполняем сброс экшена
				this->_action = action_t::NONE;
			// Если функция обратного вызова при подключении/отключении установлена
			if(this->_callback.is("active"))
				// Выводим функцию обратного вызова
				this->_callback.call <const mode_t> ("active", mode_t::CONNECT);
		} break;
		// Если агент является клиентом WebSocket
		case static_cast <uint8_t> (agent_t::WEBSOCKET): {
			// Если активирован режим работы с HTTP/2 протоколом
			if(this->_http2.mode){
				// Список заголовков для запроса
				vector <nghttp2_nv> nva;
				// Выполняем получение параметров запроса
				awh::web_t::query_t query = this->_ws.http.query();
				// Выполняем установки версии протокола
				query.ver = 2.0f;
				// Выполняем установку параметры ответа сервера
				this->_ws.http.query(std::move(query));
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок запроса
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
					// Получаем бинарные данные REST запроса
					const auto & buffer = this->_ws.http.request(this->_scheme.url);
					// Если бинарные данные запроса получены
					if(!buffer.empty())
						// Выводим параметры запроса
						cout << string(buffer.begin(), buffer.end()) << endl << endl;
				#endif
				// Выполняем запрос на получение заголовков
				const auto & headers = this->_ws.http.request2(this->_scheme.url);
				// Выполняем перебор всех заголовков HTTP/2 запроса
				for(auto & header : headers){
					// Выполняем добавление метода запроса
					nva.push_back({
						(uint8_t *) header.first.c_str(),
						(uint8_t *) header.second.c_str(),
						header.first.size(),
						header.second.size(),
						NGHTTP2_NV_FLAG_NONE
					});
				}
				// Выполняем запрос на удалённый сервер
				this->_http2.id = nghttp2_submit_request(this->_http2.ctx, nullptr, nva.data(), nva.size(), nullptr, this);
				// Если запрос не получилось отправить
				if(this->_http2.id < 0){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(this->_http2.id));
					// Выполняем закрытие подключения
					core->close(this->_aid);
					// Выходим из функции
					return;
				}{
					// Результат фиксации сессии
					int rv = -1;
					// Фиксируем отправленный результат
					if((rv = nghttp2_session_send(this->_http2.ctx)) != 0){
						// Выводим сообщение об полученной ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
						// Выходим из функции
						return;
					}
				}
			// Если активирован режим работы с HTTP/1.1 протоколом
			} else {
				// Получаем бинарные данные REST запроса
				const auto & buffer = this->_ws.http.request(this->_scheme.url);
				// Если бинарные данные запроса получены
				if(!buffer.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим заголовок запроса
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
						// Выводим параметры запроса
						cout << string(buffer.begin(), buffer.end()) << endl << endl;
					#endif
					// Выполняем отправку сообщения на сервер
					core->write(buffer.data(), buffer.size(), this->_aid);
				}
			}
			// Если экшен соответствует, выполняем его сброс
			if(this->_action == action_t::CONNECT)
				// Выполняем сброс экшена
				this->_action = action_t::NONE;
		} break;
	}
}
/**
 * actionDisconnect Метод обработки экшена отключения от сервера
 */
void awh::client::WEB::actionDisconnect() noexcept {
	// Определяем протокол клиента
	switch(static_cast <uint8_t> (this->_agent)){
		// Если агент является клиентом HTTP
		case static_cast <uint8_t> (agent_t::HTTP): {
			// Если список ответов получен
			if(!this->_web.requests.empty()){
				// Получаем параметры запроса
				const auto & query = this->_web.http.query();
				// Если нужно произвести запрос заново
				if(!this->_stopped && ((query.code == 201) || (query.code == 301) ||
				   (query.code == 302) || (query.code == 303) || (query.code == 307) ||
				   (query.code == 308) || (query.code == 401) || (query.code == 407))){
					// Если статус ответа требует произвести авторизацию или заголовок перенаправления указан
					if((query.code == 401) || (query.code == 407) || this->_web.http.isHeader("location")){
						// Получаем новый адрес запроса
						const uri_t::url_t & url = this->_web.http.getUrl();
						// Если адрес запроса получен
						if(!url.empty()){
							// Увеличиваем количество попыток
							this->_attempt++;
							// Получаем объект запроса
							req_t & request = this->_web.requests.front();
							// Устанавливаем новый адрес запроса
							this->_scheme.url = std::forward <const uri_t::url_t> (url);
							// Получаем параметры адреса запроса
							request.query = this->_web.uri.query(this->_scheme.url);
							// Выполняем очистку оставшихся данных
							this->_web.buffer.clear();
							// Выполняем установку следующего экшена на открытие подключения
							this->_action = action_t::OPEN;
							// Завершаем работу
							return;
						}
					}
				}
			}
			// Если подключение является постоянным
			if(this->_scheme.alive){
				// Выполняем очистку оставшихся данных
				this->_web.buffer.clear();
				// Если экшен соответствует, выполняем его сброс
				if(this->_action == action_t::DISCONNECT)
					// Выполняем сброс экшена
					this->_action = action_t::NONE;
				// Если функция обратного вызова при подключении/отключении установлена
				if(this->_callback.is("active"))
					// Выводим функцию обратного вызова
					this->_callback.call <const mode_t> ("active", mode_t::DISCONNECT);
			// Если подключение не является постоянным
			} else {
				// Выполняем очистку оставшихся данных
				this->_web.buffer.clear();
				// Выполняем очистку списка запросов
				this->_web.requests.clear();
				// Очищаем адрес сервера
				this->_scheme.url.clear();
				// Выполняем сброс параметров запроса
				this->flush();
				// Выполняем зануление идентификатора адъютанта
				this->_aid = 0;
				// Завершаем работу
				if(this->_unbind) const_cast <client::core_t *> (this->_core)->stop();
				// Если экшен соответствует, выполняем его сброс
				if(this->_action == action_t::DISCONNECT)
					// Выполняем сброс экшена
					this->_action = action_t::NONE;
				// Если функция обратного вызова при подключении/отключении установлена
				if(this->_callback.is("active"))
					// Выводим функцию обратного вызова
					this->_callback.call <const mode_t> ("active", mode_t::DISCONNECT);
			}
		} break;
		// Если агент является клиентом WebSocket
		case static_cast <uint8_t> (agent_t::WEBSOCKET): {
			// Получаем параметры запроса
			const auto & query = this->_ws.http.query();
			// Если нужно произвести запрос заново
			if(!this->_stopped && ((query.code == 301) || (query.code == 308) || (query.code == 401) || (query.code == 407))){
				// Если статус ответа требует произвести авторизацию или заголовок перенаправления указан
				if((query.code == 401) || (query.code == 407) || this->_ws.http.isHeader("location")){
					// Выполняем установку следующего экшена на открытие подключения
					this->_action = action_t::OPEN;
					// Выходим из функции
					return;
				}
			}
			// Если подключение является постоянным
			if(this->_scheme.alive){
				// Выполняем очистку оставшихся данных
				this->_ws.buffer.payload.clear();
				// Выполняем очистку оставшихся фрагментов
				this->_ws.buffer.fragmes.clear();
			// Если подключение не является постоянным
			} else {
				// Выполняем сброс параметров запроса
				this->flush();
				// Завершаем работу
				if(this->_unbind) const_cast <client::core_t *> (this->_core)->stop();
			}
			// Если экшен соответствует, выполняем его сброс
			if(this->_action == action_t::DISCONNECT)
				// Выполняем сброс экшена
				this->_action = action_t::NONE;
			// Если функция обратного вызова при подключении/отключении установлена
			if(this->_callback.is("active"))
				// Выводим функцию обратного вызова
				this->_callback.call <const mode_t> ("active", mode_t::DISCONNECT);
		} break;
	}
}
/**
 * actionProxyRead Метод обработки экшена чтения с прокси-сервера
 */
void awh::client::WEB::actionProxyRead() noexcept {
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->_core);
	// Определяем тип прокси-сервера
	switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
		// Если прокси-сервер является Socks5
		case static_cast <uint8_t> (proxy_t::type_t::SOCKS5): {
			// Если данные не получены
			if(!this->_scheme.proxy.socks5.isEnd()){
				// Выполняем парсинг входящих данных
				this->_scheme.proxy.socks5.parse(this->_web.buffer.data(), this->_web.buffer.size());
				// Получаем данные запроса
				const auto & buffer = this->_scheme.proxy.socks5.get();
				// Если данные получены
				if(!buffer.empty()){
					// Выполняем очистку буфера данных
					this->_web.buffer.clear();
					// Выполняем отправку запроса на сервер
					core->write(buffer.data(), buffer.size(), this->_aid);
					// Если экшен соответствует, выполняем его сброс
					if(this->_action == action_t::PROXY_READ)
						// Выполняем сброс экшена
						this->_action = action_t::NONE;
					// Завершаем работу
					return;
				// Если данные все получены
				} else if(this->_scheme.proxy.socks5.isEnd()) {
					// Выполняем очистку буфера данных
					this->_web.buffer.clear();
					// Если рукопожатие выполнено
					if(this->_scheme.proxy.socks5.isHandshake()){
						// Выполняем переключение на работу с сервером
						core->switchProxy(this->_aid);
						// Если экшен соответствует, выполняем его сброс
						if(this->_action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->_action = action_t::NONE;
						// Завершаем работу
						return;
					// Если рукопожатие не выполнено
					} else {
						// Устанавливаем код ответа
						const u_int code = this->_scheme.proxy.socks5.code();
						// Устанавливаем сообщение ответа
						const string & message = this->_scheme.proxy.socks5.message(code);
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Если заголовки получены
							if(!message.empty()){
								// Данные WEB ответа
								const string & answer = this->_fmk->format("SOCKS5 %u %s\r\n", code, message.c_str());
								// Выводим заголовок ответа
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры ответа
								cout << string(answer.begin(), answer.end()) << endl;
							}
						#endif
						// Если экшен соответствует, выполняем его сброс
						if(this->_action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->_action = action_t::NONE;
						// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
						if(this->_callback.is("response"))
							// Выводим функцию обратного вызова
							this->_callback.call <const u_int, const string &> ("response", code, message);
						// Завершаем работу
						core->close(this->_aid);
						// Завершаем работу
						return;
					}
				}
			}
		} break;
		// Если прокси-сервер является HTTP
		case static_cast <uint8_t> (proxy_t::type_t::HTTP): {
			// Определяем протокол клиента
			switch(static_cast <uint8_t> (this->_agent)){
				// Если агент является клиентом HTTP
				case static_cast <uint8_t> (agent_t::HTTP):
					// Выполняем парсинг полученных данных
					this->_scheme.proxy.http.parse(this->_web.buffer.data(), this->_web.buffer.size());
				break;
				// Если агент является клиентом WebSocket
				case static_cast <uint8_t> (agent_t::WEBSOCKET):
					// Выполняем парсинг полученных данных
					this->_scheme.proxy.http.parse(this->_ws.buffer.payload.data(), this->_ws.buffer.payload.size());
				break;
			}
			// Если все данные получены
			if(this->_scheme.proxy.http.isEnd()){
				// Определяем протокол клиента
				switch(static_cast <uint8_t> (this->_agent)){
					// Если агент является клиентом HTTP
					case static_cast <uint8_t> (agent_t::HTTP):
						// Выполняем очистку буфера данных
						this->_web.buffer.clear();
					break;
					// Если агент является клиентом WebSocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем очистку буфера данных
						this->_ws.buffer.payload.clear();
					break;
				}
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					{
						// Получаем данные ответа
						const auto & response = this->_scheme.proxy.http.response(true);
						// Если параметры ответа получены
						if(!response.empty()){
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры ответа
							cout << string(response.begin(), response.end()) << endl;
							// Если тело ответа существует
							if(!this->_scheme.proxy.http.body().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->_fmk->format("<body %u>", this->_scheme.proxy.http.body().size()) << endl;
						}
					}
				#endif
				// Объект работы с функциями обратного вызова
				fn_t callback(this->_log);
				// Получаем объект запроса
				req_t & request = this->_web.requests.front();
				// Получаем параметры запроса
				const auto & query = this->_scheme.proxy.http.query();
				// Получаем статус ответа
				awh::http_t::stath_t status = this->_scheme.proxy.http.getAuth();
				// Если выполнять редиректы запрещено
				if(!this->_redirects && (status == awh::http_t::stath_t::RETRY)){
					// Если нужно произвести запрос заново
					if((query.code == 201) || (query.code == 301) ||
					   (query.code == 302) || (query.code == 303) ||
					   (query.code == 307) || (query.code == 308))
						// Запрещаем выполнять редирект
						status = awh::http_t::stath_t::GOOD;
				}
				// Выполняем проверку авторизации
				switch(static_cast <uint8_t> (status)){
					// Если нужно попытаться ещё раз
					case static_cast <uint8_t> (awh::http_t::stath_t::RETRY): {
						// Если попытки повторить переадресацию ещё не закончились
						if(!(this->_stopped = (this->_attempt >= this->_attempts))){
							// Если адрес запроса получен
							if(!this->_scheme.proxy.url.empty()){
								// Увеличиваем количество попыток
								this->_attempt++;
								// Если соединение является постоянным
								if(this->_scheme.proxy.http.isAlive())
									// Устанавливаем новый экшен выполнения
									this->_action = action_t::PROXY_CONNECT;
								// Если соединение должно быть закрыто
								else {
									// Если экшен соответствует, выполняем его сброс
									if(this->_action == action_t::PROXY_READ)
										// Выполняем сброс экшена
										this->_action = action_t::NONE;
									// Завершаем работу
									core->close(this->_aid);
								}
								// Завершаем работу
								return;
							}
						}
					} break;
					// Если запрос выполнен удачно
					case static_cast <uint8_t> (awh::http_t::stath_t::GOOD): {
						// Выполняем сброс количества попыток
						this->_attempt = 0;
						// Выполняем переключение на работу с сервером
						core->switchProxy(this->_aid);
						// Если экшен соответствует, выполняем его сброс
						if(this->_action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->_action = action_t::NONE;
						// Завершаем работу
						return;
					} break;
					// Если запрос неудачный
					case static_cast <uint8_t> (awh::http_t::stath_t::FAULT): {
						// Устанавливаем флаг принудительной остановки
						this->_stopped = true;
						// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
						if(this->_callback.is("entity"))
							// Устанавливаем полученную функцию обратного вызова
							callback.set <void (const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const u_int, const string, const vector <char>)> ("entity"), query.code, query.message, this->_scheme.proxy.http.body());
						// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
						if(this->_callback.is("headers"))
							// Выводим функцию обратного вызова
							callback.set <void (const u_int, const string, const unordered_multimap <string, string>)> ("headers", this->_callback.get <void (const u_int, const string, const unordered_multimap <string, string>)> ("headers"), query.code, query.message, this->_scheme.proxy.http.headers());
					} break;
				}
				// Если экшен соответствует, выполняем его сброс
				if(this->_action == action_t::PROXY_READ)
					// Выполняем сброс экшена
					this->_action = action_t::NONE;
				// Если функция обратного вызова на вывод полученных заголовков установлена, выводим сообщение
				if(callback.is("headers"))
					// Выполняем функцию обратного вызова дисконнекта
					callback.bind <const u_int, const string, const unordered_multimap <string, string>> ("headers");
				// Если функция обратного вызова на вывод полученного тела установлена, выводим сообщение
				if(callback.is("entity"))
					// Выполняем функцию обратного вызова дисконнекта
					callback.bind <const u_int, const string, const vector <char>> ("entity");
				// Завершаем работу
				core->close(this->_aid);
				// Завершаем работу
				return;
			}
		} break;
		// Иначе завершаем работу
		default: {
			// Если экшен соответствует, выполняем его сброс
			if(this->_action == action_t::PROXY_READ)
				// Выполняем сброс экшена
				this->_action = action_t::NONE;
			// Завершаем работу
			core->close(this->_aid);
		}
	}
}
/**
 * actionProxyConnect Метод обработки экшена подключения к прокси-серверу
 */
void awh::client::WEB::actionProxyConnect() noexcept {
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->_core);
	// Определяем тип прокси-сервера
	switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
		// Если прокси-сервер является Socks5
		case static_cast <uint8_t> (proxy_t::type_t::SOCKS5): {
			// Выполняем сброс состояния Socks5 парсера
			this->_scheme.proxy.socks5.reset();
			// Устанавливаем URL адрес запроса
			this->_scheme.proxy.socks5.url(this->_scheme.url);
			// Выполняем создание буфера запроса
			this->_scheme.proxy.socks5.parse();
			// Получаем данные запроса
			const auto & buffer = this->_scheme.proxy.socks5.get();
			// Если данные получены
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				core->write(buffer.data(), buffer.size(), this->_aid);
		} break;
		// Если прокси-сервер является HTTP
		case static_cast <uint8_t> (proxy_t::type_t::HTTP): {
			// Выполняем сброс состояния HTTP парсера
			this->_scheme.proxy.http.reset();
			// Выполняем очистку параметров HTTP запроса
			this->_scheme.proxy.http.clear();
			// Получаем бинарные данные WEB запроса
			const auto & buffer = this->_scheme.proxy.http.proxy(this->_scheme.url);
			// Если бинарные данные запроса получены
			if(!buffer.empty()){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок запроса
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST PROXY ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры запроса
					cout << string(buffer.begin(), buffer.end()) << endl;
				#endif
				// Выполняем отправку сообщения на сервер
				core->write(buffer.data(), buffer.size(), this->_aid);
			}
		} break;
		// Иначе завершаем работу
		default: core->close(this->_aid);
	}
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::PROXY_CONNECT)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
}
/**
 * prepare Метод выполнения препарирования полученных данных
 * @return результат препарирования
 */
awh::client::WEB::status_t awh::client::WEB::prepare() noexcept {
	// Результат работы функции
	status_t result = status_t::STOP;
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->_core);
	// Определяем протокол клиента
	switch(static_cast <uint8_t> (this->_agent)){
		// Если агент является клиентом HTTP
		case static_cast <uint8_t> (agent_t::HTTP): {
			// Получаем параметры запроса
			const auto & query = this->_web.http.query();
			// Получаем статус ответа
			awh::http_t::stath_t status = this->_web.http.getAuth();
			// Если выполнять редиректы запрещено
			if(!this->_redirects && (status == awh::http_t::stath_t::RETRY)){
				// Если нужно произвести запрос заново
				if((query.code == 201) || (query.code == 301) ||
				   (query.code == 302) || (query.code == 303) ||
				   (query.code == 307) || (query.code == 308))
						// Запрещаем выполнять редирект
						status = awh::http_t::stath_t::GOOD;
			}
			// Выполняем анализ результата авторизации
			switch(static_cast <uint8_t> (status)){
				// Если нужно попытаться ещё раз
				case static_cast <uint8_t> (awh::http_t::stath_t::RETRY): {
					// Если попытки повторить переадресацию ещё не закончились
					if(!(this->_stopped = (this->_attempt >= this->_attempts))){
						// Получаем новый адрес запроса
						const uri_t::url_t & url = this->_web.http.getUrl();
						// Если адрес запроса получен
						if(!url.empty()){
							// Увеличиваем количество попыток
							this->_attempt++;
							// Выполняем проверку соответствие протоколов
							const bool schema = (this->_fmk->compare(url.schema, this->_scheme.url.schema));
							// Устанавливаем новый адрес запроса
							this->_scheme.url = std::forward <const uri_t::url_t> (url);
							// Получаем объект запроса
							req_t & request = this->_web.requests.front();
							// Получаем параметры адреса запроса
							request.query = this->_web.uri.query(this->_scheme.url);
							// Выполняем очистку оставшихся данных
							this->_web.buffer.clear();
							// Если экшен соответствует, выполняем его сброс
							if(this->_action == action_t::READ)
								// Выполняем сброс экшена
								this->_action = action_t::NONE;
							// Если соединение является постоянным
							if(schema && this->_web.http.isAlive())
								// Отправляем повторный запрос
								this->send();
							// Если нам необходимо отключиться
							else core->close(this->_aid);
							// Завершаем работу
							return status_t::SKIP;
						}
					}
				} break;
				// Если запрос выполнен удачно
				case static_cast <uint8_t> (awh::http_t::stath_t::GOOD): {
					// Выполняем сброс количества попыток
					this->_attempt = 0;
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(this->_callback.is("entity"))
						// Устанавливаем полученную функцию обратного вызова
						this->_web.callback.set <void (const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const u_int, const string, const vector <char>)> ("entity"), query.code, query.message, this->_web.http.body());
					// Устанавливаем размер стопбайт
					if(!this->_web.http.isAlive()){
						// Выполняем очистку оставшихся данных
						this->_web.buffer.clear();
						// Завершаем работу
						core->close(this->_aid);
						// Выполняем завершение работы
						return status_t::STOP;
					}
					// Если объект ещё не удалён
					if(!this->_web.requests.empty())
						// Выполняем удаление объекта запроса
						this->_web.requests.erase(this->_web.requests.begin());
					// Завершаем обработку
					return status_t::NEXT;
				} break;
				// Если запрос неудачный
				case static_cast <uint8_t> (awh::http_t::stath_t::FAULT): {
					// Устанавливаем флаг принудительной остановки
					this->_stopped = true;
					// Если возникла ошибка выполнения запроса
					if((query.code >= 400) && (query.code < 500)){
						// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
						if(this->_callback.is("entity"))
							// Устанавливаем полученную функцию обратного вызова
							this->_web.callback.set <void (const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const u_int, const string, const vector <char>)> ("entity"), query.code, query.message, this->_web.http.body());
						// Если объект ещё не удалён
						if(!this->_web.requests.empty())
							// Выполняем удаление объекта запроса
							this->_web.requests.erase(this->_web.requests.begin());
						// Завершаем обработку
						return status_t::NEXT;
					}
				} break;
			}
			// Выполняем очистку оставшихся данных
			this->_web.buffer.clear();
			// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
			if(this->_callback.is("entity"))
				// Устанавливаем полученную функцию обратного вызова
				this->_web.callback.set <void (const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const u_int, const string, const vector <char>)> ("entity"), query.code, query.message, this->_web.http.body());
			// Завершаем работу
			core->close(this->_aid);
			// Выполняем завершение работы
			return status_t::STOP;
		} break;
		// Если агент является клиентом WebSocket
		case static_cast <uint8_t> (agent_t::WEBSOCKET): {
			// Если рукопожатие не выполнено
			if(!reinterpret_cast <http_t *> (&this->_ws.http)->isHandshake()){
				// Получаем параметры запроса
				auto query = this->_ws.http.query();
				// Выполняем проверку авторизации
				switch(static_cast <uint8_t> (this->_ws.http.getAuth())){
					// Если нужно попытаться ещё раз
					case static_cast <uint8_t> (http_t::stath_t::RETRY): {
						// Если попытка повторить авторизацию ещё не проводилась
						if(!(this->_stopped = (this->_attempt >= this->_attempts))){
							// Получаем новый адрес запроса
							this->_scheme.url = this->_ws.http.getUrl();
							// Если адрес запроса получен
							if(!this->_scheme.url.empty()){
								// Увеличиваем количество попыток
								this->_attempt++;
								// Выполняем очистку оставшихся данных
								this->_ws.buffer.payload.clear();
								// Завершаем работу
								return status_t::SKIP;
							}
						}
						// Создаём сообщение
						this->_ws.mess = ws::mess_t(query.code, this->_ws.http.message(query.code));
						// Выводим сообщение
						this->webSocketError(this->_ws.mess);
					} break;
					// Если запрос выполнен удачно
					case static_cast <uint8_t> (http_t::stath_t::GOOD): {
						// Если рукопожатие выполнено
						if(this->_ws.http.isHandshake()){
							// Выполняем сброс количества попыток
							this->_attempt = 0;
							// Очищаем список фрагментированных сообщений
							this->_ws.buffer.fragmes.clear();
							// Получаем флаг шифрованных данных
							this->_ws.crypt = this->_ws.http.isCrypt();
							// Получаем поддерживаемый метод компрессии
							this->_compress = this->_ws.http.compress();
							// Получаем размер скользящего окна сервера
							this->_ws.server.wbit = this->_ws.http.wbitServer();
							// Получаем размер скользящего окна клиента
							this->_ws.client.wbit = this->_ws.http.wbitClient();
							// Обновляем контрольную точку времени получения данных
							this->_ws.point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
							// Разрешаем перехватывать контекст компрессии для клиента
							this->_ws.hash.takeoverCompress(this->_ws.http.clientTakeover());
							// Разрешаем перехватывать контекст компрессии для сервера
							this->_ws.hash.takeoverDecompress(this->_ws.http.serverTakeover());
							// Если разрешено в лог выводим информационные сообщения
							if(!this->_ws.noinfo)
								// Выводим в лог сообщение об удачной авторизации не WebSocket-сервере
								this->_log->print("authorization on the WebSocket-server was successful", log_t::flag_t::INFO);
							// Если функция обратного вызова при подключении/отключении установлена
							if(this->_callback.is("active"))
								// Выводим функцию обратного вызова
								this->_callback.call <const mode_t> ("active", mode_t::CONNECT);
							// Завершаем работу
							return status_t::NEXT;
						// Сообщаем, что рукопожатие не выполнено
						} else {
							// Если код ответа не является отрицательным
							if(query.code < 400){
								// Устанавливаем код ответа
								query.code = 403;
								// Заменяем ответ сервера
								this->_ws.http.query(query);
							}
							// Создаём сообщение
							this->_ws.mess = ws::mess_t(query.code, this->_ws.http.message(query.code));
							// Выводим сообщение
							this->webSocketError(this->_ws.mess);
						}
					} break;
					// Если запрос неудачный
					case static_cast <uint8_t> (http_t::stath_t::FAULT): {
						// Устанавливаем флаг принудительной остановки
						this->_stopped = true;
						// Создаём сообщение
						this->_ws.mess = ws::mess_t(query.code, query.message);
						// Выводим сообщение
						this->webSocketError(this->_ws.mess);
					} break;
				}
				// Завершаем работу
				return status_t::STOP;
			// Если рукопожатие выполнено
			} else if(this->_ws.allow.receive) {
				// Флаг удачного получения данных
				bool receive = false;
				// Создаём буфер сообщения
				vector <char> buffer;
				// Создаём объект шапки фрейма
				ws::frame_t::head_t head;
				// Выполняем обработку полученных данных
				while(!this->_ws.close && this->_ws.allow.receive){
					// Выполняем чтение фрейма WebSocket
					const auto & data = this->_ws.frame.methods.get(head, this->_ws.buffer.payload.data(), this->_ws.buffer.payload.size());
					// Если буфер данных получен
					if(!data.empty()){
						// Проверяем состояние флагов RSV2 и RSV3
						if(head.rsv[1] || head.rsv[2]){
							// Создаём сообщение
							this->_ws.mess = ws::mess_t(1002, "RSV2 and RSV3 must be clear");
							// Выводим сообщение
							this->webSocketError(this->_ws.mess);
							// Выполняем реконнект
							return status_t::NEXT;
						}
						// Если флаг компресси включён а данные пришли не сжатые
						if(head.rsv[0] && ((this->_compress == http_t::compress_t::NONE) ||
						  (head.optcode == ws::frame_t::opcode_t::CONTINUATION) ||
						  ((static_cast <uint8_t> (head.optcode) > 0x07) && (static_cast <uint8_t> (head.optcode) < 0x0b)))){
							// Создаём сообщение
							this->_ws.mess = ws::mess_t(1002, "RSV1 must be clear");
							// Выводим сообщение
							this->webSocketError(this->_ws.mess);
							// Выполняем реконнект
							return status_t::NEXT;
						}
						// Если опкоды требуют финального фрейма
						if(!head.fin && (static_cast <uint8_t> (head.optcode) > 0x07) && (static_cast <uint8_t> (head.optcode) < 0x0b)){
							// Создаём сообщение
							this->_ws.mess = ws::mess_t(1002, "FIN must be set");
							// Выводим сообщение
							this->webSocketError(this->_ws.mess);
							// Выполняем реконнект
							return status_t::NEXT;
						}
						// Определяем тип ответа
						switch(static_cast <uint8_t> (head.optcode)){
							// Если ответом является PING
							case static_cast <uint8_t> (ws::frame_t::opcode_t::PING):
								// Отправляем ответ серверу
								this->pong(string(data.begin(), data.end()));
							break;
							// Если ответом является PONG
							case static_cast <uint8_t> (ws::frame_t::opcode_t::PONG):
								// Если идентификатор адъютанта совпадает
								if(memcmp(to_string(this->_aid).c_str(), data.data(), data.size()) == 0)
									// Обновляем контрольную точку
									this->_ws.point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
							break;
							// Если ответом является TEXT
							case static_cast <uint8_t> (ws::frame_t::opcode_t::TEXT):
							// Если ответом является BINARY
							case static_cast <uint8_t> (ws::frame_t::opcode_t::BINARY): {
								// Запоминаем полученный опкод
								this->_ws.frame.opcode = head.optcode;
								// Запоминаем, что данные пришли сжатыми
								this->_ws.deflate = (head.rsv[0] && (this->_compress != http_t::compress_t::NONE));
								// Если сообщение замаскированно
								if(head.mask){
									// Создаём сообщение
									this->_ws.mess = ws::mess_t(1002, "masked frame from server");
									// Выводим сообщение
									this->webSocketError(this->_ws.mess);
									// Выполняем реконнект
									return status_t::NEXT;
								// Если список фрагментированных сообщений существует
								} else if(!this->_ws.buffer.fragmes.empty()) {
									// Очищаем список фрагментированных сообщений
									this->_ws.buffer.fragmes.clear();
									// Создаём сообщение
									this->_ws.mess = ws::mess_t(1002, "opcode for subsequent fragmented messages should not be set");
									// Выводим сообщение
									this->webSocketError(this->_ws.mess);
									// Выполняем реконнект
									return status_t::NEXT;
								// Если сообщение является не последнем
								} else if(!head.fin)
									// Заполняем фрагментированное сообщение
									this->_ws.buffer.fragmes.insert(this->_ws.buffer.fragmes.end(), data.begin(), data.end());
								// Если сообщение является последним
								else buffer = std::forward <const vector <char>> (data);
							} break;
							// Если ответом является CONTINUATION
							case static_cast <uint8_t> (ws::frame_t::opcode_t::CONTINUATION): {
								// Заполняем фрагментированное сообщение
								this->_ws.buffer.fragmes.insert(this->_ws.buffer.fragmes.end(), data.begin(), data.end());
								// Если сообщение является последним
								if(head.fin){
									// Выполняем копирование всех собранных сегментов
									buffer = std::forward <const vector <char>> (this->_ws.buffer.fragmes);
									// Очищаем список фрагментированных сообщений
									this->_ws.buffer.fragmes.clear();
								}
							} break;
							// Если ответом является CLOSE
							case static_cast <uint8_t> (ws::frame_t::opcode_t::CLOSE): {
								// Извлекаем сообщение
								this->_ws.mess = this->_ws.frame.methods.message(data);
								// Выводим сообщение
								this->webSocketError(this->_ws.mess);
								// Выполняем реконнект
								return status_t::NEXT;
							} break;
						}
					}
					// Если парсер обработал какое-то количество байт
					if((receive = ((head.frame > 0) && !this->_ws.buffer.payload.empty()))){
						// Если размер буфера больше количества удаляемых байт
						if((receive = (this->_ws.buffer.payload.size() >= head.frame)))
							// Удаляем количество обработанных байт
							this->_ws.buffer.payload.assign(this->_ws.buffer.payload.begin() + head.frame, this->_ws.buffer.payload.end());
							// vector <decltype(this->_ws.buffer.payload)::value_type> (this->_ws.buffer.payload.begin() + head.frame, this->_ws.buffer.payload.end()).swap(this->_ws.buffer.payload);
					}
					// Если сообщения получены
					if(!buffer.empty()){
						// Если тредпул активирован
						if(this->_ws.threads.is())
							// Добавляем в тредпул новую задачу на извлечение полученных сообщений
							this->_ws.threads.push(std::bind(&web_t::extraction, this, buffer, (this->_ws.frame.opcode == ws::frame_t::opcode_t::TEXT)));
						// Если тредпул не активирован, выполняем извлечение полученных сообщений
						else this->extraction(buffer, (this->_ws.frame.opcode == ws::frame_t::opcode_t::TEXT));
						// Очищаем буфер полученного сообщения
						buffer.clear();
					}
					// Если данные мы все получили, выходим
					if(!receive || this->_ws.buffer.payload.empty()) break;
				}
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * webSocketError Метод вывода сообщений об ошибках работы клиента
 * @param message сообщение с описанием ошибки
 */
void awh::client::WEB::webSocketError(const ws::mess_t & message) const noexcept {
	// Если агент является клиентом WebSocket
	if(this->_agent == agent_t::WEBSOCKET){
		// Очищаем список буффер бинарных данных
		const_cast <web_t *> (this)->_ws.buffer.payload.clear();
		// Очищаем список фрагментированных сообщений
		const_cast <web_t *> (this)->_ws.buffer.fragmes.clear();
		// Если код ошибки указан
		if(message.code > 0){
			// Если сообщение об ошибке пришло
			if(!message.text.empty()){
				// Если тип сообщения получен
				if(!message.type.empty())
					// Выводим в лог сообщение
					this->_log->print("%s - %s [%u]", log_t::flag_t::WARNING, message.type.c_str(), message.text.c_str(), message.code);
				// Иначе выводим сообщение в упрощёном виде
				else this->_log->print("%s [%u]", log_t::flag_t::WARNING, message.text.c_str(), message.code);
				// Если функция обратного вызова при подключении/отключении установлена // ++++++++++++++++++++++++++++++++++++++
				if(this->_callback.is("webSocketError"))
					// Если функция обратного вызова установлена, выводим полученное сообщение
					this->_callback.call <const u_int, const string &> ("webSocketError", message.code, message.text);
			}
		}
	// Выводим сообщение, что операция не возможна
	} else this->_log->print("error generation operation is available only to the websocket protocol", log_t::flag_t::WARNING);
}
/**
 * extraction Метод извлечения полученных данных
 * @param buffer данные в чистом виде полученные с сервера
 * @param utf8   данные передаются в текстовом виде
 */
void awh::client::WEB::extraction(const vector <char> & buffer, const bool utf8) noexcept {
	// Если буфер данных передан
	if(!buffer.empty() && !this->_ws.freeze && this->_callback.is("message")){
		// Если данные пришли в сжатом виде
		if(this->_ws.deflate && (this->_compress != http_t::compress_t::NONE)){
			// Декомпрессионные данные
			vector <char> data;
			// Определяем метод компрессии
			switch(static_cast <uint8_t> (this->_compress)){
				// Если метод компрессии выбран Deflate
				case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
					// Устанавливаем размер скользящего окна
					this->_ws.hash.wbit(this->_ws.server.wbit);
					// Добавляем хвост в полученные данные
					this->_ws.hash.setTail(* const_cast <vector <char> *> (&buffer));
					// Выполняем декомпрессию полученных данных
					data = this->_ws.hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
				} break;
				// Если метод компрессии выбран GZip
				case static_cast <uint8_t> (http_t::compress_t::GZIP):
					// Выполняем декомпрессию полученных данных
					data = this->_ws.hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::GZIP);
				break;
				// Если метод компрессии выбран Brotli
				case static_cast <uint8_t> (http_t::compress_t::BROTLI):
					// Выполняем декомпрессию полученных данных
					data = this->_ws.hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BROTLI);
				break;
			}
			// Если данные получены
			if(!data.empty()){
				// Если нужно производить дешифрование
				if(this->_ws.crypt){
					// Выполняем шифрование переданных данных
					const auto & res = this->_ws.hash.decrypt(data.data(), data.size());
					// Если данные сообщения получилось удачно расшифровать // ++++++++++++++++++++++++++++++++++++++
					if(!res.empty())
						// Выводим данные полученного сообщения
						this->_callback.call <const vector <char> &, const bool> ("message", res, utf8);
					// Иначе выводим сообщение так - как оно пришло
					else this->_callback.call <const vector <char> &, const bool> ("message", data, utf8);
				// Отправляем полученный результат
				} else this->_callback.call <const vector <char> &, const bool> ("message", data, utf8);
			// Выводим сообщение об ошибке
			} else {
				// Создаём сообщение
				this->_ws.mess = ws::mess_t(1007, "received data decompression error");
				// Выводим сообщение
				this->webSocketError(this->_ws.mess);
				// Иначе выводим сообщение так - как оно пришло
				this->_callback.call <const vector <char> &, const bool> ("message", buffer, utf8);
				// Выполняем отправку сообщения об ошибке
				this->sendError(this->_ws.mess);
			}
		// Если функция обратного вызова установлена, выводим полученное сообщение
		} else {
			// Если нужно производить дешифрование
			if(this->_ws.crypt){
				// Выполняем шифрование переданных данных
				const auto & res = this->_ws.hash.decrypt(buffer.data(), buffer.size());
				// Если данные сообщения получилось удачно распаковать // ++++++++++++++++++++++++++++++++++++++
				if(!res.empty())
					// Выводим данные полученного сообщения
					this->_callback.call <const vector <char> &, const bool> ("message", res, utf8);
				// Иначе выводим сообщение так - как оно пришло
				else this->_callback.call <const vector <char> &, const bool> ("message", buffer, utf8);
			// Отправляем полученный результат
			} else this->_callback.call <const vector <char> &, const bool> ("message", buffer, utf8);
		}
	}
}
/**
 * GET Метод запроса в формате HTTP методом GET
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::GET, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * DEL Метод запроса в формате HTTP методом DEL
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::DEL(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::DEL, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PUT(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Получаем тело запроса
	const string body = entity.dump();
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PUT, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PUT(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result = std::forward <const vector <char>> (entity);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PUT, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Тело в формате X-WWW-Form-Urlencoded
	string body = "";
	// Переходим по всему списку тела запроса
	for(auto & param : entity){
		// Есди данные уже набраны
		if(!body.empty()) body.append("&");
		// Добавляем в список набор параметров
		body.append(this->_web.uri.encode(param.first));
		// Добавляем разделитель
		body.append("=");
		// Добавляем значение
		body.append(this->_web.uri.encode(param.second));
	}
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PUT, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::POST(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Получаем тело запроса
	const string body = entity.dump();
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::POST, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::POST(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result = std::forward <const vector <char>> (entity);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::POST, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::POST(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Тело в формате X-WWW-Form-Urlencoded
	string body = "";
	// Переходим по всему списку тела запроса
	for(auto & param : entity){
		// Есди данные уже набраны
		if(!body.empty()) body.append("&");
		// Добавляем в список набор параметров
		body.append(this->_web.uri.encode(param.first));
		// Добавляем разделитель
		body.append("=");
		// Добавляем значение
		body.append(this->_web.uri.encode(param.second));
	}
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::POST, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PATCH(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Получаем тело запроса
	const string body = entity.dump();
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PATCH, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PATCH(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result = std::forward <const vector <char>> (entity);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PATCH, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Тело в формате X-WWW-Form-Urlencoded
	string body = "";
	// Переходим по всему списку тела запроса
	for(auto & param : entity){
		// Есди данные уже набраны
		if(!body.empty()) body.append("&");
		// Добавляем в список набор параметров
		body.append(this->_web.uri.encode(param.first));
		// Добавляем разделитель
		body.append("=");
		// Добавляем значение
		body.append(this->_web.uri.encode(param.second));
	}
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PATCH, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * HEAD Метод запроса в формате HTTP методом HEAD
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::WEB::HEAD(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> entity;
	// Результат работы функции
	unordered_multimap <string, string> result = std::forward <const unordered_multimap <string, string>> (headers);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::HEAD, url, entity, result);
	// Выводим результат
	return result;
}
/**
 * TRACE Метод запроса в формате HTTP методом TRACE
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::WEB::TRACE(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> entity;
	// Результат работы функции
	unordered_multimap <string, string> result = std::forward <const unordered_multimap <string, string>> (headers);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::TRACE, url, entity, result);
	// Выводим результат
	return result;
}
/**
 * OPTIONS Метод запроса в формате HTTP методом OPTIONS
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::WEB::OPTIONS(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> entity;
	// Результат работы функции
	unordered_multimap <string, string> result = std::forward <const unordered_multimap <string, string>> (headers);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::OPTIONS, url, entity, result);
	// Выводим результат
	return result;
}
/**
 * REQUEST Метод выполнения запроса HTTP
 * @param method  метод запроса
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 */
void awh::client::WEB::REQUEST(const awh::web_t::method_t method, const uri_t::url_t & url, vector <char> & entity, unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Подписываемся на получение сообщения сервера
		this->on([this](const u_int code, const string & message) noexcept -> void {
			// Если возникла ошибка, выводим сообщение
			if(code >= 300)
				// Выводим сообщение о неудачном запросе
				this->_log->print("request failed: %u %s", log_t::flag_t::WARNING, code, message.c_str());
		});
		// Подписываемся на событие коннекта и дисконнекта клиента
		this->on([method, &url, &entity, &headers, this](const web_t::mode_t mode) noexcept -> void {
			// Если подключение выполнено
			if(mode == client::web_t::mode_t::CONNECT){
				// Создаём объект запроса
				req_t req;
				// Устанавливаем метод запроса
				req.method = method;
				// Устанавливаем тепло запроса
				req.entity = std::forward <const vector <char>> (entity);
				// Устанавливаем параметры запроса
				req.query = std::forward <const string> (this->_web.uri.query(url));
				// Запоминаем переданные заголовки
				req.headers = std::forward <const unordered_multimap <string, string>> (headers);
				// Выполняем запрос на сервер
				this->send({req, req});
			// Выполняем остановку работы модуля
			} else this->stop();
		});
		// Подписываемся на событие получения тела ответа
		this->on([&entity, this](const u_int code, const string & message, const vector <char> & data) noexcept -> void {
			
			cout << " ================== " << string(data.begin(), data.end()) << endl;
			
			// Если тело ответа получено
			if(!data.empty())
				// Формируем результат ответа
				entity.assign(data.begin(), data.end());
			// Выполняем очистку тела запроса
			else entity.clear();
			// Выполняем остановку
			// this->stop();
		});
		// Подписываем на событие получения заголовков ответа
		this->on([&headers, this](const u_int code, const string & message, const unordered_multimap <string, string> & data) noexcept -> void {
			// Если заголовки ответа получены
			if(!data.empty())
				// Извлекаем полученный список заголовков
				headers = std::forward <const unordered_multimap <string, string>> (data);
		});
		// Выполняем инициализацию подключения
		this->init(this->_web.uri.origin(url));
		// Выполняем запуск работы
		this->start();
	}
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::WEB::flush() noexcept {
	// Снимаем флаг принудительной остановки
	this->_stopped = false;
	// Определяем протокол клиента
	switch(static_cast <uint8_t> (this->_agent)){
		// Если агент является клиентом HTTP
		case static_cast <uint8_t> (agent_t::HTTP): {
			// Сбрасываем флаг принудительной остановки
			this->_active = false;
			// Выполняем очистку оставшихся данных
			this->_web.buffer.clear();
		} break;
		// Если агент является клиентом WebSocket
		case static_cast <uint8_t> (agent_t::WEBSOCKET): {
			// Снимаем флаг отключения
			this->_ws.close = false;
			// Устанавливаем флаг разрешающий обмен данных
			this->_ws.allow = allow_t();
			// Очищаем буфер данных
			this->_ws.buffer = buffer_t();
		} break;
	}
}
/**
 * init Метод инициализации WEB клиента
 * @param url      адрес WEB сервера
 * @param compress метод компрессии передаваемых сообщений
 */
void awh::client::WEB::init(const string & url, const http_t::compress_t compress) noexcept {
	// Если unix-сокет установлен
	if(this->_core->family() == scheme_t::family_t::NIX){
		// Выполняем очистку схемы сети
		this->_scheme.clear();
		// Устанавливаем метод компрессии сообщений
		this->_compress = compress;
		// Устанавливаем URL адрес запроса (как заглушка)
		this->_scheme.url = this->_web.uri.parse(this->_fmk->format("unix:%s.sock", url.c_str()));
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем установку unix-сокета 
			const_cast <client::core_t *> (this->_core)->unixSocket(url);
		#endif
	// Выполняем установку unix-сокет
	} else {
		// Если адрес сервера передан
		if(!url.empty()){
			// Выполняем очистку схемы сети
			this->_scheme.clear();
			// Устанавливаем URL адрес запроса
			this->_scheme.url = this->_web.uri.parse(url);
			/**
			 * Если операционной системой не является Windows
			 */
			#if !defined(_WIN32) && !defined(_WIN64)
				// Удаляем unix-сокет ранее установленный
				const_cast <client::core_t *> (this->_core)->removeUnixSocket();
			#endif
		}
	}
	// Устанавливаем метод компрессии сообщений
	this->_compress = compress;
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const mode_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const mode_t)> ("active", callback);
}
/** 
 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const vector <char> &)> ("chunks", callback);
}
/** 
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const u_int, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_web.http.on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const u_int, const string &)> ("response", callback);
}
/** 
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_web.http.on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const string &, const string &)> ("header", callback);
}
/** 
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const u_int, const string &, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const u_int, const string &, const vector <char> &)> ("entity", callback);
}
/** 
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_web.http.on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const u_int, const string &, const unordered_multimap <string, string> &)> ("headers", callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для WebSocket/1.1
	this->_ws.http.on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_web.http.on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/2 и WebSocket/2
	this->_callback.set <void (const vector <char> &, const awh::http_t *)> ("chunking", callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const awh::core_t::status_t status, awh::core_t * core)> ("events", callback);
}
/**
 * sendTimeout Метод отправки сигнала таймаута
 */
void awh::client::WEB::sendTimeout() noexcept {
	// Определяем протокол клиента
	switch(static_cast <uint8_t> (this->_agent)){
		// Если агент является клиентом HTTP
		case static_cast <uint8_t> (agent_t::HTTP): {
			// Если подключение выполнено
			if(this->_core->working())
				// Отправляем сигнал принудительного таймаута
				const_cast <client::core_t *> (this->_core)->sendTimeout(this->_aid);
		} break;
		// Если агент является клиентом WebSocket
		case static_cast <uint8_t> (agent_t::WEBSOCKET): {
			// Если подключение выполнено
			if(this->_core->working() && this->_ws.allow.send)
				// Отправляем сигнал принудительного таймаута
				const_cast <client::core_t *> (this->_core)->sendTimeout(this->_aid);
		} break;
	}
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::WEB::sendError(const ws::mess_t & mess) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_ws.allow.send && (this->_aid > 0)){
		// Запрещаем получение данных
		this->_ws.allow.receive = false;
		// Получаем объект биндинга ядра TCP/IP
		client::core_t * core = const_cast <client::core_t *> (this->_core);
		// Выполняем остановку получения данных
		core->disabled(engine_t::method_t::READ, this->_aid);
		// Если код ошибки относится к WebSocket
		if(mess.code >= 1000){
			// Получаем буфер сообщения
			const auto & buffer = this->_ws.frame.methods.message(mess);
			// Если данные сообщения получены
			if((this->_stopped = !buffer.empty())){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим отправляемое сообщение
					cout << this->_fmk->format("%s [%u]", mess.text.c_str(), mess.code) << endl << endl;
				#endif
				// Если активирован режим работы с HTTP/2 протоколом
				if(this->_http2.mode){
					// Список файловых дескрипторов
					int fds[2];
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
						const int rv = _pipe(fds, 4096, O_BINARY);
					/**
					 * Для всех остальных операционных систем
					 */
					#else
						// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
						const int rv = ::pipe(fds);
					#endif
					// Выполняем подписку на основной канал передачи данных
					if(rv != 0){
						// Выводим в лог сообщение
						this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
						// Выполняем закрытие подключения
						core->close(this->_aid);
						// Выходим из функции
						return;
					}
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Если данные небыли записаны в сокет
						if(static_cast <int> (_write(fds[1], buffer.data(), buffer.size())) != static_cast <int> (buffer.size())){
							// Выполняем закрытие сокета для чтения
							_close(fds[0]);
							// Выполняем закрытие сокета для записи
							_close(fds[1]);
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выполняем закрытие подключения
							core->close(this->_aid);
							// Выходим из функции
							return;
						}
					/**
					 * Для всех остальных операционных систем
					 */
					#else
						// Если данные небыли записаны в сокет
						if(static_cast <int> (::write(fds[1], buffer.data(), buffer.size())) != static_cast <int> (buffer.size())){
							// Выполняем закрытие сокета для чтения
							::close(fds[0]);
							// Выполняем закрытие сокета для записи
							::close(fds[1]);
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выполняем закрытие подключения
							core->close(this->_aid);
							// Выходим из функции
							return;
						}
					#endif
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Выполняем закрытие подключения
						_close(fds[1]);
					/**
					 * Для всех остальных операционных систем
					 */
					#else
						// Выполняем закрытие подключения
						::close(fds[1]);
					#endif
					// Создаём объект передачи данных тела полезной нагрузки
					nghttp2_data_provider data;
					// Зануляем передаваемый контекст
					data.source.ptr = nullptr;
					// Устанавливаем файловый дескриптор
					data.source.fd = fds[0];
					// Устанавливаем функцию обратного вызова
					data.read_callback = &web_t::readHttp2;
					{
						// Результат фиксации сессии
						int rv = -1;
						// Выполняем формирование данных фрейма для отправки
						if((rv = nghttp2_submit_data(this->_http2.ctx, NGHTTP2_FLAG_END_STREAM, this->_http2.id, &data)) != 0){
							// Выводим сообщение об полученной ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
							// Выходим из функции
							return;
						}
						// Фиксируем отправленный результат
						if((rv = nghttp2_session_send(this->_http2.ctx)) != 0){
							// Выводим сообщение об полученной ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
							// Выходим из функции
							return;
						}
					}
				// Выполняем отправку сообщения на сервер
				} else core->write(buffer.data(), buffer.size(), this->_aid);
				// Выходим из функции
				return;
			}
		}
		// Завершаем работу
		const_cast <client::core_t *> (this->_core)->close(this->_aid);
	}
}
/**
 * send Метод отправки сообщения на сервер
 * @param reqs список запросов
 */
void awh::client::WEB::send(const vector <req_t> & reqs) noexcept {
	// Если список запросов установлен
	if(!reqs.empty() || !this->_web.requests.empty()){
		// Выполняем сброс параметров запроса
		this->flush();
		// Если объект списка запросов не существует
		if(this->_web.requests.empty())
			// Выполняем получение списка запросов
			this->_web.requests = std::forward <const vector <req_t>> (reqs);
		// Выполняем запрос на удалённый сервер
		this->submit(this->_web.requests.front());
	}
}
/**
 * send Метод отправки сообщения на сервер
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param utf8    данные передаются в текстовом виде
 */
void awh::client::WEB::send(const char * message, const size_t size, const bool utf8) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_ws.allow.send){
		// Выполняем блокировку отправки сообщения
		this->_ws.allow.send = !this->_ws.allow.send;
		// Если рукопожатие выполнено
		if((message != nullptr) && (size > 0) && this->_ws.http.isHandshake() && (this->_aid > 0)){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Если отправляемое сообщение является текстом
				if(utf8)
					// Выводим параметры ответа
					cout << string(message, size) << endl << endl;
				// Выводим сообщение о выводе чанка полезной нагрузки
				else cout << this->_fmk->format("<bytes %u>", size) << endl << endl;
			#endif
			// Буфер сжатых данных
			vector <char> buffer;
			// Создаём объект заголовка для отправки
			ws::frame_t::head_t head(true, true);
			// Если нужно производить шифрование
			if(this->_ws.crypt){
				// Выполняем шифрование переданных данных
				buffer = this->_ws.hash.encrypt(message, size);
				// Если данные зашифрованны
				if(!buffer.empty()){
					// Заменяем сообщение для передачи
					message = buffer.data();
					// Заменяем размер сообщения
					(* const_cast <size_t *> (&size)) = buffer.size();
				}
			}
			// Устанавливаем опкод сообщения
			head.optcode = (utf8 ? ws::frame_t::opcode_t::TEXT : ws::frame_t::opcode_t::BINARY);
			// Указываем, что сообщение передаётся в сжатом виде
			head.rsv[0] = ((size >= 1024) && (this->_compress != http_t::compress_t::NONE));
			// Если необходимо сжимать сообщение перед отправкой
			if(head.rsv[0]){
				// Компрессионные данные
				vector <char> data;
				// Определяем метод компрессии
				switch(static_cast <uint8_t> (this->_compress)){
					// Если метод компрессии выбран Deflate
					case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
						// Устанавливаем размер скользящего окна
						this->_ws.hash.wbit(this->_ws.client.wbit);
						// Выполняем компрессию полученных данных
						data = this->_ws.hash.compress(message, size, hash_t::method_t::DEFLATE);
						// Удаляем хвост в полученных данных
						this->_ws.hash.rmTail(data);
					} break;
					// Если метод компрессии выбран GZip
					case static_cast <uint8_t> (http_t::compress_t::GZIP):
						// Выполняем компрессию полученных данных
						data = this->_ws.hash.compress(message, size, hash_t::method_t::GZIP);
					break;
					// Если метод компрессии выбран Brotli
					case static_cast <uint8_t> (http_t::compress_t::BROTLI):
						// Выполняем компрессию полученных данных
						data = this->_ws.hash.compress(message, size, hash_t::method_t::BROTLI);
					break;
				}
				// Если сжатие данных прошло удачно
				if(!data.empty()){
					// Выполняем перемещение данных
					buffer = std::forward <vector <char>> (data);
					// Заменяем сообщение для передачи
					message = buffer.data();
					// Заменяем размер сообщения
					(* const_cast <size_t *> (&size)) = buffer.size();
				// Снимаем флаг сжатых данных
				} else head.rsv[0] = false;
			}
			// Если активирован режим работы с HTTP/2 протоколом
			if(this->_http2.mode){
				// Список файловых дескрипторов
				int fds[2];
				/**
				 * Методы только для OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
					const int rv = _pipe(fds, 4096, O_BINARY);
				/**
				 * Для всех остальных операционных систем
				 */
				#else
					// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
					const int rv = ::pipe(fds);
				#endif
				// Выполняем подписку на основной канал передачи данных
				if(rv != 0){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					// Выполняем закрытие подключения
					const_cast <client::core_t *> (this->_core)->close(this->_aid);
					// Выходим из функции
					return;
				}
				// Если требуется фрагментация сообщения
				if(size > this->_ws.frame.size){
					// Бинарный буфер чанка данных
					vector <char> chunk(this->_ws.frame.size);
					// Смещение в бинарном буфере
					size_t start = 0, stop = this->_ws.frame.size;
					// Выполняем разбивку полезной нагрузки на сегменты
					while(stop < size){
						// Увеличиваем длину чанка
						stop += this->_ws.frame.size;
						// Если длина чанка слишком большая, компенсируем
						stop = (stop > size ? size : stop);
						// Устанавливаем флаг финального сообщения
						head.fin = (stop == size);
						// Формируем чанк бинарных данных
						chunk.assign(message + start, message + stop);
						// Создаём буфер для отправки
						const auto & buffer = this->_ws.frame.methods.set(head, chunk.data(), chunk.size());
						// Если бинарный буфер для отправки данных получен
						if(!buffer.empty()){
							/**
							 * Методы только для OS Windows
							 */
							#if defined(_WIN32) || defined(_WIN64)
								// Если данные небыли записаны в сокет
								if(static_cast <int> (_write(fds[1], buffer.data(), buffer.size())) != static_cast <int> (buffer.size())){
									// Выполняем закрытие сокета для чтения
									_close(fds[0]);
									// Выполняем закрытие сокета для записи
									_close(fds[1]);
									// Выводим в лог сообщение
									this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
									// Выполняем закрытие подключения
									const_cast <client::core_t *> (this->_core)->close(this->_aid);
									// Выходим из функции
									return;
								}
							/**
							 * Для всех остальных операционных систем
							 */
							#else
								// Если данные небыли записаны в сокет
								if(static_cast <int> (::write(fds[1], buffer.data(), buffer.size())) != static_cast <int> (buffer.size())){
									// Выполняем закрытие сокета для чтения
									::close(fds[0]);
									// Выполняем закрытие сокета для записи
									::close(fds[1]);
									// Выводим в лог сообщение
									this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
									// Выполняем закрытие подключения
									const_cast <client::core_t *> (this->_core)->close(this->_aid);
									// Выходим из функции
									return;
								}
							#endif
						}
						// Выполняем сброс RSV1
						head.rsv[0] = false;
						// Устанавливаем опкод сообщения
						head.optcode = ws::frame_t::opcode_t::CONTINUATION;
						// Увеличиваем смещение в буфере
						start = stop;
					}
				// Если фрагментация сообщения не требуется
				} else {
					// Создаём буфер для отправки
					const auto & buffer = this->_ws.frame.methods.set(head, message, size);
					// Если бинарный буфер для отправки данных получен
					if(!buffer.empty()){
						/**
						 * Методы только для OS Windows
						 */
						#if defined(_WIN32) || defined(_WIN64)
							// Если данные небыли записаны в сокет
							if(static_cast <int> (_write(fds[1], buffer.data(), buffer.size())) != static_cast <int> (buffer.size())){
								// Выполняем закрытие сокета для чтения
								_close(fds[0]);
								// Выполняем закрытие сокета для записи
								_close(fds[1]);
								// Выводим в лог сообщение
								this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
								// Выполняем закрытие подключения
								const_cast <client::core_t *> (this->_core)->close(this->_aid);
								// Выходим из функции
								return;
							}
						/**
						 * Для всех остальных операционных систем
						 */
						#else
							// Если данные небыли записаны в сокет
							if(static_cast <int> (::write(fds[1], buffer.data(), buffer.size())) != static_cast <int> (buffer.size())){
								// Выполняем закрытие сокета для чтения
								::close(fds[0]);
								// Выполняем закрытие сокета для записи
								::close(fds[1]);
								// Выводим в лог сообщение
								this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
								// Выполняем закрытие подключения
								const_cast <client::core_t *> (this->_core)->close(this->_aid);
								// Выходим из функции
								return;
							}
						#endif
					}
				}
				/**
				 * Методы только для OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем закрытие подключения
					_close(fds[1]);
				/**
				 * Для всех остальных операционных систем
				 */
				#else
					// Выполняем закрытие подключения
					::close(fds[1]);
				#endif
				// Создаём объект передачи данных тела полезной нагрузки
				nghttp2_data_provider data;
				// Зануляем передаваемый контекст
				data.source.ptr = nullptr;
				// Устанавливаем файловый дескриптор
				data.source.fd = fds[0];
				// Устанавливаем функцию обратного вызова
				data.read_callback = &web_t::readHttp2;
				{
					// Результат фиксации сессии
					int rv = -1;
					// Выполняем формирование данных фрейма для отправки
					if((rv = nghttp2_submit_data(this->_http2.ctx, NGHTTP2_FLAG_NONE, this->_http2.id, &data)) != 0){
						// Выводим сообщение об полученной ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
						// Выходим из функции
						return;
					}
					// Фиксируем отправленный результат
					if((rv = nghttp2_session_send(this->_http2.ctx)) != 0){
						// Выводим сообщение об полученной ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
						// Выходим из функции
						return;
					}
				}
			// Если активирован режим работы с HTTP/1.1 протоколом
			} else {
				// Если требуется фрагментация сообщения
				if(size > this->_ws.frame.size){
					// Бинарный буфер чанка данных
					vector <char> chunk(this->_ws.frame.size);
					// Смещение в бинарном буфере
					size_t start = 0, stop = this->_ws.frame.size;
					// Выполняем разбивку полезной нагрузки на сегменты
					while(stop < size){
						// Увеличиваем длину чанка
						stop += this->_ws.frame.size;
						// Если длина чанка слишком большая, компенсируем
						stop = (stop > size ? size : stop);
						// Устанавливаем флаг финального сообщения
						head.fin = (stop == size);
						// Формируем чанк бинарных данных
						chunk.assign(message + start, message + stop);
						// Создаём буфер для отправки
						const auto & buffer = this->_ws.frame.methods.set(head, chunk.data(), chunk.size());
						// Если бинарный буфер для отправки данных получен
						if(!buffer.empty())
							// Отправляем серверу сообщение
							const_cast <client::core_t *> (this->_core)->write(buffer.data(), buffer.size(), this->_aid);
						// Выполняем сброс RSV1
						head.rsv[0] = false;
						// Устанавливаем опкод сообщения
						head.optcode = ws::frame_t::opcode_t::CONTINUATION;
						// Увеличиваем смещение в буфере
						start = stop;
					}
				// Если фрагментация сообщения не требуется
				} else {
					// Создаём буфер для отправки
					const auto & buffer = this->_ws.frame.methods.set(head, message, size);
					// Если бинарный буфер для отправки данных получен
					if(!buffer.empty())
						// Отправляем серверу сообщение
						const_cast <client::core_t *> (this->_core)->write(buffer.data(), buffer.size(), this->_aid);
				}
			}
		}
		// Выполняем разблокировку отправки сообщения
		this->_ws.allow.send = !this->_ws.allow.send;
	}
}
/**
 * setOrigin Метод установки списка разрешенных источников для HTTP/2
 * @param origins список разрешённых источников
 */
/*
void awh::client::WEB::setOrigin(const vector <string> & origins) noexcept {
	// Выполняем установку списка источников
	this->_origins = origins;
}
*/
/**
 * sendOrigin Метод отправки списка разрешенных источников для HTTP/2
 * @param origins список разрешённых источников
 */
/*
void awh::client::WEB::sendOrigin(const vector <string> & origins) noexcept {
	// Если сессия HTTP/2 активна
	if(this->_http2.mode && (this->_http2.ctx != nullptr)){
		// Список источников для установки на сервере
		vector <nghttp2_origin_entry> ov;
		// Если список источников передан
		if(!origins.empty()){
			// Выполняем перебор списка источников
			for(auto & origin : origins)
				// Выполняем добавление источника в списку
				ov.push_back({(uint8_t *) origin.c_str(), origin.size()});
		}
		// Результат выполнения поерации
		int rv = -1;
		// Выполняем установку фрейма полученных источников
		if((rv = nghttp2_submit_origin(this->_http2.ctx, NGHTTP2_FLAG_NONE, (!ov.empty() ? ov.data() : nullptr), ov.size())) != 0){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Выходим из функции
			return;
		}
		// Фиксируем отправленный результат
		if((rv = nghttp2_session_send(this->_http2.ctx)) != 0){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Выходим из функции
			return;
		}
	}
}
*/
/**
 * open Метод открытия подключения
 */
void awh::client::WEB::open() noexcept {
	// Если подключение уже выполнено
	if(this->_scheme.status.real == scheme_t::mode_t::CONNECT){
		// Если подключение производится через, прокси-сервер
		if(this->_scheme.isProxy())
			// Выполняем запуск функции подключения для прокси-сервера
			this->actionProxyConnect();
		// Выполняем запуск функции подключения
		else this->actionConnect();
	// Если биндинг уже запущен, выполняем запрос на сервер
	} else const_cast <client::core_t *> (this->_core)->open(this->_scheme.sid);
}
/**
 * stop Метод остановки клиента
 */
void awh::client::WEB::stop() noexcept {
	// Устанавливаем флаг принудительной остановки
	this->_active = true;
	// Если подключение выполнено
	if(this->_core->working()){
		// Выполняем очистку списка запросов
		this->_web.requests.clear();
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Завершаем работу, если разрешено остановить
		if(this->_unbind) const_cast <client::core_t *> (this->_core)->stop();
		// Если завершать работу запрещено, просто отключаемся
		else {
			/**
			 * Если установлено постоянное подключение
			 * нам нужно заблокировать автоматический реконнект.
			 */
			// Считываем значение флага
			const bool alive = this->_scheme.alive;
			// Выполняем отключение флага постоянного подключения
			this->_scheme.alive = false;
			// Выполняем отключение клиента
			const_cast <client::core_t *> (this->_core)->close(this->_aid);
			// Восстанавливаем предыдущее значение флага
			this->_scheme.alive = alive;
		}
	}
}
/**
 * start Метод запуска клиента
 */
void awh::client::WEB::start() noexcept {
	// Определяем протокол клиента
	switch(static_cast <uint8_t> (this->_agent)){
		// Если агент является клиентом HTTP
		case static_cast <uint8_t> (agent_t::HTTP): {
			// Если адрес URL запроса передан
			if(!this->_scheme.url.empty()){
				// Если биндинг не запущен
				if(!this->_core->working())
					// Выполняем запуск биндинга
					const_cast <client::core_t *> (this->_core)->start();
				// Если биндинг уже запущен, выполняем запрос на сервер
				else this->open();
			}
		} break;
		// Если агент является клиентом WebSocket
		case static_cast <uint8_t> (agent_t::WEBSOCKET): {
			// Если адрес URL запроса передан
			if(!this->_ws.freeze && !this->_scheme.url.empty()){
				// Если биндинг не запущен, выполняем запуск биндинга
				if(!this->_core->working())
					// Выполняем запуск биндинга
					const_cast <client::core_t *> (this->_core)->start();
				// Выполняем запрос на сервер
				else this->open();
			}
			// Снимаем с паузы клиент
			this->_ws.freeze = false;
		} break;
	}
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::WEB::pause() noexcept {
	// Ставим работу клиента на паузу
	this->_ws.freeze = true;
}
/**
 * sub Метод получения выбранного сабпротокола
 * @return выбранный сабпротокол
 */
const string & awh::client::WEB::sub() const noexcept {
	// Выводим выбранный сабпротокол
	return this->_ws.http.sub();
}
/**
 * sub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::client::WEB::sub(const string & sub) noexcept {
	// Устанавливаем подпротокол
	if(!sub.empty()) this->_ws.http.sub(sub);
}
/**
 * subs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::client::WEB::subs(const vector <string> & subs) noexcept {
	// Если список подпротоколов получен
	if(!subs.empty()) this->_ws.http.subs(subs);
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::client::WEB::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->_scheme.marker.read = read;
	// Устанавливаем количество байт на запись
	this->_scheme.marker.write = write;
	// Если минимальный размер данных для чтения, не установлен
	if(this->_scheme.marker.read.min == 0)
		// Устанавливаем размер минимальных для чтения данных по умолчанию
		this->_scheme.marker.read.min = BUFFER_READ_MIN;
	// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
	if(this->_scheme.marker.write.max == 0)
		// Устанавливаем размер максимальных записываемых данных по умолчанию
		this->_scheme.marker.write.max = BUFFER_WRITE_MAX;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::WEB::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
	// Устанавливаем количество секунд на подключение
	this->_scheme.timeouts.connect = connect;
}
/**
 * multiThreads Метод активации многопоточности
 * @param threads количество потоков для активации
 * @param mode    флаг активации/деактивации мультипоточности
 */
void awh::client::WEB::multiThreads(const size_t threads, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Если многопоточность ещё не активированна
		if(!this->_ws.threads.is())
			// Выполняем инициализацию пула потоков
			this->_ws.threads.init(threads);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->_ws.threads.wait();
			// Выполняем инициализацию нового тредпула
			this->_ws.threads.init(threads);
		}
		// Устанавливаем простое чтение базы событий
		const_cast <client::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->_ws.threads.wait();
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::WEB::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Если URI параметры переданы
	if(!uri.empty()){
		// Устанавливаем семейство интернет протоколов
		this->_scheme.proxy.family = family;
		// Устанавливаем параметры прокси-сервера
		this->_scheme.proxy.url = this->_web.uri.parse(uri);
		// Если данные параметров прокси-сервера получены
		if(!this->_scheme.proxy.url.empty()){
			// Если протокол подключения SOCKS5
			if(this->_fmk->compare(this->_scheme.proxy.url.schema, "socks5")){
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = proxy_t::type_t::SOCKS5;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.socks5.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			// Если протокол подключения HTTP
			} else if(this->_fmk->compare(this->_scheme.proxy.url.schema, "http") || this->_fmk->compare(this->_scheme.proxy.url.schema, "https")) {
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = proxy_t::type_t::HTTP;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.http.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			}
		}
	}
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::WEB::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->_web.http.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::client::WEB::segmentSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0) this->_ws.frame.size = size;
}
/**
 * attempts Метод установки общего количества попыток
 * @param attempts общее количество попыток
 */
void awh::client::WEB::attempts(const uint8_t attempts) noexcept {
	// Если количество попыток передано, устанавливаем его
	if(attempts > 0) this->_attempts = attempts;
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::WEB::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = (flags.count(flag_t::NOT_STOP) == 0);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.count(flag_t::ALIVE) > 0);
	// Устанавливаем флаг разрешающий выполнять редиректы
	this->_redirects = (flags.count(flag_t::REDIRECTS) > 0);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_ws.noinfo = (flags.count(flag_t::NOT_INFO) > 0);
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_ws.client.takeOver = (flags.count(flag_t::TAKEOVER_CLIENT) > 0);
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_ws.server.takeOver = (flags.count(flag_t::TAKEOVER_SERVER) > 0);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->_core)->noInfo(flags.count(flag_t::NOT_INFO) > 0);
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->_core)->verifySSL(flags.count(flag_t::VERIFY_SSL) > 0);
	// Если протокол подключения желательно установить HTTP/2
	if(this->_core->proto() == engine_t::proto_t::HTTP2)
		// Активируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(this->_scheme.alive);
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::WEB::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента
		this->_ws.http.userAgent(userAgent);
		// Устанавливаем пользовательского агента
		this->_web.http.userAgent(userAgent);
		// Устанавливаем пользовательского агента для прокси-сервера
		this->_scheme.proxy.http.userAgent(userAgent);
	}
}
/**
 * compress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::client::WEB::compress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_compress = compress;
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::WEB::user(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty()){
		// Устанавливаем логин и пароль пользователя
		this->_ws.http.user(login, password);
		// Устанавливаем логин и пароль пользователя
		this->_web.http.user(login, password);
	}
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::WEB::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::WEB::serv(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса
	this->_ws.http.serv(id, name, ver);
	// Устанавливаем данные сервиса
	this->_web.http.serv(id, name, ver);
	// Устанавливаем данные сервиса для прокси-сервера
	this->_scheme.proxy.http.serv(id, name, ver);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::WEB::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем флаг шифрования
	this->_ws.crypt = !pass.empty();
	// Устанавливаем соль шифрования
	this->_ws.hash.salt(salt);
	// Устанавливаем пароль шифрования
	this->_ws.hash.pass(pass);
	// Устанавливаем размер шифрования
	this->_ws.hash.cipher(cipher);
	// Если шифрование установлено
	if(this->_ws.crypt){
		// Устанавливаем параметры шифрования для WebSocket-клиента
		this->_ws.http.crypto(pass, salt, cipher);
		// Устанавливаем параметры шифрования для HTTP-клиента
		this->_web.http.crypto(pass, salt, cipher);
	}
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WEB::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->_ws.http.authType(type, hash);
	// Если объект авторизации создан
	this->_web.http.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WEB::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->_scheme.proxy.http.authType(type, hash);
}
/**
 * WEB Конструктор
 * @param agent идентификатор агента
 * @param core  объект сетевого ядра
 * @param fmk   объект фреймворка
 * @param log   объект для работы с логами
 */
awh::client::WEB::WEB(const agent_t agent, const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _aid(0), _unbind(true), _active(false), _stopped(false), _redirects(false),
 _attempt(0), _attempts(10), _web(fmk, log), _ws(fmk, log), _agent(agent),
 _action(action_t::NONE), _compress(awh::http_t::compress_t::NONE),
 _callback(log), _scheme(fmk, log), _fmk(fmk), _log(log), _core(core) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&web_t::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("persist", std::bind(&web_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&web_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&web_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем событие на подключение к прокси-серверу
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connectProxy", std::bind(&web_t::proxyConnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&web_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("write", std::bind(&web_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем событие на чтение данных с прокси-сервера
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("readProxy", std::bind(&web_t::proxyReadCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем событие на активацию шифрованного TLS канала
	this->_scheme.callback.set <bool (const uri_t::url_t &, const size_t, const size_t, awh::core_t *)> ("tls", std::bind(&web_t::enableTLSCallback, this, _1, _2, _3, _4));
	// Устанавливаем функцию обработки вызова для получения чанков для WebSocket-клиента
	this->_ws.http.on(std::bind(&web_t::chunking, this, _1, _2));
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_web.http.on(std::bind(&web_t::chunking, this, _1, _2));
	// Добавляем схемы сети в сетевое ядро
	const_cast <client::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем функцию активации ядра клиента
	const_cast <client::core_t *> (this->_core)->callback(std::bind(&web_t::eventsCallback, this, _1, _2));
	// Если агент является клиентом WebSocket
	if(this->_agent == agent_t::WEBSOCKET){
		// Активируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(true);
		// Активируем асинхронный режим работы
		const_cast <client::core_t *> (this->_core)->mode(client::core_t::mode_t::ASYNC);
	}
}
