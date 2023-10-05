/**
 * @file: ws2.cpp
 * @date: 2022-10-04
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
#include <server/web/ws2.hpp>

/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket2::connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём адъютанта
		this->_scheme.set(aid);
		// Выполняем активацию HTTP/2 протокола
		web2_t::connectCallback(aid, sid, core);
		// Выполняем проверку инициализирован ли протокол HTTP/2 для текущего клиента
		auto it = this->_sessions.find(aid);
		// Если проктокол интернета HTTP/2 инициализирован для клиента
		if(it != this->_sessions.end()){
			// Получаем параметры подключения адъютанта
			ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
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
				adj->server.takeover = this->_server.takeover;
				// Устанавливаем флаг перехвата контекста декомпрессии
				adj->client.takeover = this->_client.takeover;
				// Разрешаем перехватывать контекст компрессии
				adj->hash.takeoverCompress(this->_server.takeover);
				// Разрешаем перехватывать контекст декомпрессии
				adj->hash.takeoverDecompress(this->_client.takeover);
				// Разрешаем перехватывать контекст для клиента
				adj->http.takeover(awh::web_t::hid_t::CLIENT, this->_client.takeover);
				// Разрешаем перехватывать контекст для сервера
				adj->http.takeover(awh::web_t::hid_t::SERVER, this->_server.takeover);
				// Устанавливаем данные сервиса
				adj->http.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
				// Если сабпротоколы установлены
				if(!this->_subs.empty())
					// Устанавливаем поддерживаемые сабпротоколы
					adj->http.subs(this->_subs);
				// Если список расширений установлены
				if(!this->_extensions.empty())
					// Устанавливаем список поддерживаемых расширений
					adj->http.extensions(this->_extensions);
				// Если размер фрейма установлен
				if(this->_frameSize > 0)
					// Выполняем установку размера фрейма
					adj->frame.size = this->_frameSize;
				// Устанавливаем метод компрессии поддерживаемый сервером
				adj->http.compress(this->_scheme.compress);
				// Если сервер требует авторизацию
				if(this->_authType != auth_t::type_t::NONE){
					// Определяем тип авторизации
					switch(static_cast <uint8_t> (this->_authType)){
						// Если тип авторизации Basic
						case static_cast <uint8_t> (auth_t::type_t::BASIC): {
							// Устанавливаем параметры авторизации
							adj->http.authType(this->_authType);
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
							adj->http.authType(this->_authType, this->_authHash);
							// Если функция обратного вызова для обработки чанков установлена
							if(this->_callback.is("extractPassword"))
								// Устанавливаем функцию извлечения пароля
								adj->http.extractPassCallback(this->_callback.get <string (const string &)> ("extractPassword"));
						} break;
					}
				}
			}
		// Если протокол HTTP/2 для клиента не инициализирован
		} else {
			// Выполняем установку сетевого ядра
			this->_ws1._core = this->_core;
			// Устанавливаем флаг перехвата контекста компрессии
			this->_ws1._server.takeover = this->_server.takeover;
			// Устанавливаем флаг перехвата контекста декомпрессии
			this->_ws1._client.takeover = this->_client.takeover;
			// Устанавливаем метод компрессии поддерживаемый сервером
			this->_ws1._scheme.compress = this->_scheme.compress;
			// Если HTTP-заголовки установлены
			if(!this->_headers.empty())
				// Выполняем установку HTTP-заголовков
				this->_ws1.setHeaders(this->_headers);
			// Если сабпротоколы установлены
			if(!this->_subs.empty())
				// Устанавливаем поддерживаемые сабпротоколы
				this->_ws1.subs(this->_subs);
			// Если список расширений установлены
			if(!this->_extensions.empty())
				// Устанавливаем список поддерживаемых расширений
				this->_ws1.extensions(this->_extensions);
			// Если многопоточность активированна
			if(this->_thr.is()){
				// Выполняем завершение всех активных потоков
				this->_thr.wait();
				// Выполняем инициализацию нового тредпула
				this->_ws1.multiThreads(this->_threads);
			}
			// Выполняем переброс вызова коннекта на клиент WebSocket
			this->_ws1.connectCallback(aid, sid, core);
		}
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выводим функцию обратного вызова
			this->_callback.call <const uint64_t, const mode_t> ("active", aid, mode_t::CONNECT);
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении клиента
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket2::disconnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Выполняем поиск адъютанта в списке активных сессий
		auto it = this->_sessions.find(aid);
		// Если активная сессия найдена
		if(it != this->_sessions.end())
			// Выполняем закрытие подключения
			it->second->close();
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены и переключение протокола на HTTP/2 не выполнено
		if((adj != nullptr) && (adj->proto != engine_t::proto_t::HTTP2))
			// Добавляем в очередь список мусорных адъютантов
			this->_ws1._garbage.emplace(aid, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
		// Добавляем в очередь список мусорных адъютантов
		this->_garbage.emplace(aid, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
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
void awh::server::WebSocket2::readCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если подключение закрыто
			if(adj->close){
				// Принудительно выполняем отключение лкиента
				dynamic_cast <server::core_t *> (core)->close(aid);
				// Выходим из функции
				return;
			}
			// Если протокол подключения является HTTP/2
			if(core->proto(aid) == engine_t::proto_t::HTTP2){
				// Если получение данных не разрешено
				if(!adj->allow.receive)
					// Выходим из функции
					return;
				// Выполняем поиск адъютанта в списке активных сессий
				auto it = this->_sessions.find(aid);
				// Если активная сессия найдена
				if(it != this->_sessions.end()){
					// Выполняем извлечение полученного чанка данных из сокета
					ssize_t bytes = nghttp2_session_mem_recv(it->second->session, (const uint8_t *) buffer, size);
					// Если данные не прочитаны, выводим ошибку и выходим
					if(bytes < 0){
						// Выводим сообщение об полученной ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выводим функцию обратного вызова
							this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, nghttp2_strerror(static_cast <int> (bytes)));
						// Выходим из функции
						return;
					}
					// Фиксируем полученный результат
					if((bytes = nghttp2_session_send(it->second->session)) != 0){
						// Выводим сообщение об полученной ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выводим функцию обратного вызова
							this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(static_cast <int> (bytes)));
						// Выходим из функции
						return;
					}
				}
			// Если активирован режим работы с HTTP/1.1 протоколом, выполняем переброс вызова чтения на клиент WebSocket
			} else this->_ws1.readCallback(buffer, size, aid, sid, core);
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
void awh::server::WebSocket2::writeCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если переключение протокола на HTTP/2 не выполнено
			if(adj->proto != engine_t::proto_t::HTTP2)
				// Выполняем переброс вызова записи клиенту WebSocket
				this->_ws1.writeCallback(buffer, size, aid, sid, core);
			// Если переключение протокола на HTTP/2 выполнено
			else {
				// Если необходимо выполнить закрыть подключение
				if(!adj->close && adj->stopped){
					// Устанавливаем флаг закрытия подключения
					adj->close = !adj->close;
					// Выполняем поиск адъютанта в списке активных сессий
					auto it = this->_sessions.find(aid);
					// Если активная сессия найдена
					if(it != this->_sessions.end())
						// Выполняем закрытие подключения
						it->second->close();
					// Принудительно выполняем отключение лкиента
					dynamic_cast <server::core_t *> (core)->close(aid);
				}
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
void awh::server::WebSocket2::persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если переключение протокола на HTTP/2 не выполнено
			if(adj->proto != engine_t::proto_t::HTTP2)
				// Выполняем переброс персистентного вызова
				this->_ws1.persistCallback(aid, sid, core);
			// Если переключение протокола на HTTP/2 выполнено
			else {
				// Получаем текущий штамп времени
				const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
				// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
				if(adj->close || ((stamp - adj->point) >= (PERSIST_INTERVAL * 5)))
					// Завершаем работу
					dynamic_cast <server::core_t *> (core)->close(aid);
				// Отправляем запрос адъютанту
				else this->ping(aid, core, ::to_string(aid));
			}
		}
	}
}
/**
 * frameSignal Метод обратного вызова при получении фрейма заголовков HTTP/2
 * @param sid   идентификатор потока
 * @param aid   идентификатор адъютанта
 * @param type  тип полученного фрейма
 * @param flags флаг полученного фрейма
 * @return      статус полученных данных
 */
