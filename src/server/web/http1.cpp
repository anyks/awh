/**
 * @file: http1.cpp
 * @date: 2022-10-01
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
#include <server/web/http1.hpp>

/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Http1::connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём адъютанта
		this->_scheme.set(aid);
		// Получаем параметры подключения адъютанта
		web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Выполняем установку идентификатора объекта
			adj->http.id(aid);
			// Устанавливаем размер чанка
			adj->http.chunk(this->_chunkSize);
			// Устанавливаем метод компрессии поддерживаемый сервером
			adj->http.compress(this->_scheme.compress);
			// Устанавливаем данные сервиса
			adj->http.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
			// Если функция обратного вызова для обработки чанков установлена
			if(this->_callback.is("chunking"))
				// Устанавливаем функцию обработки вызова для получения чанков
				adj->http.on((function <void (const uint64_t, const vector <char> &, const awh::http_t *)>) std::bind(this->_callback.get <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking"), _1, _2, _3));
			// Устанавливаем функцию обработки вызова для получения чанков
			else adj->http.on(std::bind(&ws1_t::chunking, this, _1, _2, _3));
			// Если функция обратного вызова на полученного заголовка с клиента установлена
			if(this->_callback.is("header"))
				// Устанавливаем функцию обратного вызова для получения заголовков запроса
				adj->http.on((function <void (const uint64_t, const string &, const string &)>) std::bind(this->_callback.get <void (const int32_t, const uint64_t, const string &, const string &)> ("header"), aid, _1, _2, _3));
			// Если функция обратного вызова на вывод запроса клиента установлена
			if(this->_callback.is("request"))
				// Устанавливаем функцию обратного вызова для получения запроса клиента
				adj->http.on((function <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)>) std::bind(this->_callback.get <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request"), aid, _1, _2, _3));
			// Если функция обратного вызова на вывод полученных заголовков с клиента установлена
			if(this->_callback.is("headers"))
				// Устанавливаем функцию обратного вызова для получения всех заголовков запроса
				adj->http.on((function <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)>) std::bind(this->_callback.get <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers"), aid, _1, _2, _3, _4));
			// Если требуется установить параметры шифрования
			if(this->_crypto.mode)
				// Устанавливаем параметры шифрования
				adj->http.crypto(this->_crypto.pass, this->_crypto.salt, this->_crypto.cipher);
			// Определяем тип авторизации
			switch(static_cast <uint8_t> (this->_service.type)){
				// Если тип авторизации Basic
				case static_cast <uint8_t> (auth_t::type_t::BASIC): {
					// Устанавливаем параметры авторизации
					adj->http.authType(this->_service.type);
					// Если функция обратного вызова для обработки чанков установлена
					if(this->_callback.is("checkPassword"))
						// Устанавливаем функцию проверки авторизации
						adj->http.authCallback(std::bind(this->_callback.get <bool (const uint64_t, const string &, const string &)> ("checkPassword"), aid, _1, _2));
				} break;
				// Если тип авторизации Digest
				case static_cast <uint8_t> (auth_t::type_t::DIGEST): {
					// Устанавливаем название сервера
					adj->http.realm(this->_service.realm);
					// Устанавливаем временный ключ сессии сервера
					adj->http.opaque(this->_service.opaque);
					// Устанавливаем параметры авторизации
					adj->http.authType(this->_service.type, this->_service.hash);
					// Если функция обратного вызова для обработки чанков установлена
					if(this->_callback.is("extractPassword"))
						// Устанавливаем функцию извлечения пароля
						adj->http.extractPassCallback(std::bind(this->_callback.get <string (const uint64_t, const string &)> ("extractPassword"), aid, _1));
				} break;
			}
			// Если функция обратного вызова при подключении/отключении установлена
			if(this->_callback.is("active"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const mode_t> ("active", aid, mode_t::CONNECT);
		}
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении клиента
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Http1::disconnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Выполняем отключение подключившегося адъютанта
		this->disconnect(aid);
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выводим функцию обратного вызова
			this->_callback.call <const uint64_t, const mode_t> ("active", aid, mode_t::DISCONNECT);
	}
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с клиента
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Http1::readCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует WebSocket-у
		if((it != this->_agents.end()) && (it->second == agent_t::WEBSOCKET))
			// Выполняем передачу данных клиенту WebSocket
			this->_ws1.readCallback(buffer, size, aid, sid, core);
		// Иначе выполняем обработку входящих данных как Web-сервер
		else {
			// Получаем параметры подключения адъютанта
			web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Если параметры подключения адъютанта получены
			if(adj != nullptr){
				// Если подключение закрыто
				if(adj->close){
					// Принудительно выполняем отключение лкиента
					dynamic_cast <server::core_t *> (core)->close(aid);
					// Выходим из функции
					return;
				}
				// Добавляем полученные данные в буфер
				adj->buffer.insert(adj->buffer.end(), buffer, buffer + size);
				// Если функция обратного вызова активности потока установлена
				if(!adj->mode && (adj->mode = this->_callback.is("stream")))
					// Выводим функцию обратного вызова
					this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", 1, aid, mode_t::OPEN);
				// Выполняем обработку полученных данных
				while(!adj->close){
					// Выполняем парсинг полученных данных
					size_t bytes = adj->http.parse(adj->buffer.data(), adj->buffer.size());
					// Если все данные получены
					if(adj->http.isEnd()){
						// Если включён режим отладки
						#if defined(DEBUG_MODE)
							{
								// Получаем данные запроса
								const auto & request = adj->http.process(http_t::process_t::REQUEST, adj->http.request());
								// Если параметры запроса получены
								if(!request.empty()){
									// Выводим заголовок запроса
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
									// Выводим параметры запроса
									cout << string(request.begin(), request.end()) << endl << endl;
									// Если тело запроса существует
									if(!adj->http.body().empty())
										// Выводим сообщение о выводе чанка тела
										cout << this->_fmk->format("<body %u>", adj->http.body().size()) << endl << endl;
									// Иначе устанавливаем перенос строки
									else cout << endl;
								}
							}
						#endif
						// Если подключение не установлено как постоянное
						if(!this->_service.alive && !adj->alive){
							// Увеличиваем количество выполненных запросов
							adj->requests++;
							// Если количество выполненных запросов превышает максимальный
							if(adj->requests >= this->_maxRequests)
								// Устанавливаем флаг закрытия подключения
								adj->close = true;
							// Получаем текущий штамп времени
							else adj->point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
						// Выполняем сброс количества выполненных запросов
						} else adj->requests = 0;
						// Получаем флаг шифрованных данных
						adj->crypt = adj->http.isCrypt();
						// Получаем поддерживаемый метод компрессии
						adj->compress = adj->http.compress();
						// Выполняем проверку авторизации
						switch(static_cast <uint8_t> (adj->http.getAuth())){
							// Если запрос выполнен удачно
							case static_cast <uint8_t> (http_t::stath_t::GOOD): {
								// Если заголовок Upgrade установлен
								if(adj->http.isHeader("upgrade")){
									// Выполняем извлечение заголовка Upgrade
									const string & header = adj->http.header("upgrade");
									// Если запрашиваемый протокол соответствует WebSocket
									if(this->_webSocket && this->_fmk->compare(header, "websocket"))
										// Выполняем инициализацию WebSocket-сервера
										this->websocket(aid, sid, core);
									// Если протокол запрещён или не поддерживается
									else {
										// Выполняем сброс состояния HTTP парсера
										adj->http.clear();
										// Выполняем сброс состояния HTTP парсера
										adj->http.reset();
										// Выполняем очистку буфера полученных данных
										adj->buffer.clear();
										// Формируем запрос авторизации
										const auto & response = adj->http.reject(awh::web_t::res_t(static_cast <u_int> (500), "Requested protocol is not supported by this server"));
										// Если ответ получен
										if(!response.empty()){
											// Тело полезной нагрузки
											vector <char> payload;
											/**
											 * Если включён режим отладки
											 */
											#if defined(DEBUG_MODE)
												// Выводим заголовок ответа
												cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
												// Выводим параметры ответа
												cout << string(response.begin(), response.end()) << endl << endl;
											#endif
											// Отправляем ответ адъютанту
											dynamic_cast <server::core_t *> (core)->write(response.data(), response.size(), aid);
											// Получаем данные тела запроса
											while(!(payload = adj->http.payload()).empty()){
												/**
												 * Если включён режим отладки
												 */
												#if defined(DEBUG_MODE)
													// Выводим сообщение о выводе чанка полезной нагрузки
													cout << this->_fmk->format("<chunk %u>", payload.size()) << endl << endl;
												#endif
												// Если тела данных для отправки больше не осталось
												if(adj->http.body().empty())
													// Если подключение не установлено как постоянное, устанавливаем флаг завершения работы
													adj->stopped = (!this->_service.alive && !adj->alive);
												// Отправляем тело клиенту
												dynamic_cast <server::core_t *> (core)->write(payload.data(), payload.size(), aid);
											}
										// Выполняем отключение адъютанта
										} else dynamic_cast <server::core_t *> (core)->close(aid);
										// Если функция обратного вызова активности потока установлена
										if(this->_callback.is("stream"))
											// Выводим функцию обратного вызова
											this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", 1, aid, mode_t::CLOSE);
										// Если функция обратного вызова на на вывод ошибок установлена
										if(this->_callback.is("error"))
											// Выводим функцию обратного вызова
											this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "Requested protocol is not supported by this server");
										// Если установлена функция отлова завершения запроса
										if(this->_callback.is("end"))
											// Выводим функцию обратного вызова
											this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", 1, aid, direct_t::RECV);
									}
									// Завершаем обработку
									goto Next;
								}
								// Выполняем извлечение параметров запроса
								const auto & request = adj->http.request();
								// Если функция обратного вызова активности потока установлена
								if(this->_callback.is("stream"))
									// Выводим функцию обратного вызова
									this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", 1, aid, mode_t::CLOSE);
								// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
								if(!adj->http.body().empty() && this->_callback.is("entity"))
									// Выполняем функцию обратного вызова
									this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &> ("entity", 1, aid, request.method, request.url, adj->http.body());
								// Выполняем сброс состояния HTTP парсера
								adj->http.clear();
								// Выполняем сброс состояния HTTP парсера
								adj->http.reset();
								// Если функция обратного вызова на получение удачного запроса установлена
								if(this->_callback.is("handshake"))
									// Выполняем функцию обратного вызова
									this->_callback.call <const int32_t, const uint64_t, const agent_t> ("handshake", 1, aid, agent_t::HTTP);
								// Если установлена функция отлова завершения запроса
								if(this->_callback.is("end"))
									// Выводим функцию обратного вызова
									this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", 1, aid, direct_t::RECV);
								// Выполняем добавление агнета
								this->_agents.emplace(aid, agent_t::HTTP);
								// Завершаем обработку
								goto Next;
							} break;
							// Если запрос неудачный
							case static_cast <uint8_t> (http_t::stath_t::FAULT): {
								// Выполняем сброс состояния HTTP парсера
								adj->http.clear();
								// Выполняем сброс состояния HTTP парсера
								adj->http.reset();
								// Выполняем очистку буфера полученных данных
								adj->buffer.clear();
								// Формируем запрос авторизации
								const auto & response = adj->http.reject(awh::web_t::res_t(static_cast <u_int> (401)));
								// Если ответ получен
								if(!response.empty()){
									// Тело полезной нагрузки
									vector <char> payload;
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										// Выводим заголовок ответа
										cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
										// Выводим параметры ответа
										cout << string(response.begin(), response.end()) << endl << endl;
									#endif
									// Отправляем ответ адъютанту
									dynamic_cast <server::core_t *> (core)->write(response.data(), response.size(), aid);
									// Получаем данные тела запроса
									while(!(payload = adj->http.payload()).empty()){
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											// Выводим сообщение о выводе чанка полезной нагрузки
											cout << this->_fmk->format("<chunk %u>", payload.size()) << endl << endl;
										#endif
										// Отправляем тело клиенту
										dynamic_cast <server::core_t *> (core)->write(payload.data(), payload.size(), aid);
									}
								// Выполняем отключение адъютанта
								} else dynamic_cast <server::core_t *> (core)->close(aid);
								// Если функция обратного вызова активности потока установлена
								if(this->_callback.is("stream"))
									// Выводим функцию обратного вызова
									this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", 1, aid, mode_t::CLOSE);
								// Если функция обратного вызова на на вывод ошибок установлена
								if(this->_callback.is("error"))
									// Выводим функцию обратного вызова
									this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "authorization failed");
								// Если установлена функция отлова завершения запроса
								if(this->_callback.is("end"))
									// Выводим функцию обратного вызова
									this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", 1, aid, direct_t::RECV);
								// Выходим из функции
								return;
							}
						}
					}
					// Устанавливаем метку продолжения обработки пайплайна
					Next:
					// Если парсер обработал какое-то количество байт
					if((bytes > 0) && !adj->buffer.empty()){
						// Если размер буфера больше количества удаляемых байт
						if(adj->buffer.size() >= bytes)
							// Удаляем количество обработанных байт
							adj->buffer.assign(adj->buffer.begin() + bytes, adj->buffer.end());
						// Если байт в буфере меньше, просто очищаем буфер
						else adj->buffer.clear();
						// Если данных для обработки не осталось, выходим
						if(adj->buffer.empty()) break;
					// Если данных для обработки недостаточно, выходим
					} else break;
				}
			}
		}
	}
}
/**
 * writeCallback Функция обратного вызова при записи сообщение адъютанту
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Http1::writeCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует WebSocket-у
		if((it != this->_agents.end()) && (it->second == agent_t::WEBSOCKET))
			// Выполняем передачу данных клиенту WebSocket
			this->_ws1.writeCallback(buffer, size, aid, sid, core);
		// Иначе выполняем обработку входящих данных как Web-сервер
		else {
			// Получаем параметры подключения адъютанта
			web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Если параметры подключения адъютанта получены
			if(adj != nullptr){
				// Если необходимо выполнить закрыть подключение
				if(!adj->close && adj->stopped){
					// Устанавливаем флаг закрытия подключения
					adj->close = !adj->close;
					// Принудительно выполняем отключение лкиента
					const_cast <server::core_t *> (this->_core)->close(aid);
				}
			}
		}
	}
}
/**
 * websocket Метод инициализации WebSocket протокола
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Http1::websocket(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём адъютанта
		this->_ws1._scheme.set(aid);
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_ws1._scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если данные необходимо зашифровать
			if(this->_crypto.mode){
				// Устанавливаем соль шифрования
				adj->hash.salt(this->_crypto.salt);
				// Устанавливаем пароль шифрования
				adj->hash.pass(this->_crypto.pass);
				// Устанавливаем размер шифрования
				adj->hash.cipher(this->_crypto.cipher);
			}
			// Выполняем установку идентификатора объекта
			adj->http.id(aid);
			// Устанавливаем флаг перехвата контекста компрессии
			adj->server.takeover = this->_ws1._server.takeover;
			// Устанавливаем флаг перехвата контекста декомпрессии
			adj->client.takeover = this->_ws1._client.takeover;
			// Разрешаем перехватывать контекст компрессии
			adj->hash.takeoverCompress(this->_ws1._server.takeover);
			// Разрешаем перехватывать контекст декомпрессии
			adj->hash.takeoverDecompress(this->_ws1._client.takeover);
			// Разрешаем перехватывать контекст для клиента
			adj->http.takeover(awh::web_t::hid_t::CLIENT, this->_ws1._client.takeover);
			// Разрешаем перехватывать контекст для сервера
			adj->http.takeover(awh::web_t::hid_t::SERVER, this->_ws1._server.takeover);
			// Устанавливаем данные сервиса
			adj->http.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
			// Если сабпротоколы установлены
			if(!this->_ws1._subprotocols.empty())
				// Устанавливаем поддерживаемые сабпротоколы
				adj->http.subprotocols(this->_ws1._subprotocols);
			// Если список расширений установлены
			if(!this->_ws1._extensions.empty())
				// Устанавливаем список поддерживаемых расширений
				adj->http.extensions(this->_ws1._extensions);
			// Если размер фрейма установлен
			if(this->_ws1._frameSize > 0)
				// Выполняем установку размера фрейма
				adj->frame.size = this->_ws1._frameSize;
			// Устанавливаем метод компрессии поддерживаемый сервером
			adj->http.compress(this->_scheme.compress);
			// Если сервер требует авторизацию
			if(this->_service.type != auth_t::type_t::NONE){
				// Определяем тип авторизации
				switch(static_cast <uint8_t> (this->_service.type)){
					// Если тип авторизации Basic
					case static_cast <uint8_t> (auth_t::type_t::BASIC): {
						// Устанавливаем параметры авторизации
						adj->http.authType(this->_service.type);
						// Если функция обратного вызова для обработки чанков установлена
						if(this->_callback.is("checkPassword"))
							// Устанавливаем функцию проверки авторизации
							adj->http.authCallback(this->_callback.get <bool (const string &, const string &)> ("checkPassword"));
					} break;
					// Если тип авторизации Digest
					case static_cast <uint8_t> (auth_t::type_t::DIGEST): {
						// Устанавливаем название сервера
						adj->http.realm(this->_service.realm);
						// Устанавливаем временный ключ сессии сервера
						adj->http.opaque(this->_service.opaque);
						// Устанавливаем параметры авторизации
						adj->http.authType(this->_service.type, this->_service.hash);
						// Если функция обратного вызова для обработки чанков установлена
						if(this->_callback.is("extractPassword"))
							// Устанавливаем функцию извлечения пароля
							adj->http.extractPassCallback(this->_callback.get <string (const string &)> ("extractPassword"));
					} break;
				}
			}
			// Получаем параметры подключения адъютанта
			web_scheme_t::coffer_t * web = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Если параметры подключения адъютанта получены
			if(web != nullptr){
				// Буфер данных для записи в сокет
				vector <char> buffer;
				// Выполняем установку параметров запроса
				adj->http.request(web->http.request());
				// Выполняем установку полученных заголовков
				adj->http.headers(web->http.headers());
				// Выполняем коммит полученного результата
				adj->http.commit();
				// Метод компрессии данных
				http_t::compress_t compress = http_t::compress_t::NONE;
				// Если рукопожатие выполнено
				if(adj->http.isHandshake(http_t::process_t::REQUEST)){
					// Получаем метод компрессии HTML данных
					compress = adj->http.compression();
					// Проверяем версию протокола
					if(!adj->http.checkVer()){
						// Получаем бинарные данные REST ответа
						buffer = web->http.reject(awh::web_t::res_t(static_cast <u_int> (400), "Unsupported protocol version"));
						// Завершаем работу
						goto End;
					}
					// Проверяем ключ адъютанта
					if(!adj->http.checkKey()){
						// Получаем бинарные данные REST ответа
						buffer = web->http.reject(awh::web_t::res_t(static_cast <u_int> (400), "Wrong client key"));
						// Завершаем работу
						goto End;
					}
					// Выполняем сброс состояния HTTP-парсера
					adj->http.clear();
					// Получаем флаг шифрованных данных
					adj->crypt = adj->http.isCrypt();
					// Если клиент согласился на шифрование данных
					if(adj->crypt)
						// Устанавливаем параметры шифрования
						adj->http.crypto(this->_crypto.pass, this->_crypto.salt, this->_crypto.cipher);
					// Получаем поддерживаемый метод компрессии
					adj->compress = adj->http.compress();
					// Получаем размер скользящего окна сервера
					adj->server.wbit = adj->http.wbit(awh::web_t::hid_t::SERVER);
					// Получаем размер скользящего окна клиента
					adj->client.wbit = adj->http.wbit(awh::web_t::hid_t::CLIENT);
					// Если разрешено выполнять перехват контекста компрессии для сервера
					if(adj->http.takeover(awh::web_t::hid_t::SERVER))
						// Разрешаем перехватывать контекст компрессии для клиента
						adj->hash.takeoverCompress(true);
					// Если разрешено выполнять перехват контекста компрессии для клиента
					if(adj->http.takeover(awh::web_t::hid_t::CLIENT))
						// Разрешаем перехватывать контекст компрессии для сервера
						adj->hash.takeoverDecompress(true);
					// Если заголовки для передаче клиенту установлены
					if(!this->_ws1._headers.empty())
						// Выполняем установку HTTP-заголовков
						adj->http.headers(this->_ws1._headers);
					// Получаем бинарные данные REST-ответа клиенту
					buffer = adj->http.process(http_t::process_t::RESPONSE, awh::web_t::res_t(static_cast <u_int> (101)));
					// Если бинарные данные ответа получены
					if(!buffer.empty()){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры ответа
							cout << string(buffer.begin(), buffer.end()) << endl << endl;
						#endif
						// Выполняем установку сетевого ядра
						this->_ws1._core = dynamic_cast <server::core_t *> (core);
						// Выполняем отправку данных адъютанту
						dynamic_cast <server::core_t *> (core)->write(buffer.data(), buffer.size(), aid);
						// Выполняем извлечение параметров запроса
						const auto & request = adj->http.request();
						// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
						if(!adj->http.body().empty() && this->_callback.is("entity"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &> ("entity", adj->sid, aid, request.method, request.url, adj->http.body());
						// Если функция обратного вызова активности потока установлена
						if(this->_callback.is("stream"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", adj->sid, aid, mode_t::OPEN);
						// Если функция обратного вызова на получение удачного запроса установлена
						if(this->_callback.is("handshake"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const int32_t, const uint64_t, const agent_t> ("handshake", adj->sid, aid, agent_t::WEBSOCKET);
						// Если установлена функция отлова завершения запроса
						if(this->_callback.is("end"))
							// Выводим функцию обратного вызова
							this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", adj->sid, aid, direct_t::SEND);
						// Выполняем добавление агнета
						this->_agents.emplace(aid, agent_t::WEBSOCKET);
						// Завершаем работу
						return;
					// Формируем ответ, что страница не доступна
					} else buffer = web->http.reject(awh::web_t::res_t(static_cast <u_int> (500)));
				// Сообщаем, что рукопожатие не выполнено
				} else buffer = web->http.reject(awh::web_t::res_t(static_cast <u_int> (403), "Handshake failed"));
				// Устанавливаем метку завершения запроса
				End:
				// Если бинарные данные запроса получены, отправляем клиенту
				if(!buffer.empty()){
					// Тело полезной нагрузки
					vector <char> payload;
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
						// Выводим параметры ответа
						cout << string(buffer.begin(), buffer.end()) << endl << endl;
					#endif
					// Устанавливаем метод компрессии данных ответа
					web->http.compress(compress);
					// Выполняем извлечение параметров запроса
					const auto & request = adj->http.request();
					// Получаем параметры ответа
					const auto response = adj->http.response();
					// Выполняем отправку заголовков сообщения
					dynamic_cast <server::core_t *> (core)->write(buffer.data(), buffer.size(), aid);
					// Получаем данные тела ответа
					while(!(payload = web->http.payload()).empty()){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение о выводе чанка полезной нагрузки
							cout << this->_fmk->format("<chunk %u>", payload.size()) << endl << endl;
						#endif
						// Устанавливаем флаг закрытия подключения
						adj->stopped = (!web->http.isAlive() && web->http.body().empty());
						// Выполняем отправку чанков
						dynamic_cast <server::core_t *> (core)->write(payload.data(), payload.size(), aid);
					}
					// Если получение данных нужно остановить
					if(adj->stopped)
						// Выполняем запрет на получение входящих данных
						dynamic_cast <server::core_t *> (core)->disabled(engine_t::method_t::READ, aid);
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(!web->http.body().empty() && this->_callback.is("entity"))
						// Выполняем функцию обратного вызова
						this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &> ("entity", adj->sid, aid, request.method, request.url, web->http.body());
					// Если функция обратного вызова активности потока установлена
					if(this->_callback.is("stream"))
						// Выполняем функцию обратного вызова
						this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", adj->sid, aid, mode_t::CLOSE);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, response.message);
					// Если установлена функция отлова завершения запроса
					if(this->_callback.is("end")){
						// Выводим функцию обратного вызова
						this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", adj->sid, aid, direct_t::RECV);
						// Выводим функцию обратного вызова
						this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", adj->sid, aid, direct_t::SEND);
					}
					// Выполняем сброс состояния HTTP парсера
					adj->http.clear();
					// Выполняем сброс состояния HTTP парсера
					adj->http.reset();
					// Выполняем очистку буфера данных
					adj->buffer.payload.clear();
					// Завершаем работу
					return;
				}
				// Завершаем работу
				dynamic_cast <server::core_t *> (core)->close(aid);
			}
		}
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Http1::persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует WebSocket-у
		if((it != this->_agents.end()) && (it->second == agent_t::WEBSOCKET))
			// Выполняем передачу данных клиенту WebSocket
			this->_ws1.persistCallback(aid, sid, core);
		// Иначе выполняем обработку входящих данных как Web-сервер
		else {
			// Получаем параметры подключения адъютанта
			web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Если параметры подключения адъютанта получены
			if((adj != nullptr) && ((!adj->alive && !this->_service.alive) || adj->close)){
				// Если адъютант давно должен был быть отключён, отключаем его
				if(adj->close || !adj->http.isAlive())
					// Выполняем отключение клиента от сервера
					dynamic_cast <server::core_t *> (core)->close(aid);
				// Иначе проверяем прошедшее время
				else {
					// Получаем текущий штамп времени
					const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
					if((stamp - adj->point) >= this->_timeAlive)
						// Завершаем работу
						dynamic_cast <server::core_t *> (core)->close(aid);
				}
			}
		}
	}
}
/**
 * erase Метод удаления отключившихся адъютантов
 * @param aid идентификатор адъютанта
 */
