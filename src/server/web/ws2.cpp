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
 * connectEvents Метод обратного вызова при подключении к серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::server::Websocket2::connectEvents(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
		// Создаём брокера
		this->_scheme.set(bid);
		// Выполняем активацию HTTP/2 протокола
		web2_t::connectEvents(bid, sid);
		// Выполняем проверку инициализирован ли протокол HTTP/2 для текущего клиента
		auto i = this->_sessions.find(bid);
		// Если проктокол интернета HTTP/2 инициализирован для клиента
		if(i != this->_sessions.end()){
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
				// Устанавливаем режим работы буфера
				options->buffer.payload = buffer_t::mode_t::COPY;
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
				if(this->_callbacks.is("error"))
					// Устанавливаем функцию обратного вызова для вывода ошибок
					options->http.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_callbacks.get <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error"));
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
			this->_ws1._scheme.compressors = this->_scheme.compressors;
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
				this->_thr.stop();
				// Выполняем инициализацию нового тредпула
				this->_ws1.multiThreads(this->_threads);
			}
			// Выполняем переброс вызова коннекта на клиент Websocket
			this->_ws1.connectEvents(bid, sid);
		}
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callbacks.is("active"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const uint64_t, const mode_t)> ("active", bid, mode_t::CONNECT);
	}
}
/**
 * disconnectEvents Метод обратного вызова при отключении клиента
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::server::Websocket2::disconnectEvents(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
		// Выполняем поиск брокера в списке активных сессий
		auto i = this->_sessions.find(bid);
		// Если активная сессия найдена
		if(i != this->_sessions.end())
			// Выполняем закрытие подключения
			i->second->close();
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
 */
void awh::server::Websocket2::readEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
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
				if(options->close)
					// Выходим из функции
					return;
				// Выполняем установку протокола подключения
				options->proto = this->_core->proto(bid);
				// Если протокол подключения является HTTP/2
				if(options->proto == engine_t::proto_t::HTTP2){
					// Если получение данных не разрешено
					if(!options->allow.receive)
						// Выходим из функции
						return;
					// Выполняем поиск брокера в списке активных сессий
					auto i = this->_sessions.find(bid);
					// Если активная сессия найдена
					if(i != this->_sessions.end()){
						// Если прочитать данные фрейма не удалось, выходим из функции
						if(!i->second->frame(reinterpret_cast <const uint8_t *> (buffer), size)){
							// Выполняем закрытие подключения
							web2_t::close(bid);
							// Выходим из функции
							return;
						}
					}
				// Если активирован режим работы с HTTP/1.1 протоколом, выполняем переброс вызова чтения на клиент Websocket
				} else this->_ws1.readEvents(buffer, size, bid, sid);
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
 */
void awh::server::Websocket2::writeEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0)){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если переключение протокола на HTTP/2 не выполнено
			if(options->proto != engine_t::proto_t::HTTP2)
				// Выполняем переброс вызова записи клиенту Websocket
				this->_ws1.writeEvents(buffer, size, bid, sid);
		}
	}
}
/**
 * callbacksEvents Метод отлавливания событий контейнера функций обратного вызова
 * @param event событие контейнера функций обратного вызова
 * @param idw   идентификатор функции обратного вызова
 * @param name  название функции обратного вызова
 * @param dump  дамп данных функции обратного вызова
 */
void awh::server::Websocket2::callbacksEvents(const fn_t::event_t event, const uint64_t idw, const string & name, const fn_t::dump_t * dump) noexcept {
	// Определяем входящее событие контейнера функций обратного вызова
	switch(static_cast <uint8_t> (event)){
		// Если событием является установка функции обратного вызова
		case static_cast <uint8_t> (fn_t::event_t::SET): {
			// Если дамп функции обратного вызова передан и событие не является событием подключения
			if((dump != nullptr) && !this->_fmk->compare(name, "active")){
				// Создаём локальный контейнер функций обратного вызова
				fn_t callbacks(this->_log);
				// Выполняем установку функции обратного вызова
				callbacks.dump(idw, * dump);
				// Если функции обратного вызова установлены
				if(!callbacks.empty())
					// Выполняем установку функций обратного вызова для Websocket-сервера
					this->_ws1.callbacks(std::move(callbacks));
			}
		} break;
	}
}
/**
 * beginSignal Метод начала получения фрейма заголовков HTTP/2
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @return    статус полученных данных
 */
int32_t awh::server::Websocket2::beginSignal(const int32_t sid, const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Устанавливаем новый идентификатор потока
		options->sid = sid;
		// Выполняем сброс флага рукопожатия
		options->shake = false;
		// Выполняем очистку HTTP-парсера
		options->http.clear();
		// Выполняем сброс состояния HTTP-парсера
		options->http.reset();
		// Очищаем буфер собранных данных
		options->buffer.payload.clear();
		// Выполняем очистку оставшихся фрагментов
		options->buffer.fragmes.clear();
	}
	// Выводим результат
	return 0;
}
/**
 * closedSignal Метод завершения работы потока
 * @param sid   идентификатор потока
 * @param bid   идентификатор брокера
 * @param error флаг ошибки если присутствует
 * @return      статус полученных данных
 */
