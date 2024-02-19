/**
 * @file: ws1.cpp
 * @date: 2022-10-03
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
#include <server/web/ws1.hpp>

/**
 * connectEvents Метод обратного вызова при подключении к серверу
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Websocket1::connectEvents(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём брокера
		this->_scheme.set(bid);
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Выполняем установку идентификатора объекта
			options->http.id(bid);
			// Устанавливаем размер чанка
			options->http.chunk(this->_chunkSize);
			// Устанавливаем флаг шифрования
			options->http.encryption(this->_encryption.mode);
			// Устанавливаем флаг перехвата контекста компрессии
			options->server.takeover = this->_server.takeover;
			// Устанавливаем флаг перехвата контекста декомпрессии
			options->client.takeover = this->_client.takeover;
			// Устанавливаем список компрессоров поддерживаемый сервером
			options->http.compressors(this->_scheme.compressors);
			// Разрешаем перехватывать контекст компрессии
			options->hash.takeoverCompress(this->_server.takeover);
			// Разрешаем перехватывать контекст декомпрессии
			options->hash.takeoverDecompress(this->_client.takeover);
			// Разрешаем перехватывать контекст для клиента
			options->http.takeover(awh::web_t::hid_t::CLIENT, this->_client.takeover);
			// Разрешаем перехватывать контекст для сервера
			options->http.takeover(awh::web_t::hid_t::SERVER, this->_server.takeover);
			// Устанавливаем данные сервиса
			options->http.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
			// Если сабпротоколы установлены
			if(!this->_subprotocols.empty())
				// Устанавливаем поддерживаемые сабпротоколы
				options->http.subprotocols(this->_subprotocols);
			// Если список расширений установлены
			if(!this->_extensions.empty())
				// Устанавливаем список поддерживаемых расширений
				options->http.extensions(this->_extensions);
			// Если размер фрейма установлен
			if(this->_frameSize > 0)
				// Выполняем установку размера фрейма
				options->frame.size = this->_frameSize;
			// Если функция обратного вызова для обработки чанков установлена
			if(!this->_callbacks.is("chunking"))
				// Устанавливаем функцию обработки вызова для получения чанков
				options->http.callback <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", std::bind(&ws1_t::chunking, this, _1, _2, _3));
			// Устанавливаем функцию обработки вызова для получения чанков
			else options->http.callback <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", this->_callbacks.get <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking"));
			// Если функция обратного вызова на полученного заголовка с клиента установлена
			if(this->_callbacks.is("header"))
				// Устанавливаем функцию обратного вызова для получения заголовков запроса
				options->http.callback <void (const uint64_t, const string &, const string &)> ("header", std::bind(this->_callbacks.get <void (const int32_t, const uint64_t, const string &, const string &)> ("header"), 1, _1, _2, _3));
			// Если функция обратного вызова на вывод запроса клиента установлена
			if(this->_callbacks.is("request"))
				// Устанавливаем функцию обратного вызова для получения запроса клиента
				options->http.callback <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request", std::bind(this->_callbacks.get <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request"), 1, _1, _2, _3));
			// Если функция обратного вызова на вывод полученных заголовков с клиента установлена
			if(this->_callbacks.is("headers"))
				// Устанавливаем функцию обратного вызова для получения всех заголовков запроса
				options->http.callback <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headersRequest", std::bind(this->_callbacks.get <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers"), 1, _1, _2, _3, _4));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callbacks.is("error"))
				// Устанавливаем функцию обратного вызова для вывода ошибок
				options->http.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_callbacks.get <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error"));
			// Если сервер требует авторизацию
			if(this->_service.type != auth_t::type_t::NONE){
				// Определяем тип авторизации
				switch(static_cast <uint8_t> (this->_service.type)){
					// Если тип авторизации Basic
					case static_cast <uint8_t> (auth_t::type_t::BASIC): {
						// Устанавливаем параметры авторизации
						options->http.authType(this->_service.type);
						// Если функция обратного вызова для обработки чанков установлена
						if(this->_callbacks.is("checkPassword"))
							// Устанавливаем функцию проверки авторизации
							options->http.authCallback(std::bind(this->_callbacks.get <bool (const uint64_t, const string &, const string &)> ("checkPassword"), bid, _1, _2));
					} break;
					// Если тип авторизации Digest
					case static_cast <uint8_t> (auth_t::type_t::DIGEST): {
						// Устанавливаем название сервера
						options->http.realm(this->_service.realm);
						// Устанавливаем временный ключ сессии сервера
						options->http.opaque(this->_service.opaque);
						// Устанавливаем параметры авторизации
						options->http.authType(this->_service.type, this->_service.hash);
						// Если функция обратного вызова для обработки чанков установлена
						if(this->_callbacks.is("extractPassword"))
							// Устанавливаем функцию извлечения пароля
							options->http.extractPassCallback(std::bind(this->_callbacks.get <string (const uint64_t, const string &)> ("extractPassword"), bid, _1));
					} break;
				}
			}
			// Если функция обратного вызова при подключении/отключении установлена
			if(this->_callbacks.is("active"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const mode_t)> ("active", bid, mode_t::CONNECT);
		}
	}
}
/**
 * disconnectEvents Метод обратного вызова при отключении клиента
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Websocket1::disconnectEvents(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Выполняем отключение подключившегося брокера
		this->disconnect(bid);
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callbacks.is("active"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const uint64_t, const mode_t)> ("active", bid, mode_t::DISCONNECT);
	}
}
/**
 * readEvents Метод обратного вызова при чтении сообщения с клиента
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Websocket1::readEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Флаг выполнения обработки полученных данных
		bool process = false;
		// Если установлена функция обратного вызова для вывода данных в сыром виде
		if(!(process = !this->_callbacks.is("raw")))
			// Выполняем функцию обратного вызова
			process = this->_callbacks.call <bool (const uint64_t, const char *, const size_t)> ("raw", bid, buffer, size);
		// Если обработка полученных данных разрешена
		if(process){
			// Получаем параметры активного клиента
			scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Если подключение закрыто
				if(options->close){
					// Принудительно выполняем отключение лкиента
					dynamic_cast <server::core_t *> (core)->close(bid);
					// Выходим из функции
					return;
				}
				// Если разрешено получение данных
				if(options->allow.receive){
					// Добавляем полученные данные в буфер
					options->buffer.payload.insert(options->buffer.payload.end(), buffer, buffer + size);
					// Если рукопожатие не выполнено
					if(!reinterpret_cast <http_t &> (options->http).is(http_t::state_t::HANDSHAKE)){
						// Выполняем парсинг полученных данных
						const size_t bytes = options->http.parse(options->buffer.payload.data(), options->buffer.payload.size());
						// Если парсер обработал какое-то количество байт
						if((bytes > 0) && !options->buffer.payload.empty()){
							// Если размер буфера больше количества удаляемых байт
							if(options->buffer.payload.size() >= bytes)
								// Удаляем количество обработанных байт
								// options->buffer.payload.assign(options->buffer.payload.begin() + bytes, options->buffer.payload.end());
								vector <decltype(options->buffer.payload)::value_type> (options->buffer.payload.begin() + bytes, options->buffer.payload.end()).swap(options->buffer.payload);
							// Если байт в буфере меньше, просто очищаем буфер
							else options->buffer.payload.clear();
						}
						// Если все данные получены
						if(options->http.is(http_t::state_t::END)){
							// Буфер данных для записи в сокет
							vector <char> buffer;
							// Выполняем создание объекта для генерации HTTP-ответа
							http_t http(this->_fmk, this->_log);
							// Если подключение не постоянное
							if(!options->http.is(http_t::state_t::ALIVE))
								// Устанавливаем правила закрытия подключения
								http.header("Сonnection", "close");
							// Устанавливаем метод компрессии данных ответа
							http.compression(options->http.compression());
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Получаем объект работы с HTTP-запросами
									const http_t & http = reinterpret_cast <http_t &> (options->http);
									// Получаем данные запроса
									const auto & request = http.process(http_t::process_t::REQUEST, http.request());
									// Если параметры запроса получены
									if(!request.empty()){
										// Выводим заголовок запроса
										cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
										// Выводим параметры запроса
										cout << string(request.begin(), request.end()) << endl;
										// Если тело запроса существует
										if(!http.empty(awh::http_t::suite_t::BODY))
											// Выводим сообщение о выводе чанка тела
											cout << this->_fmk->format("<body %u>", http.body().size()) << endl << endl;
										// Иначе устанавливаем перенос строки
										else cout << endl;
									}
								}
							#endif
							// Если выполняется запрос метода CONNECT
							if(options->http.request().method == awh::web_t::method_t::CONNECT){
								// Получаем бинарные данные REST ответа
								buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (505), "Requested protocol is not supported by this server"));
								// Выполняем запрет работы подключения
								goto Reject;
							}
							// Выполняем проверку авторизации
							switch(static_cast <uint8_t> (options->http.auth())){
								// Если запрос выполнен удачно
								case static_cast <uint8_t> (http_t::status_t::GOOD): {
									// Если рукопожатие выполнено
									if(options->http.handshake(http_t::process_t::REQUEST)){
										// Проверяем версию протокола
										if(!options->http.check(ws_core_t::flag_t::VERSION)){
											// Получаем бинарные данные REST ответа
											buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (505), "Unsupported protocol version"));
											// Завершаем работу
											break;
										}
										// Проверяем ключ брокера
										if(!options->http.check(ws_core_t::flag_t::KEY)){
											// Получаем бинарные данные REST ответа
											buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (400), "Wrong client key"));
											// Завершаем работу
											break;
										}
										// Выполняем очистку HTTP-парсера
										options->http.clear();
										// Получаем флаг шифрованных данных
										options->crypted = options->http.crypted();
										// Если клиент согласился на шифрование данных
										if(this->_encryption.mode){
											// Устанавливаем соль шифрования
											options->hash.salt(this->_encryption.salt);
											// Устанавливаем пароль шифрования
											options->hash.pass(this->_encryption.pass);
											// Устанавливаем размер шифрования
											options->hash.cipher(this->_encryption.cipher);
											// Устанавливаем параметры шифрования
											options->http.encryption(this->_encryption.pass, this->_encryption.salt, this->_encryption.cipher);
										}
										// Получаем поддерживаемый метод компрессии
										options->compress = options->http.compression();
										// Получаем размер скользящего окна сервера
										options->server.wbit = options->http.wbit(awh::web_t::hid_t::SERVER);
										// Получаем размер скользящего окна клиента
										options->client.wbit = options->http.wbit(awh::web_t::hid_t::CLIENT);
										// Если разрешено выполнять перехват контекста компрессии для сервера
										if(options->http.takeover(awh::web_t::hid_t::SERVER))
											// Разрешаем перехватывать контекст компрессии для клиента
											options->hash.takeoverCompress(true);
										// Если разрешено выполнять перехват контекста компрессии для клиента
										if(options->http.takeover(awh::web_t::hid_t::CLIENT))
											// Разрешаем перехватывать контекст компрессии для сервера
											options->hash.takeoverDecompress(true);
										// Если заголовки для передаче клиенту установлены
										if(!this->_headers.empty())
											// Выполняем установку HTTP-заголовков
											options->http.headers(this->_headers);
										// Получаем бинарные данные REST-ответа клиенту
										buffer = options->http.process(http_t::process_t::RESPONSE, awh::web_t::res_t(static_cast <u_int> (101)));
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
											// Выполняем отправку данных брокеру
											dynamic_cast <server::core_t *> (core)->write(buffer.data(), buffer.size(), bid);
											// Выполняем извлечение параметров запроса
											const auto & request = options->http.request();
											// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
											if(!options->http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", options->sid, bid, request.method, request.url, options->http.body());
											// Если функция обратного вызова на вывод полученных данных запроса клиента установлена
											if(this->_callbacks.is("complete"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", options->sid, bid, request.method, request.url, options->http.body(), options->http.headers());
											// Если функция обратного вызова активности потока установлена
											if(this->_callbacks.is("stream"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", options->sid, bid, mode_t::OPEN);
											// Если функция обратного вызова на получение удачного запроса установлена
											if(this->_callbacks.is("handshake"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const agent_t)> ("handshake", options->sid, bid, agent_t::WEBSOCKET);
											// Если установлена функция отлова завершения запроса
											if(this->_callbacks.is("end"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", options->sid, bid, direct_t::SEND);
											// Завершаем работу
											return;
										// Формируем ответ, что страница не доступна
										} else buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (500)));
									// Сообщаем, что рукопожатие не выполнено
									} else buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (403), "Handshake failed"));
								} break;
								// Если запрос неудачный
								case static_cast <uint8_t> (http_t::status_t::FAULT): {
									// Если сервер требует авторизацию
									if(this->_service.type != auth_t::type_t::NONE){
										// Выполняем удаление заголовка закрытия подключения
										http.rm(http_t::suite_t::HEADER, "Сonnection");
										// Определяем тип авторизации
										switch(static_cast <uint8_t> (this->_service.type)){
											// Если тип авторизации Basic
											case static_cast <uint8_t> (auth_t::type_t::BASIC): {
												// Устанавливаем параметры авторизации
												http.authType(this->_service.type);
												// Если функция обратного вызова для обработки чанков установлена
												if(this->_callbacks.is("checkPassword"))
													// Устанавливаем функцию проверки авторизации
													http.authCallback(std::bind(this->_callbacks.get <bool (const uint64_t, const string &, const string &)> ("checkPassword"), bid, _1, _2));
											} break;
											// Если тип авторизации Digest
											case static_cast <uint8_t> (auth_t::type_t::DIGEST): {
												// Устанавливаем название сервера
												http.realm(this->_service.realm);
												// Устанавливаем временный ключ сессии сервера
												http.opaque(this->_service.opaque);
												// Устанавливаем параметры авторизации
												http.authType(this->_service.type, this->_service.hash);
												// Если функция обратного вызова для обработки чанков установлена
												if(this->_callbacks.is("extractPassword"))
													// Устанавливаем функцию извлечения пароля
													http.extractPassCallback(std::bind(this->_callbacks.get <string (const uint64_t, const string &)> ("extractPassword"), bid, _1));
											} break;
										}
									}
									// Формируем запрос авторизации
									buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (401)));
								} break;
								// Если результат определить не получилось
								default: buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (506), "Unknown request"));
							}
							// Устанавливаем метку запрета протокола
							Reject:
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
								// Выполняем извлечение параметров запроса
								const auto & request = options->http.request();
								// Получаем параметры ответа
								const auto response = options->http.response();
								// Выполняем отправку заголовков сообщения
								dynamic_cast <server::core_t *> (core)->write(buffer.data(), buffer.size(), bid);
								// Получаем данные тела ответа
								while(!(payload = http.payload()).empty()){
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										// Выводим сообщение о выводе чанка полезной нагрузки
										cout << this->_fmk->format("<chunk %zu>", payload.size()) << endl << endl;
									#endif
									// Устанавливаем флаг закрытия подключения
									options->stopped = (!http.is(http_t::state_t::ALIVE) && http.empty(awh::http_t::suite_t::BODY));
									// Выполняем отправку чанков
									dynamic_cast <server::core_t *> (core)->write(payload.data(), payload.size(), bid);
								}
								// Если получение данных нужно остановить
								if(options->stopped)
									// Выполняем запрет на получение входящих данных
									dynamic_cast <server::core_t *> (core)->events(core_t::mode_t::DISABLED, engine_t::method_t::READ, bid);
								// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
								if(!options->http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", options->sid, bid, request.method, request.url, options->http.body());
								// Если функция обратного вызова на вывод полученных данных запроса клиента установлена
								if(this->_callbacks.is("complete"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", options->sid, bid, request.method, request.url, options->http.body(), options->http.headers());
								// Если функция обратного вызова активности потока установлена
								if(this->_callbacks.is("stream"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", options->sid, bid, mode_t::CLOSE);
								// Если функция обратного вызова на на вывод ошибок установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, response.message);
								// Если установлена функция отлова завершения запроса
								if(this->_callbacks.is("end")){
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", options->sid, bid, direct_t::RECV);
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", options->sid, bid, direct_t::SEND);
								}
								// Выполняем очистку HTTP-парсера
								options->http.clear();
								// Выполняем сброс состояния HTTP-парсера
								options->http.reset();
								// Выполняем очистку буфера данных
								options->buffer.payload.clear();
								// Завершаем работу
								return;
							}
							// Завершаем работу
							dynamic_cast <server::core_t *> (core)->close(bid);
						}
						// Завершаем работу
						return;
					// Если рукопожатие выполнено
					} else if(options->allow.receive) {
						// Флаг удачного получения данных
						bool receive = false;
						// Создаём буфер сообщения
						vector <char> buffer;
						// Создаём объект шапки фрейма
						ws::frame_t::head_t head;
						// Выполняем обработку полученных данных
						while(!options->close && options->allow.receive){
							// Выполняем чтение фрейма Websocket
							const auto & data = options->frame.methods.get(head, options->buffer.payload.data(), options->buffer.payload.size());
							// Если буфер данных получен
							if(!data.empty()){
								// Проверяем состояние флагов RSV2 и RSV3
								if(head.rsv[1] || head.rsv[2]){
									// Создаём сообщение
									options->mess = ws::mess_t(1002, "RSV2 and RSV3 must be clear");
									// Выполняем отключение брокера
									goto Stop;
								}
								// Если флаг компресси включён а данные пришли не сжатые
								if(head.rsv[0] && ((options->compress == http_t::compress_t::NONE) ||
								  (head.optcode == ws::frame_t::opcode_t::CONTINUATION) ||
								  ((static_cast <uint8_t> (head.optcode) > 0x07) && (static_cast <uint8_t> (head.optcode) < 0x0b)))){
									// Создаём сообщение
									options->mess = ws::mess_t(1002, "RSV1 must be clear");
									// Выполняем отключение брокера
									goto Stop;
								}
								// Если опкоды требуют финального фрейма
								if(!head.fin && (static_cast <uint8_t> (head.optcode) > 0x07) && (static_cast <uint8_t> (head.optcode) < 0x0b)){
									// Создаём сообщение
									options->mess = ws::mess_t(1002, "FIN must be set");
									// Выполняем отключение брокера
									goto Stop;
								}
								// Определяем тип ответа
								switch(static_cast <uint8_t> (head.optcode)){
									// Если ответом является PING
									case static_cast <uint8_t> (ws::frame_t::opcode_t::PING):
										// Отправляем ответ брокеру
										this->pong(bid, core, string(data.begin(), data.end()));
									break;
									// Если ответом является PONG
									case static_cast <uint8_t> (ws::frame_t::opcode_t::PONG):
										// Если идентификатор брокера совпадает
										if(::memcmp(::to_string(bid).c_str(), data.data(), data.size()) == 0)
											// Обновляем контрольную точку
											options->point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
									break;
									// Если ответом является TEXT
									case static_cast <uint8_t> (ws::frame_t::opcode_t::TEXT):
									// Если ответом является BINARY
									case static_cast <uint8_t> (ws::frame_t::opcode_t::BINARY): {
										// Запоминаем полученный опкод
										options->frame.opcode = head.optcode;
										// Запоминаем, что данные пришли сжатыми
										options->inflate = (head.rsv[0] && (options->compress != http_t::compress_t::NONE));
										// Если сообщение не замаскированно
										if(!head.mask){
											// Создаём сообщение
											options->mess = ws::mess_t(1002, "Not masked frame from client");
											// Выполняем отключение брокера
											goto Stop;
										// Если список фрагментированных сообщений существует
										} else if(!options->buffer.fragmes.empty()) {
											// Очищаем список фрагментированных сообщений
											options->buffer.fragmes.clear();
											// Создаём сообщение
											options->mess = ws::mess_t(1002, "Opcode for subsequent fragmented messages should not be set");
											// Выполняем отключение брокера
											goto Stop;
										// Если сообщение является не последнем
										} else if(!head.fin)
											// Заполняем фрагментированное сообщение
											options->buffer.fragmes.insert(options->buffer.fragmes.end(), data.begin(), data.end());
										// Если сообщение является последним
										else buffer = std::forward <const vector <char>> (data);
									} break;
									// Если ответом является CONTINUATION
									case static_cast <uint8_t> (ws::frame_t::opcode_t::CONTINUATION): {
										// Заполняем фрагментированное сообщение
										options->buffer.fragmes.insert(options->buffer.fragmes.end(), data.begin(), data.end());
										// Если сообщение является последним
										if(head.fin){
											// Выполняем копирование всех собранных сегментов
											buffer = std::forward <const vector <char>> (options->buffer.fragmes);
											// Очищаем список фрагментированных сообщений
											options->buffer.fragmes.clear();
										}
									} break;
									// Если ответом является CLOSE
									case static_cast <uint8_t> (ws::frame_t::opcode_t::CLOSE): {
										// Создаём сообщение
										options->mess = options->frame.methods.message(data);
										// Выводим сообщение об ошибке
										this->error(bid, options->mess);
										// Завершаем работу
										dynamic_cast <server::core_t *> (core)->close(bid);
										// Если функция обратного вызова активности потока установлена
										if(this->_callbacks.is("stream"))
											// Выполняем функцию обратного вызова
											this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", options->sid, bid, mode_t::CLOSE);
										// Выходим из функции
										return;
									} break;
								}
							}
							// Если парсер обработал какое-то количество байт
							if((receive = ((head.frame > 0) && !options->buffer.payload.empty()))){
								// Если размер буфера больше количества удаляемых байт
								if((receive = (options->buffer.payload.size() >= head.frame)))
									// Удаляем количество обработанных байт
									// options->buffer.payload.assign(options->buffer.payload.begin() + head.frame, options->buffer.payload.end());
									vector <decltype(options->buffer.payload)::value_type> (options->buffer.payload.begin() + head.frame, options->buffer.payload.end()).swap(options->buffer.payload);
							}
							// Если сообщения получены
							if(!buffer.empty()){
								// Если тредпул активирован
								if(this->_thr.is())
									// Добавляем в тредпул новую задачу на извлечение полученных сообщений
									this->_thr.push(std::bind(&ws1_t::extraction, this, bid, buffer, (options->frame.opcode == ws::frame_t::opcode_t::TEXT)));
								// Если тредпул не активирован, выполняем извлечение полученных сообщений
								else this->extraction(bid, buffer, (options->frame.opcode == ws::frame_t::opcode_t::TEXT));
								// Очищаем буфер полученного сообщения
								buffer.clear();
							}
							// Если данные мы все получили, выходим
							if(!receive || options->buffer.payload.empty()) break;
						}
						// Выходим из функции
						return;
					}
					// Устанавливаем метку остановки брокера
					Stop:
					// Отправляем серверу сообщение
					this->sendError(bid, options->mess);
					// Если функция обратного вызова активности потока установлена
					if(this->_callbacks.is("stream"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", options->sid, bid, mode_t::CLOSE);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callbacks.is("error"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", bid, log_t::flag_t::WARNING, http::error_t::WEBSOCKET, this->_fmk->format("%s [%u]", options->mess.code, options->mess.text.c_str()));
					// Если установлена функция отлова завершения запроса
					if(this->_callbacks.is("end"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", options->sid, bid, direct_t::RECV);
				}
			}
		}
	}
}
/**
 * writeEvents Метод обратного вызова при записи сообщение брокеру
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Websocket1::writeEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если необходимо выполнить закрыть подключение
			if(!options->close && options->stopped){
				// Устанавливаем флаг закрытия подключения
				options->close = !options->close;
				// Принудительно выполняем отключение лкиента
				dynamic_cast <server::core_t *> (core)->close(bid);
			}
		}
	}
}
/**
 * error Метод вывода сообщений об ошибках работы брокера
 * @param bid     идентификатор брокера
 * @param message сообщение с описанием ошибки
 */