int awh::server::WebSocket2::frameSignal(const int32_t sid, const uint64_t aid, const uint8_t type, const uint8_t flags) noexcept {
	// Получаем параметры подключения адъютанта
	ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Если идентификатор сессии клиента совпадает
		if(adj->sid == sid){
			// Выполняем определение типа фрейма
			switch(type){
				// Если мы получили входящие данные тела ответа
				case NGHTTP2_DATA: {
					
					cout << " ±±±±±±±±±±±±±±±±±2 " << sid << " == " << aid << endl;
					
					// Если рукопожатие не выполнено
					if(!adj->shake){
						// Если мы получили флаг завершения потока
						if(flags & NGHTTP2_FLAG_END_STREAM){
							// Выполняем коммит полученного результата
							adj->http.commit();
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Получаем объект работы с HTTP-запросами
									const http_t & http = reinterpret_cast <http_t &> (adj->http);
									// Если тело ответа существует
									if(!http.body().empty())
										// Выводим сообщение о выводе чанка тела
										cout << this->_fmk->format("<body %u>", http.body().size()) << endl << endl;
									// Иначе устанавливаем перенос строки
									else cout << endl;
								}
							#endif
							// Получаем метод компрессии HTML данных
							const http_t::compress_t compress = adj->http.compression();
							// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
							if(!adj->http.body().empty() && this->_callback.is("entity")){
								// Выполняем извлечение параметров запроса
								const auto & request = adj->http.request();
								// Выполняем функцию обратного вызова
								this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &> ("entity", adj->sid, aid, request.method, request.url, adj->http.body());
							}
							// Выполняем сброс состояния HTTP парсера
							adj->http.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->http.reset();
							// Выполняем очистку буфера данных
							adj->buffer.payload.clear();
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выводим функцию обратного вызова
								this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, "Invalid HTTP request made");
						}
					// Если рукопожатие выполнено
					} else if(adj->allow.receive) {
						// Если мы получили неустановленный флаг или флаг завершения потока
						if((flags & NGHTTP2_FLAG_NONE) || (flags & NGHTTP2_FLAG_END_STREAM)){
							// Флаг удачного получения данных
							bool receive = false;
							// Создаём буфер сообщения
							vector <char> buffer;
							// Создаём объект шапки фрейма
							ws::frame_t::head_t head;
							// Выполняем обработку полученных данных
							while(!adj->close && adj->allow.receive){
								// Выполняем чтение фрейма WebSocket
								const auto & data = adj->frame.methods.get(head, adj->buffer.payload.data(), adj->buffer.payload.size());
								// Если буфер данных получен
								if(!data.empty()){
									// Проверяем состояние флагов RSV2 и RSV3
									if(head.rsv[1] || head.rsv[2]){
										// Создаём сообщение
										adj->mess = ws::mess_t(1002, "RSV2 and RSV3 must be clear");
										// Выполняем отключение адъютанта
										goto Stop;
									}
									// Если флаг компресси включён а данные пришли не сжатые
									if(head.rsv[0] && ((adj->compress == http_t::compress_t::NONE) ||
									  (head.optcode == ws::frame_t::opcode_t::CONTINUATION) ||
									  ((static_cast <uint8_t> (head.optcode) > 0x07) && (static_cast <uint8_t> (head.optcode) < 0x0b)))){
										// Создаём сообщение
										adj->mess = ws::mess_t(1002, "RSV1 must be clear");
										// Выполняем отключение адъютанта
										goto Stop;
									}
									// Если опкоды требуют финального фрейма
									if(!head.fin && (static_cast <uint8_t> (head.optcode) > 0x07) && (static_cast <uint8_t> (head.optcode) < 0x0b)){
										// Создаём сообщение
										adj->mess = ws::mess_t(1002, "FIN must be set");
										// Выполняем отключение адъютанта
										goto Stop;
									}
									// Определяем тип ответа
									switch(static_cast <uint8_t> (head.optcode)){
										// Если ответом является PING
										case static_cast <uint8_t> (ws::frame_t::opcode_t::PING):
											// Отправляем ответ адъютанту
											this->pong(aid, const_cast <server::core_t *> (this->_core), string(data.begin(), data.end()));
										break;
										// Если ответом является PONG
										case static_cast <uint8_t> (ws::frame_t::opcode_t::PONG): {
											// Если идентификатор адъютанта совпадает
											if(::memcmp(::to_string(aid).c_str(), data.data(), data.size()) == 0)
												// Обновляем контрольную точку
												adj->point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
										} break;
										// Если ответом является TEXT
										case static_cast <uint8_t> (ws::frame_t::opcode_t::TEXT):
										// Если ответом является BINARY
										case static_cast <uint8_t> (ws::frame_t::opcode_t::BINARY): {
											// Запоминаем полученный опкод
											adj->frame.opcode = head.optcode;
											// Запоминаем, что данные пришли сжатыми
											adj->deflate = (head.rsv[0] && (adj->compress != http_t::compress_t::NONE));
											// Если сообщение не замаскированно
											if(!head.mask){
												// Создаём сообщение
												adj->mess = ws::mess_t(1002, "Not masked frame from client");
												// Выполняем отключение адъютанта
												goto Stop;
											// Если список фрагментированных сообщений существует
											} else if(!adj->buffer.fragmes.empty()) {
												// Очищаем список фрагментированных сообщений
												adj->buffer.fragmes.clear();
												// Создаём сообщение
												adj->mess = ws::mess_t(1002, "Opcode for subsequent fragmented messages should not be set");
												// Выполняем отключение адъютанта
												goto Stop;
											// Если сообщение является не последнем
											} else if(!head.fin)
												// Заполняем фрагментированное сообщение
												adj->buffer.fragmes.insert(adj->buffer.fragmes.end(), data.begin(), data.end());
											// Если сообщение является последним
											else buffer = std::forward <const vector <char>> (data);
										} break;
										// Если ответом является CONTINUATION
										case static_cast <uint8_t> (ws::frame_t::opcode_t::CONTINUATION): {
											// Заполняем фрагментированное сообщение
											adj->buffer.fragmes.insert(adj->buffer.fragmes.end(), data.begin(), data.end());
											// Если сообщение является последним
											if(head.fin){
												// Выполняем копирование всех собранных сегментов
												buffer = std::forward <const vector <char>> (adj->buffer.fragmes);
												// Очищаем список фрагментированных сообщений
												adj->buffer.fragmes.clear();
											}
										} break;
										// Если ответом является CLOSE
										case static_cast <uint8_t> (ws::frame_t::opcode_t::CLOSE): {
											// Создаём сообщение
											adj->mess = adj->frame.methods.message(data);
											// Выводим сообщение об ошибке
											this->error(aid, adj->mess);
											// Завершаем работу
											const_cast <server::core_t *> (this->_core)->close(aid);
											// Выходим из функции
											return NGHTTP2_ERR_CALLBACK_FAILURE;
										} break;
									}
								}
								// Если парсер обработал какое-то количество байт
								if((receive = ((head.frame > 0) && !adj->buffer.payload.empty()))){
									// Если размер буфера больше количества удаляемых байт
									if((receive = (adj->buffer.payload.size() >= head.frame)))
										// Удаляем количество обработанных байт
										adj->buffer.payload.assign(adj->buffer.payload.begin() + head.frame, adj->buffer.payload.end());
										// vector <decltype(adj->buffer.payload)::value_type> (adj->buffer.payload.begin() + head.frame, adj->buffer.payload.end()).swap(adj->buffer.payload);
								}
								// Если сообщения получены
								if(!buffer.empty()){
									// Если тредпул активирован
									if(this->_thr.is())
										// Добавляем в тредпул новую задачу на извлечение полученных сообщений
										this->_thr.push(std::bind(&ws2_t::extraction, this, aid, buffer, (adj->frame.opcode == ws::frame_t::opcode_t::TEXT)));
									// Если тредпул не активирован, выполняем извлечение полученных сообщений
									else this->extraction(aid, buffer, (adj->frame.opcode == ws::frame_t::opcode_t::TEXT));
									// Очищаем буфер полученного сообщения
									buffer.clear();
								}
								// Если данные мы все получили, выходим
								if(!receive || adj->buffer.payload.empty()) break;
							}
							// Выходим из функции
							return 0;
						}
						// Устанавливаем метку остановки адъютанта
						Stop:
						// Отправляем серверу сообщение
						this->sendError(aid, adj->mess);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выводим функцию обратного вызова
							this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::WARNING, http::error_t::WEBSOCKET, this->_fmk->format("%s [%u]", adj->mess.code, adj->mess.text.c_str()));
					}
				} break;
				// Если мы получили входящие данные заголовков ответа
				case NGHTTP2_HEADERS: {
					// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
					if(flags & NGHTTP2_FLAG_END_HEADERS){
						// Выполняем извлечение параметров запроса
						const auto & request = adj->http.request();
						// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
						if(this->_callback.is("request"))
							// Выводим функцию обратного вызова
							this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &> ("request", adj->sid, aid, request.method, request.url);
						// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
						if(this->_callback.is("headers"))
							// Выводим функцию обратного вызова
							this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &> ("headers", adj->sid, aid, request.method, request.url, adj->http.headers());
						// Если рукопожатие не выполнено
						if(!reinterpret_cast <http_t &> (adj->http).isHandshake()){
							// Метод компрессии данных
							http_t::compress_t compress = http_t::compress_t::NONE;
							// Ответ клиенту по умолчанию успешный
							awh::web_t::res_t response(2.0f, static_cast <u_int> (200));
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Получаем объект работы с HTTP-запросами
									const http_t & http = reinterpret_cast <http_t &> (adj->http);
									// Получаем данные ответа
									const auto & response = http.process(http_t::process_t::RESPONSE, true);
									// Если параметры ответа получены
									if(!response.empty())
										// Выводим параметры ответа
										cout << string(response.begin(), response.end()) << endl;
								}
							#endif
							// Выполняем проверку авторизации
							switch(static_cast <uint8_t> (adj->http.getAuth())){
								// Если запрос выполнен удачно
								case static_cast <uint8_t> (http_t::stath_t::GOOD): {
									// Если рукопожатие выполнено
									if((adj->shake = adj->http.isHandshake())){
										// Получаем метод компрессии HTML данных
										compress = adj->http.compression();
										// Проверяем версию протокола
										if(!adj->http.checkVer()){
											// Выполняем сброс состояния HTTP парсера
											adj->http.clear();
											// Получаем бинарные данные REST запроса
											response = awh::web_t::res_t(2.0f, static_cast <u_int> (400), "Unsupported protocol version");
											// Завершаем работу
											break;
										}
										// Проверяем ключ адъютанта
										if(!adj->http.checkKey()){
											// Выполняем сброс состояния HTTP парсера
											adj->http.clear();
											// Получаем бинарные данные REST запроса
											response = awh::web_t::res_t(2.0f, static_cast <u_int> (400), "Wrong client key");
											// Завершаем работу
											break;
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
										if(!this->_headers.empty())
											// Выполняем установку HTTP-заголовков
											adj->http.headers(this->_headers);
										// Получаем заголовки ответа удалённому клиенту
										const auto & headers = adj->http.process2(http_t::process_t::RESPONSE, response);
										// Если бинарные данные ответа получены
										if(!headers.empty()){
											// Выполняем поиск адъютанта в списке активных сессий
											auto it = this->_sessions.find(aid);
											// Если активная сессия найдена
											if(it != this->_sessions.end()){
												/**
												 * Если включён режим отладки
												 */
												#if defined(DEBUG_MODE)
													{
														// Выводим заголовок ответа
														cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
														// Получаем бинарные данные REST-ответа
														const auto & buffer = adj->http.process(http_t::process_t::RESPONSE, response);
														// Если бинарные данные ответа получены
														if(!buffer.empty())
															// Выводим параметры ответа
															cout << string(buffer.begin(), buffer.end()) << endl << endl;
													}
												#endif
												// Список заголовков для ответа
												vector <nghttp2_nv> nva;
												// Выполняем перебор всех заголовков HTTP/2 ответа
												for(auto & header : headers){
													// Выполняем добавление метода ответа
													nva.push_back({
														(uint8_t *) header.first.c_str(),
														(uint8_t *) header.second.c_str(),
														header.first.size(),
														header.second.size(),
														NGHTTP2_NV_FLAG_NONE
													});
												}
												// Выполняем ответ подключившемуся клиенту
												int rv = nghttp2_submit_response(it->second->session, adj->sid, nva.data(), nva.size(), nullptr);
												// Если запрос не получилось отправить
												if(rv < 0){
													// Выводим в лог сообщение
													this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
													// Если функция обратного вызова на на вывод ошибок установлена
													if(this->_callback.is("error"))
														// Выводим функцию обратного вызова
														this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
													// Выполняем закрытие подключения
													const_cast <server::core_t *> (this->_core)->close(aid);
													// Выходим из функции
													return NGHTTP2_ERR_CALLBACK_FAILURE;
												}{
													// Фиксируем отправленный результат
													if((rv = nghttp2_session_send(it->second->session)) != 0){
														// Выводим сообщение об полученной ошибке
														this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
														// Если функция обратного вызова на на вывод ошибок установлена
														if(this->_callback.is("error"))
															// Выводим функцию обратного вызова
															this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
														// Выполняем закрытие подключения
														const_cast <server::core_t *> (this->_core)->close(aid);
														// Выходим из функции
														return NGHTTP2_ERR_CALLBACK_FAILURE;
													}
												}
												// Если функция обратного вызова активности потока установлена
												if(this->_callback.is("stream"))
													// Выполняем функцию обратного вызова
													this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", adj->sid, aid, mode_t::OPEN);
												// Если функция обратного вызова на получение удачного запроса установлена
												if(this->_callback.is("goodRequest"))
													// Выполняем функцию обратного вызова
													this->_callback.call <const int32_t, const uint64_t> ("goodRequest", adj->sid, aid);
												// Завершаем работу
												return 0;
											}
										// Выполняем реджект
										} else {
											// Выполняем сброс состояния HTTP парсера
											adj->http.clear();
											// Выполняем сброс состояния HTTP парсера
											adj->http.reset();
											// Формируем ответ, что страница не доступна
											response = awh::web_t::res_t(2.0f, static_cast <u_int> (500));
										}
									// Сообщаем, что рукопожатие не выполнено
									} else {
										// Выполняем сброс состояния HTTP парсера
										adj->http.clear();
										// Выполняем сброс состояния HTTP парсера
										adj->http.reset();
										// Формируем ответ, что страница не доступна
										response = awh::web_t::res_t(2.0f, static_cast <u_int> (403));
									}
								} break;
								// Если запрос неудачный
								case static_cast <uint8_t> (http_t::stath_t::FAULT): {
									// Выполняем сброс состояния HTTP парсера
									adj->http.clear();
									// Выполняем сброс состояния HTTP парсера
									adj->http.reset();
									// Формируем ответ на запрос об авторизации
									response = awh::web_t::res_t(2.0f, static_cast <u_int> (401));
								} break;
							}
							// Устанавливаем метод компрессии данных ответа
							adj->http.compress(compress);
							// Получаем заголовки ответа удалённому клиенту
							const auto & headers = adj->http.reject2(response);
							// Если бинарные данные ответа получены
							if(!headers.empty()){
								// Выполняем поиск адъютанта в списке активных сессий
								auto it = this->_sessions.find(aid);
								// Если активная сессия найдена
								if(it != this->_sessions.end()){
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										{
											// Выводим заголовок ответа
											cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
											// Получаем бинарные данные REST-ответа
											const auto & buffer = adj->http.process(http_t::process_t::RESPONSE, response);
											// Если бинарные данные ответа получены
											if(!buffer.empty())
												// Выводим параметры ответа
												cout << string(buffer.begin(), buffer.end()) << endl << endl;
										}
									#endif
									// Список заголовков для ответа
									vector <nghttp2_nv> nva;
									// Выполняем перебор всех заголовков HTTP/2 ответа
									for(auto & header : headers){
										// Выполняем добавление метода ответа
										nva.push_back({
											(uint8_t *) header.first.c_str(),
											(uint8_t *) header.second.c_str(),
											header.first.size(),
											header.second.size(),
											NGHTTP2_NV_FLAG_NONE
										});
									}
									// Выполняем ответ подключившемуся клиенту
									int rv = nghttp2_submit_response(it->second->session, adj->sid, nva.data(), nva.size(), nullptr);
									// Если запрос не получилось отправить
									if(rv < 0){
										// Выводим в лог сообщение
										this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
										// Если функция обратного вызова на на вывод ошибок установлена
										if(this->_callback.is("error"))
											// Выводим функцию обратного вызова
											this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
										// Выполняем закрытие подключения
										const_cast <server::core_t *> (this->_core)->close(aid);
										// Выходим из функции
										return NGHTTP2_ERR_CALLBACK_FAILURE;
									}{
										// Фиксируем отправленный результат
										if((rv = nghttp2_session_send(it->second->session)) != 0){
											// Выводим сообщение об полученной ошибке
											this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
											// Если функция обратного вызова на на вывод ошибок установлена
											if(this->_callback.is("error"))
												// Выводим функцию обратного вызова
												this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
											// Выполняем закрытие подключения
											const_cast <server::core_t *> (this->_core)->close(aid);
											// Выходим из функции
											return NGHTTP2_ERR_CALLBACK_FAILURE;
										}
									}
									// Завершаем работу
									return 0;
								}
							}
						}
						// Завершаем работу
						const_cast <server::core_t *> (this->_core)->close(aid);
					}
				} break;
			}
		}
	}
	// Выводим результат
	return 0;
}
/**
 * chunkSignal Метод обратного вызова при получении чанка HTTP/2
 * @param sid    идентификатор потока
 * @param aid    идентификатор адъютанта
 * @param buffer буфер данных который содержит полученный чанк
 * @param size   размер полученного буфера данных чанка
 * @return       статус полученных данных
 */
