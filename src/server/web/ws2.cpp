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
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket2::connectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём брокера
		this->_scheme.set(bid);
		// Выполняем активацию HTTP/2 протокола
		web2_t::connectCallback(bid, sid, core);
		// Выполняем проверку инициализирован ли протокол HTTP/2 для текущего клиента
		auto it = this->_sessions.find(bid);
		// Если проктокол интернета HTTP/2 инициализирован для клиента
		if(it != this->_sessions.end()){
			// Получаем параметры активного клиента
			ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
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
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Устанавливаем функцию обратного вызова для вывода ошибок
					options->http.on((function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)>) this->_callback.get <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error"));
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
				// Если сервер требует авторизацию
				if(this->_service.type != auth_t::type_t::NONE){
					// Определяем тип авторизации
					switch(static_cast <uint8_t> (this->_service.type)){
						// Если тип авторизации Basic
						case static_cast <uint8_t> (auth_t::type_t::BASIC): {
							// Устанавливаем параметры авторизации
							options->http.authType(this->_service.type);
							// Если функция обратного вызова для обработки чанков установлена
							if(this->_callback.is("checkPassword"))
								// Устанавливаем функцию проверки авторизации
								options->http.authCallback(std::bind(this->_callback.get <bool (const uint64_t, const string &, const string &)> ("checkPassword"), bid, _1, _2));
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
							if(this->_callback.is("extractPassword"))
								// Устанавливаем функцию извлечения пароля
								options->http.extractPassCallback(std::bind(this->_callback.get <string (const uint64_t, const string &)> ("extractPassword"), bid, _1));
						} break;
					}
				}
			}
		// Если протокол HTTP/2 для клиента не инициализирован
		} else {
			// Устанавливаем флаг перехвата контекста компрессии
			this->_ws1._server.takeover = this->_server.takeover;
			// Устанавливаем флаг перехвата контекста декомпрессии
			this->_ws1._client.takeover = this->_client.takeover;
			// Устанавливаем метод компрессии поддерживаемый сервером
			this->_ws1._scheme.compressors = this->_scheme.compressors;
			// Выполняем установку сетевого ядра
			this->_ws1._core = dynamic_cast <server::core_t *> (core);
			// Если HTTP-заголовки установлены
			if(!this->_headers.empty())
				// Выполняем установку HTTP-заголовков
				this->_ws1.setHeaders(this->_headers);
			// Если сабпротоколы установлены
			if(!this->_subprotocols.empty())
				// Устанавливаем поддерживаемые сабпротоколы
				this->_ws1.subprotocols(this->_subprotocols);
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
			this->_ws1.connectCallback(bid, sid, core);
		}
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выводим функцию обратного вызова
			this->_callback.call <const uint64_t, const mode_t> ("active", bid, mode_t::CONNECT);
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении клиента
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket2::disconnectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Выполняем поиск брокера в списке активных сессий
		auto it = this->_sessions.find(bid);
		// Если активная сессия найдена
		if(it != this->_sessions.end())
			// Выполняем закрытие подключения
			it->second->close();
		// Выполняем отключение подключившегося брокера
		this->disconnect(bid);
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выводим функцию обратного вызова
			this->_callback.call <const uint64_t, const mode_t> ("active", bid, mode_t::DISCONNECT);
	}
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с клиента
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::WebSocket2::readCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Получаем параметры активного клиента
		ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если подключение закрыто
			if(options->close){
				// Принудительно выполняем отключение лкиента
				dynamic_cast <server::core_t *> (core)->close(bid);
				// Выходим из функции
				return;
			}
			// Выполняем установку протокола подключения
			options->proto = core->proto(bid);
			// Если протокол подключения является HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Если получение данных не разрешено
				if(!options->allow.receive)
					// Выходим из функции
					return;
				// Выполняем поиск брокера в списке активных сессий
				auto it = this->_sessions.find(bid);
				// Если активная сессия найдена
				if(it != this->_sessions.end()){
					// Если прочитать данные фрейма не удалось, выходим из функции
					if(!it->second->frame((const uint8_t *) buffer, size)){
						// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
						it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), dynamic_cast <server::core_t *> (core), bid));
						// Выходим из функции
						return;
					}
				}
			// Если активирован режим работы с HTTP/1.1 протоколом, выполняем переброс вызова чтения на клиент WebSocket
			} else this->_ws1.readCallback(buffer, size, bid, sid, core);
		}
	}
}
/**
 * writeCallback Функция обратного вызова при записи сообщение брокеру
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::WebSocket2::writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры активного клиента
		ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если переключение протокола на HTTP/2 не выполнено
			if(options->proto != engine_t::proto_t::HTTP2)
				// Выполняем переброс вызова записи клиенту WebSocket
				this->_ws1.writeCallback(buffer, size, bid, sid, core);
		}
	}
}
/**
 * chunkSignal Метод обратного вызова при получении чанка HTTP/2
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param buffer буфер данных который содержит полученный чанк
 * @param size   размер полученного буфера данных чанка
 * @return       статус полученных данных
 */