void awh::server::Http1::erase(const uint64_t aid) noexcept {
	// Если список отключившихся адъютантов не пустой
	if(!this->_disconected.empty()){
		/**
		 * eraseFn Функция удаления отключившегося адъютанта
		 * @param aid идентификатор адъютанта
		 */
		auto eraseFn = [this](const uint64_t aid) noexcept -> void {
			// Выполняем поиск агента которому соответствует клиент
			auto it = this->_agents.find(aid);
			// Если агент найден в списке активных агентов
			if(it != this->_agents.end()){
				// Если агент соответствует серверу WebSocket
				if(it->second == agent_t::WEBSOCKET)
					// Выполняем удаление отключённого адъютанта
					this->_ws1.erase(it->first);
				// Выполняем удаление активного агента
				this->_agents.erase(it);
			}
			// Получаем параметры подключения адъютанта
			web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Если параметры подключения адъютанта получены
			if(adj != nullptr){
				// Устанавливаем флаг отключения
				adj->close = true;
				// Выполняем очистку оставшихся данных
				adj->buffer.clear();
			}
			// Выполняем удаление параметров адъютанта
			this->_scheme.rm(aid);
		};
		// Получаем текущее значение времени
		const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
		// Если идентификатор адъютанта передан
		if(aid > 0){
			// Выполняем поиск указанного адъютанта
			auto it = this->_disconected.find(aid);
			// Если данные отключившегося адъютанта найдены
			if((it != this->_disconected.end()) && ((date - it->second) >= 10000)){
				// Выполняем удаление отключившегося адъютанта
				eraseFn(it->first);
				// Выполняем удаление адъютанта
				this->_disconected.erase(it);
			}
		// Если идентификатор адъютанта не передан
		} else {
			// Выполняем переход по всему списку отключившихся адъютантов
			for(auto it = this->_disconected.begin(); it != this->_disconected.end();){
				// Если адъютант уже давно отключился
				if((date - it->second) >= 10000){
					// Выполняем удаление отключившегося адъютанта
					eraseFn(it->first);
					// Выполняем удаление объекта адъютантов из списка отключившихся
					it = this->_disconected.erase(it);
				// Выполняем пропуск адъютанта
				} else ++it;
			}
		}
	}
}
/**
 * init Метод инициализации WEB-сервера
 * @param socket   unix-сокет для биндинга
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Http1::init(const string & socket, const http_t::compress_t compress) noexcept {
	// Устанавливаем тип компрессии
	this->_scheme.compress = compress;
	// Выполняем инициализацию родительского объекта
	web_t::init(socket, compress);
}
/**
 * init Метод инициализации WEB-сервера
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Http1::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
	// Устанавливаем тип компрессии
	this->_scheme.compress = compress;
	// Выполняем инициализацию родительского объекта
	web_t::init(port, host, compress);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param aid  идентификатор адъютанта
 * @param mess отправляемое сообщение об ошибке
 */