int awh::server::WebSocket2::chunkSignal(const int32_t sid, const uint64_t aid, const uint8_t * buffer, const size_t size) noexcept {
	// Получаем параметры подключения адъютанта
	ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Если идентификатор сессии клиента совпадает
		if(adj->sid == sid){
			// Если функция обратного вызова на перехват входящих чанков установлена
			if(this->_callback.is("chunking"))
				// Выводим функцию обратного вызова
				this->_callback.call <const vector <char> &, const awh::http_t *> ("chunking", vector <char> (buffer, buffer + size), &adj->http);
			// Если функция перехвата полученных чанков не установлена
			else {
				// Если подключение закрыто
				if(adj->close){
					// Принудительно выполняем отключение лкиента
					const_cast <server::core_t *> (this->_core)->close(aid);
					// Выходим из функции
					return 0;
				}
				// Если рукопожатие не выполнено
				if(!adj->shake)
					// Добавляем полученный чанк в тело данных
					adj->http.body(vector <char> (buffer, buffer + size));
				// Если рукопожатие выполнено
				else if(adj->allow.receive)
					// Добавляем полученные данные в буфер
					adj->buffer.payload.insert(adj->buffer.payload.end(), buffer, buffer + size);
				// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
				if(this->_callback.is("chunks"))
					// Выводим функцию обратного вызова
					this->_callback.call <const int32_t, const uint64_t, const vector <char> &> ("chunks", sid, aid, vector <char> (buffer, buffer + size));
			}
		}
	}
	// Выводим результат
	return 0;
}
/**
 * beginSignal Метод начала получения фрейма заголовков HTTP/2
 * @param sid идентификатор потока
 * @param aid идентификатор адъютанта
 * @return    статус полученных данных
 */