void awh::server::Websocket1::error(const uint64_t bid, const ws::mess_t & message) const noexcept {
	// Получаем параметры активного клиента
	scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Очищаем список буффер бинарных данных
		options->buffer.payload.clear();
		// Очищаем список фрагментированных сообщений
		options->buffer.fragmes.clear();
	}
	// Если идентификатор брокера передан и код ошибки указан
	if((bid > 0) && (message.code > 0)){
		// Если сообщение об ошибке пришло
		if(!message.text.empty()){
			// Если тип сообщения получен
			if(!message.type.empty())
				// Выводим в лог сообщение
				this->_log->print("%s - %s [%u]", log_t::flag_t::WARNING, message.type.c_str(), message.text.c_str(), message.code);
			// Иначе выводим сообщение в упрощёном виде
			else this->_log->print("%s [%u]", log_t::flag_t::WARNING, message.text.c_str(), message.code);
			// Если функция обратного вызова при получении ошибки Websocket установлена
			if(this->_callbacks.is("errorWebsocket"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const u_int, const string &)> ("errorWebsocket", bid, message.code, message.text);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", bid, log_t::flag_t::WARNING, http::error_t::WEBSOCKET, this->_fmk->format("%s [%u]", message.code, message.text.c_str()));
		}
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param bid    идентификатор брокера
 * @param buffer данные в чистом виде полученные с сервера
 * @param text   данные передаются в текстовом виде
 */
void awh::server::Websocket1::extraction(const uint64_t bid, const vector <char> & buffer, const bool text) noexcept {
	// Если буфер данных передан
	if((bid > 0) && !buffer.empty() && this->_callbacks.is("messageWebsocket")){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Выполняем блокировку потока	
			const lock_guard <recursive_mutex> lock(options->mtx);
			// Если данные пришли в сжатом виде
			if(options->inflate && (options->compress != http_t::compress_t::NONE)){
				// Декомпрессионные данные
				vector <char> data;
				// Определяем метод компрессии
				switch(static_cast <uint8_t> (options->compress)){
					// Если метод компрессии выбран LZ4
					case static_cast <uint8_t> (http_t::compress_t::LZ4):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::LZ4);
					break;
					// Если метод компрессии выбран Zstandard
					case static_cast <uint8_t> (http_t::compress_t::ZSTD):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::ZSTD);
					break;
					// Если метод компрессии выбран LZma
					case static_cast <uint8_t> (http_t::compress_t::LZMA):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::LZMA);
					break;
					// Если метод компрессии выбран Brotli
					case static_cast <uint8_t> (http_t::compress_t::BROTLI):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BROTLI);
					break;
					// Если метод компрессии выбран BZip2
					case static_cast <uint8_t> (http_t::compress_t::BZIP2):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BZIP2);
					break;
					// Если метод компрессии выбран GZip
					case static_cast <uint8_t> (http_t::compress_t::GZIP):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::GZIP);
					break;
					// Если метод компрессии выбран Deflate
					case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
						// Устанавливаем размер скользящего окна
						options->hash.wbit(options->client.wbit);
						// Добавляем хвост в полученные данные
						options->hash.setTail(* const_cast <vector <char> *> (&buffer));
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
					} break;
				}
				// Если данные получены
				if(!data.empty()){
					// Если нужно производить дешифрование
					if(options->crypted){
						// Выполняем шифрование переданных данных
						const auto & res = options->hash.decrypt(data.data(), data.size());
						// Отправляем полученный результат
						if(!res.empty()){
							// Выводим данные полученного сообщения
							this->_callbacks.call <void (const uint64_t, const vector <char> &, const bool)> ("messageWebsocket", bid, res, text);
							// Выходим из функции
							return;
						}
					}
					// Выводим сообщение так - как оно пришло
					this->_callbacks.call <void (const uint64_t, const vector <char> &, const bool)> ("messageWebsocket", bid, data, text);
				// Выводим сообщение об ошибке
				} else if(this->_core != nullptr) {
					// Создаём сообщение
					options->mess = ws::mess_t(1007, "Received data decompression error");
					// Получаем буфер сообщения
					data = options->frame.methods.message(options->mess);
					// Если данные сообщения получены
					if((options->stopped = !data.empty())){
						// Выполняем запрет на получение входящих данных
						const_cast <server::core_t *> (this->_core)->events(core_t::mode_t::DISABLED, engine_t::method_t::READ, bid);
						// Выполняем отправку сообщения брокеру
						const_cast <server::core_t *> (this->_core)->write(data.data(), data.size(), bid);
					// Завершаем работу
					} else const_cast <server::core_t *> (this->_core)->close(bid);
				}
			// Если функция обратного вызова установлена, выводим полученное сообщение
			} else {
				// Если нужно производить дешифрование
				if(options->crypted){
					// Выполняем шифрование переданных данных
					const auto & res = options->hash.decrypt(buffer.data(), buffer.size());
					// Отправляем полученный результат
					if(!res.empty()){
						// Отправляем полученный результат
						this->_callbacks.call <void (const uint64_t, const vector <char> &, const bool)> ("messageWebsocket", bid, res, text);
						// Выходим из функции
						return;
					}
				}
				// Выводим сообщение так - как оно пришло
				this->_callbacks.call <void (const uint64_t, const vector <char> &, const bool)> ("messageWebsocket", bid, buffer, text);
			}
		}
	}
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param bid  идентификатор брокера
 * @param core объект сетевого ядра
 * @param      message сообщение для отправки
 */