int32_t awh::server::Websocket2::closedSignal(const int32_t sid, const uint64_t bid, const http2_t::error_t error) noexcept {
	// Если разрешено выполнить остановку
	if((this->_core != nullptr) && (error != awh::http2_t::error_t::NONE))
		// Выполняем закрытие подключения
		web2_t::close(bid);
	// Если функция обратного вызова активности потока установлена
	if(this->_callbacks.is("stream"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, bid, mode_t::CLOSE);
	// Выводим результат
	return 0;
}
/**
 * headerSignal Метод обратного вызова при получении заголовка HTTP/2
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @param key данные ключа заголовка
 * @param val данные значения заголовка
 * @return    статус полученных данных
 */
int32_t awh::server::Websocket2::headerSignal(const int32_t sid, const uint64_t bid, const string & key, const string & val) noexcept {
	// Получаем параметры активного клиента
	scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Устанавливаем полученные заголовки
		options->http.header2(key, val);
		// Если функция обратного вызова на полученного заголовка с сервера установлена
		if(this->_callbacks.is("header"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const int32_t, const uint64_t, const string &, const string &)> ("header", sid, bid, key, val);
	}
	// Выводим результат
	return 0;
}
/**
 * chunkSignal Метод обратного вызова при получении чанка HTTP/2
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param buffer буфер данных который содержит полученный чанк
 * @param size   размер полученного буфера данных чанка
 * @return       статус полученных данных
 */
int32_t awh::server::Websocket2::chunkSignal(const int32_t sid, const uint64_t bid, const uint8_t * buffer, const size_t size) noexcept {
	// Получаем параметры активного клиента
	scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Если функция обратного вызова на перехват входящих чанков установлена
		if(this->_callbacks.is("chunking"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", bid, vector <char> (buffer, buffer + size), &options->http);
		// Если функция перехвата полученных чанков не установлена
		else if(this->_core != nullptr) {
			// Если подключение закрыто
			if(options->close)
				// Выходим из функции
				return 0;
			// Если рукопожатие не выполнено
			if(!options->shake)
				// Добавляем полученный чанк в тело данных
				options->http.payload(vector <char> (buffer, buffer + size));
			// Если рукопожатие выполнено
			else if(options->allow.receive) {
				// Обнуляем время последнего ответа на пинг
				options->point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
				// Обновляем время отправленного пинга
				options->sendPing = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
				// Добавляем полученные данные в буфер
				options->buffer.payload.emplace(reinterpret_cast <const char *> (buffer), size);
			}
			// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
			if(this->_callbacks.is("chunks"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const int32_t, const uint64_t, const vector <char> &)> ("chunks", sid, bid, vector <char> (buffer, buffer + size));
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
 * @param frame  тип полученного фрейма
 * @param flags  флаги полученного фрейма
 * @return       статус полученных данных
 */
int32_t awh::server::Websocket2::frameSignal(const int32_t sid, const uint64_t bid, const http2_t::direct_t direct, const http2_t::frame_t frame, const set <http2_t::flag_t> & flags) noexcept {
	// Определяем направление передачи фрейма
	switch(static_cast <uint8_t> (direct)){
		// Если производится передача фрейма на сервер
		case static_cast <uint8_t> (http2_t::direct_t::SEND): {
			// Если мы получили флаг завершения потока
			if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
				// Получаем параметры активного клиента
				scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
				// Если параметры активного клиента получены
				if((this->_core != nullptr) && (options != nullptr)){
					// Если необходимо выполнить закрыть подключение
					if(!options->close && options->stopped){
						// Устанавливаем флаг закрытия подключения
						options->close = !options->close;
						// Выполняем поиск брокера в списке активных сессий
						auto i = this->_sessions.find(bid);
						// Если активная сессия найдена
						if(i != this->_sessions.end()){
							// Выполняем закрытие подключения
							i->second->close();
							// Выполняем закрытие подключения
							web2_t::close(bid);
						// Принудительно выполняем отключение лкиента
						} else const_cast <server::core_t *> (this->_core)->close(bid);
					}
				}
				// Если установлена функция отлова завершения запроса
				if(this->_callbacks.is("end"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, bid, direct_t::SEND);
				// Выходим из функции
				return 0;
			}
		} break;
		// Если производится получения фрейма с сервера
		case static_cast <uint8_t> (http2_t::direct_t::RECV): {
			// Получаем параметры активного клиента
			scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if((this->_core != nullptr) && (options != nullptr)){
				// Выполняем определение типа фрейма
				switch(static_cast <uint8_t> (frame)){
					// Если мы получили входящие данные тела ответа
					case static_cast <uint8_t> (http2_t::frame_t::DATA): {
						// Если рукопожатие выполнено
						if(options->shake && options->allow.receive){
							// Если мы получили неустановленный флаг или флаг завершения потока
							if(flags.empty() || (flags.find(http2_t::flag_t::END_STREAM) != flags.end())){
								// Флаг удачного получения данных
								bool receive = false;
								// Создаём объект шапки фрейма
								ws::frame_t::head_t head;
								// Выполняем обработку полученных данных
								while(!options->close && options->allow.receive && !options->buffer.payload.empty()){
									// Выполняем чтение фрейма Websocket
									const auto & payload = options->frame.methods.get(head, static_cast <buffer_t::data_t> (options->buffer.payload), static_cast <size_t> (options->buffer.payload));
									// Если буфер данных получен
									if(!payload.empty()){
										// Определяем тип ответа
										switch(static_cast <uint8_t> (head.optcode)){
											// Если ответом является PING
											case static_cast <uint8_t> (ws::frame_t::opcode_t::PING):
												// Отправляем ответ брокеру
												this->pong(bid, string(payload.begin(), payload.end()));
											break;
											// Если ответом является PONG
											case static_cast <uint8_t> (ws::frame_t::opcode_t::PONG): {
												// Если идентификатор брокера совпадает
												if(::memcmp(::to_string(bid).c_str(), payload.data(), payload.size()) == 0)
													// Обновляем контрольную точку
													options->point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
											} break;
											// Если ответом является TEXT
											case static_cast <uint8_t> (ws::frame_t::opcode_t::TEXT):
											// Если ответом является BINARY
											case static_cast <uint8_t> (ws::frame_t::opcode_t::BINARY): {
												// Запоминаем полученный опкод
												options->frame.opcode = head.optcode;
												// Запоминаем, что данные пришли сжатыми
												options->inflate = (head.rsv[0] && (options->compressor != http_t::compressor_t::NONE));
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
													options->buffer.fragmes.insert(options->buffer.fragmes.end(), payload.begin(), payload.end());
												// Если сообщение является последним
												else {
													// Если тредпул активирован
													if(this->_thr.is())
														// Добавляем в тредпул новую задачу на извлечение полученных сообщений
														this->_thr.push(std::bind(&ws2_t::extraction, this, bid, payload, (options->frame.opcode == ws::frame_t::opcode_t::TEXT)));
													// Если тредпул не активирован, выполняем извлечение полученных сообщений
													else this->extraction(bid, payload, (options->frame.opcode == ws::frame_t::opcode_t::TEXT));
												}
											} break;
											// Если ответом является CONTINUATION
											case static_cast <uint8_t> (ws::frame_t::opcode_t::CONTINUATION): {
												// Заполняем фрагментированное сообщение
												options->buffer.fragmes.insert(options->buffer.fragmes.end(), payload.begin(), payload.end());
												// Если сообщение является последним
												if(head.fin){
													// Если тредпул активирован
													if(this->_thr.is())
														// Добавляем в тредпул новую задачу на извлечение полученных сообщений
														this->_thr.push(std::bind(&ws2_t::extraction, this, bid, options->buffer.fragmes, (options->frame.opcode == ws::frame_t::opcode_t::TEXT)));
													// Если тредпул не активирован, выполняем извлечение полученных сообщений
													else this->extraction(bid, options->buffer.fragmes, (options->frame.opcode == ws::frame_t::opcode_t::TEXT));
													// Очищаем список фрагментированных сообщений
													options->buffer.fragmes.clear();
												}
											} break;
											// Если ответом является CLOSE
											case static_cast <uint8_t> (ws::frame_t::opcode_t::CLOSE): {
												// Создаём сообщение
												options->mess = options->frame.methods.message(payload);
												// Выводим сообщение об ошибке
												this->error(bid, options->mess);
												// Завершаем работу клиента
												if(!(options->stopped = web2_t::reject(sid, bid, http2_t::error_t::STREAM_CLOSED)))
													// Завершаем работу
													const_cast <server::core_t *> (this->_core)->close(bid);
												// Если мы получили флаг завершения потока
												if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
													// Если установлена функция отлова завершения запроса
													if(this->_callbacks.is("end"))
														// Выполняем функцию обратного вызова
														this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, bid, direct_t::RECV);
												}
												// Выходим из функции
												return NGHTTP2_ERR_CALLBACK_FAILURE;
											}
										}
										// Если парсер обработал какое-то количество байт
										if((head.frame > 0) && !options->buffer.payload.empty()){
											// Если размер буфера больше количества удаляемых байт
											if((receive = (static_cast <size_t> (options->buffer.payload) >= head.frame)))
												// Удаляем количество обработанных байт
												options->buffer.payload.erase(head.frame);
										}
									// Если мы получили ошибку получения фрейма
									} else if(head.state == ws::frame_t::state_t::BAD) {
										// Создаём сообщение
										options->mess = options->frame.methods.message(head, (options->compressor != http_t::compressor_t::NONE));
										// Выполняем отключение брокера
										goto Stop;
									}
									// Если данные мы все получили, выходим
									if(!receive || payload.empty() || options->buffer.payload.empty())
										// Выходим из условия
										break;
								}
								// Фиксируем изменение в буфере
								options->buffer.payload.commit();
								// Если мы получили флаг завершения потока
								if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
									// Если установлена функция отлова завершения запроса
									if(this->_callbacks.is("end"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, bid, direct_t::RECV);
								}
								// Выходим из функции
								return 0;
							}
							// Устанавливаем метку остановки брокера
							Stop:
							// Отправляем серверу сообщение
							this->sendError(bid, options->mess);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", bid, log_t::flag_t::WARNING, http::error_t::WEBSOCKET, this->_fmk->format("%s [%u]", options->mess.code, options->mess.text.c_str()));
						}
					} break;
					// Если мы получили входящие данные заголовков ответа
					case static_cast <uint8_t> (http2_t::frame_t::HEADERS): {
						// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
						if(flags.find(http2_t::flag_t::END_HEADERS) != flags.end()){
							// Выполняем коммит полученного результата
							options->http.commit();
							// Выполняем извлечение параметров запроса
							const auto & request = options->http.request();
							// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
							if(this->_callbacks.is("request"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request", options->sid, bid, request.method, request.url);
							// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
							if(this->_callbacks.is("headers"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", options->sid, bid, request.method, request.url, options->http.headers());
							// Если рукопожатие не выполнено
							if(!reinterpret_cast <http_t &> (options->http).is(http_t::state_t::HANDSHAKE)){
								// Ответ клиенту по умолчанию успешный
								awh::web_t::res_t response(2.0f, static_cast <uint32_t> (200));
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									{
										// Выполняем создание объекта для вывода HTTP-запроса
										http_t http(this->_fmk, this->_log);
										// Устанавливаем параметры запроса
										http.request(options->http.request());
										// Устанавливаем заголовки запроса
										http.headers(options->http.headers());
										// Устанавливаем компрессор запроса
										http.compression(options->http.compression());
										// Выполняем коммит полученного результата
										http.commit();
										// Получаем данные запроса
										const auto & buffer = http.process(http_t::process_t::REQUEST, http.request());
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
												response = awh::web_t::res_t(2.0f, static_cast <uint32_t> (505), "Unsupported protocol version");
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
											options->compressor = options->http.compression();
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
														// Выполняем создание объекта для вывода HTTP-ответа
														http_t http(this->_fmk, this->_log);
														// Устанавливаем параметры ответа
														http.response(response);
														// Устанавливаем заголовки ответа
														http.headers(options->http.headers());
														// Устанавливаем компрессор ответа
														http.compression(options->http.compression());
														// Выполняем коммит полученного результата
														http.commit();
														// Выводим заголовок ответа
														cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
														// Получаем данные ответа
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
													if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
														// Если установлена функция отлова завершения запроса
														if(this->_callbacks.is("end"))
															// Выполняем функцию обратного вызова
															this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", options->sid, bid, direct_t::RECV);
													}
													// Выходим из функции
													return NGHTTP2_ERR_CALLBACK_FAILURE;
												}
												// Если функция обратного вызова активности потока установлена
												if(this->_callbacks.is("stream"))
													// Выполняем функцию обратного вызова
													this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", options->sid, bid, mode_t::OPEN);
												// Если функция обратного вызова на получение удачного запроса установлена
												if(this->_callbacks.is("handshake"))
													// Выполняем функцию обратного вызова
													this->_callbacks.call <void (const int32_t, const uint64_t, const agent_t)> ("handshake", options->sid, bid, agent_t::WEBSOCKET);
												// Если функция обратного вызова на вывод полученных данных запроса клиента установлена
												if(this->_callbacks.is("complete"))
													// Выполняем функцию обратного вызова
													this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", options->sid, bid, request.method, request.url, options->http.body(), options->http.headers());
												// Если мы получили флаг завершения потока
												if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
													// Если установлена функция отлова завершения запроса
													if(this->_callbacks.is("end"))
														// Выполняем функцию обратного вызова
														this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", options->sid, bid, direct_t::RECV);
												}
												// Завершаем работу
												return 0;
											// Формируем ответ, что произошла внутренняя ошибка сервера
											} else response = awh::web_t::res_t(2.0f, static_cast <uint32_t> (500));
										// Формируем ответ, что страница не доступна
										} else response = awh::web_t::res_t(2.0f, static_cast <uint32_t> (403), "Handshake failed");
									} break;
									// Если запрос неудачный
									case static_cast <uint8_t> (http_t::status_t::FAULT):
										// Формируем ответ на запрос об авторизации
										response = awh::web_t::res_t(2.0f, static_cast <uint32_t> (401));
									break;
									// Если результат определить не получилось
									default: response = awh::web_t::res_t(2.0f, static_cast <uint32_t> (506), "Unknown request");
								}
								// Выполняем очистку HTTP-парсера
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
											// Выполняем создание объекта для вывода HTTP-ответа
											http_t http(this->_fmk, this->_log);
											// Устанавливаем параметры ответа
											http.response(response);
											// Устанавливаем заголовки ответа
											http.headers(options->http.headers());
											// Устанавливаем компрессор ответа
											http.compression(options->http.compression());
											// Выполняем коммит полученного результата
											http.commit();
											// Выводим заголовок ответа
											cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
											// Получаем данные ответа
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
									if(options->http.empty(awh::http_t::suite_t::BODY))
										// Устанавливаем флаг завершения потока
										flag = http2_t::flag_t::END_STREAM;
									// Выполняем заголовки запроса на сервер
									const int32_t sid = web2_t::send(options->sid, bid, headers, flag);
									// Если запрос не получилось отправить
									if(sid < 0){
										// Если мы получили флаг завершения потока
										if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
											// Если установлена функция отлова завершения запроса
											if(this->_callbacks.is("end"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", options->sid, bid, direct_t::RECV);
										}
										// Выходим из функции
										return NGHTTP2_ERR_CALLBACK_FAILURE;
									}
									// Если тело запроса существует
									if(!options->http.empty(awh::http_t::suite_t::BODY)){
										// Тело HTTP-запроса
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
											if(options->http.empty(awh::http_t::suite_t::BODY))
												// Устанавливаем флаг завершения потока
												flag = http2_t::flag_t::END_STREAM;
											// Выполняем отправку тела запроса на сервер
											if(!web2_t::send(options->sid, bid, entity.data(), entity.size(), flag)){
												// Если мы получили флаг завершения потока
												if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
													// Если установлена функция отлова завершения запроса
													if(this->_callbacks.is("end"))
														// Выполняем функцию обратного вызова
														this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", options->sid, bid, direct_t::RECV);
												}
												// Выходим из функции
												return NGHTTP2_ERR_CALLBACK_FAILURE;
											}
										}
									}
									// Если мы получили флаг завершения потока
									if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
										// Если установлена функция отлова завершения запроса
										if(this->_callbacks.is("end"))
											// Выполняем функцию обратного вызова
											this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", options->sid, bid, direct_t::RECV);
									}
									// Завершаем работу
									return 0;
								}
							}
							// Выполняем закрытие подключения
							web2_t::close(bid);
							// Если мы получили флаг завершения потока
							if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
								// Если установлена функция отлова завершения запроса
								if(this->_callbacks.is("end"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", options->sid, bid, direct_t::RECV);
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
 * error Метод вывода сообщений об ошибках работы брокера
 * @param bid     идентификатор брокера
 * @param message сообщение с описанием ошибки
 */
void awh::server::Websocket2::error(const uint64_t bid, const ws::mess_t & message) const noexcept {
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
				this->_callbacks.call <void (const uint64_t, const uint32_t, const string &)> ("errorWebsocket", bid, message.code, message.text);
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
void awh::server::Websocket2::extraction(const uint64_t bid, const vector <char> & buffer, const bool text) noexcept {
	// Если буфер данных передан
	if((bid > 0) && !buffer.empty() && this->_callbacks.is("messageWebsocket")){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Выполняем блокировку потока	
			const lock_guard <recursive_mutex> lock(options->mtx);
			// Если данные пришли в сжатом виде
			if(options->inflate && (options->compressor != http_t::compressor_t::NONE)){
				// Декомпрессионные данные
				vector <char> data;
				// Определяем метод компрессии
				switch(static_cast <uint8_t> (options->compressor)){
					// Если метод компрессии выбран LZ4
					case static_cast <uint8_t> (http_t::compressor_t::LZ4):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::LZ4);
					break;
					// Если метод компрессии выбран Zstandard
					case static_cast <uint8_t> (http_t::compressor_t::ZSTD):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::ZSTD);
					break;
					// Если метод компрессии выбран LZma
					case static_cast <uint8_t> (http_t::compressor_t::LZMA):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::LZMA);
					break;
					// Если метод компрессии выбран Brotli
					case static_cast <uint8_t> (http_t::compressor_t::BROTLI):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BROTLI);
					break;
					// Если метод компрессии выбран BZip2
					case static_cast <uint8_t> (http_t::compressor_t::BZIP2):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BZIP2);
					break;
					// Если метод компрессии выбран GZip
					case static_cast <uint8_t> (http_t::compressor_t::GZIP):
						// Выполняем декомпрессию полученных данных
						data = options->hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::GZIP);
					break;
					// Если метод компрессии выбран Deflate
					case static_cast <uint8_t> (http_t::compressor_t::DEFLATE): {
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
 * ping Метод проверки доступности сервера
 * @param bid идентификатор брокера
 * @param     message сообщение для отправки
 */
void awh::server::Websocket2::ping(const uint64_t bid, const string & message) noexcept {
	// Если необходимые данные переданы
	if((bid > 0) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если отправка сообщений разблокированна
		if((options != nullptr) && options->allow.send){
			// Создаём буфер для отправки
			const auto & buffer = options->frame.methods.ping(message);
			// Если буфер данных получен
			if(!buffer.empty()){
				// Выполняем отправку сообщения клиенту
				if(web2_t::send(options->sid, bid, buffer.data(), buffer.size(), http2_t::flag_t::NONE))
					// Обновляем время отправленного пинга
					options->sendPing = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
			}
		}
	}
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param bid идентификатор брокера
 * @param     message сообщение для отправки
 */
void awh::server::Websocket2::pong(const uint64_t bid, const string & message) noexcept {
	// Если необходимые данные переданы
	if((bid > 0) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
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
 * erase Метод удаления отключившихся брокеров
 * @param bid идентификатор брокера
 */
void awh::server::Websocket2::erase(const uint64_t bid) noexcept {
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
				// Если переключение протокола на HTTP/2 не выполнено
				if(options->proto != engine_t::proto_t::HTTP2)
					// Выполняем очистку отключившихся брокеров у Websocket-сервера
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
			auto i = this->_disconected.find(bid);
			// Если данные отключившегося брокера найдены
			if((i != this->_disconected.end()) && ((date - i->second) >= 3000)){
				// Если установлена функция детекции удаление брокера сообщений установлена
				if(this->_callbacks.is("erase"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint64_t)> ("erase", bid);
				// Выполняем удаление отключившегося брокера
				eraseFn(i->first);
				// Выполняем удаление брокера
				this->_disconected.erase(i);
			}
		// Если идентификатор брокера не передан
		} else {
			// Выполняем переход по всему списку отключившихся брокеров
			for(auto i = this->_disconected.begin(); i != this->_disconected.end();){
				// Если брокер уже давно отключился
				if((date - i->second) >= 3000){
					// Если установлена функция детекции удаление брокера сообщений установлена
					if(this->_callbacks.is("erase"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint64_t)> ("erase", i->first);
					// Выполняем удаление отключившегося брокера
					eraseFn(i->first);
					// Выполняем удаление объекта брокеров из списка отключившихся
					i = this->_disconected.erase(i);
				// Выполняем пропуск брокера
				} else ++i;
			}
		}
	}
}
/**
 * disconnect Метод отключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Websocket2::disconnect(const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены и переключение протокола на HTTP/2 не выполнено
	if((options != nullptr) && (options->proto != engine_t::proto_t::HTTP2))
		// Добавляем в очередь список отключившихся клиентов
		this->_ws1.disconnect(bid);
	// Добавляем в очередь список отключившихся клиентов
	this->_disconected.emplace(bid, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
}
/**
 * pinging Метод таймера выполнения пинга клиента
 * @param tid идентификатор таймера
 */
void awh::server::Websocket2::pinging(const uint16_t tid) noexcept {
	// Если данные существуют
	if((tid > 0) && (this->_core != nullptr)){
		// Если разрешено выполнять пинги
		if(this->_pinging){
			// Выполняем перебор всех активных клиентов
			for(auto & item : this->_scheme.get()){
				// Если подключение клиента активно
				if(!item.second->close){
					// Если переключение протокола на HTTP/2 не выполнено
					if(item.second->proto != engine_t::proto_t::HTTP2)
						// Выполняем переброс события пинга в модуль Websocket
						this->_ws1.pinging(tid);
					// Если переключение протокола на HTTP/2 выполнено
					else if(item.second->allow.receive) {
						// Если рукопожатие выполнено
						if(item.second->shake){
							// Получаем текущий штамп времени
							const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
							// Если брокер не ответил на пинг больше двух интервалов, отключаем его
							if((this->_waitPong > 0) && ((stamp - item.second->point) >= this->_waitPong)){
								// Создаём сообщение
								item.second->mess = ws::mess_t(1005, "PING response not received");
								// Отправляем серверу сообщение
								this->sendError(item.first, item.second->mess);
							// Если время с предыдущего пинга прошло больше половины времени пинга
							} else if((this->_waitPong > 0) && (this->_pingInterval > 0) && ((stamp - item.second->sendPing) > (this->_pingInterval / 2)))
								// Отправляем запрос брокеру
								this->ping(item.first, ::to_string(item.first));
						// Если рукопожатие не выполнено и пинг не прошёл
						} else if(!web2_t::ping(item.first))
							// Выполняем закрытие подключения
							web2_t::close(item.first);
					}
				}
			}
		}
	}
}
/**
 * init Метод инициализации Websocket-сервера
 * @param socket      unix-сокет для биндинга
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Websocket2::init(const string & socket, const vector <http_t::compressor_t> & compressors) noexcept {
	// Устанавливаем писок поддерживаемых компрессоров
	this->_scheme.compressors = compressors;
	// Выполняем инициализацию родительского объекта
	web2_t::init(socket, compressors);
}
/**
 * init Метод инициализации Websocket-сервера
 * @param port        порт сервера
 * @param host        хост сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Websocket2::init(const uint32_t port, const string & host, const vector <http_t::compressor_t> & compressors) noexcept {
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
void awh::server::Websocket2::sendError(const uint64_t bid, const ws::mess_t & mess) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Если код ошибки относится к Websocket
		if(mess.code >= 1000){
			// Получаем параметры активного клиента
			scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr)
				// Запрещаем получение данных
				options->allow.receive = false;
			// Если отправка сообщений разблокированна
			if((options != nullptr) && options->allow.send){
				// Если переключение протокола на HTTP/2 не выполнено
				if(options->proto != engine_t::proto_t::HTTP2)
					// Выполняем отправку сообщение об ошибке на клиент Websocket
					this->_ws1.sendError(bid, mess);
				// Если переключение протокола на HTTP/2 выполнено
				else if(!options->stopped) {
					// Получаем буфер сообщения
					const auto & buffer = options->frame.methods.message(mess);
					// Если данные сообщения получены
					if((options->stopped = !buffer.empty())){
						// Выполняем отправку сообщения клиенту
						web2_t::send(options->sid, bid, buffer.data(), buffer.size(), http2_t::flag_t::END_STREAM);
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ SEND ERROR ^^^^^^^^^\x1B[0m" << endl;
							// Выводим отправляемое сообщение
							cout << this->_fmk->format("%s [%u]", mess.text.c_str(), mess.code) << endl << endl;
						#endif
						// Выводим сообщение об ошибке
						this->error(bid, mess);
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
 * @return        результат отправки сообщения
 */
bool awh::server::Websocket2::sendMessage(const uint64_t bid, const vector <char> & message, const bool text) noexcept {
	// Результат работы функции
	bool result = false;
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && (bid > 0) && !message.empty()){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если отправка сообщений разблокированна
		if((options != nullptr) && options->allow.send){
			// Выполняем блокировку отправки сообщения
			options->allow.send = !options->allow.send;
			// Если переключение протокола на HTTP/2 не выполнено
			if(options->proto != engine_t::proto_t::HTTP2)
				// Выполняем отправку сообщения клиенту Websocket
				result = this->_ws1.sendMessage(bid, message, text);
			// Если переключение протокола на HTTP/2 выполнено
			else {
				// Если рукопожатие выполнено
				if(options->http.handshake(http_t::process_t::REQUEST)){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ SEND MESSAGE ^^^^^^^^^\x1B[0m" << endl;
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
					head.rsv[0] = ((message.size() >= 1024) && (options->compressor != http_t::compressor_t::NONE));
					// Если необходимо сжимать сообщение перед отправкой
					if(head.rsv[0]){
						// Компрессионные данные
						vector <char> data;
						// Определяем метод компрессии
						switch(static_cast <uint8_t> (options->compressor)){
							// Если метод компрессии выбран LZ4
							case static_cast <uint8_t> (http_t::compressor_t::LZ4):
								// Выполняем компрессию полученных данных
								data = options->hash.compress(message.data(), message.size(), hash_t::method_t::LZ4);
							break;
							// Если метод компрессии выбран Zstandard
							case static_cast <uint8_t> (http_t::compressor_t::ZSTD):
								// Выполняем компрессию полученных данных
								data = options->hash.compress(message.data(), message.size(), hash_t::method_t::ZSTD);
							break;
							// Если метод компрессии выбран LZma
							case static_cast <uint8_t> (http_t::compressor_t::LZMA):
								// Выполняем компрессию полученных данных
								data = options->hash.compress(message.data(), message.size(), hash_t::method_t::LZMA);
							break;
							// Если метод компрессии выбран Brotli
							case static_cast <uint8_t> (http_t::compressor_t::BROTLI):
								// Выполняем компрессию полученных данных
								data = options->hash.compress(message.data(), message.size(), hash_t::method_t::BROTLI);
							break;
							// Если метод компрессии выбран BZip2
							case static_cast <uint8_t> (http_t::compressor_t::BZIP2):
								// Выполняем компрессию полученных данных
								data = options->hash.compress(message.data(), message.size(), hash_t::method_t::BZIP2);
							break;
							// Если метод компрессии выбран GZip
							case static_cast <uint8_t> (http_t::compressor_t::GZIP):
								// Выполняем компрессию полученных данных
								data = options->hash.compress(message.data(), message.size(), hash_t::method_t::GZIP);
							break;
							// Если метод компрессии выбран Deflate
							case static_cast <uint8_t> (http_t::compressor_t::DEFLATE): {
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
								// Выполняем отправку сообщения на клиенту
								result = web2_t::send(options->sid, bid, payload.data(), payload.size(), http2_t::flag_t::NONE);
							// Иначе просто выходим
							else break;
							// Выполняем сброс RSV1
							head.rsv[0] = false;
							// Устанавливаем опкод сообщения
							head.optcode = ws::frame_t::opcode_t::CONTINUATION;
							// Увеличиваем смещение в буфере
							start = stop;
							// Если запрос не отправлен
							if(!result)
								// Выходим из цикла
								break;
						}
					// Если фрагментация сообщения не требуется
					} else {
						// Создаём буфер для отправки
						const auto & payload = options->frame.methods.set(head, message.data(), message.size());
						// Если бинарный буфер для отправки данных получен
						if(!payload.empty())
							// Выполняем отправку сообщения на клиенту
							result = web2_t::send(options->sid, bid, payload.data(), payload.size(), http2_t::flag_t::NONE);
					}
				}
			}
			// Выполняем разблокировку отправки сообщения
			options->allow.send = !options->allow.send;
		}
	}
	// Выводим результат
	return result;
}
/**
 * send Метод отправки данных в бинарном виде клиенту
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 * @return       результат отправки сообщения
 */
bool awh::server::Websocket2::send(const uint64_t bid, const char * buffer, const size_t size) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0))
		// Выполняем отправку заголовков ответа клиенту
		return const_cast <server::core_t *> (this->_core)->send(buffer, size, bid);
	// Сообщаем что ничего не найдено
	return false;
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::server::Websocket2::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем добавление функций обратного вызова в основноной модуль
	web2_t::callbacks(callbacks);
	{
		// Создаём локальный контейнер функций обратного вызова
		fn_t callbacks(this->_log);
		// Выполняем установку функции обратного вызова для вывода бинарных данных в сыром виде полученных с клиента
		callbacks.set("raw", this->_callbacks);
		// Выполняем установку функции обратного вызова при завершении запроса
		callbacks.set("end", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие получения ошибки
		callbacks.set("error", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного тела данных с клиента
		callbacks.set("entity", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного чанка бинарных данных с клиента
		callbacks.set("chunks", this->_callbacks);
		// Выполняем установку функции обратного вызова активности потока
		callbacks.set("stream", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие активации брокера на сервере
		callbacks.set("accept", this->_callbacks);
		// Выполняем установку функции обратного вызова полученного заголовка с клиента
		callbacks.set("header", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода запроса клиента к серверу
		callbacks.set("request", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученных заголовков с клиента
		callbacks.set("headers", this->_callbacks);
		// Выполняем установку функции обратного вызова для перехвата полученных чанков
		callbacks.set("chunking", this->_callbacks);
		// Выполняем установку функции завершения выполнения запроса
		callbacks.set("complete", this->_callbacks);
		// Выполняем установку функции обратного вызова при выполнении рукопожатия
		callbacks.set("handshake", this->_callbacks);
		// Выполняем установку функции обратного вызова для обработки авторизации
		callbacks.set("checkPassword", this->_callbacks);
		// Выполняем установку функции обратного вызова для извлечения пароля
		callbacks.set("extractPassword", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие получения ошибок Websocket
		callbacks.set("errorWebsocket", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие получения сообщений Websocket
		callbacks.set("messageWebsocket", this->_callbacks);
		// Если функции обратного вызова установлены
		if(!callbacks.empty())
			// Выполняем установку функций обратного вызова для Websocket-сервера
			this->_ws1.callbacks(std::move(callbacks));
	}
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
uint32_t awh::server::Websocket2::port(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.port(bid);
}
/**
 * agent Метод извлечения агента клиента
 * @param bid идентификатор брокера
 * @return    агент к которому относится подключённый клиент
 */
awh::server::web_t::agent_t awh::server::Websocket2::agent(const uint64_t bid) const noexcept {
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
const string & awh::server::Websocket2::ip(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::Websocket2::mac(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.mac(bid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Websocket2::stop() noexcept {
	// Выполняем остановку работы основного модуля
	web2_t::stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::Websocket2::start() noexcept {
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr){
		// Если биндинг не запущен
		if(!this->_core->working())
			// Выполняем запуск биндинга
			const_cast <server::core_t *> (this->_core)->start();
		// Если биндинг уже запущен, выполняем запуск
		else this->openEvents(this->_scheme.id);
	}
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Websocket2::close(const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
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
			auto i = this->_sessions.find(bid);
			// Если активная сессия найдена
			if(i != this->_sessions.end()){
				// Выполняем закрытие подключения
				i->second->close();
				// Выполняем закрытие подключения
				web2_t::close(bid);
			// Выполняем отключение брокера
			} else const_cast <server::core_t *> (this->_core)->close(bid);
		}
	}
}
/**
 * waitPong Метод установки времени ожидания ответа WebSocket-клиента
 * @param sec время ожидания в секундах
 */
void awh::server::Websocket2::waitPong(const time_t sec) noexcept {
	// Выполняем установку времени ожидания для WebSocket/1.1
	this->_ws1.waitPong(sec);
	// Выполняем установку времени ожидания
	this->_waitPong = (sec * 1000);
}
/**
 * pingInterval Метод установки интервала времени выполнения пингов
 * @param sec интервал времени выполнения пингов в секундах
 */
void awh::server::Websocket2::pingInterval(const time_t sec) noexcept {
	// Выполняем установку интервала времени выполнения пингов в секундах для WebSocket/1.1
	this->_ws1.pingInterval(sec);
	// Выполняем установку интервала времени выполнения пингов в секундах
	this->_pingInterval = (sec * 1000);
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::server::Websocket2::subprotocol(const string & subprotocol) noexcept {
	// Устанавливаем сабпротокол
	if(!subprotocol.empty())
		// Выполняем установку сабпротокола
		this->_subprotocols.emplace(subprotocol);
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::server::Websocket2::subprotocols(const set <string> & subprotocols) noexcept {
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
const set <string> & awh::server::Websocket2::subprotocols(const uint64_t bid) const noexcept {
	// Результат работы функции
	static const set <string> result;
	// Получаем параметры активного клиента
	scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
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
void awh::server::Websocket2::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка расширений
	this->_extensions = extensions;
}
/**
 * extensions Метод извлечения списка расширений
 * @param bid идентификатор брокера
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::Websocket2::extensions(const uint64_t bid) const noexcept {
	// Результат работы функции
	static const vector <vector <string>> result;
	// Получаем параметры активного клиента
	scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Если переключение протокола на HTTP/2 не выполнено
		if(options->proto != engine_t::proto_t::HTTP2)
			// Выполняем извлечение списка поддерживаемых расширений Websocket
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
void awh::server::Websocket2::multiThreads(const uint16_t count, const bool mode) noexcept {
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
			this->_thr.stop();
			// Выполняем инициализацию нового тредпула
			this->_thr.init(this->_threads);
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
void awh::server::Websocket2::total(const uint16_t total) noexcept {
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr)
		// Устанавливаем максимальное количество одновременных подключений
		const_cast <server::core_t *> (this->_core)->total(this->_scheme.id, total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::Websocket2::segmentSize(const size_t size) noexcept {
	// Если максимальный размер фрейма больше самого максимального значения
	if(static_cast <uint32_t> (size) > http2_t::MAX_FRAME_SIZE_MAX)
		// Выполняем уменьшение размера сегмента
		const_cast <size_t &> (size) = static_cast <size_t> (http2_t::MAX_FRAME_SIZE_MAX);
	// Если максимальный размер фрейма меньше самого минимального значения
	else if(static_cast <uint32_t> (size) < http2_t::MAX_FRAME_SIZE_MIN)
		// Выполняем уменьшение размера сегмента
		const_cast <size_t &> (size) = static_cast <size_t> (http2_t::MAX_FRAME_SIZE_MIN);
	// Устанавливаем размер одного сегмента фрейма
	this->_frameSize = size;
	// Выполняем установку размеров сегментов фрейма для Websocket-сервера
	this->_ws1.segmentSize(size);
	// Выполняем извлечение максимального размера фрейма
	auto i = this->_settings.find(http2_t::settings_t::FRAME_SIZE);
	// Если размер максимального размера фрейма уже установлен
	if(i != this->_settings.end())
		// Выполняем замену максимального размера фрейма
		i->second = static_cast <uint32_t> (this->_frameSize);
	// Если размер максимального размера фрейма ещё не установлен
	else this->_settings.emplace(http2_t::settings_t::FRAME_SIZE, static_cast <uint32_t> (this->_frameSize));
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Websocket2::compressors(const vector <http_t::compressor_t> & compressors) noexcept {
	// Устанавливаем список поддерживаемых компрессоров
	this->_scheme.compressors = compressors;
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Websocket2::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
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
void awh::server::Websocket2::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для Websocket-сервера
	this->_ws1.mode(flags);
	// Активируем выполнение пинга
	this->_pinging = (flags.find(flag_t::NOT_PING) == flags.end());
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_complete = (flags.find(flag_t::NOT_STOP) == flags.end());
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.find(flag_t::ALIVE) != flags.end());
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_client.takeover = (flags.find(flag_t::TAKEOVER_CLIENT) != flags.end());
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_server.takeover = (flags.find(flag_t::TAKEOVER_SERVER) != flags.end());
	// Если сетевое ядро установлено
	if(this->_core != nullptr)
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <server::core_t *> (this->_core)->verbose(flags.find(flag_t::NOT_INFO) == flags.end());
}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::server::Websocket2::settings(const map <awh::http2_t::settings_t, uint32_t> & settings) noexcept {
	// Выполняем установку основных настроек протокола HTTP/2
	web2_t::settings(settings);
	// Если метод CONNECT не установлен, разрешаем его по умолчанию
	if(this->_settings.find(awh::http2_t::settings_t::CONNECT) == this->_settings.end())
		// Выполняем установку разрешения использования метода CONNECT
		this->_settings.emplace(awh::http2_t::settings_t::CONNECT, 1);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Websocket2::alive(const bool mode) noexcept {
	// Выполняем установку долгоживущего подключения
	web2_t::alive(mode);
	// Выполняем установку долгоживущего подключения для Websocket-сервера
	this->_ws1.alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param sec время жизни подключения
 */
void awh::server::Websocket2::alive(const time_t sec) noexcept {
	// Выполняем установку времени жизни подключения
	web2_t::alive(sec);
	// Выполняем установку времени жизни подключения для Websocket-сервера
	this->_ws1.alive(sec);
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::server::Websocket2::core(const server::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку сетевого ядра
		web_t::core(core);
		// Добавляем схемы сети в сетевое ядро
		const_cast <server::core_t *> (this->_core)->scheme(&this->_scheme);
		// Если многопоточность активированна
		if(this->_thr.is() || this->_ws1._thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
		// Устанавливаем событие на запуск системы
		const_cast <server::core_t *> (this->_core)->callback <void (const uint16_t)> ("open", std::bind(&ws2_t::openEvents, this, _1));
		// Устанавливаем событие подключения
		const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("connect", std::bind(&ws2_t::connectEvents, this, _1, _2));
		// Устанавливаем событие отключения
		const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("disconnect", std::bind(&ws2_t::disconnectEvents, this, _1, _2));
		// Устанавливаем функцию чтения данных
		const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", std::bind(&ws2_t::readEvents, this, _1, _2, _3, _4));
		// Устанавливаем функцию записи данных
		const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&ws2_t::writeEvents, this, _1, _2, _3, _4));
		// Добавляем событие аццепта брокера
		const_cast <server::core_t *> (this->_core)->callback <bool (const string &, const string &, const uint32_t, const uint64_t)> ("accept", std::bind(&ws2_t::acceptEvents, this, _1, _2, _3, _4));
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_thr.is() || this->_ws1._thr.is()){
			// Выполняем завершение всех активных потоков
			this->_thr.stop();
			// Выполняем завершение всех активных потоков
			this->_ws1._thr.stop();
			// Снимаем режим простого чтения базы событий
			const_cast <server::core_t *> (this->_core)->easily(false);
		}
		// Удаляем схему сети из сетевого ядра
		const_cast <server::core_t *> (this->_core)->remove(this->_scheme.id);
		// Выполняем удаление объекта сетевого ядра
		web_t::core(core);
	}
}
/**
 * setHeaders Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::server::Websocket2::setHeaders(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку заголовков которые нужно передать клиенту
	this->_headers = headers;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Websocket2::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Websocket2::realm(const string & realm) noexcept {
	// Устанавливаем название сервера
	web2_t::realm(realm);
	// Устанавливаем название сервера для Websocket-сервера
	this->_ws1.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Websocket2::opaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	web2_t::opaque(opaque);
	// Устанавливаем временный ключ сессии сервера для Websocket-сервера
	this->_ws1.opaque(opaque);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::Websocket2::chunk(const size_t size) noexcept {
	// Устанавливаем размера чанка
	web2_t::chunk(size);
	// Устанавливаем размера чанка для Websocket-сервера
	this->_ws1.chunk(size);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Websocket2::maxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	web2_t::maxRequests(max);
	// Устанавливаем максимальное количество запросов для Websocket-сервера
	this->_ws1.maxRequests(max);
}
/**
 * ident Метод установки идентификации сервера
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Websocket2::ident(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификацию сервера
	web2_t::ident(id, name, ver);
	// Устанавливаем идентификацию сервера для Websocket-сервера
	this->_ws1.ident(id, name, ver);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Websocket2::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	web2_t::authType(type, hash);
	// Устанавливаем тип авторизации для Websocket-сервера
	this->_ws1.authType(type, hash);
}
/**
 * crypted Метод получения флага шифрования
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::server::Websocket2::crypted(const uint64_t bid) const noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если переключение протокола на HTTP/2 не выполнено
			if(options->proto != engine_t::proto_t::HTTP2)
				// Выводим установленный флаг шифрования клиента Websocket
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
void awh::server::Websocket2::encrypt(const uint64_t bid, const bool mode) noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если переключение протокола на HTTP/2 не выполнено
			if(options->proto != engine_t::proto_t::HTTP2)
				// Устанавливаем флаг шифрования для клиента Websocket
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
void awh::server::Websocket2::encryption(const bool mode) noexcept {
	// Устанавливаем флага шифрования
	web2_t::encryption(mode);
	// Устанавливаем флага шифрования для Websocket-сервера
	this->_ws1.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Websocket2::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования
	web2_t::encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для Websocket-сервера
	this->_ws1.encryption(pass, salt, cipher);
}
/**
 * Websocket2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Websocket2::Websocket2(const fmk_t * fmk, const log_t * log) noexcept :
web2_t(fmk, log), _waitPong(_pingInterval * 2), _threads(0), _frameSize(0), _ws1(fmk, log), _scheme(fmk, log) {
	// Выполняем установку список настроек протокола HTTP/2
	this->settings();
	// Если размер фрейма не установлен
	if(this->_frameSize == 0)
		// Устанавливаем размер сегментов фрейма
		this->_frameSize = static_cast <size_t> (http2_t::MAX_FRAME_SIZE_MIN);
}
/**
 * Websocket2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Websocket2::Websocket2(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(core, fmk, log), _waitPong(_pingInterval * 2), _threads(0), _frameSize(0), _ws1(fmk, log), _scheme(fmk, log) {
	// Выполняем установку список настроек протокола HTTP/2
	this->settings();
	// Если размер фрейма не установлен
	if(this->_frameSize == 0)
		// Устанавливаем размер сегментов фрейма
		this->_frameSize = static_cast <size_t> (http2_t::MAX_FRAME_SIZE_MIN);
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->scheme(&this->_scheme);
	// Устанавливаем событие на запуск системы
	const_cast <server::core_t *> (this->_core)->callback <void (const uint16_t)> ("open", std::bind(&ws2_t::openEvents, this, _1));
	// Устанавливаем событие подключения
	const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("connect", std::bind(&ws2_t::connectEvents, this, _1, _2));
	// Устанавливаем событие отключения
	const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("disconnect", std::bind(&ws2_t::disconnectEvents, this, _1, _2));
	// Устанавливаем функцию чтения данных
	const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", std::bind(&ws2_t::readEvents, this, _1, _2, _3, _4));
	// Устанавливаем функцию записи данных
	const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&ws2_t::writeEvents, this, _1, _2, _3, _4));
	// Добавляем событие аццепта брокера
	const_cast <server::core_t *> (this->_core)->callback <bool (const string &, const string &, const uint32_t, const uint64_t)> ("accept", std::bind(&ws2_t::acceptEvents, this, _1, _2, _3, _4));
}
/**
 * ~Websocket2 Деструктор
 */
awh::server::Websocket2::~Websocket2() noexcept {
	// Если многопоточность активированна
	if(this->_thr.is())
		// Выполняем завершение всех активных потоков
		this->_thr.stop();
}