int awh::server::WebSocket2::beginSignal(const int32_t sid, const uint64_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Если идентификатор сессии клиента совпадает
		if(adj->sid == sid){
			// Выполняем сброс флага рукопожатия
			adj->shake = false;
			// Выполняем очистку параметров HTTP запроса
			adj->http.clear();
			// Очищаем буфер собранных данных
			adj->buffer.payload.clear();
			// Выполняем очистку оставшихся фрагментов
			adj->buffer.fragmes.clear();
		}
	}
	// Выводим результат
	return 0;
}
/**
 * closedSignal Метод завершения работы потока
 * @param sid   идентификатор потока
 * @param aid   идентификатор адъютанта
 * @param error флаг ошибки HTTP/2 если присутствует
 * @return      статус полученных данных
 */
int awh::server::WebSocket2::closedSignal(const int32_t sid, const uint64_t aid, const uint32_t error) noexcept {
	// Флаг отключения от сервера
	bool stop = false;
	// Определяем тип получаемой ошибки
	switch(error){
		// Если получена ошибка протокола
		case 0x1: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::WARNING, sid, aid, "PROTOCOL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::WARNING, http::error_t::HTTP2_PROTOCOL, this->_fmk->format("Stream %d closed with error=%s", sid, "PROTOCOL_ERROR"));
		} break;
		// Если получена ошибка реализации
		case 0x2: {
			// Выполняем установку флага остановки
			stop = true;
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "INTERNAL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_INTERNAL, this->_fmk->format("Stream %d closed with error=%s", sid, "INTERNAL_ERROR"));
		} break;
		// Если получена ошибка превышения предела управления потоком
		case 0x3: {
			// Выполняем установку флага остановки
			stop = true;
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "FLOW_CONTROL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_FLOW_CONTROL, this->_fmk->format("Stream %d closed with error=%s", sid, "FLOW_CONTROL_ERROR"));
		} break;
		// Если установка не подтверждённа
		case 0x4: {
			// Выполняем установку флага остановки
			stop = true;
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "SETTINGS_TIMEOUT");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SETTINGS_TIMEOUT, this->_fmk->format("Stream %d closed with error=%s", sid, "SETTINGS_TIMEOUT"));
		} break;
		// Если получен кадр для завершения потока
		case 0x5: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::WARNING, sid, aid, "STREAM_CLOSED");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::WARNING, http::error_t::HTTP2_STREAM_CLOSED, this->_fmk->format("Stream %d closed with error=%s", sid, "STREAM_CLOSED"));
		} break;
		// Если размер кадра некорректен
		case 0x6: {
			// Выполняем установку флага остановки
			stop = true;
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "FRAME_SIZE_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_FRAME_SIZE, this->_fmk->format("Stream %d closed with error=%s", sid, "FRAME_SIZE_ERROR"));
		} break;
		// Если поток не обработан
		case 0x7: {
			// Выполняем установку флага остановки
			stop = true;
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "REFUSED_STREAM");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_REFUSED_STREAM, this->_fmk->format("Stream %d closed with error=%s", sid, "REFUSED_STREAM"));
		} break;
		// Если поток аннулирован
		case 0x8: {
			// Выполняем установку флага остановки
			stop = true;
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "CANCEL");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_CANCEL, this->_fmk->format("Stream %d closed with error=%s", sid, "CANCEL"));
		} break;
		// Если состояние компрессии не обновлено
		case 0x9: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::WARNING, sid, aid, "COMPRESSION_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::WARNING, http::error_t::HTTP2_COMPRESSION, this->_fmk->format("Stream %d closed with error=%s", sid, "COMPRESSION_ERROR"));
		} break;
		// Если получена ошибка TCP-соединения для метода CONNECT
		case 0xA: {
			// Выполняем установку флага остановки
			stop = true;
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "CONNECT_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_CONNECT, this->_fmk->format("Stream %d closed with error=%s", sid, "CONNECT_ERROR"));
		} break;
		// Если превышена емкость для обработки
		case 0xB: {
			// Выполняем установку флага остановки
			stop = true;
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "ENHANCE_YOUR_CALM");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_ENHANCE_YOUR_CALM, this->_fmk->format("Stream %d closed with error=%s", sid, "ENHANCE_YOUR_CALM"));
		} break;
		// Если согласованные параметры TLS не приемлемы
		case 0xC: {
			// Выполняем установку флага остановки
			stop = true;
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "INADEQUATE_SECURITY");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_INADEQUATE_SECURITY, this->_fmk->format("Stream %d closed with error=%s", sid, "INADEQUATE_SECURITY"));
		} break;
		// Если для запроса используется HTTP/1.1
		case 0xD: {
			// Выполняем установку флага остановки
			stop = true;
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%d] closed with error=%s", log_t::flag_t::CRITICAL, sid, aid, "HTTP_1_1_REQUIRED");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_HTTP_1_1_REQUIRED, this->_fmk->format("Stream %d closed with error=%s", sid, "HTTP_1_1_REQUIRED"));
		} break;
	}
	// Если функция обратного вызова активности потока установлена
	if(this->_callback.is("stream"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", sid, aid, mode_t::CLOSE);
	// Если разрешено выполнить остановку
	if(stop){
		// Выполняем поиск адъютанта в списке активных сессий
		auto it = this->_sessions.find(aid);
		// Если активная сессия найдена
		if(it != this->_sessions.end()){
			// Если закрытие подключения не выполнено
			if(!it->second->close()){
				// Выполняем отключение от сервера
				const_cast <server::core_t *> (this->_core)->close(aid);
				// Выводим сообщение об ошибке
				return NGHTTP2_ERR_CALLBACK_FAILURE;
			// Выполняем отключение от сервера
			} else const_cast <server::core_t *> (this->_core)->close(aid);
		}
	}
	// Выводим результат
	return 0;
}
/**
 * headerSignal Метод обратного вызова при получении заголовка HTTP/2
 * @param sid идентификатор потока
 * @param aid идентификатор адъютанта
 * @param key данные ключа заголовка
 * @param val данные значения заголовка
 * @return    статус полученных данных
 */
int awh::server::WebSocket2::headerSignal(const int32_t sid, const uint64_t aid, const string & key, const string & val) noexcept {
	// Получаем параметры подключения адъютанта
	ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Если идентификатор сессии клиента совпадает
		if(adj->sid == sid){
			// Устанавливаем полученные заголовки
			adj->http.header2(key, val);
			// Если функция обратного вызова на полученного заголовка с сервера установлена
			if(this->_callback.is("header"))
				// Выводим функцию обратного вызова
				this->_callback.call <const int32_t, const uint64_t, const string &, const string &> ("header", sid, aid, key, val);
		}
	}
	// Выводим результат
	return 0;
}
/**
 * error Метод вывода сообщений об ошибках работы адъютанта
 * @param aid     идентификатор адъютанта
 * @param message сообщение с описанием ошибки
 */