void awh::server::Websocket1::pong(const uint64_t bid, awh::core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((bid > 0) && (core != nullptr)){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если отправка сообщений разблокированна
		if((options != nullptr) && options->allow.send){
			// Создаём буфер для отправки
			const auto & buffer = options->frame.methods.pong(message);
			// Если буфер данных получен
			if(!buffer.empty())
				// Выполняем отправку сообщения брокеру
				dynamic_cast <server::core_t *> (core)->write(buffer.data(), buffer.size(), bid);
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param bid  идентификатор брокера
 * @param core объект сетевого ядра
 * @param      message сообщение для отправки
 */
void awh::server::Websocket1::ping(const uint64_t bid, awh::core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((bid > 0) && (core != nullptr) && core->working()){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если отправка сообщений разблокированна
		if((options != nullptr) && options->allow.send){
			// Создаём буфер для отправки
			const auto & buffer = options->frame.methods.ping(message);
			// Если буфер данных получен
			if(!buffer.empty())
				// Выполняем отправку сообщения брокеру
				dynamic_cast <server::core_t *> (core)->write(buffer.data(), buffer.size(), bid);
		}
	}
}
/**
 * erase Метод удаления отключившихся брокеров
 * @param bid идентификатор брокера
 */
void awh::server::Websocket1::erase(const uint64_t bid) noexcept {
	// Если список отключившихся брокеров не пустой
	if(!this->_disconected.empty()){
		/**
		 * eraseFn Функция удаления отключившегося брокера
		 * @param bid идентификатор брокера
		 */
		auto eraseFn = [this](const uint64_t bid) noexcept -> void {
			// Получаем параметры активного клиента
			scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Устанавливаем флаг отключения
				options->close = true;
				// Выполняем очистку оставшихся данных
				options->buffer.payload.clear();
				// Выполняем очистку оставшихся фрагментов
				options->buffer.fragmes.clear();
			}
			// Выполняем удаление параметров брокера
			this->_scheme.rm(bid);
		};
		// Получаем текущее значение времени
		const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
		// Если идентификатор брокера передан
		if(bid > 0){
			// Выполняем поиск указанного брокера
			auto it = this->_disconected.find(bid);
			// Если данные отключившегося брокера найдены
			if((it != this->_disconected.end()) && ((date - it->second) >= 5000)){
				// Если установлена функция детекции удаление брокера сообщений установлена
				if(this->_callbacks.is("erase"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint64_t)> ("erase", bid);
				// Выполняем удаление отключившегося брокера
				eraseFn(it->first);
				// Выполняем удаление брокера
				this->_disconected.erase(it);
			}
		// Если идентификатор брокера не передан
		} else {
			// Выполняем переход по всему списку отключившихся брокеров
			for(auto it = this->_disconected.begin(); it != this->_disconected.end();){
				// Если брокер уже давно отключился
				if((date - it->second) >= 5000){
					// Если установлена функция детекции удаление брокера сообщений установлена
					if(this->_callbacks.is("erase"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint64_t)> ("erase", it->first);
					// Выполняем удаление отключившегося брокера
					eraseFn(it->first);
					// Выполняем удаление объекта брокеров из списка отключившихся
					it = this->_disconected.erase(it);
				// Выполняем пропуск брокера
				} else ++it;
			}
		}
	}
}
/**
 * pinging Метод таймера выполнения пинга клиента
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::Websocket1::pinging(const uint16_t tid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((tid > 0) && (core != nullptr) && (this->_core != nullptr)){
		// Выполняем перебор всех активных клиентов
		for(auto & item : this->_scheme.get()){
			// Получаем текущий штамп времени
			const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
			// Если брокер не ответил на пинг больше двух интервалов, отключаем его
			if(item.second->close || ((stamp - item.second->point) >= (PING_INTERVAL * 5)))
				// Завершаем работу
				const_cast <server::core_t *> (this->_core)->close(item.first);
			// Отправляем запрос брокеру
			else this->ping(item.first, const_cast <server::core_t *> (this->_core), ::to_string(item.first));
		}
	}
}
/**
 * init Метод инициализации Websocket-сервера
 * @param socket      unix-сокет для биндинга
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Websocket1::init(const string & socket, const vector <http_t::compress_t> & compressors) noexcept {
	// Устанавливаем список поддерживаемых компрессоров
	this->_scheme.compressors = compressors;
	// Выполняем инициализацию родительского объекта
	web_t::init(socket, compressors);
}
/**
 * init Метод инициализации Websocket-сервера
 * @param port        порт сервера
 * @param host        хост сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Websocket1::init(const u_int port, const string & host, const vector <http_t::compress_t> & compressors) noexcept {
	// Устанавливаем список поддерживаемых компрессоров
	this->_scheme.compressors = compressors;
	// Выполняем инициализацию родительского объекта
	web_t::init(port, host, compressors);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param bid  идентификатор брокера
 * @param mess отправляемое сообщение об ошибке
 */
void awh::server::Websocket1::sendError(const uint64_t bid, const ws::mess_t & mess) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Если код ошибки относится к Websocket
		if(mess.code >= 1000){
			// Получаем параметры активного клиента
			scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
			// Получаем объект биндинга ядра TCP/IP
			server::core_t * core = const_cast <server::core_t *> (this->_core);
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Запрещаем получение данных
				options->allow.receive = false;
				// Выполняем остановку получения данных
				core->events(core_t::mode_t::DISABLED, engine_t::method_t::READ, bid);
			}
			// Если отправка сообщений разблокированна
			if((options != nullptr) && options->allow.send){
				// Получаем буфер сообщения
				const auto & buffer = options->frame.methods.message(mess);
				// Если данные сообщения получены
				if((options->stopped = !buffer.empty())){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
						// Выводим отправляемое сообщение
						cout << this->_fmk->format("%s [%u]", mess.text.c_str(), mess.code) << endl << endl;
					#endif
					// Выполняем отправку сообщения брокеру
					core->write(buffer.data(), buffer.size(), bid);
					// Выходим из функции
					return;
				}
			}
		}
		// Завершаем работу
		const_cast <server::core_t *> (this->_core)->close(bid);
	}
}
/**
 * sendMessage Метод отправки сообщения клиенту
 * @param bid     идентификатор брокера
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 */
void awh::server::Websocket1::sendMessage(const uint64_t bid, const vector <char> & message, const bool text) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && (bid > 0) && !message.empty()){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если отправка сообщений разблокированна
		if((options != nullptr) && options->allow.send){
			// Выполняем блокировку отправки сообщения
			options->allow.send = !options->allow.send;
			// Если рукопожатие выполнено
			if(options->http.handshake(http_t::process_t::REQUEST)){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Если отправляемое сообщение является текстом
					if(text)
						// Выводим параметры ответа
						cout << string(message.begin(), message.end()) << endl << endl;
					// Выводим сообщение о выводе чанка полезной нагрузки
					else cout << this->_fmk->format("<bytes %zu>", message.size()) << endl << endl;
				#endif
				// Создаём объект заголовка для отправки
				ws::frame_t::head_t head(true, false);
				// Если нужно производить шифрование
				if(options->crypted){
					// Выполняем шифрование переданных данных
					const auto & payload = options->hash.encrypt(message.data(), message.size());
					// Если данные зашифрованны
					if(!payload.empty())
						// Заменяем сообщение для передачи
						const_cast <vector <char> &> (message).assign(payload.begin(), payload.end());
				}
				// Устанавливаем опкод сообщения
				head.optcode = (text ? ws::frame_t::opcode_t::TEXT : ws::frame_t::opcode_t::BINARY);
				// Указываем, что сообщение передаётся в сжатом виде
				head.rsv[0] = ((message.size() >= 1024) && (options->compress != http_t::compress_t::NONE));
				// Если необходимо сжимать сообщение перед отправкой
				if(head.rsv[0]){
					// Компрессионные данные
					vector <char> data;
					// Определяем метод компрессии
					switch(static_cast <uint8_t> (options->compress)){
						// Если метод компрессии выбран LZ4
						case static_cast <uint8_t> (http_t::compress_t::LZ4):
							// Выполняем компрессию полученных данных
							data = options->hash.compress(message.data(), message.size(), hash_t::method_t::LZ4);
						break;
						// Если метод компрессии выбран Zstandard
						case static_cast <uint8_t> (http_t::compress_t::ZSTD):
							// Выполняем компрессию полученных данных
							data = options->hash.compress(message.data(), message.size(), hash_t::method_t::ZSTD);
						break;
						// Если метод компрессии выбран LZma
						case static_cast <uint8_t> (http_t::compress_t::LZMA):
							// Выполняем компрессию полученных данных
							data = options->hash.compress(message.data(), message.size(), hash_t::method_t::LZMA);
						break;
						// Если метод компрессии выбран Brotli
						case static_cast <uint8_t> (http_t::compress_t::BROTLI):
							// Выполняем компрессию полученных данных
							data = options->hash.compress(message.data(), message.size(), hash_t::method_t::BROTLI);
						break;
						// Если метод компрессии выбран BZip2
						case static_cast <uint8_t> (http_t::compress_t::BZIP2):
							// Выполняем компрессию полученных данных
							data = options->hash.compress(message.data(), message.size(), hash_t::method_t::BZIP2);
						break;
						// Если метод компрессии выбран GZip
						case static_cast <uint8_t> (http_t::compress_t::GZIP):
							// Выполняем компрессию полученных данных
							data = options->hash.compress(message.data(), message.size(), hash_t::method_t::GZIP);
						break;
						// Если метод компрессии выбран Deflate
						case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
							// Устанавливаем размер скользящего окна
							options->hash.wbit(options->server.wbit);
							// Выполняем компрессию полученных данных
							data = options->hash.compress(message.data(), message.size(), hash_t::method_t::DEFLATE);
							// Удаляем хвост в полученных данных
							options->hash.rmTail(data);
						} break;
					}
					// Если сжатие данных прошло удачно
					if(!data.empty())
						// Заменяем сообщение для передачи
						const_cast <vector <char> &> (message).assign(data.begin(), data.end());
					// Снимаем флаг сжатых данных
					else head.rsv[0] = !head.rsv[0];
				}
				// Если требуется фрагментация сообщения
				if(message.size() > options->frame.size){
					// Бинарный буфер чанка данных
					vector <char> chunk(options->frame.size);
					// Смещение в бинарном буфере
					size_t start = 0, stop = options->frame.size;
					// Выполняем разбивку полезной нагрузки на сегменты
					while(stop < message.size()){
						// Увеличиваем длину чанка
						stop += options->frame.size;
						// Если длина чанка слишком большая, компенсируем
						stop = (stop > message.size() ? message.size() : stop);
						// Устанавливаем флаг финального сообщения
						head.fin = (stop == message.size());
						// Формируем чанк бинарных данных
						chunk.assign(message.data() + start, message.data() + stop);
						// Создаём буфер для отправки
						const auto & payload = options->frame.methods.set(head, chunk.data(), chunk.size());
						// Если бинарный буфер для отправки данных получен
						if(!payload.empty())
							// Отправляем серверу сообщение
							const_cast <server::core_t *> (this->_core)->write(payload.data(), payload.size(), bid);
						// Иначе просто выходим
						else break;
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
					const auto & payload = options->frame.methods.set(head, message.data(), message.size());
					// Если бинарный буфер для отправки данных получен
					if(!payload.empty())
						// Отправляем серверу сообщение
						const_cast <server::core_t *> (this->_core)->write(payload.data(), payload.size(), bid);
				}
			}
			// Выполняем разблокировку отправки сообщения
			options->allow.send = !options->allow.send;
		}
	}
}
/**
 * send Метод отправки данных в бинарном виде клиенту
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 */
void awh::server::Websocket1::send(const uint64_t bid, const char * buffer, const size_t size) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0))
		// Выполняем отправку заголовков ответа клиенту
		const_cast <server::core_t *> (this->_core)->write(buffer, size, bid);
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::server::Websocket1::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем добавление функций обратного вызова в основноной модуль
	web_t::callbacks(callbacks);
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
u_int awh::server::Websocket1::port(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.port(bid);
}
/**
 * agent Метод извлечения агента клиента
 * @param bid идентификатор брокера
 * @return    агент к которому относится подключённый клиент
 */