void awh::server::Http1::sendError(const uint64_t aid, const ws::mess_t & mess) noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует WebSocket-у
		if((it != this->_agents.end()) && (it->second == agent_t::WEBSOCKET))
			// Выполняем отправку ошибки клиенту WebSocket
			this->_ws1.sendError(aid, mess);
	}
}
/**
 * sendMessage Метод отправки сообщения клиенту
 * @param aid     идентификатор адъютанта
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 */
void awh::server::Http1::sendMessage(const uint64_t aid, const vector <char> & message, const bool text) noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует WebSocket-у
		if((it != this->_agents.end()) && (it->second == agent_t::WEBSOCKET))
			// Выполняем передачу данных клиенту WebSocket
			this->_ws1.sendMessage(aid, message, text);
	}
}
/**
 * send Метод отправки тела сообщения клиенту
 * @param aid    идентификатор адъютанта
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::Http1::send(const uint64_t aid, const char * buffer, const size_t size, const bool end) noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы верные
	if((result = (this->_core->working() && (buffer != nullptr) && (size > 0)))){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует HTTP-протоколу
		if((it == this->_agents.end()) || (it->second == agent_t::HTTP)){
			// Получаем параметры подключения адъютанта
			web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Если параметры подключения адъютанта получены
			if(adj != nullptr){
				// Тело WEB сообщения
				vector <char> entity;
				// Выполняем сброс данных тела
				adj->http.clearBody();
				// Устанавливаем тело запроса
				adj->http.body(vector <char> (buffer, buffer + size));
				// Получаем данные тела запроса
				while(!(entity = adj->http.payload()).empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка тела
						cout << this->_fmk->format("<chunk %u>", entity.size()) << endl << endl;
					#endif
					// Устанавливаем флаг закрытия подключения
					adj->stopped = (end && adj->http.body().empty());
					// Выполняем отправку тела запроса на сервер
					const_cast <server::core_t *> (this->_core)->write(entity.data(), entity.size(), aid);
				}
			}
		}
	}
	// Выводим значение по умолчанию
	return result;
}
/**
 * send Метод отправки заголовков клиенту
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для адъютанта
 * @param mess    отправляемое сообщение об ошибке
 * @param headers заголовки отправляемые клиенту
 * @param end     размер сообщения в байтах
 * @return        идентификатор нового запроса
 */