int awh::server::WebSocket2::chunkSignal(const int32_t sid, const uint64_t bid, const uint8_t * buffer, const size_t size) noexcept {
	// Получаем параметры активного клиента
	ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Если функция обратного вызова на перехват входящих чанков установлена
		if(this->_callback.is("chunking"))
			// Выводим функцию обратного вызова
			this->_callback.call <const uint64_t, const vector <char> &, const awh::http_t *> ("chunking", bid, vector <char> (buffer, buffer + size), &options->http);
		// Если функция перехвата полученных чанков не установлена
		else if(this->_core != nullptr) {
			// Если подключение закрыто
			if(options->close){
				// Принудительно выполняем отключение лкиента
				const_cast <server::core_t *> (this->_core)->close(bid);
				// Выходим из функции
				return 0;
			}
			// Если рукопожатие не выполнено
			if(!options->shake)
				// Добавляем полученный чанк в тело данных
				options->http.payload(vector <char> (buffer, buffer + size));
			// Если рукопожатие выполнено
			else if(options->allow.receive)
				// Добавляем полученные данные в буфер
				options->buffer.payload.insert(options->buffer.payload.end(), buffer, buffer + size);
			// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
			if(this->_callback.is("chunks"))
				// Выводим функцию обратного вызова
				this->_callback.call <const int32_t, const uint64_t, const vector <char> &> ("chunks", sid, bid, vector <char> (buffer, buffer + size));
		}
	}
	// Выводим результат
	return 0;
}
/**
 * frameSignal Метод обратного вызова при получении фрейма заголовков HTTP/2
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param direct направление передачи фрейма
 * @param type   тип полученного фрейма
 * @param flags  флаг полученного фрейма
 * @return       статус полученных данных
 */