awh::server::web_t::agent_t awh::server::Websocket1::agent(const uint64_t bid) const noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) bid;
	// Выводим сообщение, что ничего не найдено
	return agent_t::WEBSOCKET;
}
/**
 * ip Метод получения IP-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::Websocket1::ip(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::Websocket1::mac(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.mac(bid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Websocket1::stop() noexcept {
	// Выполняем остановку работы основного модуля
	web_t::stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::Websocket1::start() noexcept {
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr){
		// Если биндинг не запущен
		if(!this->_core->working())
			// Выполняем запуск биндинга
			const_cast <server::core_t *> (this->_core)->start();
		// Если биндинг уже запущен, выполняем запуск
		else this->openEvents(this->_scheme.sid, dynamic_cast <awh::core_t *> (const_cast <server::core_t *> (this->_core)));
	}
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Websocket1::close(const uint64_t bid) noexcept {
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены, устанавливаем флаг закрытия подключения
		if(options != nullptr){
			// Устанавливаем флаг закрытия подключения брокера
			options->close = true;
			// Выполняем отключение брокера
			const_cast <server::core_t *> (this->_core)->close(bid);
		}
	}
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::server::Websocket1::subprotocol(const string & subprotocol) noexcept {
	// Устанавливаем сабпротокол
	if(!subprotocol.empty())
		// Выполняем установку сабпротокола
		this->_subprotocols.emplace(subprotocol);
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::server::Websocket1::subprotocols(const set <string> & subprotocols) noexcept {
	// Если список сабпротоколов получен
	if(!subprotocols.empty())
		// Выполняем установку сабпротоколов
		this->_subprotocols = subprotocols;
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @param bid идентификатор брокера
 * @return    список выбранных сабпротоколов
 */