int32_t awh::server::Http1::send(const uint64_t aid, const u_int code, const string & mess, const unordered_multimap <string, string> & headers, const bool end) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если заголовки запроса переданы
	if((result = (this->_core->working() && !headers.empty()))){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует HTTP-протоколу
		if((it == this->_agents.end()) || (it->second == agent_t::HTTP)){
			// Получаем параметры подключения адъютанта
			web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Если параметры подключения адъютанта получены
			if(adj != nullptr){
				// Выполняем очистку параметров HTTP запроса
				adj->http.clear();
				// Устанавливаем заголовоки запроса
				adj->http.headers(headers);
				// Если сообщение ответа не установлено
				if(mess.empty())
					// Выполняем установку сообщения по умолчанию
					const_cast <string &> (mess) = adj->http.message(code);
				// Формируем ответ на запрос клиента
				awh::web_t::res_t response(code, mess);
				// Получаем заголовки ответа удалённому клиенту
				const auto & headers = adj->http.process(http_t::process_t::RESPONSE, response);
				// Если заголовки запроса получены
				if(!headers.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим заголовок запроса
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
						// Выводим параметры запроса
						cout << string(headers.begin(), headers.end()) << endl << endl;
					#endif
					// Устанавливаем флаг закрытия подключения
					adj->stopped = end;
					// Выполняем отправку заголовков запроса на сервер
					const_cast <server::core_t *> (this->_core)->write(headers.data(), headers.size(), aid);
					// Устанавливаем результат
					result = 1;
				}
			}
		}
	}
	// Выводим значение по умолчанию
	return result;
}
/**
 * send Метод отправки сообщения адъютанту
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для адъютанта
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Http1::send(const uint64_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto it = this->_agents.find(aid);
		// Если агент соответствует HTTP-протоколу
		if((it == this->_agents.end()) || (it->second == agent_t::HTTP)){
			// Получаем параметры подключения адъютанта
			web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Если параметры подключения адъютанта получены
			if(adj != nullptr){
				// Тело полезной нагрузки
				vector <char> payload;
				// Устанавливаем полезную нагрузку
				adj->http.body(entity);
				// Устанавливаем заголовки ответа
				adj->http.headers(headers);
				// Если подключение не установлено как постоянное, но подключение долгоживущее
				if(!this->_service.alive && !adj->alive && adj->http.isAlive())
					// Указываем сколько запросов разрешено выполнить за указанный интервал времени
					adj->http.header("Keep-Alive", this->_fmk->format("timeout=%d, max=%d", this->_timeAlive / 1000, this->_maxRequests));
				// Если сообщение ответа не установлено
				if(mess.empty())
					// Выполняем установку сообщения по умолчанию
					const_cast <string &> (mess) = adj->http.message(code);
				// Формируем запрос авторизации
				const auto & response = adj->http.process(http_t::process_t::RESPONSE, awh::web_t::res_t(static_cast <u_int> (code), mess));
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры ответа
					cout << string(response.begin(), response.end()) << endl << endl;
				#endif
				// Если тело данных не установлено для отправки
				if(adj->http.body().empty())
					// Если подключение не установлено как постоянное, устанавливаем флаг завершения работы
					adj->stopped = (!this->_service.alive && !adj->alive);
				// Отправляем серверу сообщение
				const_cast <server::core_t *> (this->_core)->write(response.data(), response.size(), aid);
				// Получаем данные полезной нагрузки ответа
				while(!(payload = adj->http.payload()).empty()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка полезной нагрузки
						cout << this->_fmk->format("<chunk %u>", payload.size()) << endl << endl;
					#endif
					// Если тела данных для отправки больше не осталось
					if(adj->http.body().empty())
						// Если подключение не установлено как постоянное, устанавливаем флаг завершения работы
						adj->stopped = (!this->_service.alive && !adj->alive);
					// Отправляем тело клиенту
					const_cast <server::core_t *> (this->_core)->write(payload.data(), payload.size(), aid);
				}
				// Если установлена функция отлова завершения запроса
				if(this->_callback.is("end"))
					// Выводим функцию обратного вызова
					this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", 1, aid, direct_t::SEND);
			}
		}
	}
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const uint64_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для извлечения пароля
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <string (const uint64_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <bool (const uint64_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const uint64_t, const vector <char> &, const bool)> callback) noexcept {
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const int32_t, const uint64_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функция обратного вызова при выполнении рукопожатия
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const int32_t, const uint64_t, const agent_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const int32_t, const uint64_t, const direct_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const int32_t, const uint64_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const int32_t, const uint64_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода запроса клиента к серверу
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http1::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * port Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    порт подключения адъютанта
 */