int awh::server::WebSocket2::frameSignal(const int32_t sid, const uint64_t bid, const http2_t::direct_t direct, const uint8_t type, const uint8_t flags) noexcept {
	// Определяем направление передачи фрейма
	switch(static_cast <uint8_t> (direct)){
		// Если производится передача фрейма на сервер
		case static_cast <uint8_t> (http2_t::direct_t::SEND): {
			// Если мы получили флаг завершения потока
			if(flags & NGHTTP2_FLAG_END_STREAM){
				// Получаем параметры активного клиента
				ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
				// Если параметры активного клиента получены
				if((this->_core != nullptr) && (options != nullptr)){
					// Если необходимо выполнить закрыть подключение
					if(!options->close && options->stopped){
						// Устанавливаем флаг закрытия подключения
						options->close = !options->close;
						// Выполняем поиск брокера в списке активных сессий
						auto it = this->_sessions.find(bid);
						// Если активная сессия найдена
						if(it != this->_sessions.end()){
							// Выполняем закрытие подключения
							it->second->close();
							// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
							it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), bid));
						// Принудительно выполняем отключение лкиента
						} else const_cast <server::core_t *> (this->_core)->close(bid);
					}
				}
				// Если установлена функция отлова завершения запроса
				if(this->_callback.is("end"))
					// Выводим функцию обратного вызова
					this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", sid, bid, direct_t::SEND);
				// Выходим из функции
				return 0;
			}
		} break;
		// Если производится получения фрейма с сервера
		case static_cast <uint8_t> (http2_t::direct_t::RECV): {
			// Получаем параметры активного клиента
			ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if((this->_core != nullptr) && (options != nullptr)){
				// Выполняем определение типа фрейма
				switch(type){
					// Если мы получили входящие данные тела ответа
					case NGHTTP2_DATA: {
						// Если рукопожатие выполнено
						if(options->shake && options->allow.receive){
							// Если мы получили неустановленный флаг или флаг завершения потока
							if((flags == NGHTTP2_FLAG_NONE) || (flags & NGHTTP2_FLAG_END_STREAM)){
								// Флаг удачного получения данных
								bool receive = false;
								// Создаём буфер сообщения
								vector <char> buffer;
								// Создаём объект шапки фрейма
								ws::frame_t::head_t head;
								// Выполняем обработку полученных данных
								while(!options->close && options->allow.receive){
									// Выполняем чтение фрейма WebSocket
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
												this->pong(bid, const_cast <server::core_t *> (this->_core), string(data.begin(), data.end()));
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
												// Завершаем работу клиента
												if(!(options->stopped = web2_t::reject(sid, bid, 400)))
													// Завершаем работу
													const_cast <server::core_t *> (this->_core)->close(bid);
												// Если мы получили флаг завершения потока
												if(flags & NGHTTP2_FLAG_END_STREAM){
													// Если установлена функция отлова завершения запроса
													if(this->_callback.is("end"))
														// Выводим функцию обратного вызова
														this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", sid, bid, direct_t::RECV);
												}
												// Выходим из функции
												return NGHTTP2_ERR_CALLBACK_FAILURE;
											} break;
										}
									}
									// Если парсер обработал какое-то количество байт
									if((receive = ((head.frame > 0) && !options->buffer.payload.empty()))){
										// Если размер буфера больше количества удаляемых байт
										if((receive = (options->buffer.payload.size() >= head.frame)))
											// Удаляем количество обработанных байт
											options->buffer.payload.assign(options->buffer.payload.begin() + head.frame, options->buffer.payload.end());
											// vector <decltype(options->buffer.payload)::value_type> (options->buffer.payload.begin() + head.frame, options->buffer.payload.end()).swap(options->buffer.payload);
									}
									// Если сообщения получены
									if(!buffer.empty()){
										// Если тредпул активирован
										if(this->_thr.is())
											// Добавляем в тредпул новую задачу на извлечение полученных сообщений
											this->_thr.push(std::bind(&ws2_t::extraction, this, bid, buffer, (options->frame.opcode == ws::frame_t::opcode_t::TEXT)));
										// Если тредпул не активирован, выполняем извлечение полученных сообщений
										else this->extraction(bid, buffer, (options->frame.opcode == ws::frame_t::opcode_t::TEXT));
										// Очищаем буфер полученного сообщения
										buffer.clear();
									}
									// Если данные мы все получили, выходим
									if(!receive || options->buffer.payload.empty()) break;
								}
								// Если мы получили флаг завершения потока
								if(flags & NGHTTP2_FLAG_END_STREAM){
									// Если установлена функция отлова завершения запроса
									if(this->_callback.is("end"))
										// Выводим функцию обратного вызова
										this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", sid, bid, direct_t::RECV);
								}
								// Выходим из функции
								return 0;
							}
							// Устанавливаем метку остановки брокера
							Stop:
							// Отправляем серверу сообщение
							this->sendError(bid, options->mess);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выводим функцию обратного вызова
								this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::WARNING, http::error_t::WEBSOCKET, this->_fmk->format("%s [%u]", options->mess.code, options->mess.text.c_str()));
						}
					} break;
					// Если мы получили входящие данные заголовков ответа
					case NGHTTP2_HEADERS: {
						// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
						if(flags & NGHTTP2_FLAG_END_HEADERS){
							// Выполняем коммит полученного результата
							options->http.commit();
							// Выполняем извлечение параметров запроса
							const auto & request = options->http.request();
							// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
							if(this->_callback.is("request"))
								// Выводим функцию обратного вызова
								this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &> ("request", options->sid, bid, request.method, request.url);
							// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
							if(this->_callback.is("headers"))
								// Выводим функцию обратного вызова
								this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &> ("headers", options->sid, bid, request.method, request.url, options->http.headers());
							// Если рукопожатие не выполнено
							if(!reinterpret_cast <http_t &> (options->http).is(http_t::state_t::HANDSHAKE)){
								// Ответ клиенту по умолчанию успешный
								awh::web_t::res_t response(2.0f, static_cast <u_int> (200));
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									{
										// Получаем объект работы с HTTP-запросами
										const http_t & http = reinterpret_cast <http_t &> (options->http);
										// Получаем бинарные данные REST-ответа
										const auto & buffer = http.process(http_t::process_t::REQUEST, options->http.request());
										// Если параметры ответа получены
										if(!buffer.empty())
											// Выводим параметры ответа
											cout << string(buffer.begin(), buffer.end()) << endl << endl;
									}
								#endif
								// Выполняем проверку авторизации
								switch(static_cast <uint8_t> (options->http.auth())){
									// Если запрос выполнен удачно
									case static_cast <uint8_t> (http_t::status_t::GOOD): {
										// Если рукопожатие выполнено
										if((options->shake = options->http.handshake(http_t::process_t::REQUEST))){
											// Проверяем версию протокола
											if(!options->http.check(ws_core_t::flag_t::VERSION)){
												// Получаем бинарные данные REST запроса
												response = awh::web_t::res_t(2.0f, static_cast <u_int> (505), "Unsupported protocol version");
												// Завершаем работу
												break;
											}
											// Выполняем сброс состояния HTTP-парсера
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
											// Получаем заголовки ответа удалённому клиенту
											const auto & headers = options->http.process2(http_t::process_t::RESPONSE, response);
											// Если бинарные данные ответа получены
											if(!headers.empty()){
												/**
												 * Если включён режим отладки
												 */
												#if defined(DEBUG_MODE)
													{
														// Выводим заголовок ответа
														cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
														// Получаем объект работы с HTTP-запросами
														const http_t & http = reinterpret_cast <http_t &> (options->http);
														// Получаем бинарные данные REST-ответа
														const auto & buffer = http.process(http_t::process_t::RESPONSE, response);
														// Если бинарные данные ответа получены
														if(!buffer.empty())
															// Выводим параметры ответа
															cout << string(buffer.begin(), buffer.end()) << endl << endl;
													}
												#endif
												// Выполняем ответ подключившемуся клиенту
												int32_t sid = web2_t::send(options->sid, bid, headers, http2_t::flag_t::NONE);
												// Если запрос не получилось отправить
												if(sid < 0){
													// Если мы получили флаг завершения потока
													if(flags & NGHTTP2_FLAG_END_STREAM){
														// Если установлена функция отлова завершения запроса
														if(this->_callback.is("end"))
															// Выводим функцию обратного вызова
															this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", options->sid, bid, direct_t::RECV);
													}
													// Выходим из функции
													return NGHTTP2_ERR_CALLBACK_FAILURE;
												}
												// Если функция обратного вызова активности потока установлена
												if(this->_callback.is("stream"))
													// Выполняем функцию обратного вызова
													this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", options->sid, bid, mode_t::OPEN);
												// Если функция обратного вызова на получение удачного запроса установлена
												if(this->_callback.is("handshake"))
													// Выполняем функцию обратного вызова
													this->_callback.call <const int32_t, const uint64_t, const agent_t> ("handshake", options->sid, bid, agent_t::WEBSOCKET);
												// Если мы получили флаг завершения потока
												if(flags & NGHTTP2_FLAG_END_STREAM){
													// Если установлена функция отлова завершения запроса
													if(this->_callback.is("end"))
														// Выводим функцию обратного вызова
														this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", options->sid, bid, direct_t::RECV);
												}
												// Завершаем работу
												return 0;
											// Формируем ответ, что произошла внутренняя ошибка сервера
											} else response = awh::web_t::res_t(2.0f, static_cast <u_int> (500));
										// Формируем ответ, что страница не доступна
										} else response = awh::web_t::res_t(2.0f, static_cast <u_int> (403), "Handshake failed");
									} break;
									// Если запрос неудачный
									case static_cast <uint8_t> (http_t::status_t::FAULT):
										// Формируем ответ на запрос об авторизации
										response = awh::web_t::res_t(2.0f, static_cast <u_int> (401));
									break;
									// Если результат определить не получилось
									default: response = awh::web_t::res_t(2.0f, static_cast <u_int> (506), "Unknown request");
								}
								// Выполняем очистку данных HTTP-парсера
								options->http.clear();
								// Выполняем сброс состояния HTTP-парсера
								options->http.reset();
								// Получаем заголовки ответа удалённому клиенту
								const auto & headers = options->http.reject2(response);
								// Если бинарные данные ответа получены
								if(!headers.empty()){
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										{
											// Выводим заголовок ответа
											cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
											// Получаем объект работы с HTTP-запросами
											const http_t & http = reinterpret_cast <http_t &> (options->http);
											// Получаем бинарные данные REST-ответа
											const auto & buffer = http.process(http_t::process_t::RESPONSE, response);
											// Если бинарные данные ответа получены
											if(!buffer.empty())
												// Выводим параметры ответа
												cout << string(buffer.begin(), buffer.end()) << endl << endl;
										}
									#endif
									// Флаг отправляемого фрейма
									http2_t::flag_t flag = http2_t::flag_t::NONE;
									// Если тело запроса не существует
									if(options->http.body().empty())
										// Устанавливаем флаг завершения потока
										flag = http2_t::flag_t::END_STREAM;
									// Выполняем заголовки запроса на сервер
									const int32_t sid = web2_t::send(options->sid, bid, headers, flag);
									// Если запрос не получилось отправить
									if(sid < 0){
										// Если мы получили флаг завершения потока
										if(flags & NGHTTP2_FLAG_END_STREAM){
											// Если установлена функция отлова завершения запроса
											if(this->_callback.is("end"))
												// Выводим функцию обратного вызова
												this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", options->sid, bid, direct_t::RECV);
										}
										// Выходим из функции
										return NGHTTP2_ERR_CALLBACK_FAILURE;
									}
									// Если тело запроса существует
									if(!options->http.body().empty()){
										// Тело WEB запроса
										vector <char> entity;
										// Получаем данные тела запроса
										while(!(entity = options->http.payload()).empty()){
											/**
											 * Если включён режим отладки
											 */
											#if defined(DEBUG_MODE)
												// Выводим сообщение о выводе чанка тела
												cout << this->_fmk->format("<chunk %zu>", entity.size()) << endl << endl;
											#endif
											// Если нужно установить флаг закрытия потока
											if(options->http.body().empty())
												// Устанавливаем флаг завершения потока
												flag = http2_t::flag_t::END_STREAM;
											// Выполняем отправку тела запроса на сервер
											if(!web2_t::send(options->sid, bid, entity.data(), entity.size(), flag)){
												// Если мы получили флаг завершения потока
												if(flags & NGHTTP2_FLAG_END_STREAM){
													// Если установлена функция отлова завершения запроса
													if(this->_callback.is("end"))
														// Выводим функцию обратного вызова
														this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", options->sid, bid, direct_t::RECV);
												}
												// Выходим из функции
												return NGHTTP2_ERR_CALLBACK_FAILURE;
											}
										}
									}
									// Если мы получили флаг завершения потока
									if(flags & NGHTTP2_FLAG_END_STREAM){
										// Если установлена функция отлова завершения запроса
										if(this->_callback.is("end"))
											// Выводим функцию обратного вызова
											this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", options->sid, bid, direct_t::RECV);
									}
									// Завершаем работу
									return 0;
								}
							}
							// Выполняем поиск брокера в списке активных сессий
							auto it = this->_sessions.find(bid);
							// Если активная сессия найдена
							if(it != this->_sessions.end())
								// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
								it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), bid));
							// Завершаем работу
							else const_cast <server::core_t *> (this->_core)->close(bid);
							// Если мы получили флаг завершения потока
							if(flags & NGHTTP2_FLAG_END_STREAM){
								// Если установлена функция отлова завершения запроса
								if(this->_callback.is("end"))
									// Выводим функцию обратного вызова
									this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", options->sid, bid, direct_t::RECV);
							}
						}
					} break;
				}
			}
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * closedSignal Метод завершения работы потока
 * @param sid   идентификатор потока
 * @param bid   идентификатор брокера
 * @param error флаг ошибки HTTP/2 если присутствует
 * @return      статус полученных данных
 */