const set <string> & awh::server::Websocket1::subprotocols(const uint64_t bid) const noexcept {
	// Результат работы функции
	static const set <string> result;
	// Получаем параметры активного клиента
	scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr)
		// Выводим согласованный сабпротокол
		return options->http.subprotocols();
	// Выводим результат по умолчанию
	return result;
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::server::Websocket1::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка расширений
	this->_extensions = extensions;
}
/**
 * extensions Метод извлечения списка расширений
 * @param bid идентификатор брокера
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::Websocket1::extensions(const uint64_t bid) const noexcept {
	// Результат работы функции
	static const vector <vector <string>> result;
	// Получаем параметры активного клиента
	scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr)
		// Выполняем установку списка расширений Websocket
		return options->http.extensions();
	// Выводим результат по умолчанию
	return result;
}
/**
 * multiThreads Метод активации многопоточности
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::server::Websocket1::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Если многопоточность ещё не активированна
		if(!this->_thr.is())
			// Выполняем инициализацию пула потоков
			this->_thr.init(count);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->_thr.stop();
			// Выполняем инициализацию нового тредпула
			this->_thr.init(count);
		}
		// Если сетевое ядро установлено
		if(this->_core != nullptr)
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->_thr.stop();
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Websocket1::total(const u_short total) noexcept {
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr)
		// Устанавливаем максимальное количество одновременных подключений
		const_cast <server::core_t *> (this->_core)->total(this->_scheme.sid, total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::Websocket1::segmentSize(const size_t size) noexcept {
	// Если размер сегмента фрейма передан
	if(size > 0)
		// Устанавливаем размер одного сегмента фрейма
		this->_frameSize = size;
	// Иначе устанавливаем размер сегментов по умолчанию
	else this->_frameSize = 0xFA000;
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::Websocket1::clusterAutoRestart(const bool mode) noexcept {
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr)
		// Выполняем установку флага автоматического перезапуска
		const_cast <server::core_t *> (this->_core)->clusterAutoRestart(this->_scheme.sid, mode);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Websocket1::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Websocket1::compressors(const vector <http_t::compress_t> & compressors) noexcept {
	// Устанавливаем список поддерживаемых компрессоров
	this->_scheme.compressors = compressors;
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::server::Websocket1::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = (flags.count(flag_t::NOT_STOP) == 0);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.count(flag_t::ALIVE) > 0);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_client.takeover = (flags.count(flag_t::TAKEOVER_CLIENT) > 0);
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_server.takeover = (flags.count(flag_t::TAKEOVER_SERVER) > 0);
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
void awh::server::Websocket1::alive(const bool mode) noexcept {
	// Выполняем установку долгоживущего подключения
	web_t::alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Websocket1::alive(const time_t time) noexcept {
	// Выполняем установку времени жизни подключения
	web_t::alive(time);
}
/*
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::server::Websocket1::core(const server::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку сетевого ядра
		web_t::core(core);
		// Добавляем схемы сети в сетевое ядро
		const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
		// Если многопоточность активированна
		if(this->_thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_thr.is()){
			// Выполняем завершение всех активных потоков
			this->_thr.stop();
			// Снимаем режим простого чтения базы событий
			const_cast <server::core_t *> (this->_core)->easily(false);
		}
		// Удаляем схему сети из сетевого ядра
		const_cast <server::core_t *> (this->_core)->remove(this->_scheme.sid);
		// Выполняем удаление объекта сетевого ядра
		web_t::core(core);
	}
}
/**
 * setHeaders Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::server::Websocket1::setHeaders(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку заголовков которые нужно передать клиенту
	this->_headers = headers;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Websocket1::waitTimeDetect(const time_t read, const time_t write) noexcept {
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
void awh::server::Websocket1::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
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
 * crypted Метод получения флага шифрования
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::server::Websocket1::crypted(const uint64_t bid) const noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr)
			// Выводим установленный флаг шифрования
			return options->crypted;
	}
	// Выводим результат
	return false;
}
/**
 * encrypt Метод активации шифрования для клиента
 * @param bid  идентификатор брокера
 * @param mode флаг активации шифрования
 */