u_int awh::server::Http1::port(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getPort(aid);
}
/**
 * agent Метод извлечения агента клиента
 * @param aid идентификатор адъютанта
 * @return    агент к которому относится подключённый клиент
 */
awh::server::web_t::agent_t awh::server::Http1::agent(const uint64_t aid) const noexcept {
	// Выполняем поиск нужного нам агента
	auto it = this->_agents.find(aid);
	// Если агент клиента найден
	if(it != this->_agents.end())
		// Выводим идентификатор агента
		return it->second;
	// Выводим сообщение, что ничего не найдено
	return agent_t::HTTP;
}
/**
 * ip Метод получения IP-адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::Http1::ip(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getIp(aid);
}
/**
 * mac Метод получения MAC-адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::Http1::mac(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getMac(aid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Http1::stop() noexcept {
	// Выполняем остановку работы основного модуля
	web_t::stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::Http1::start() noexcept {
	// Если биндинг не запущен
	if(!this->_core->working())
		// Выполняем запуск биндинга
		const_cast <server::core_t *> (this->_core)->start();
	// Если биндинг уже запущен, выполняем запуск
	else this->openCallback(this->_scheme.sid, dynamic_cast <awh::core_t *> (const_cast <server::core_t *> (this->_core)));
}
/**
 * close Метод закрытия подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::Http1::close(const uint64_t aid) noexcept {
	// Выполняем поиск агента которому соответствует клиент
	auto it = this->_agents.find(aid);
	// Если агент соответствует WebSocket-у
	if((it != this->_agents.end()) && (it->second == agent_t::WEBSOCKET))
		// Выполняем закрытие подключения клиента WebSocket
		this->_ws1.close(aid);
	// Иначе выполняем обработку входящих данных как Web-сервер
	else {
		// Получаем параметры подключения адъютанта
		web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
		if(adj != nullptr){
			// Устанавливаем флаг закрытия подключения адъютанта
			adj->close = true;
			// Выполняем отключение адъютанта
			const_cast <server::core_t *> (this->_core)->close(aid);
		}
	}
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::server::Http1::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку сабпротокола
	this->_ws1.subprotocol(subprotocol);
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::server::Http1::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку сабпротоколов
	this->_ws1.subprotocols(subprotocols);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @param aid идентификатор адъютанта
 * @return    список выбранных сабпротоколов
 */