void awh::server::WebSocket2::error(const uint64_t aid, const ws::mess_t & message) const noexcept {
	// Получаем параметры подключения адъютанта
	ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Очищаем список буффер бинарных данных
		adj->buffer.payload.clear();
		// Очищаем список фрагментированных сообщений
		adj->buffer.fragmes.clear();
	}
	// Если идентификатор адъютанта передан и код ошибки указан
	if((aid > 0) && (message.code > 0)){
		// Если сообщение об ошибке пришло
		if(!message.text.empty()){
			// Если тип сообщения получен
			if(!message.type.empty())
				// Выводим в лог сообщение
				this->_log->print("%s - %s [%u]", log_t::flag_t::WARNING, message.type.c_str(), message.text.c_str(), message.code);
			// Иначе выводим сообщение в упрощёном виде
			else this->_log->print("%s [%u]", log_t::flag_t::WARNING, message.text.c_str(), message.code);
			// Если функция обратного вызова при получении ошибки WebSocket установлена
			if(this->_callback.is("wserror"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const u_int, const string &> ("wserror", aid, message.code, message.text);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::WARNING, http::error_t::WEBSOCKET, this->_fmk->format("%s [%u]", message.code, message.text.c_str()));
		}
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param aid    идентификатор адъютанта
 * @param buffer данные в чистом виде полученные с сервера
 * @param text   данные передаются в текстовом виде
 */
void awh::server::WebSocket2::extraction(const uint64_t aid, const vector <char> & buffer, const bool text) noexcept {
	// Если буфер данных передан
	if((aid > 0) && !buffer.empty() && this->_callback.is("message")){
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Выполняем блокировку потока	
			const lock_guard <recursive_mutex> lock(adj->mtx);
			// Если данные пришли в сжатом виде
			if(adj->deflate && (adj->compress != http_t::compress_t::NONE)){
				// Декомпрессионные данные
				vector <char> data;
				// Определяем метод компрессии
				switch(static_cast <uint8_t> (adj->compress)){
					// Если метод компрессии выбран Deflate
					case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
						// Устанавливаем размер скользящего окна
						adj->hash.wbit(adj->client.wbit);
						// Добавляем хвост в полученные данные
						adj->hash.setTail(* const_cast <vector <char> *> (&buffer));
						// Выполняем декомпрессию полученных данных
						data = adj->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
					} break;
					// Если метод компрессии выбран GZip
					case static_cast <uint8_t> (http_t::compress_t::GZIP):
						// Выполняем декомпрессию полученных данных
						data = adj->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::GZIP);
					break;
					// Если метод компрессии выбран Brotli
					case static_cast <uint8_t> (http_t::compress_t::BROTLI):
						// Выполняем декомпрессию полученных данных
						data = adj->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BROTLI);
					break;
				}
				// Если данные получены
				if(!data.empty()){
					// Если нужно производить дешифрование
					if(adj->crypt){
						// Выполняем шифрование переданных данных
						const auto & res = adj->hash.decrypt(data.data(), data.size());
						// Отправляем полученный результат
						if(!res.empty()){
							// Выводим данные полученного сообщения
							this->_callback.call <const uint64_t, const vector <char> &, const bool> ("message", aid, res, text);
							// Выходим из функции
							return;
						}
					}
					// Выводим сообщение так - как оно пришло
					this->_callback.call <const uint64_t, const vector <char> &, const bool> ("message", aid, data, text);
				// Выводим сообщение об ошибке
				} else {
					// Создаём сообщение
					adj->mess = ws::mess_t(1007, "Received data decompression error");
					// Получаем буфер сообщения
					data = adj->frame.methods.message(adj->mess);
					// Если данные сообщения получены
					if((adj->stopped = !data.empty()))
						// Выполняем отправку сообщения клиенту
						web2_t::send(adj->sid, aid, data.data(), data.size(), true);
					// Завершаем работу
					else const_cast <server::core_t *> (this->_core)->close(aid);
				}
			// Если функция обратного вызова установлена, выводим полученное сообщение
			} else {
				// Если нужно производить дешифрование
				if(adj->crypt){
					// Выполняем шифрование переданных данных
					const auto & res = adj->hash.decrypt(buffer.data(), buffer.size());
					// Отправляем полученный результат
					if(!res.empty()){
						// Отправляем полученный результат
						this->_callback.call <const uint64_t, const vector <char> &, const bool> ("message", aid, res, text);
						// Выходим из функции
						return;
					}
				}
				// Выводим сообщение так - как оно пришло
				this->_callback.call <const uint64_t, const vector <char> &, const bool> ("message", aid, buffer, text);
			}
		}
	}
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 * @param      message сообщение для отправки
 */