void awh::server::Websocket1::encrypt(const uint64_t bid, const bool mode) noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr)
			// Устанавливаем флаг шифрования для клиента
			options->crypted = mode;
	}
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::server::Websocket1::encryption(const bool mode) noexcept {
	// Устанавливаем флага шифрования
	web_t::encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Websocket1::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования
	web_t::encryption(pass, salt, cipher);
}
/**
 * Websocket1 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Websocket1::Websocket1(const fmk_t * fmk, const log_t * log) noexcept : web_t(fmk, log), _frameSize(0xFA000), _scheme(fmk, log) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callbacks.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&ws1_t::openEvents, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callbacks.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&ws1_t::connectEvents, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callbacks.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&ws1_t::disconnectEvents, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callbacks.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&ws1_t::readEvents, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callbacks.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws1_t::writeEvents, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта брокера
	this->_scheme.callbacks.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&ws1_t::acceptEvents, this, _1, _2, _3, _4, _5));
}
/**
 * Websocket1 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Websocket1::Websocket1(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : web_t(core, fmk, log), _frameSize(0xFA000), _scheme(fmk, log) {
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем событие на запуск системы
	this->_scheme.callbacks.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&ws1_t::openEvents, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callbacks.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&ws1_t::connectEvents, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callbacks.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&ws1_t::disconnectEvents, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callbacks.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&ws1_t::readEvents, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callbacks.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws1_t::writeEvents, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта брокера
	this->_scheme.callbacks.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&ws1_t::acceptEvents, this, _1, _2, _3, _4, _5));
}
/**
 * ~Websocket1 Деструктор
 */
awh::server::Websocket1::~Websocket1() noexcept {
	// Если многопоточность активированна
	if(this->_thr.is())
		// Выполняем завершение всех потоков
		this->_thr.stop();
}