int awh::server::WebSocket2::closedSignal(const int32_t sid, const uint64_t bid, const uint32_t error) noexcept {
	// Определяем тип получаемой ошибки
	switch(error){
		// Если получена ошибка протокола
		case 0x1: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "PROTOCOL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_PROTOCOL, this->_fmk->format("Stream %d closed with error=%s", sid, "PROTOCOL_ERROR"));
		} break;
		// Если получена ошибка реализации
		case 0x2: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "INTERNAL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_INTERNAL, this->_fmk->format("Stream %d closed with error=%s", sid, "INTERNAL_ERROR"));
		} break;
		// Если получена ошибка превышения предела управления потоком
		case 0x3: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "FLOW_CONTROL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_FLOW_CONTROL, this->_fmk->format("Stream %d closed with error=%s", sid, "FLOW_CONTROL_ERROR"));
		} break;
		// Если установка не подтверждённа
		case 0x4: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "SETTINGS_TIMEOUT");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SETTINGS_TIMEOUT, this->_fmk->format("Stream %d closed with error=%s", sid, "SETTINGS_TIMEOUT"));
		} break;
		// Если получен кадр для завершения потока
		case 0x5: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "STREAM_CLOSED");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_STREAM_CLOSED, this->_fmk->format("Stream %d closed with error=%s", sid, "STREAM_CLOSED"));
		} break;
		// Если размер кадра некорректен
		case 0x6: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "FRAME_SIZE_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_FRAME_SIZE, this->_fmk->format("Stream %d closed with error=%s", sid, "FRAME_SIZE_ERROR"));
		} break;
		// Если поток не обработан
		case 0x7: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "REFUSED_STREAM");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_REFUSED_STREAM, this->_fmk->format("Stream %d closed with error=%s", sid, "REFUSED_STREAM"));
		} break;
		// Если поток аннулирован
		case 0x8: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "CANCEL");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_CANCEL, this->_fmk->format("Stream %d closed with error=%s", sid, "CANCEL"));
		} break;
		// Если состояние компрессии не обновлено
		case 0x9: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "COMPRESSION_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_COMPRESSION, this->_fmk->format("Stream %d closed with error=%s", sid, "COMPRESSION_ERROR"));
		} break;
		// Если получена ошибка TCP-соединения для метода CONNECT
		case 0xA: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "CONNECT_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_CONNECT, this->_fmk->format("Stream %d closed with error=%s", sid, "CONNECT_ERROR"));
		} break;
		// Если превышена емкость для обработки
		case 0xB: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "ENHANCE_YOUR_CALM");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_ENHANCE_YOUR_CALM, this->_fmk->format("Stream %d closed with error=%s", sid, "ENHANCE_YOUR_CALM"));
		} break;
		// Если согласованные параметры SSL не приемлемы
		case 0xC: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "INADEQUATE_SECURITY");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_INADEQUATE_SECURITY, this->_fmk->format("Stream %d closed with error=%s", sid, "INADEQUATE_SECURITY"));
		} break;
		// Если для запроса используется HTTP/1.1
		case 0xD: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d [ID=%zu] closed with error=%s", log_t::flag_t::CRITICAL, sid, bid, "HTTP_1_1_REQUIRED");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_HTTP_1_1_REQUIRED, this->_fmk->format("Stream %d closed with error=%s", sid, "HTTP_1_1_REQUIRED"));
		} break;
	}
	// Если разрешено выполнить остановку
	if((this->_core != nullptr) && (error > 0x00)){
		// Выполняем поиск брокера в списке активных сессий
		auto it = this->_sessions.find(bid);
		// Если активная сессия найдена
		if(it != this->_sessions.end())
			// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
			it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), bid));
	}
	// Если функция обратного вызова активности потока установлена
	if(this->_callback.is("stream"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", sid, bid, mode_t::CLOSE);
	// Выводим результат
	return 0;
}
/**
 * beginSignal Метод начала получения фрейма заголовков HTTP/2
 * @param sid  идентификатор потока
 * @param bid  идентификатор брокера
 * @param head идентификатор заголовка
 * @return     статус полученных данных
 */
int awh::server::WebSocket2::beginSignal(const int32_t sid, const uint64_t bid, const http2_t::head_t head) noexcept {
	// Если заголовок соответствует HTTP-заголовку
	if(head == http2_t::head_t::HEADER){
		// Получаем параметры активного клиента
		ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Устанавливаем новый идентификатор потока
			options->sid = sid;
			// Выполняем сброс флага рукопожатия
			options->shake = false;
			// Выполняем очистку параметров HTTP запроса
			options->http.clear();
			// Очищаем буфер собранных данных
			options->buffer.payload.clear();
			// Выполняем очистку оставшихся фрагментов
			options->buffer.fragmes.clear();
		}
	}
	// Выводим результат
	return 0;
}
/**
 * headerSignal Метод обратного вызова при получении заголовка HTTP/2
 * @param sid  идентификатор потока
 * @param bid  идентификатор брокера
 * @param key  данные ключа заголовка
 * @param val  данные значения заголовка
 * @param head идентификатор заголовка
 * @return     статус полученных данных
 */