const set <string> & awh::server::Http1::subprotocols(const uint64_t aid) const noexcept {
	// Выполняем извлечение выбранных сабпротоколов
	return this->_ws1.subprotocols(aid);
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::server::Http1::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку список поддерживаемых расширений
	this->_ws1.extensions(extensions);
}
/**
 * extensions Метод извлечения списка расширений
 * @param aid идентификатор адъютанта
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::Http1::extensions(const uint64_t aid) const noexcept {
	// Выполняем извлечение списка поддерживаемых расширений
	return this->_ws1.extensions(aid);
}
/**
 * multiThreads Метод активации многопоточности
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::server::Http1::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Если многопоточность ещё не активированна
		if(!this->_ws1._thr.is())
			// Выполняем инициализацию пула потоков
			this->_ws1._thr.init(count);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->_ws1._thr.wait();
			// Выполняем инициализацию нового тредпула
			this->_ws1._thr.init(count);
		}
		// Если сетевое ядро установлено
		if(this->_core != nullptr)
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->_ws1._thr.wait();
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Http1::total(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	const_cast <server::core_t *> (this->_core)->total(this->_scheme.sid, total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::Http1::segmentSize(const size_t size) noexcept {
	// Выполняем установку размеров сегмента фрейма
	this->_ws1.segmentSize(size);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::Http1::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага автоматического перезапуска
	const_cast <server::core_t *> (this->_core)->clusterAutoRestart(this->_scheme.sid, mode);
}
/**
 * compress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::Http1::compress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_scheme.compress = compress;
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Http1::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::server::Http1::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для WebSocket-сервера
	this->_ws1.mode(flags);
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = (flags.count(flag_t::NOT_STOP) == 0);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.count(flag_t::ALIVE) > 0);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг разрешающий выполнять подключение к протоколу WebSocket
	this->_webSocket = (flags.count(flag_t::WEBSOCKET_ENABLE) > 0);
	// Если сетевое ядро установлено
	if(this->_core != nullptr){
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <server::core_t *> (this->_core)->noInfo(flags.count(flag_t::NOT_INFO) > 0);
		// Выполняем установку флага проверки домена
		const_cast <server::core_t *> (this->_core)->verifySSL(flags.count(flag_t::VERIFY_SSL) > 0);
	}
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Http1::alive(const bool mode) noexcept {
	// Выполняем установку долгоживущего подключения
	web_t::alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Http1::alive(const time_t time) noexcept {
	// Выполняем установку времени жизни подключения
	web_t::alive(time);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Http1::alive(const uint64_t aid, const bool mode) noexcept {
	// Получаем параметры подключения адъютанта
	web_scheme_t::coffer_t * adj = const_cast <web_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr)
		// Устанавливаем флаг пдолгоживущего подключения
		adj->alive = mode;
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::server::Http1::core(const server::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
		// Активируем персистентный запуск для работы пингов
		const_cast <server::core_t *> (this->_core)->persistEnable(true);
		// Добавляем схемы сети в сетевое ядро
		const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
		// Устанавливаем функцию активации ядра сервера
		const_cast <server::core_t *> (this->_core)->on(std::bind(&http1_t::eventsCallback, this, _1, _2));
		// Если многопоточность активированна
		if(this->_ws1._thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_ws1._thr.is()){
			// Выполняем завершение всех активных потоков
			this->_ws1._thr.wait();
			// Снимаем режим простого чтения базы событий
			const_cast <server::core_t *> (this->_core)->easily(false);
		}
		// Деактивируем персистентный запуск для работы пингов
		const_cast <server::core_t *> (this->_core)->persistEnable(false);
		// Удаляем схему сети из сетевого ядра
		const_cast <server::core_t *> (this->_core)->remove(this->_scheme.sid);
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
	}
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Http1::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::Http1::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
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
 * Http1 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Http1::Http1(const fmk_t * fmk, const log_t * log) noexcept : web_t(fmk, log), _webSocket(false), _ws1(fmk, log), _scheme(fmk, log) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&http1_t::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&http1_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&http1_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&http1_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&http1_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&http1_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта адъютанта
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&http1_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
/**
 * Http1 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Http1::Http1(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : web_t(core, fmk, log), _webSocket(false), _ws1(fmk, log), _scheme(fmk, log) {
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&http1_t::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&http1_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&http1_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&http1_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&http1_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&http1_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта адъютанта
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&http1_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
