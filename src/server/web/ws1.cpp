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
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket1::connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём адъютанта
		this->_scheme.set(aid);
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
			// Если функция обратного вызова для обработки чанков установлена
			if(this->_callback.is("chunking"))
				// Устанавливаем функцию обработки вызова для получения чанков
				adj->http.on(this->_callback.get <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking"));
			// Устанавливаем функцию обработки вызова для получения чанков
			else adj->http.on(std::bind(&ws1_t::chunking, this, _1, _2, _3));
			// Если функция обратного вызова на полученного заголовка с клиента установлена
			if(this->_callback.is("header"))
				// Устанавливаем функцию обратного вызова для получения заголовков запроса
				adj->http.on((function <void (const uint64_t, const string &, const string &)>) std::bind(this->_callback.get <void (const int32_t, const uint64_t, const string &, const string &)> ("header"), 1, _1, _2, _3));
			// Если функция обратного вызова на вывод запроса клиента установлена
			if(this->_callback.is("request"))
				// Устанавливаем функцию обратного вызова для получения запроса клиента
				adj->http.on((function <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)>) std::bind(this->_callback.get <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request"), 1, _1, _2, _3));
			// Если функция обратного вызова на вывод полученных заголовков с клиента установлена
			if(this->_callback.is("headers"))
				// Устанавливаем функцию обратного вызова для получения всех заголовков запроса
				adj->http.on((function <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)>) std::bind(this->_callback.get <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers"), 1, _1, _2, _3, _4));
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
void awh::server::WebSocket1::disconnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
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
void awh::server::WebSocket1::readCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
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
			// Если разрешено получение данных
			if(adj->allow.receive){
				// Добавляем полученные данные в буфер
				adj->buffer.payload.insert(adj->buffer.payload.end(), buffer, buffer + size);
				// Если рукопожатие не выполнено
				if(!reinterpret_cast <http_t &> (adj->http).isHandshake()){
					// Выполняем парсинг полученных данных
					const size_t bytes = adj->http.parse(adj->buffer.payload.data(), adj->buffer.payload.size());
					// Если все данные получены
					if(adj->http.isEnd()){
						// Буфер данных для записи в сокет
						vector <char> buffer;
						// Выполняем создание объекта для генерации HTTP-ответа
						http_t http(this->_fmk, this->_log);
						// Метод компрессии данных
						http_t::compress_t compress = http_t::compress_t::NONE;
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							{
								// Получаем объект работы с HTTP запросами
								const http_t & http = reinterpret_cast <http_t &> (adj->http);
								// Получаем данные запроса
								const auto & request = http.process(http_t::process_t::REQUEST, true);
								// Если параметры запроса получены
								if(!request.empty()){
									// Выводим заголовок запроса
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
									// Выводим параметры запроса
									cout << string(request.begin(), request.end()) << endl;
									// Если тело запроса существует
									if(!http.body().empty())
										// Выводим сообщение о выводе чанка тела
										cout << this->_fmk->format("<body %u>", http.body().size()) << endl << endl;
									// Иначе устанавливаем перенос строки
									else cout << endl;
								}
							}
						#endif
						// Выполняем проверку авторизации
						switch(static_cast <uint8_t> (adj->http.getAuth())){
							// Если запрос выполнен удачно
							case static_cast <uint8_t> (http_t::stath_t::GOOD): {
								// Если рукопожатие выполнено
								if(adj->http.isHandshake(http_t::process_t::REQUEST)){
									// Получаем метод компрессии HTML данных
									compress = adj->http.compression();
									// Проверяем версию протокола
									if(!adj->http.checkVer()){
										// Получаем бинарные данные REST ответа
										buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (400), "Unsupported protocol version"));
										// Завершаем работу
										break;
									}
									// Проверяем ключ адъютанта
									if(!adj->http.checkKey()){
										// Получаем бинарные данные REST ответа
										buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (400), "Wrong client key"));
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
										// Выполняем отправку данных адъютанту
										dynamic_cast <server::core_t *> (core)->write(buffer.data(), buffer.size(), aid);
										// Есла данных передано больше чем обработано
										if(adj->buffer.payload.size() > bytes)
											// Удаляем количество обработанных байт
											adj->buffer.payload.assign(adj->buffer.payload.begin() + bytes, adj->buffer.payload.end());
											// vector <decltype(adj->buffer.payload)::value_type> (adj->buffer.payload.begin() + bytes, adj->buffer.payload.end()).swap(adj->buffer.payload);
										// Если данных в буфере больше нет, очищаем буфер собранных данных
										else adj->buffer.payload.clear();
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
											this->_callback.call <const int32_t, const uint64_t> ("handshake", adj->sid, aid);
										// Если установлена функция отлова завершения запроса
										if(this->_callback.is("end"))
											// Выводим функцию обратного вызова
											this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", adj->sid, aid, direct_t::SEND);
										// Завершаем работу
										return;
									// Формируем ответ, что страница не доступна
									} else buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (500)));
								// Сообщаем, что рукопожатие не выполнено
								} else buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (403), "Handshake failed"));
							} break;
							// Если запрос неудачный
							case static_cast <uint8_t> (http_t::stath_t::FAULT):
								// Формируем запрос авторизации
								buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (401)));
							break;
							// Если результат определить не получилось
							default: buffer = http.reject(awh::web_t::res_t(static_cast <u_int> (500), "Unknown request"));
						}
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
							http.compress(compress);
							// Выполняем извлечение параметров запроса
							const auto & request = adj->http.request();
							// Получаем параметры ответа
							const auto response = adj->http.response();
							// Выполняем отправку заголовков сообщения
							dynamic_cast <server::core_t *> (core)->write(buffer.data(), buffer.size(), aid);
							// Получаем данные тела ответа
							while(!(payload = http.payload()).empty()){
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Выводим сообщение о выводе чанка полезной нагрузки
									cout << this->_fmk->format("<chunk %u>", payload.size()) << endl;
								#endif
								// Устанавливаем флаг закрытия подключения
								adj->stopped = http.body().empty();
								// Выполняем отправку чанков
								dynamic_cast <server::core_t *> (core)->write(payload.data(), payload.size(), aid);
							}
							// Если получение данных нужно остановить
							if(adj->stopped)
								// Выполняем запрет на получение входящих данных
								dynamic_cast <server::core_t *> (core)->disabled(engine_t::method_t::READ, aid);
							// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
							if(!adj->http.body().empty() && this->_callback.is("entity"))
								// Выполняем функцию обратного вызова
								this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &> ("entity", adj->sid, aid, request.method, request.url, adj->http.body());
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
					// Завершаем работу
					return;
				// Если рукопожатие выполнено
				} else if(adj->allow.receive) {
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
									this->pong(aid, core, string(data.begin(), data.end()));
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
									dynamic_cast <server::core_t *> (core)->close(aid);
									// Если функция обратного вызова активности потока установлена
									if(this->_callback.is("stream"))
										// Выполняем функцию обратного вызова
										this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", adj->sid, aid, mode_t::CLOSE);
									// Выходим из функции
									return;
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
								this->_thr.push(std::bind(&ws1_t::extraction, this, aid, buffer, (adj->frame.opcode == ws::frame_t::opcode_t::TEXT)));
							// Если тредпул не активирован, выполняем извлечение полученных сообщений
							else this->extraction(aid, buffer, (adj->frame.opcode == ws::frame_t::opcode_t::TEXT));
							// Очищаем буфер полученного сообщения
							buffer.clear();
						}
						// Если данные мы все получили, выходим
						if(!receive || adj->buffer.payload.empty()) break;
					}
					// Выходим из функции
					return;
				}
				// Устанавливаем метку остановки адъютанта
				Stop:
				// Отправляем серверу сообщение
				this->sendError(aid, adj->mess);
				// Если функция обратного вызова активности потока установлена
				if(this->_callback.is("stream"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", adj->sid, aid, mode_t::CLOSE);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::WARNING, http::error_t::WEBSOCKET, this->_fmk->format("%s [%u]", adj->mess.code, adj->mess.text.c_str()));
				// Если установлена функция отлова завершения запроса
				if(this->_callback.is("end"))
					// Выводим функцию обратного вызова
					this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", adj->sid, aid, direct_t::RECV);
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
void awh::server::WebSocket1::writeCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если необходимо выполнить закрыть подключение
			if(!adj->close && adj->stopped){
				// Устанавливаем флаг закрытия подключения
				adj->close = !adj->close;
				// Принудительно выполняем отключение лкиента
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
void awh::server::WebSocket1::persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
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
/**
 * error Метод вывода сообщений об ошибках работы адъютанта
 * @param aid     идентификатор адъютанта
 * @param message сообщение с описанием ошибки
 */
void awh::server::WebSocket1::error(const uint64_t aid, const ws::mess_t & message) const noexcept {
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
void awh::server::WebSocket1::extraction(const uint64_t aid, const vector <char> & buffer, const bool text) noexcept {
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
					if((adj->stopped = !data.empty())){
						// Выполняем запрет на получение входящих данных
						const_cast <server::core_t *> (this->_core)->disabled(engine_t::method_t::READ, aid);
						// Выполняем отправку сообщения адъютанту
						const_cast <server::core_t *> (this->_core)->write(data.data(), data.size(), aid);
					// Завершаем работу
					} else const_cast <server::core_t *> (this->_core)->close(aid);
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
void awh::server::WebSocket1::pong(const uint64_t aid, awh::core_t * core, const string & message) noexcept {
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
				// Выполняем отправку сообщения адъютанту
				dynamic_cast <server::core_t *> (core)->write(buffer.data(), buffer.size(), aid);
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 * @param      message сообщение для отправки
 */
void awh::server::WebSocket1::ping(const uint64_t aid, awh::core_t * core, const string & message) noexcept {
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
				// Выполняем отправку сообщения адъютанту
				dynamic_cast <server::core_t *> (core)->write(buffer.data(), buffer.size(), aid);
		}
	}
}
/**
 * garbage Метод удаления мусорных адъютантов
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket1::garbage(const u_short tid, awh::core_t * core) noexcept {
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
	web_t::garbage(tid, core);
}
/**
 * init Метод инициализации WebSocket-сервера
 * @param socket   unix-сокет для биндинга
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::WebSocket1::init(const string & socket, const http_t::compress_t compress) noexcept {
	// Устанавливаем тип компрессии
	this->_scheme.compress = compress;
	// Выполняем инициализацию родительского объекта
	web_t::init(socket, compress);
}
/**
 * init Метод инициализации WebSocket-сервера
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::WebSocket1::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
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
void awh::server::WebSocket1::sendError(const uint64_t aid, const ws::mess_t & mess) noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Если код ошибки относится к WebSocket
		if(mess.code >= 1000){
			// Получаем параметры подключения адъютанта
			ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Получаем объект биндинга ядра TCP/IP
			server::core_t * core = const_cast <server::core_t *> (this->_core);
			// Если параметры подключения адъютанта получены
			if(adj != nullptr){
				// Запрещаем получение данных
				adj->allow.receive = false;
				// Выполняем остановку получения данных
				core->disabled(engine_t::method_t::READ, aid);
			}
			// Если отправка сообщений разблокированна
			if((adj != nullptr) && adj->allow.send){
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
					// Выполняем отправку сообщения адъютанту
					core->write(buffer.data(), buffer.size(), aid);
					// Выходим из функции
					return;
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
void awh::server::WebSocket1::send(const uint64_t aid, const char * message, const size_t size, const bool text) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && (aid > 0) && (size > 0) && (message != nullptr)){
		// Получаем параметры подключения адъютанта
		ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если отправка сообщений разблокированна
		if((adj != nullptr) && adj->allow.send){
			// Выполняем блокировку отправки сообщения
			adj->allow.send = !adj->allow.send;
			// Если рукопожатие выполнено
			if(adj->http.isHandshake(http_t::process_t::REQUEST)){
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
							// Отправляем серверу сообщение
							const_cast <server::core_t *> (this->_core)->write(buffer.data(), buffer.size(), aid);
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
						// Отправляем серверу сообщение
						const_cast <server::core_t *> (this->_core)->write(buffer.data(), buffer.size(), aid);
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
void awh::server::WebSocket1::on(function <void (const uint64_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для извлечения пароля
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <string (const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <bool (const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих ошибок
	this->_callback.set <void (const uint64_t, const u_int, const string &)> ("wserror", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const uint64_t, const vector <char> &, const bool)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих сообщений
	this->_callback.set <void (const uint64_t, const vector <char> &, const bool)> ("message", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функция обратного вызова при выполнении рукопожатия
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const int32_t, const uint64_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const int32_t, const uint64_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const int32_t, const uint64_t, const direct_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const int32_t, const uint64_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const int32_t, const uint64_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции вывода запроса клиента к серверу
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с клиента
 * @param callback функция обратного вызова
 */
void awh::server::WebSocket1::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * port Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    порт подключения адъютанта
 */
u_int awh::server::WebSocket1::port(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getPort(aid);
}
/**
 * ip Метод получения IP-адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::WebSocket1::ip(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getIp(aid);
}
/**
 * mac Метод получения MAC-адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::WebSocket1::mac(const uint64_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getMac(aid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::WebSocket1::stop() noexcept {
	// Выполняем остановку работы основного модуля
	web_t::stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::WebSocket1::start() noexcept {
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
void awh::server::WebSocket1::close(const uint64_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr){
		// Устанавливаем флаг закрытия подключения адъютанта
		adj->close = true;
		// Выполняем отключение адъютанта
		const_cast <server::core_t *> (this->_core)->close(aid);
	}
}
/**
 * sub Метод установки сабпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::server::WebSocket1::sub(const string & sub) noexcept {
	// Устанавливаем сабпротокол
	if(!sub.empty())
		// Выполняем установку сабпротокола
		this->_subs.push_back(sub);
}
/**
 * subs Метод установки списка сабпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::server::WebSocket1::subs(const vector <string> & subs) noexcept {
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
const string & awh::server::WebSocket1::sub(const uint64_t aid) const noexcept {
	// Результат работы функции
	static const string result = "";
	// Получаем параметры подключения адъютанта
	ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr)
		// Выводим согласованный сабпротокол
		return adj->http.sub();
	// Выводим результат по умолчанию
	return result;
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::server::WebSocket1::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка расширений
	this->_extensions = extensions;
}
/**
 * extensions Метод извлечения списка расширений
 * @param aid идентификатор адъютанта
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::WebSocket1::extensions(const uint64_t aid) const noexcept {
	// Результат работы функции
	static const vector <vector <string>> result;
	// Получаем параметры подключения адъютанта
	ws_scheme_t::coffer_t * adj = const_cast <ws_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr)
		// Выполняем установку списка расширений WebSocket
		return adj->http.extensions();
	// Выводим результат по умолчанию
	return result;
}
/**
 * multiThreads Метод активации многопоточности
 * @param threads количество потоков для активации
 * @param mode    флаг активации/деактивации мультипоточности
 */
void awh::server::WebSocket1::multiThreads(const size_t threads, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Если многопоточность ещё не активированна
		if(!this->_thr.is())
			// Выполняем инициализацию пула потоков
			this->_thr.init(threads);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->_thr.wait();
			// Выполняем инициализацию нового тредпула
			this->_thr.init(threads);
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
void awh::server::WebSocket1::total(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	const_cast <server::core_t *> (this->_core)->total(this->_scheme.sid, total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::WebSocket1::segmentSize(const size_t size) noexcept {
	// Если размер сегмента фрейма передан
	if(size > 0)
		// Устанавливаем размер одного сегмента фрейма
		this->_frameSize = size;
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::WebSocket1::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага автоматического перезапуска
	const_cast <server::core_t *> (this->_core)->clusterAutoRestart(this->_scheme.sid, mode);
}
/**
 * compress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::WebSocket1::compress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_scheme.compress = compress;
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::WebSocket1::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
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
void awh::server::WebSocket1::mode(const set <flag_t> & flags) noexcept {
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
void awh::server::WebSocket1::alive(const bool mode) noexcept {
	// Выполняем установку долгоживущего подключения
	web_t::alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::WebSocket1::alive(const time_t time) noexcept {
	// Выполняем установку времени жизни подключения
	web_t::alive(time);
}
/*
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::server::WebSocket1::core(const server::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
		// Активируем персистентный запуск для работы пингов
		const_cast <server::core_t *> (this->_core)->persistEnable(true);
		// Добавляем схемы сети в сетевое ядро
		const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
		// Устанавливаем функцию активации ядра сервера
		const_cast <server::core_t *> (this->_core)->on(std::bind(&ws1_t::eventsCallback, this, _1, _2));
		// Если многопоточность активированна
		if(this->_thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_thr.is()){
			// Выполняем завершение всех активных потоков
			this->_thr.wait();
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
void awh::server::WebSocket1::setHeaders(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку заголовков которые нужно передать клиенту
	this->_headers = headers;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::WebSocket1::waitTimeDetect(const time_t read, const time_t write) noexcept {
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
void awh::server::WebSocket1::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
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
 * WebSocket1 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::WebSocket1::WebSocket1(const fmk_t * fmk, const log_t * log) noexcept : web_t(fmk, log), _frameSize(0), _scheme(fmk, log) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&ws1_t::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&ws1_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&ws1_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&ws1_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&ws1_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws1_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта адъютанта
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&ws1_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
/**
 * WebSocket1 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::WebSocket1::WebSocket1(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : web_t(core, fmk, log), _frameSize(0), _scheme(fmk, log) {
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&ws1_t::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&ws1_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&ws1_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&ws1_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&ws1_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws1_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта адъютанта
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&ws1_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
/**
 * ~WebSocket1 Деструктор
 */
awh::server::WebSocket1::~WebSocket1() noexcept {
	// Если многопоточность активированна
	if(this->_thr.is())
		// Выполняем завершение всех потоков
		this->_thr.wait();
}