int awh::server::WebSocket2::headerSignal(const int32_t sid, const uint64_t bid, const string & key, const string & val, const http2_t::head_t head) noexcept {
	// Если заголовок соответствует HTTP-заголовку
	if(head == http2_t::head_t::HEADER){
		// Получаем параметры активного клиента
		ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Устанавливаем полученные заголовки
			options->http.header2(key, val);
			// Если функция обратного вызова на полученного заголовка с сервера установлена
			if(this->_callback.is("header"))
				// Выводим функцию обратного вызова
				this->_callback.call <const int32_t, const uint64_t, const string &, const string &> ("header", sid, bid, key, val);
		}
	}
	// Выводим результат
	return 0;
}
/**
 * error Метод вывода сообщений об ошибках работы брокера
 * @param bid     идентификатор брокера
 * @param message сообщение с описанием ошибки
 */
void awh::server::WebSocket2::error(const uint64_t bid, const ws::mess_t & message) const noexcept {
	// Получаем параметры активного клиента
	ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
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
			// Если функция обратного вызова при получении ошибки WebSocket установлена
			if(this->_callback.is("wserror"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const u_int, const string &> ("wserror", bid, message.code, message.text);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::WARNING, http::error_t::WEBSOCKET, this->_fmk->format("%s [%u]", message.code, message.text.c_str()));
		}
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param bid    идентификатор брокера
 * @param buffer данные в чистом виде полученные с сервера
 * @param text   данные передаются в текстовом виде
 */
void awh::server::WebSocket2::extraction(const uint64_t bid, const vector <char> & buffer, const bool text) noexcept {
	// Если буфер данных передан
	if((bid > 0) && !buffer.empty() && this->_callback.is("message")){
		// Получаем параметры активного клиента
		ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
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
					// Если метод компрессии выбран Deflate
					case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
						// Устанавливаем размер скользящего окна
						options->hash.wbit(options->client.wbit);
						// Добавляем хвост в полученные данные
						options->hash.setTail(* const_cast <vector <char> *> (&buffer));
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
					} break;
					// Если метод компрессии выбран GZip
					case static_cast <uint8_t> (http_t::compress_t::GZIP):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::GZIP);
					break;
					// Если метод компрессии выбран Brotli
					case static_cast <uint8_t> (http_t::compress_t::BROTLI):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BROTLI);
					break;
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
							this->_callback.call <const uint64_t, const vector <char> &, const bool> ("message", bid, res, text);
							// Выходим из функции
							return;
						}
					}
					// Выводим сообщение так - как оно пришло
					this->_callback.call <const uint64_t, const vector <char> &, const bool> ("message", bid, data, text);
				// Выводим сообщение об ошибке
				} else if(this->_core != nullptr) {
					// Создаём сообщение
					options->mess = ws::mess_t(1007, "Received data decompression error");
					// Получаем буфер сообщения
					data = options->frame.methods.message(options->mess);
					// Если данные сообщения получены
					if((options->stopped = !data.empty()))
						// Выполняем отправку сообщения клиенту
						web2_t::send(options->sid, bid, data.data(), data.size(), http2_t::flag_t::END_STREAM);
					// Завершаем работу
					else const_cast <server::core_t *> (this->_core)->close(bid);
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
						this->_callback.call <const uint64_t, const vector <char> &, const bool> ("message", bid, res, text);
						// Выходим из функции
						return;
					}
				}
				// Выводим сообщение так - как оно пришло
				this->_callback.call <const uint64_t, const vector <char> &, const bool> ("message", bid, buffer, text);
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
void awh::server::WebSocket2::pong(const uint64_t bid, awh::core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((bid > 0) && (core != nullptr)){
		// Получаем параметры активного клиента
		ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если отправка сообщений разблокированна
		if((options != nullptr) && options->allow.send){
			// Создаём буфер для отправки
			const auto & buffer = options->frame.methods.pong(message);
			// Если буфер данных получен
			if(!buffer.empty())
				// Выполняем отправку сообщения клиенту
				web2_t::send(options->sid, bid, buffer.data(), buffer.size(), http2_t::flag_t::NONE);
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param bid  идентификатор брокера
 * @param core объект сетевого ядра
 * @param      message сообщение для отправки
 */
void awh::server::WebSocket2::ping(const uint64_t bid, awh::core_t * core, const string & message) noexcept {
	// Если необходимые данные переданы
	if((bid > 0) && (core != nullptr) && core->working()){
		// Получаем параметры активного клиента
		ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если отправка сообщений разблокированна
		if((options != nullptr) && options->allow.send){
			// Создаём буфер для отправки
			const auto & buffer = options->frame.methods.ping(message);
			// Если буфер данных получен
			if(!buffer.empty())
				// Выполняем отправку сообщения клиенту
				web2_t::send(options->sid, bid, buffer.data(), buffer.size(), http2_t::flag_t::NONE);
		}
	}
}
/**
 * erase Метод удаления отключившихся брокеров
 * @param bid идентификатор брокера
 */
void awh::server::WebSocket2::erase(const uint64_t bid) noexcept {
	// Если список отключившихся брокеров не пустой
	if(!this->_disconected.empty()){
		/**
		 * eraseFn Функция удаления отключившегося брокера
		 * @param bid идентификатор брокера
		 */
		auto eraseFn = [this](const uint64_t bid) noexcept -> void {
			// Получаем параметры активного клиента
			ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Устанавливаем флаг отключения
				options->close = true;
				// Выполняем очистку оставшихся данных
				options->buffer.payload.clear();
				// Выполняем очистку оставшихся фрагментов
				options->buffer.fragmes.clear();
				// Если переключение протокола на HTTP/2 не выполнено
				if(options->proto != engine_t::proto_t::HTTP2)
					// Выполняем очистку отключившихся брокеров у WebSocket-сервера
					this->_ws1.erase(bid);
				// Выполняем удаление созданной ранее сессии HTTP/2
				else this->_sessions.erase(bid);
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
 * disconnect Метод отключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::WebSocket2::disconnect(const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены и переключение протокола на HTTP/2 не выполнено
	if((options != nullptr) && (options->proto != engine_t::proto_t::HTTP2))
		// Добавляем в очередь список отключившихся клиентов
		this->_ws1.disconnect(bid);
	// Добавляем в очередь список отключившихся клиентов
	this->_disconected.emplace(bid, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
}
/**
 * pinging Метод таймера выполнения пинга клиента
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket2::pinging(const uint16_t tid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((tid > 0) && (core != nullptr) && (this->_core != nullptr)){
		// Выполняем перебор всех активных клиентов
		for(auto & item : this->_scheme.get()){
			// Если переключение протокола на HTTP/2 не выполнено
			if(item.second->proto != engine_t::proto_t::HTTP2)
				// Выполняем переброс события пинга в модуль WebSocket
				this->_ws1.pinging(tid, core);
			// Если переключение протокола на HTTP/2 выполнено
			else if(item.second->allow.receive) {
				// Если рукопожатие выполнено
				if(item.second->shake){
					// Получаем текущий штамп времени
					const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					// Если брокер не ответил на пинг больше двух интервалов, отключаем его
					if(item.second->close || ((stamp - item.second->point) >= (PING_INTERVAL * 5)))
						// Завершаем работу
						const_cast <server::core_t *> (this->_core)->close(item.first);
					// Отправляем запрос брокеру
					else this->ping(item.first, const_cast <server::core_t *> (this->_core), ::to_string(item.first));
				// Если рукопожатие не выполнено и пинг не прошёл
				} else if(!web2_t::ping(item.first)) {
					// Выполняем поиск брокера в списке активных сессий
					auto it = this->_sessions.find(item.first);
					// Если активная сессия найдена
					if(it != this->_sessions.end())
						// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
						it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), item.first));
				}
			}
		}
	}
}
/**
 * init Метод инициализации WebSocket-сервера
 * @param socket      unix-сокет для биндинга
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::WebSocket2::init(const string & socket, const vector <http_t::compress_t> & compressors) noexcept {
	// Устанавливаем писок поддерживаемых компрессоров
	this->_scheme.compressors = compressors;
	// Выполняем инициализацию родительского объекта
	web2_t::init(socket, compressors);
}
/**
 * init Метод инициализации WebSocket-сервера
 * @param port        порт сервера
 * @param host        хост сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::WebSocket2::init(const u_int port, const string & host, const vector <http_t::compress_t> & compressors) noexcept {
	// Устанавливаем писок поддерживаемых компрессоров
	this->_scheme.compressors = compressors;
	// Выполняем инициализацию родительского объекта
	web2_t::init(port, host, compressors);
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param bid  идентификатор брокера
 * @param mess отправляемое сообщение об ошибке
 */
void awh::server::WebSocket2::sendError(const uint64_t bid, const ws::mess_t & mess) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Если код ошибки относится к WebSocket
		if(mess.code >= 1000){
			// Получаем параметры активного клиента
			ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr)
				// Запрещаем получение данных
				options->allow.receive = false;
			// Если отправка сообщений разблокированна
			if((options != nullptr) && options->allow.send){
				// Если переключение протокола на HTTP/2 не выполнено
				if(options->proto != engine_t::proto_t::HTTP2)
					// Выполняем отправку сообщение об ошибке на клиент WebSocket
					this->_ws1.sendError(bid, mess);
				// Если переключение протокола на HTTP/2 выполнено
				else {
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
						// Выполняем отправку сообщения клиенту
						web2_t::send(options->sid, bid, buffer.data(), buffer.size(), http2_t::flag_t::END_STREAM);
						// Выходим из функции
						return;
					}
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
void awh::server::WebSocket2::sendMessage(const uint64_t bid, const vector <char> & message, const bool text) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && (bid > 0) && !message.empty()){
		// Получаем параметры активного клиента
		ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если отправка сообщений разблокированна
		if((options != nullptr) && options->allow.send){
			// Выполняем блокировку отправки сообщения
			options->allow.send = !options->allow.send;
			// Если переключение протокола на HTTP/2 не выполнено
			if(options->proto != engine_t::proto_t::HTTP2)
				// Выполняем отправку сообщения клиенту WebSocket
				this->_ws1.sendMessage(bid, message, text);
			// Если переключение протокола на HTTP/2 выполнено
			else {
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
							// Если метод компрессии выбран Deflate
							case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
								// Устанавливаем размер скользящего окна
								options->hash.wbit(options->server.wbit);
								// Выполняем компрессию полученных данных
								data = options->hash.compress(message.data(), message.size(), hash_t::method_t::DEFLATE);
								// Удаляем хвост в полученных данных
								options->hash.rmTail(data);
							} break;
							// Если метод компрессии выбран GZip
							case static_cast <uint8_t> (http_t::compress_t::GZIP):
								// Выполняем компрессию полученных данных
								data = options->hash.compress(message.data(), message.size(), hash_t::method_t::GZIP);
							break;
							// Если метод компрессии выбран Brotli
							case static_cast <uint8_t> (http_t::compress_t::BROTLI):
								// Выполняем компрессию полученных данных
								data = options->hash.compress(message.data(), message.size(), hash_t::method_t::BROTLI);
							break;
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
								// Выполняем отправку сообщения на клиенту
								web2_t::send(options->sid, bid, payload.data(), payload.size(), http2_t::flag_t::NONE);
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
							// Выполняем отправку сообщения на клиенту
							web2_t::send(options->sid, bid, payload.data(), payload.size(), http2_t::flag_t::NONE);
					}
				}
			}
			// Выполняем разблокировку отправки сообщения
			options->allow.send = !options->allow.send;
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
void awh::server::WebSocket2::on(function <string (const uint64_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <bool (const uint64_t, const string &, const string &)> callback) noexcept {
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
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации брокера на сервере
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
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const uint64_t, const vector <char> &, const bool)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
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
 * on Метод установки функция обратного вызова при выполнении рукопожатия
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const int32_t, const uint64_t, const agent_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket2::on(function <void (const int32_t, const uint64_t, const direct_t)> callback) noexcept {
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
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
u_int awh::server::WebSocket2::port(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.port(bid);
}
/**
 * agent Метод извлечения агента клиента
 * @param bid идентификатор брокера
 * @return    агент к которому относится подключённый клиент
 */
awh::server::web_t::agent_t awh::server::WebSocket2::agent(const uint64_t bid) const noexcept {
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
const string & awh::server::WebSocket2::ip(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::WebSocket2::mac(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.mac(bid);
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
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr){
		// Если биндинг не запущен
		if(!this->_core->working())
			// Выполняем запуск биндинга
			const_cast <server::core_t *> (this->_core)->start();
		// Если биндинг уже запущен, выполняем запуск
		else this->openCallback(this->_scheme.sid, dynamic_cast <awh::core_t *> (const_cast <server::core_t *> (this->_core)));
	}
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::WebSocket2::close(const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены, устанавливаем флаг закрытия подключения
	if(options != nullptr){
		// Если переключение протокола на HTTP/2 не выполнено
		if(options->proto != engine_t::proto_t::HTTP2)
			// Выполняем закрытие подключения
			this->_ws1.close(bid);
		// Выполняем закрытие текущего подключения
		else if(this->_core != nullptr) {
			// Устанавливаем флаг закрытия подключения брокера
			options->close = true;
			// Выполняем поиск брокера в списке активных сессий
			auto it = this->_sessions.find(bid);
			// Если активная сессия найдена
			if(it != this->_sessions.end()){
				// Выполняем закрытие подключения
				it->second->close();
				// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
				it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), bid));
			// Выполняем отключение брокера
			} else const_cast <server::core_t *> (this->_core)->close(bid);
		}
	}
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::server::WebSocket2::subprotocol(const string & subprotocol) noexcept {
	// Устанавливаем сабпротокол
	if(!subprotocol.empty())
		// Выполняем установку сабпротокола
		this->_subprotocols.emplace(subprotocol);
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::server::WebSocket2::subprotocols(const set <string> & subprotocols) noexcept {
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
const set <string> & awh::server::WebSocket2::subprotocols(const uint64_t bid) const noexcept {
	// Результат работы функции
	static const set <string> result;
	// Получаем параметры активного клиента
	ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Если переключение протокола на HTTP/2 не выполнено
		if(options->proto != engine_t::proto_t::HTTP2)
			// Выводим согласованный сабпротокол
			return this->_ws1.subprotocols(bid);
		// Выполняем извлечение согласованного сабпротокола для текущего подключения
		else return options->http.subprotocols();
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
 * @param bid идентификатор брокера
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::WebSocket2::extensions(const uint64_t bid) const noexcept {
	// Результат работы функции
	static const vector <vector <string>> result;
	// Получаем параметры активного клиента
	ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Если переключение протокола на HTTP/2 не выполнено
		if(options->proto != engine_t::proto_t::HTTP2)
			// Выполняем извлечение списка поддерживаемых расширений WebSocket
			return this->_ws1.extensions(bid);
		// Выполняем извлечение списка поддерживаемых расширений для текущего подключения
		else return options->http.extensions();
	}
	// Выводим результат по умолчанию
	return result;
}
/**
 * multiThreads Метод активации многопоточности
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::server::WebSocket2::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Выполняем установку количества активных ядер
		this->_threads = count;
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
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr)
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
	// Иначе устанавливаем размер сегментов по умолчанию
	else this->_frameSize = 0xFA000;
	// Выполняем установку размеров сегментов фрейма для WebSocket-сервера
	this->_ws1.segmentSize(size);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::WebSocket2::clusterAutoRestart(const bool mode) noexcept {
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
void awh::server::WebSocket2::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
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
void awh::server::WebSocket2::compressors(const vector <http_t::compress_t> & compressors) noexcept {
	// Устанавливаем список поддерживаемых компрессоров
	this->_scheme.compressors = compressors;
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
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::server::WebSocket2::settings(const map <settings_t, uint32_t> & settings) noexcept {
	// Выполняем установку основных настроек протокола HTTP/2
	web2_t::settings(settings);
	// Если метод CONNECT не установлен, разрешаем его по умолчанию
	if(this->_settings.count(settings_t::CONNECT) == 0)
		// Выполняем установку разрешения использования метода CONNECT
		this->_settings.emplace(settings_t::CONNECT, 1);
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
 * crypted Метод получения флага шифрования
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::server::WebSocket2::crypted(const uint64_t bid) const noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если переключение протокола на HTTP/2 не выполнено
			if(options->proto != engine_t::proto_t::HTTP2)
				// Выводим установленный флаг шифрования клиента WebSocket
				return this->_ws1.crypted(bid);
			// Выводим установленный флаг шифрования
			return options->crypted;
		}
	}
	// Выводим результат
	return false;
}
/**
 * encrypt Метод активации шифрования для клиента
 * @param bid  идентификатор брокера
 * @param mode флаг активации шифрования
 */
void awh::server::WebSocket2::encrypt(const uint64_t bid, const bool mode) noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		ws_scheme_t::options_t * options = const_cast <ws_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если переключение протокола на HTTP/2 не выполнено
			if(options->proto != engine_t::proto_t::HTTP2)
				// Устанавливаем флаг шифрования для клиента WebSocket
				this->_ws1.encrypt(bid, mode);
			// Устанавливаем флаг шифрования для клиента
			else options->crypted = mode;
		}
	}
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::server::WebSocket2::encryption(const bool mode) noexcept {
	// Устанавливаем флага шифрования
	web2_t::encryption(mode);
	// Устанавливаем флага шифрования для WebSocket-сервера
	this->_ws1.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::WebSocket2::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования
	web2_t::encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для WebSocket-сервера
	this->_ws1.encryption(pass, salt, cipher);
}
/**
 * WebSocket2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::WebSocket2::WebSocket2(const fmk_t * fmk, const log_t * log) noexcept : web2_t(fmk, log), _threads(0), _frameSize(0xFA000), _ws1(fmk, log), _scheme(fmk, log) {
	// Выполняем установку список настроек протокола HTTP/2
	this->settings();
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&ws2_t::openCallback, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&ws2_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&ws2_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&ws2_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws2_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта брокера
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&ws2_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
/**
 * WebSocket2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::WebSocket2::WebSocket2(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : web2_t(core, fmk, log), _threads(0), _frameSize(0xFA000), _ws1(fmk, log), _scheme(fmk, log) {
	// Выполняем установку список настроек протокола HTTP/2
	this->settings();
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&ws2_t::openCallback, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&ws2_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&ws2_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&ws2_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws2_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта брокера
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
}