void awh::server::WebSocket2::pong(const uint64_t aid, awh::core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((aid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если отправка сообщений разблокированна
		if((adj != nullptr) && adj->allow.send){
			// Создаём буфер для отправки
			const auto & buffer = adj->frame.methods.pong(message);
			// Если буфер данных получен
			if(!buffer.empty())
				// Выполняем отправку сообщения клиенту
				web2_t::send(adj->sid, aid, buffer.data(), buffer.size(), false);
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 * @param      message сообщение для отправки
 */
void awh::server::WebSocket2::ping(const uint64_t aid, awh::core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((aid > 0) && (core != nullptr) && core->working()){
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если отправка сообщений разблокированна
		if((adj != nullptr) && adj->allow.send){
			// Создаём буфер для отправки
			const auto & buffer = adj->frame.methods.ping(message);
			// Если буфер данных получен
			if(!buffer.empty())
				// Выполняем отправку сообщения клиенту
				web2_t::send(adj->sid, aid, buffer.data(), buffer.size(), false);
		}
	}
}
/**
 * garbage Метод удаления мусорных адъютантов
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket2::garbage(const u_short tid, awh::core_t * core) noexcept {
	// Если список мусорных адъютантов не пустой
	if(!this->_garbage.empty()){
		// Получаем текущее значение времени
		const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
		// Выполняем переход по всему списку мусорных адъютантов
		for(auto it = this->_garbage.begin(); it != this->_garbage.end();){
			// Если адъютант уже давно удалился
			if((date - it->second) >= 10000){
				// Получаем параметры подключения адъютанта
				ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(it->first));
				// Если параметры подключения адъютанта получены
				if(adj != nullptr){
					// Устанавливаем флаг отключения
					adj->close = true;
					// Выполняем очистку оставшихся данных
					adj->buffer.payload.clear();
					// Выполняем очистку оставшихся фрагментов
					adj->buffer.fragmes.clear();
					// Если переключение протокола на HTTP/2 не выполнено
					if(adj->proto != engine_t::proto_t::HTTP2)
						// Выполняем очистку мусора у WebSocket-сервера
						this->_ws1.garbage(tid, core);
					// Выполняем удаление созданной ранее сессии HTTP/2
					else this->_sessions.erase(it->first);
				}
				// Выполняем удаление параметров адъютанта
				this->_scheme.rm(it->first);
				// Выполняем удаление объекта адъютантов из списка мусора
				it = this->_garbage.erase(it);
			// Выполняем пропуск адъютанта
			} else ++it;
		}
	}
	// Выполняем удаление мусора в родительском объекте
	web2_t::garbage(tid, core);
}
/**
 * init Метод инициализации WebSocket-сервера
 * @param socket   unix-сокет для биндинга
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::WebSocket2::init(const string & socket, const http_t::compress_t compress) noexcept {
	// Устанавливаем тип компрессии
	this->_scheme.compress = compress;
	// Выполняем инициализацию родительского объекта
	web2_t::init(socket, compress);
}
/**
 * init Метод инициализации WebSocket-сервера
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::WebSocket2::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
	// Устанавливаем тип компрессии
	this->_scheme.compress = compress;
	// Выполняем инициализацию родительского объекта
	web2_t::init(port, host, compress);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param aid  идентификатор адъютанта
 * @param mess отправляемое сообщение об ошибке
 */
void awh::server::WebSocket2::sendError(const uint64_t aid, const ws::mess_t & mess) noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Если код ошибки относится к WebSocket
		if(mess.code >= 1000){
			// Получаем параметры подключения адъютанта
			ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Если параметры подключения адъютанта получены
			if(adj != nullptr)
				// Запрещаем получение данных
				adj->allow.receive = false;
			// Если отправка сообщений разблокированна
			if((adj != nullptr) && adj->allow.send){
				// Если переключение протокола на HTTP/2 не выполнено
				if(adj->proto != engine_t::proto_t::HTTP2)
					// Выполняем отправку сообщение об ошибке на клиент WebSocket
					this->_ws1.sendError(aid, mess);
				// Если переключение протокола на HTTP/2 выполнено
				else {
					// Получаем буфер сообщения
					const auto & buffer = adj->frame.methods.message(mess);
					// Если данные сообщения получены
					if((adj->stopped = !buffer.empty())){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
							// Выводим отправляемое сообщение
							cout << this->_fmk->format("%s [%u]", mess.text.c_str(), mess.code) << endl << endl;
						#endif
						// Выполняем отправку сообщения клиенту
						web2_t::send(adj->sid, aid, buffer.data(), buffer.size(), true);
						// Выходим из функции
						return;
					}
				}
			}
		}
		// Завершаем работу
		const_cast <server::core_t *> (this->_core)->close(aid);
	}
}
/**
 * send Метод отправки сообщения клиенту
 * @param aid     идентификатор адъютанта
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param text    данные передаются в текстовом виде
 */
void awh::server::WebSocket2::send(const uint64_t aid, const char * message, const size_t size, const bool text) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && (aid > 0) && (size > 0) && (message != nullptr)){
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если отправка сообщений разблокированна
		if((adj != nullptr) && adj->allow.send){
			// Выполняем блокировку отправки сообщения
			adj->allow.send = !adj->allow.send;
			// Если переключение протокола на HTTP/2 не выполнено
			if(adj->proto != engine_t::proto_t::HTTP2)
				// Выполняем отправку сообщения клиенту WebSocket
				this->_ws1.send(aid, message, size, text);
			// Если переключение протокола на HTTP/2 выполнено
			else {
				// Если рукопожатие выполнено
				if(adj->http.isHandshake()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
						// Если отправляемое сообщение является текстом
						if(text)
							// Выводим параметры ответа
							cout << string(message, size) << endl << endl;
						// Выводим сообщение о выводе чанка полезной нагрузки
						else cout << this->_fmk->format("<bytes %u>", size) << endl << endl;
					#endif
					// Буфер сжатых данных
					vector <char> buffer;
					// Создаём объект заголовка для отправки
					ws::frame_t::head_t head(true, false);
					// Если нужно производить шифрование
					if(adj->crypt){
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
					// Устанавливаем опкод сообщения
					head.optcode = (text ? ws::frame_t::opcode_t::TEXT : ws::frame_t::opcode_t::BINARY);
					// Указываем, что сообщение передаётся в сжатом виде
					head.rsv[0] = ((size >= 1024) && (adj->compress != http_t::compress_t::NONE));
					// Если необходимо сжимать сообщение перед отправкой
					if(head.rsv[0]){
						// Компрессионные данные
						vector <char> data;
						// Определяем метод компрессии
						switch(static_cast <uint8_t> (adj->compress)){
							// Если метод компрессии выбран Deflate
							case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
								// Устанавливаем размер скользящего окна
								adj->hash.wbit(adj->server.wbit);
								// Выполняем компрессию полученных данных
								data = adj->hash.compress(message, size, hash_t::method_t::DEFLATE);
								// Удаляем хвост в полученных данных
								adj->hash.rmTail(data);
							} break;
							// Если метод компрессии выбран GZip
							case static_cast <uint8_t> (http_t::compress_t::GZIP):
								// Выполняем компрессию полученных данных
								data = adj->hash.compress(message, size, hash_t::method_t::GZIP);
							break;
							// Если метод компрессии выбран Brotli
							case static_cast <uint8_t> (http_t::compress_t::BROTLI):
								// Выполняем компрессию полученных данных
								data = adj->hash.compress(message, size, hash_t::method_t::BROTLI);
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
					// Если требуется фрагментация сообщения
					if(size > adj->frame.size){
						// Бинарный буфер чанка данных
						vector <char> chunk(adj->frame.size);
						// Смещение в бинарном буфере
						size_t start = 0, stop = adj->frame.size;
						// Выполняем разбивку полезной нагрузки на сегменты
						while(stop < size){
							// Увеличиваем длину чанка
							stop += adj->frame.size;
							// Если длина чанка слишком большая, компенсируем
							stop = (stop > size ? size : stop);
							// Устанавливаем флаг финального сообщения
							head.fin = (stop == size);
							// Формируем чанк бинарных данных
							chunk.assign(message + start, message + stop);
							// Создаём буфер для отправки
							const auto & buffer = adj->frame.methods.set(head, chunk.data(), chunk.size());
							// Если бинарный буфер для отправки данных получен
							if(!buffer.empty())
								// Выполняем отправку сообщения на клиенту
								web2_t::send(adj->sid, aid, buffer.data(), buffer.size(), false);
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
						const auto & buffer = adj->frame.methods.set(head, message, size);
						// Если бинарный буфер для отправки данных получен
						if(!buffer.empty())
							// Выполняем отправку сообщения на клиенту
							web2_t::send(adj->sid, aid, buffer.data(), buffer.size(), false);
					}
				}
			}
			// Выполняем разблокировку отправки сообщения
			adj->allow.send = !adj->allow.send;
		}
	}
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const uint64_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для извлечения пароля
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <string (const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <bool (const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих ошибок
	this->_callback.set <void (const uint64_t, const u_int, const string &)> ("wserror", callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const uint64_t, const vector <char> &, const bool)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих сообщений
	this->_callback.set <void (const uint64_t, const vector <char> &, const bool)> ("message", callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функция обратного вызова при полном получении запроса клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const int32_t, const uint64_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const int32_t, const uint64_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const int32_t, const uint64_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const int32_t, const uint64_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода запроса клиента к серверу
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * port Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    порт подключения адъютанта
 */
u_int awh::server::WebSocket2::port(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getPort(aid);
}
/**
 * ip Метод получения IP-адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::WebSocket2::ip(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getIp(aid);
}
/**
 * mac Метод получения MAC-адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::WebSocket2::mac(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getMac(aid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::WebSocket2::stop() noexcept {
	// Выполняем остановку работы основного модуля
	web2_t::stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::WebSocket2::start() noexcept {
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
void awh::server::WebSocket2::close(const uint64_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr){
		// Если переключение протокола на HTTP/2 не выполнено
		if(adj->proto != engine_t::proto_t::HTTP2)
			// Выполняем закрытие подключения
			this->_ws1.close(aid);
		// Выполняем закрытие текущего подключения
		else {
			// Устанавливаем флаг закрытия подключения адъютанта
			adj->close = true;
			// Выполняем поиск адъютанта в списке активных сессий
			auto it = this->_sessions.find(aid);
			// Если активная сессия найдена
			if(it != this->_sessions.end())
				// Выполняем закрытие подключения
				it->second->close();
			// Выполняем отключение адъютанта
			const_cast <server::core_t *> (this->_core)->close(aid);
		}
	}
}
/**
 * sub Метод установки сабпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::server::WebSocket2::sub(const string & sub) noexcept {
	// Устанавливаем сабпротокол
	if(!sub.empty())
		// Выполняем установку сабпротокола
		this->_subs.push_back(sub);
}
/**
 * subs Метод установки списка сабпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::server::WebSocket2::subs(const vector <string> & subs) noexcept {
	// Если список сабпротоколов получен
	if(!subs.empty())
		// Выполняем установку сабпротоколов
		this->_subs = subs;
}
/**
 * sub Метод получения согласованного сабпротокола
 * @param aid идентификатор адъютанта
 * @return    выбранный сабпротокол
 */
const string & awh::server::WebSocket2::sub(const uint64_t aid) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Получаем параметры подключения адъютанта
	ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Если переключение протокола на HTTP/2 не выполнено
		if(adj->proto != engine_t::proto_t::HTTP2)
			// Выводим согласованный сабпротокол
			return this->_ws1.sub(aid);
		// Выполняем извлечение согласованного сабпротокола для текущего подключения
		else return adj->http.sub();
	}
	// Выводим результат по умолчанию
	return result;
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::server::WebSocket2::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка расширений
	this->_extensions = extensions;
}
/**
 * extensions Метод извлечения списка расширений
 * @param aid идентификатор адъютанта
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::WebSocket2::extensions(const uint64_t aid) const noexcept {
	// Результат работы функции
	static const vector <vector <string>> result;
	// Получаем параметры подключения адъютанта
	ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Если переключение протокола на HTTP/2 не выполнено
		if(adj->proto != engine_t::proto_t::HTTP2)
			// Выполняем извлечение списка поддерживаемых расширений WebSocket
			return this->_ws1.extensions(aid);
		// Выполняем извлечение списка поддерживаемых расширений для текущего подключения
		else return adj->http.extensions();
	}
	// Выводим результат по умолчанию
	return result;
}
/**
 * multiThreads Метод активации многопоточности
 * @param threads количество потоков для активации
 * @param mode    флаг активации/деактивации мультипоточности
 */
void awh::server::WebSocket2::multiThreads(const size_t threads, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Выполняем установку количества активных ядер
		this->_threads = threads;
		// Если многопоточность ещё не активированна
		if(!this->_thr.is())
			// Выполняем инициализацию пула потоков
			this->_thr.init(this->_threads);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->_thr.wait();
			// Выполняем инициализацию нового тредпула
			this->_thr.init(this->_threads);
		}
		// Если сетевое ядро установлено
		if(this->_core != nullptr)
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->_thr.wait();
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::WebSocket2::total(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	const_cast <server::core_t *> (this->_core)->total(this->_scheme.sid, total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::WebSocket2::segmentSize(const size_t size) noexcept {
	// Если размер сегмента фрейма передан
	if(size > 0)
		// Устанавливаем размер одного сегмента фрейма
		this->_frameSize = size;
	// Выполняем установку размеров сегментов фрейма для WebSocket-сервера
	this->_ws1.segmentSize(size);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::WebSocket2::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага автоматического перезапуска
	const_cast <server::core_t *> (this->_core)->clusterAutoRestart(this->_scheme.sid, mode);
}
/**
 * compress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::WebSocket2::compress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_scheme.compress = compress;
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::WebSocket2::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
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
void awh::server::WebSocket2::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для WebSocket-сервера
	this->_ws1.mode(flags);
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
void awh::server::WebSocket2::alive(const bool mode) noexcept {
	// Выполняем установку долгоживущего подключения
	web2_t::alive(mode);
	// Выполняем установку долгоживущего подключения для WebSocket-сервера
	this->_ws1.alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::WebSocket2::alive(const time_t time) noexcept {
	// Выполняем установку времени жизни подключения
	web2_t::alive(time);
	// Выполняем установку времени жизни подключения для WebSocket-сервера
	this->_ws1.alive(time);
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket2::core(const server::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
		// Активируем персистентный запуск для работы пингов
		const_cast <server::core_t *> (this->_core)->persistEnable(true);
		// Добавляем схемы сети в сетевое ядро
		const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
		// Устанавливаем функцию активации ядра сервера
		const_cast <server::core_t *> (this->_core)->on(std::bind(&ws2_t::eventsCallback, this, _1, _2));
		// Если многопоточность активированна
		if(this->_thr.is() || this->_ws1._thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_thr.is() || this->_ws1._thr.is()){
			// Выполняем завершение всех активных потоков
			this->_thr.wait();
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
 * setHeaders Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::server::WebSocket2::setHeaders(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку заголовков которые нужно передать клиенту
	this->_headers = headers;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::WebSocket2::waitTimeDetect(const time_t read, const time_t write) noexcept {
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
void awh::server::WebSocket2::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
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
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::WebSocket2::realm(const string & realm) noexcept {
	// Устанавливаем название сервера
	web2_t::realm(realm);
	// Устанавливаем название сервера для WebSocket-сервера
	this->_ws1.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::WebSocket2::opaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	web2_t::opaque(opaque);
	// Устанавливаем временный ключ сессии сервера для WebSocket-сервера
	this->_ws1.opaque(opaque);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::WebSocket2::chunk(const size_t size) noexcept {
	// Устанавливаем размера чанка
	web2_t::chunk(size);
	// Устанавливаем размера чанка для WebSocket-сервера
	this->_ws1.chunk(size);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::WebSocket2::maxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	web2_t::maxRequests(max);
	// Устанавливаем максимальное количество запросов для WebSocket-сервера
	this->_ws1.maxRequests(max);
}
/**
 * ident Метод установки идентификации сервера
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::WebSocket2::ident(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификацию сервера
	web2_t::ident(id, name, ver);
	// Устанавливаем идентификацию сервера для WebSocket-сервера
	this->_ws1.ident(id, name, ver);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::WebSocket2::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	web2_t::authType(type, hash);
	// Устанавливаем тип авторизации для WebSocket-сервера
	this->_ws1.authType(type, hash);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::WebSocket2::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования
	web2_t::crypto(pass, salt, cipher);
	// Устанавливаем параметры шифрования для WebSocket-сервера
	this->_ws1.crypto(pass, salt, cipher);
}
/**
 * WebSocket2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::WebSocket2::WebSocket2(const fmk_t * fmk, const log_t * log) noexcept : web2_t(fmk, log), _threads(0), _frameSize(0), _ws1(fmk, log), _scheme(fmk, log) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&ws2_t::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&ws2_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&ws2_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&ws2_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&ws2_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws2_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта адъютанта
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&ws2_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
/**
 * WebSocket2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::WebSocket2::WebSocket2(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : web2_t(core, fmk, log), _threads(0), _frameSize(0), _ws1(fmk, log), _scheme(fmk, log) {
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&ws2_t::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&ws2_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&ws2_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&ws2_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&ws2_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws2_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта адъютанта
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&ws2_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
/**
 * ~WebSocket2 Деструктор
 */
awh::server::WebSocket2::~WebSocket2() noexcept {
	// Если многопоточность активированна
	if(this->_thr.is())
		// Выполняем завершение всех активных потоков
		this->_thr.wait();
	// Если многопоточность активированна у WebSocket/1.1 клиента
	if(this->_ws1._thr.is())
		// Выполняем завершение всех активных потоков
		this->_ws1._thr.wait();
}
