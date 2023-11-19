/**
 * @file: http2.cpp
 * @date: 2022-10-12
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
#include <server/web/http2.hpp>

/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Http2::connectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
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
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Выполняем установку идентификатора объекта
				options->http.id(bid);
				// Устанавливаем размер чанка
				options->http.chunk(this->_chunkSize);
				// Устанавливаем флаг идентичности протокола
				options->http.identity(this->_identity);
				// Устанавливаем список компрессоров поддерживаемый сервером
				options->http.compressors(this->_scheme.compressors);
				// Устанавливаем данные сервиса
				options->http.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Устанавливаем функцию обратного вызова для вывода ошибок
					options->http.on((function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)>) this->_callback.get <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error"));
				// Если требуется установить параметры шифрования
				if(this->_encryption.mode){
					// Устанавливаем флаг шифрования
					options->http.encryption(this->_encryption.mode);
					// Устанавливаем параметры шифрования
					options->http.encryption(this->_encryption.pass, this->_encryption.salt, this->_encryption.cipher);
				}
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
		// Если протокол HTTP/2 для клиента не инициализирован
		} else {
			// Выполняем установку сетевого ядра
			this->_http1._core = dynamic_cast <server::core_t *> (core);
			// Устанавливаем метод компрессии поддерживаемый сервером
			this->_http1._scheme.compressors = this->_scheme.compressors;
			// Выполняем переброс вызова коннекта на клиент WebSocket
			this->_http1.connectCallback(bid, sid, core);
		}
		// Выполняем добавление агнета
		this->_agents.emplace(bid, agent_t::HTTP);
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выполняем функцию обратного вызова
			this->_callback.call <const uint64_t, const mode_t> ("active", bid, mode_t::CONNECT);
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении клиента
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Http2::disconnectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Выполняем поиск брокера в списке активных сессий
		auto it = this->_sessions.find(bid);
		// Если активная сессия найдена
		if(it != this->_sessions.end()){
			// Выполняем закрытие подключения
			it->second->close();
			// Выполняем поиск брокера в списке активных сессий
			auto jt = this->_ws2._sessions.find(bid);
			// Если активная сессия найдена
			if(jt != this->_ws2._sessions.end())
				// Выполняем закрытие подключения
				(* jt->second.get()) = nullptr;
		}
		// Выполняем отключение подключившегося брокера
		this->disconnect(bid);
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выполняем функцию обратного вызова
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
void awh::server::Http2::readCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Флаг выполнения обработки полученных данных
		bool process = false;
		// Если установлена функция обратного вызова для вывода данных в сыром виде
		if(!(process = !this->_callback.is("raw")))
			// Выполняем функцию обратного вызова
			process = this->_callback.apply <bool, const uint64_t, const char *, const size_t> ("raw", bid, buffer, size);
		// Если обработка полученных данных разрешена
		if(process){
			// Получаем параметры активного клиента
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
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
				// Определяем протокола подключения
				switch(static_cast <uint8_t> (options->proto)){
					// Если протокол подключения соответствует HTTP/1.1
					case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
						// Выполняем поиск агента которому соответствует клиент
						auto it = this->_http1._agents.find(bid);
						// Если активный агент клиента установлен
						if(it != this->_http1._agents.end()){
							// Определяем тип активного протокола
							switch(static_cast <uint8_t> (it->second)){
								// Если протокол соответствует HTTP-протоколу
								case static_cast <uint8_t> (agent_t::HTTP):
								// Если протокол соответствует протоколу WebSocket
								case static_cast <uint8_t> (agent_t::WEBSOCKET):
									// Выполняем переброс вызова чтения клиенту HTTP
									this->_http1.readCallback(buffer, size, bid, sid, core);
								break;
							}
						}
					} break;
					// Если протокол подключения соответствует HTTP/2
					case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
						// Выполняем поиск агента которому соответствует клиент
						auto it = this->_agents.find(bid);
						// Если активный агент клиента установлен
						if(it != this->_agents.end()){
							// Определяем тип активного протокола
							switch(static_cast <uint8_t> (it->second)){
								// Если протокол соответствует HTTP-протоколу
								case static_cast <uint8_t> (agent_t::HTTP): {
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
								} break;
								// Если протокол соответствует протоколу WebSocket
								case static_cast <uint8_t> (agent_t::WEBSOCKET):
									// Выполняем переброс вызова чтения клиенту WebSocket
									this->_ws2.readCallback(buffer, size, bid, sid, core);
								break;
							}
						}
					} break;
				}
			}
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
void awh::server::Http2::writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
							// Если протокол соответствует протоколу WebSocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем переброс вызова записи клиенту HTTP
								this->_http1.writeCallback(buffer, size, bid, sid, core);
							break;
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Если необходимо выполнить закрыть подключение
								if(!options->close && options->stopped){
									// Устанавливаем флаг закрытия подключения
									options->close = !options->close;
									// Принудительно выполняем отключение лкиента
									const_cast <server::core_t *> (this->_core)->close(bid);
								}
							} break;
							// Если протокол соответствует протоколу WebSocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем переброс вызова записи клиенту WebSocket
								this->_ws2.writeCallback(buffer, size, bid, sid, core);
							break;
						}
					}
				} break;
			}
		}
	}
}
/**
 * beginSignal Метод начала получения фрейма заголовков HTTP/2
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @return    статус полученных данных
 */
int awh::server::Http2::beginSignal(const int32_t sid, const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Устанавливаем новый идентификатор потока
		options->sid = sid;
		// Выполняем очистку HTTP-парсера
		options->http.clear();
		// Выполняем сброс состояния HTTP-парсера
		options->http.reset();
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
int awh::server::Http2::closedSignal(const int32_t sid, const uint64_t bid, const awh::http2_t::error_t error) noexcept {
	// Если разрешено выполнить остановку
	if((this->_core != nullptr) && (error != awh::http2_t::error_t::NONE)){
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
 * headerSignal Метод обратного вызова при получении заголовка HTTP/2
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @param key данные ключа заголовка
 * @param val данные значения заголовка
 * @return    статус полученных данных
 */
int awh::server::Http2::headerSignal(const int32_t sid, const uint64_t bid, const string & key, const string & val) noexcept {
	// Получаем параметры активного клиента
	scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Устанавливаем полученные заголовки
		options->http.header2(key, val);
		// Если функция обратного вызова на полученного заголовка с сервера установлена
		if(this->_callback.is("header"))
			// Выполняем функцию обратного вызова
			this->_callback.call <const int32_t, const uint64_t, const string &, const string &> ("header", sid, bid, key, val);
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
int awh::server::Http2::chunkSignal(const int32_t sid, const uint64_t bid, const uint8_t * buffer, const size_t size) noexcept {
	// Получаем параметры активного клиента
	scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Если функция обратного вызова на перехват входящих чанков установлена
		if(this->_callback.is("chunking"))
			// Выполняем функцию обратного вызова
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
			// Выполняем поиск агента которому соответствует клиент
			auto it = this->_agents.find(bid);
			// Если активный агент клиента установлен
			if(it != this->_agents.end()){
				// Определяем тип активного протокола
				switch(static_cast <uint8_t> (it->second)){
					// Если протокол соответствует HTTP-протоколу
					case static_cast <uint8_t> (agent_t::HTTP):
						// Добавляем полученный чанк в тело данных
						options->http.payload(vector <char> (buffer, buffer + size));
					break;
					// Если протокол соответствует протоколу WebSocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем передачу сигнала полученных чанков в модуль WebSocket
						this->_ws2.chunkSignal(sid, bid, buffer, size);
					// Выводим результат
					return 0;
				}
			// Добавляем полученный чанк в тело данных
			} else options->http.body(vector <char> (buffer, buffer + size));
			// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
			if(this->_callback.is("chunks"))
				// Выполняем функцию обратного вызова
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
 * @param frame  тип полученного фрейма
 * @param flags  флаги полученного фрейма
 * @return       статус полученных данных
 */
int awh::server::Http2::frameSignal(const int32_t sid, const uint64_t bid, const awh::http2_t::direct_t direct, const awh::http2_t::frame_t frame, const set <awh::http2_t::flag_t> & flags) noexcept {
	// Определяем направление передачи фрейма
	switch(static_cast <uint8_t> (direct)){
		// Если производится передача фрейма на сервер
		case static_cast <uint8_t> (awh::http2_t::direct_t::SEND): {
			// Если мы получили флаг завершения потока
			if(flags.count(awh::http2_t::flag_t::END_STREAM) > 0){
				// Получаем параметры активного клиента
				scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
				// Если параметры активного клиента получены
				if((this->_core != nullptr) && (options != nullptr)){
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
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
							} break;
							// Если протокол соответствует протоколу WebSocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET): {
								// Выполняем передачу фрейма клиенту WebSocket
								this->_ws2.frameSignal(sid, bid, direct, frame, flags);
								// Выполняем поиск брокера в списке активных сессий
								auto it = this->_ws2._sessions.find(bid);
								// Если активная сессия найдена
								if(it != this->_ws2._sessions.end()){
									// Если сессия была удалена
									if(!it->second->is())
										// Выполняем копирование контекста сессии HTTP/2
										(* this->_sessions.at(bid).get()) = (* it->second.get());
								}
							} break;
						}
					}
				}
				// Если установлена функция отлова завершения запроса
				if(this->_callback.is("end"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", sid, bid, direct_t::SEND);
				// Выходим из функции
				return 0;
			}
		} break;
		// Если производится получения фрейма с сервера
		case static_cast <uint8_t> (awh::http2_t::direct_t::RECV): {
			// Получаем параметры активного клиента
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if((this->_core != nullptr) && (options != nullptr)){
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP): {
							// Выполняем определение типа фрейма
							switch(static_cast <uint8_t> (frame)){
								// Если мы получили входящие данные тела ответа
								case static_cast <uint8_t> (awh::http2_t::frame_t::DATA): {
									// Если мы получили неустановленный флаг или флаг завершения потока
									if(flags.count(awh::http2_t::flag_t::END_STREAM) > 0){
										// Выполняем коммит полученного результата
										options->http.commit();
										// Выполняем обработку полученных данных
										this->prepare(sid, bid, const_cast <server::core_t *> (this->_core));
										// Если функция обратного вызова активности потока установлена
										if(this->_callback.is("stream"))
											// Выполняем функцию обратного вызова
											this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", sid, bid, mode_t::CLOSE);
										// Если установлена функция отлова завершения запроса
										if(this->_callback.is("end"))
											// Выполняем функцию обратного вызова
											this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", sid, bid, direct_t::RECV);
									}
								} break;
								// Если мы получили входящие данные заголовков ответа
								case static_cast <uint8_t> (awh::http2_t::frame_t::HEADERS): {
									// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
									if(flags.count(awh::http2_t::flag_t::END_HEADERS) > 0){
										// Выполняем коммит полученного результата
										options->http.commit();
										// Выполняем извлечение параметров запроса
										const auto & request = options->http.request();
										// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
										if(this->_callback.is("request"))
											// Выполняем функцию обратного вызова
											this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &> ("request", sid, bid, request.method, request.url);
										// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
										if(this->_callback.is("headers"))
											// Выполняем функцию обратного вызова
											this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &> ("headers", options->sid, bid, request.method, request.url, options->http.headers());
										// Если мы получили неустановленный флаг или флаг завершения потока
										if(flags.count(awh::http2_t::flag_t::END_STREAM) > 0){
											// Выполняем обработку полученных данных
											this->prepare(sid, bid, const_cast <server::core_t *> (this->_core));
											// Если функция обратного вызова активности потока установлена
											if(this->_callback.is("stream"))
												// Выполняем функцию обратного вызова
												this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", sid, bid, mode_t::CLOSE);
											// Если установлена функция отлова завершения запроса
											if(this->_callback.is("end"))
												// Выполняем функцию обратного вызова
												this->_callback.call <const int32_t, const uint64_t, const direct_t> ("end", sid, bid, direct_t::RECV);
										// Если заголовок WebSocket активирован
										} else if(options->http.identity() == awh::http_t::identity_t::WS)
											// Выполняем обработку полученных данных
											this->prepare(sid, bid, const_cast <server::core_t *> (this->_core));
									}
								} break;
							}
						} break;
						// Если протокол соответствует протоколу WebSocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выполняем передачу фрейма клиенту WebSocket
							this->_ws2.frameSignal(sid, bid, direct, frame, flags);
						break;
					}
				}
			}
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * prepare Метод выполнения препарирования полученных данных
 * @param sid  идентификатор потока
 * @param bid  идентификатор брокера
 * @param core объект сетевого ядра
 */
void awh::server::Http2::prepare(const int32_t sid, const uint64_t bid, server::core_t * core) noexcept {
	// Получаем параметры активного клиента
	scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Если подключение не установлено как постоянное
		if(!this->_service.alive && !options->alive){
			// Увеличиваем количество выполненных запросов
			options->requests++;
			// Если количество выполненных запросов превышает максимальный
			if(options->requests >= this->_maxRequests)
				// Устанавливаем флаг закрытия подключения
				options->close = true;
		// Выполняем сброс количества выполненных запросов
		} else options->requests = 0;
		// Получаем флаг шифрованных данных
		options->crypted = options->http.crypted();
		// Получаем поддерживаемый метод компрессии
		options->compress = options->http.compression();
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
				// Если тело ответа существует
				if(!options->http.body().empty())
					// Выводим сообщение о выводе чанка тела
					cout << this->_fmk->format("<body %u>", options->http.body().size()) << endl << endl;
				// Иначе устанавливаем перенос строки
				else cout << endl;
			}
		#endif
		// Выполняем проверку авторизации
		switch(static_cast <uint8_t> (options->http.auth())){
			// Если запрос выполнен удачно
			case static_cast <uint8_t> (http_t::status_t::GOOD): {
				// Если заголовок WebSocket активирован
				if(options->http.identity() == awh::http_t::identity_t::WS){
					// Если запрашиваемый протокол соответствует WebSocket
					if(this->_webSocket)
						// Выполняем инициализацию WebSocket-сервера
						this->websocket(sid, bid, core);
					// Если протокол запрещён или не поддерживается
					else {
						// Выполняем очистку HTTP-парсера
						options->http.clear();
						// Выполняем сброс состояния HTTP-парсера
						options->http.reset();
						// Формируем ответ на запрос об авторизации
						const awh::web_t::res_t & response = awh::web_t::res_t(2.0f, static_cast <u_int> (505), "Requested protocol is not supported by this server");
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
							awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
							// Если тело запроса не существует
							if(options->http.body().empty())
								// Устанавливаем флаг завершения потока
								flag = awh::http2_t::flag_t::END_STREAM;
							// Выполняем заголовки запроса на сервер
							const int32_t sid = web2_t::send(options->sid, bid, headers, flag);
							// Если запрос не получилось отправить
							if(sid < 0)
								// Выходим из функции
								return;
							// Если тело запроса существует
							if(!options->http.body().empty()){
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
									if(options->http.body().empty() && (options->http.trailers() == 0))
										// Устанавливаем флаг завершения потока
										flag = awh::http2_t::flag_t::END_STREAM;
									// Выполняем отправку тела запроса на сервер
									if(!web2_t::send(options->sid, bid, entity.data(), entity.size(), flag))
										// Выходим из функции
										return;
								}
								// Если список трейлеров установлен
								if(options->http.trailers() > 0){
									// Выполняем извлечение трейлеров
									const auto & trailers = options->http.trailers2();
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										// Название выводимого заголовка
										string name = "";
										// Выводим заголовок трейлеров
										cout << "<Trailers>" << endl << endl;
										// Выполняем перебор всего списка отправляемых трейлеров
										for(auto & trailer : trailers){
											// Получаем название заголовка
											name = trailer.first;
											// Переводим заголовок в нормальный режим
											this->_fmk->transform(name, fmk_t::transform_t::SMART);
											// Выводим сообщение о выводе чанка тела
											cout << this->_fmk->format("%s: %s", name.c_str(), trailer.second.c_str()) << endl;
										}
										// Выводим завершение вывода информации
										cout << endl << endl;
									#endif
									// Выполняем отправку трейлеров
									if(!web2_t::send(options->sid, bid, trailers))
										// Выходим из функции
										return;
								}
							}
						// Если сообщение о закрытии подключения не отправлено
						} else if(!web2_t::reject(options->sid, bid, awh::http2_t::error_t::PROTOCOL_ERROR))
							// Выполняем отключение брокера
							dynamic_cast <server::core_t *> (core)->close(bid);
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "Requested protocol is not supported by this server");
					}
					// Завершаем обработку
					return;
				}
				// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
				if(!options->http.body().empty() && this->_callback.is("entity")){
					// Выполняем извлечение параметров запроса
					const auto & request = options->http.request();
					// Выполняем функцию обратного вызова
					this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &> ("entity", sid, bid, request.method, request.url, options->http.body());
				}
				// Если функция обратного вызова на получение удачного запроса установлена
				if(this->_callback.is("handshake"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const int32_t, const uint64_t, const agent_t> ("handshake", sid, bid, agent_t::HTTP);
			} break;
			// Если запрос неудачный
			case static_cast <uint8_t> (http_t::status_t::FAULT): {
				// Выполняем очистку HTTP-парсера
				options->http.clear();
				// Выполняем сброс состояния HTTP-парсера
				options->http.reset();
				// Ответ на запрос об авторизации
				awh::web_t::res_t response;
				// Определяем идентичность сервера
				switch(static_cast <uint8_t> (this->_identity)){
					// Если сервер соответствует HTTP-серверу
					case static_cast <uint8_t> (http_t::identity_t::HTTP):
						// Формируем ответ на запрос об авторизации
						response = awh::web_t::res_t(2.0f, static_cast <u_int> (401));
					break;
					// Если сервер соответствует PROXY-серверу
					case static_cast <uint8_t> (http_t::identity_t::PROXY):
						// Формируем ответ на запрос об авторизации
						response = awh::web_t::res_t(2.0f, static_cast <u_int> (407));
					break;
				}
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
					awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
					// Если тело запроса не существует
					if(options->http.body().empty())
						// Устанавливаем флаг завершения потока
						flag = awh::http2_t::flag_t::END_STREAM;
					// Выполняем заголовки запроса на сервер
					const int32_t sid = web2_t::send(options->sid, bid, headers, flag);
					// Если запрос не получилось отправить
					if(sid < 0)
						// Выходим из функции
						return;
					// Если тело запроса существует
					if(!options->http.body().empty()){
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
							if(options->http.body().empty() && (options->http.trailers() == 0))
								// Устанавливаем флаг завершения потока
								flag = awh::http2_t::flag_t::END_STREAM;
							// Выполняем отправку тела запроса на сервер
							if(!web2_t::send(options->sid, bid, entity.data(), entity.size(), flag))
								// Выходим из функции
								return;
						}
						// Если список трейлеров установлен
						if(options->http.trailers() > 0){
							// Выполняем извлечение трейлеров
							const auto & trailers = options->http.trailers2();
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Название выводимого заголовка
								string name = "";
								// Выводим заголовок трейлеров
								cout << "<Trailers>" << endl << endl;
								// Выполняем перебор всего списка отправляемых трейлеров
								for(auto & trailer : trailers){
									// Получаем название заголовка
									name = trailer.first;
									// Переводим заголовок в нормальный режим
									this->_fmk->transform(name, fmk_t::transform_t::SMART);
									// Выводим сообщение о выводе чанка тела
									cout << this->_fmk->format("%s: %s", name.c_str(), trailer.second.c_str()) << endl;
								}
								// Выводим завершение вывода информации
								cout << endl << endl;
							#endif
							// Выполняем отправку трейлеров
							if(!web2_t::send(options->sid, bid, trailers))
								// Выходим из функции
								return;
						}
					}
				// Если сообщение о закрытии подключения не отправлено
				} else if(!web2_t::reject(options->sid, bid, awh::http2_t::error_t::PROTOCOL_ERROR))
					// Выполняем отключение брокера
					dynamic_cast <server::core_t *> (core)->close(bid);
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выполняем функцию обратного вызова
					this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "authorization failed");
			}
		}
	}
}
/**
 * websocket Метод инициализации WebSocket протокола
 * @param sid  идентификатор потока
 * @param bid  идентификатор брокера
 * @param core объект сетевого ядра
 */
void awh::server::Http2::websocket(const int32_t sid, const uint64_t bid, server::core_t * core) noexcept {
	// Если данные переданы верные
	if((sid > 0) && (bid > 0) && (core != nullptr) && (this->_core != nullptr)){
		// Создаём брокера
		this->_ws2._scheme.set(bid);
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_ws2._scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Устанавливаем идентификатор потока
			options->sid = sid;
			// Выполняем установку идентификатора объекта
			options->http.id(bid);
			// Выполняем установку протокола подключения
			options->proto = core->proto(bid);
			// Устанавливаем флаг перехвата контекста компрессии
			options->server.takeover = this->_ws2._server.takeover;
			// Устанавливаем флаг перехвата контекста декомпрессии
			options->client.takeover = this->_ws2._client.takeover;
			// Разрешаем перехватывать контекст компрессии
			options->hash.takeoverCompress(this->_ws2._server.takeover);
			// Разрешаем перехватывать контекст декомпрессии
			options->hash.takeoverDecompress(this->_ws2._client.takeover);
			// Разрешаем перехватывать контекст для клиента
			options->http.takeover(awh::web_t::hid_t::CLIENT, this->_ws2._client.takeover);
			// Разрешаем перехватывать контекст для сервера
			options->http.takeover(awh::web_t::hid_t::SERVER, this->_ws2._server.takeover);
			// Устанавливаем данные сервиса
			options->http.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Устанавливаем функцию обратного вызова для вывода ошибок
				options->http.on((function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)>) this->_callback.get <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error"));
			// Если сабпротоколы установлены
			if(!this->_ws2._subprotocols.empty())
				// Устанавливаем поддерживаемые сабпротоколы
				options->http.subprotocols(this->_ws2._subprotocols);
			// Если список расширений установлены
			if(!this->_ws2._extensions.empty())
				// Устанавливаем список поддерживаемых расширений
				options->http.extensions(this->_ws2._extensions);
			// Если размер фрейма установлен
			if(this->_ws2._frameSize > 0)
				// Выполняем установку размера фрейма
				options->frame.size = this->_ws2._frameSize;
			// Устанавливаем метод компрессии поддерживаемый сервером
			options->http.compressors(this->_scheme.compressors);
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
			// Получаем параметры активного клиента
			scheme::web_t::options_t * web = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(web != nullptr){
				// Выполняем установку параметров запроса
				options->http.request(web->http.request());
				// Выполняем установку полученных заголовков
				options->http.headers(web->http.headers());
				// Выполняем коммит полученного результата
				options->http.commit();
				// Ответ клиенту по умолчанию успешный
				awh::web_t::res_t response(2.0f, static_cast <u_int> (200));
				// Если рукопожатие выполнено
				if((options->shake = options->http.handshake(http_t::process_t::REQUEST))){
					// Проверяем версию протокола
					if(!options->http.check(ws_core_t::flag_t::VERSION)){
						// Получаем бинарные данные REST запроса
						response = awh::web_t::res_t(2.0f, static_cast <u_int> (400), "Unsupported protocol version");
						// Завершаем работу
						goto End;
					}
					// Получаем флаг шифрованных данных
					options->crypted = options->http.crypted();
					// Выполняем очистку HTTP-парсера
					options->http.clear();
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
						// Выполняем замену активного агнета
						this->_agents.at(bid) = agent_t::WEBSOCKET;
						// Выполняем поиск брокера в списке активных сессий
						auto it = this->_sessions.find(bid);
						// Если активная сессия найдена
						if(it != this->_sessions.end()){
							// Выполняем создание нового объекта сессии HTTP/2
							auto ret = this->_ws2._sessions.emplace(bid, unique_ptr <awh::http2_t> (new awh::http2_t(this->_fmk, this->_log)));
							// Выполняем копирование контекста сессии HTTP/2
							(* ret.first->second.get()) = (* it->second.get());
						}
						// Выполняем установку сетевого ядра
						this->_ws2._core = dynamic_cast <server::core_t *> (core);
						// Выполняем ответ подключившемуся клиенту
						if(web2_t::send(options->sid, bid, headers, awh::http2_t::flag_t::NONE) < 0)
							// Выходим из функции
							return;
						// Выполняем извлечение параметров запроса
						const auto & request = options->http.request();
						// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
						if(!options->http.body().empty() && this->_callback.is("entity"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &> ("entity", options->sid, bid, request.method, request.url, options->http.body());
						// Если функция обратного вызова активности потока установлена
						if(this->_callback.is("stream"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const int32_t, const uint64_t, const mode_t> ("stream", options->sid, bid, mode_t::OPEN);
						// Если функция обратного вызова на получение удачного запроса установлена
						if(this->_callback.is("handshake"))
							// Выполняем функцию обратного вызова
							this->_callback.call <const int32_t, const uint64_t, const agent_t> ("handshake", options->sid, bid, agent_t::WEBSOCKET);
						// Завершаем работу
						return;
					// Формируем ответ, что произошла внутренняя ошибка сервера
					} else response = awh::web_t::res_t(2.0f, static_cast <u_int> (500));
				// Формируем ответ, что страница не доступна
				} else response = awh::web_t::res_t(2.0f, static_cast <u_int> (403), "Handshake failed");
				// Устанавливаем метку завершения запроса
				End:
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
					awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
					// Если тело запроса не существует
					if(options->http.body().empty())
						// Устанавливаем флаг завершения потока
						flag = awh::http2_t::flag_t::END_STREAM;
					// Выполняем ответ подключившемуся клиенту
					if(web2_t::send(options->sid, bid, headers, flag) < 0)
						// Выходим из функции
						return;
					// Если тело запроса существует
					if(!options->http.body().empty()){
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
							if(options->http.body().empty() && (options->http.trailers() == 0))
								// Устанавливаем флаг завершения потока
								flag = awh::http2_t::flag_t::END_STREAM;
							// Выполняем отправку тела запроса на сервер
							if(!web2_t::send(options->sid, bid, entity.data(), entity.size(), flag))
								// Выходим из функции
								return;
						}
						// Если список трейлеров установлен
						if(options->http.trailers() > 0){
							// Выполняем извлечение трейлеров
							const auto & trailers = options->http.trailers2();
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Название выводимого заголовка
								string name = "";
								// Выводим заголовок трейлеров
								cout << "<Trailers>" << endl << endl;
								// Выполняем перебор всего списка отправляемых трейлеров
								for(auto & trailer : trailers){
									// Получаем название заголовка
									name = trailer.first;
									// Переводим заголовок в нормальный режим
									this->_fmk->transform(name, fmk_t::transform_t::SMART);
									// Выводим сообщение о выводе чанка тела
									cout << this->_fmk->format("%s: %s", name.c_str(), trailer.second.c_str()) << endl;
								}
								// Выводим завершение вывода информации
								cout << endl << endl;
							#endif
							// Выполняем отправку трейлеров
							if(!web2_t::send(options->sid, bid, trailers))
								// Выходим из функции
								return;
						}
					}
					// Выполняем извлечение параметров запроса
					const auto & request = options->http.request();
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(!web->http.body().empty() && this->_callback.is("entity"))
						// Выполняем функцию обратного вызова
						this->_callback.call <const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &> ("entity", options->sid, bid, request.method, request.url, web->http.body());
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выполняем функцию обратного вызова
						this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, response.message);
					// Выполняем очистку HTTP-парсера
					options->http.clear();
					// Выполняем сброс состояния HTTP-парсера
					options->http.reset();
					// Завершаем работу
					return;
				}
				// Выполняем поиск брокера в списке активных сессий
				auto it = this->_sessions.find(bid);
				// Если активная сессия найдена
				if(it != this->_sessions.end())
					// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
					it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), bid));
				// Завершаем работу
				else const_cast <server::core_t *> (this->_core)->close(bid);
			}
		}
	}
}
/**
 * erase Метод удаления отключившихся брокеров
 * @param bid идентификатор брокера
 */
void awh::server::Http2::erase(const uint64_t bid) noexcept {
	// Если список отключившихся брокеров не пустой
	if(!this->_disconected.empty()){
		/**
		 * eraseFn Функция удаления отключившегося брокера
		 * @param bid идентификатор брокера
		 */
		auto eraseFn = [this](const uint64_t bid) noexcept -> void {
			// Получаем параметры активного клиента
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Устанавливаем флаг отключения
				options->close = true;
				// Определяем протокола подключения
				switch(static_cast <uint8_t> (options->proto)){
					// Если протокол подключения соответствует HTTP/1.1
					case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
						// Выполняем поиск агента которому соответствует клиент
						auto it = this->_http1._agents.find(bid);
						// Если активный агент клиента установлен
						if(it != this->_http1._agents.end()){
							// Определяем тип активного протокола
							switch(static_cast <uint8_t> (it->second)){
								// Если протокол соответствует HTTP-протоколу
								case static_cast <uint8_t> (agent_t::HTTP):
								// Если протокол соответствует протоколу WebSocket
								case static_cast <uint8_t> (agent_t::WEBSOCKET):
									// Выполняем удаление отключённого брокера HTTP-клиента
									this->_http1.erase(bid);
								break;
							}
						}
					} break;
					// Если протокол подключения соответствует HTTP/2
					case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
						// Выполняем поиск агента которому соответствует клиент
						auto it = this->_agents.find(bid);
						// Если активный агент клиента установлен
						if(it != this->_agents.end()){
							// Определяем тип активного протокола
							switch(static_cast <uint8_t> (it->second)){
								// Если протокол соответствует HTTP-протоколу
								case static_cast <uint8_t> (agent_t::HTTP):
									// Выполняем удаление созданной ранее сессии HTTP/2
									this->_sessions.erase(bid);
								break;
								// Если протокол соответствует протоколу WebSocket
								case static_cast <uint8_t> (agent_t::WEBSOCKET): {
									// Выполняем поиск брокера в списке активных сессий
									auto it = this->_ws2._sessions.find(bid);
									// Если активная сессия найдена
									if(it != this->_ws2._sessions.end())
										// Выполняем закрытие подключения
										(* it->second.get()) = nullptr;
									// Выполняем удаление созданной ранее сессии HTTP/2
									this->_sessions.erase(bid);
									// Выполняем удаление отключённого брокера WebSocket-клиента
									this->_ws2.erase(bid);
								} break;
							}
							// Выполняем удаление активного агента
							this->_agents.erase(it);
						}
					} break;
				}
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
void awh::server::Http2::disconnect(const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Определяем протокола подключения
		switch(static_cast <uint8_t> (options->proto)){
			// Если протокол подключения соответствует HTTP/1.1
			case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_http1._agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_http1._agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
						// Если протокол соответствует протоколу WebSocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Добавляем в очередь список отключившихся клиентов
							this->_http1.disconnect(bid);
						break;
					}
				}
			} break;
			// Если протокол подключения соответствует HTTP/2
			case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует протоколу WebSocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выполняем отключение брокера клиента WebSocket
							this->_ws2.disconnect(bid);
						break;
					}
				}
			} break;
		}
		// Добавляем в очередь список отключившихся клиентов
		this->_disconected.emplace(bid, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
	}
}
/**
 * pinging Метод таймера выполнения пинга клиента
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::Http2::pinging(const uint16_t tid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((tid > 0) && (core != nullptr) && (this->_core != nullptr)){
		// Выполняем перебор всех активных клиентов
		for(auto & item : this->_scheme.get()){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (item.second->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(item.first);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
							// Если протокол соответствует протоколу WebSocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем переброс события пинга в модуль HTTP
								this->_http1.pinging(tid, core);
							break;
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(item.first);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Если переключение протокола на HTTP/2 выполнено и пинг не прошёл
								if(!this->ping(item.first)){
									// Выполняем поиск брокера в списке активных сессий
									auto it = this->_sessions.find(item.first);
									// Если активная сессия найдена
									if(it != this->_sessions.end())
										// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
										it->second->on((function <void (void)>) std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), item.first));
								}
							} break;
							// Если протокол соответствует протоколу WebSocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем переброс события пинга в модуль WebSocket
								this->_ws2.pinging(tid, core);
							break;
						}
					}
				} break;
			}
		}
	}
}
/**
 * proto Метод извлечения поддерживаемого протокола подключения
 * @param bid идентификатор брокера
 * @return    поддерживаемый протокол подключения (HTTP1_1, HTTP2)
 */
awh::engine_t::proto_t awh::server::Http2::proto(const uint64_t bid) const noexcept {
	// Выводим идентификатор активного HTTP-протокола
	return this->_core->proto(bid);
}
/**
 * parser Метод извлечения объекта HTTP-парсера
 * @param bid идентификатор брокера
 * @return    объект HTTP-парсера
 */
const awh::http_t * awh::server::Http2::parser(const uint64_t bid) const noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выполняем получение объекта HTTP-парсера
								return this->_http1.parser(bid);
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выполняем получение объекта HTTP-парсера
								return &options->http;
						}
					}
				} break;
			}
		}
	}
	// Выводим результат
	return nullptr;
}
/**
 * trailers Метод получения запроса на передачу трейлеров
 * @param bid идентификатор брокера
 * @return    флаг запроса клиентом передачи трейлеров
 */
bool awh::server::Http2::trailers(const uint64_t bid) const noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выполняем получение флага запроса клиента на передачу трейлеров
								return this->_http1.trailers(bid);
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выполняем получение флага запроса клиента на передачу трейлеров
								return options->http.is(http_t::state_t::TRAILERS);
						}
					}
				} break;
			}
		}
	}
	// Выводим результат
	return false;
}
/**
 * trailer Метод установки трейлера
 * @param bid идентификатор брокера
 * @param key ключ заголовка
 * @param val значение заголовка
 */
void awh::server::Http2::trailer(const uint64_t bid, const string & key, const string & val) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выполняем установку трейлера
								this->_http1.trailer(bid, key, val);
							break;
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выполняем установку трейлера
								options->http.trailer(key, val);
							break;
						}
					}
				} break;
			}
		}
	}
}
/**
 * init Метод инициализации WebSocket-сервера
 * @param socket      unix-сокет для биндинга
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Http2::init(const string & socket, const vector <http_t::compress_t> & compressors) noexcept {
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
void awh::server::Http2::init(const u_int port, const string & host, const vector <http_t::compress_t> & compressors) noexcept {
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
void awh::server::Http2::sendError(const uint64_t bid, const ws::mess_t & mess) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует протоколу WebSocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем отправку ошибки клиенту
								this->_http1.sendError(bid, mess);
							break;
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует протоколу WebSocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем отправку ошибки клиенту WebSocket
								this->_ws2.sendError(bid, mess);
							break;
						}
					}
				} break;
			}
		}
	}
}
/**
 * sendMessage Метод отправки сообщения клиенту
 * @param bid     идентификатор брокера
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 */
void awh::server::Http2::sendMessage(const uint64_t bid, const vector <char> & message, const bool text) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует протоколу WebSocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем передачу данных клиенту
								this->_http1.sendMessage(bid, message, text);
							break;
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует протоколу WebSocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем передачу данных клиенту WebSocket
								this->_ws2.sendMessage(bid, message, text);
							break;
						}
					}
				} break;
			}
		}
	}
}
/**
 * send Метод отправки трейлеров
 * @param id      идентификатор потока HTTP/2
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @return        результат отправки данных указанному клиенту
 */
bool awh::server::Http2::send(const int32_t id, const uint64_t bid, const vector <pair <string, string>> & headers) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем отправку трейлеров
							return web2_t::send(id, bid, headers);
					}
				}
			}
		}
	}
	// Выводим значение по умолчанию
	return false;
}
/**
 * send Метод отправки тела сообщения клиенту
 * @param id     идентификатор потока HTTP
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::Http2::send(const int32_t id, const uint64_t bid, const char * buffer, const size_t size, const bool end) noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0)){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выполняем отправку тала сообщения клиенту через протокол HTTP/1.1
								return this->_http1.send(bid, buffer, size, end);
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Тело WEB сообщения
								vector <char> entity;
								// Выполняем очистку данных тела
								options->http.clear(http_t::suite_t::BODY);
								// Устанавливаем тело запроса
								options->http.body(vector <char> (buffer, buffer + size));
								// Флаг отправляемого фрейма
								awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
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
									if(end && options->http.body().empty() && (options->http.trailers() == 0))
										// Устанавливаем флаг завершения потока
										flag = awh::http2_t::flag_t::END_STREAM;
									// Выполняем отправку данных на удалённый сервер
									result = web2_t::send(id, bid, entity.data(), entity.size(), flag);
								}
								// Если список трейлеров установлен
								if(result && (options->http.trailers() > 0)){
									// Выполняем извлечение трейлеров
									const auto & trailers = options->http.trailers2();
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										// Название выводимого заголовка
										string name = "";
										// Выводим заголовок трейлеров
										cout << "<Trailers>" << endl << endl;
										// Выполняем перебор всего списка отправляемых трейлеров
										for(auto & trailer : trailers){
											// Получаем название заголовка
											name = trailer.first;
											// Переводим заголовок в нормальный режим
											this->_fmk->transform(name, fmk_t::transform_t::SMART);
											// Выводим сообщение о выводе чанка тела
											cout << this->_fmk->format("%s: %s", name.c_str(), trailer.second.c_str()) << endl;
										}
										// Выводим завершение вывода информации
										cout << endl << endl;
									#endif
									// Выполняем отправку трейлеров
									if((result = !web2_t::send(id, bid, trailers)))
										// Выходим из функции
										return result;
								}
							} break;
						}
					}
				} break;
			}
		}
	}
	// Выводим значение по умолчанию
	return result;
}
/**
 * send Метод отправки заголовков клиенту
 * @param id      идентификатор потока HTTP
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param headers заголовки отправляемые клиенту
 * @param end     размер сообщения в байтах
 * @return        идентификатор нового запроса
 */
int32_t awh::server::Http2::send(const int32_t id, const uint64_t bid, const u_int code, const string & mess, const unordered_multimap <string, string> & headers, const bool end) noexcept {
	// Если заголовки запроса переданы
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выполняем отправку заголовков клиенту через протокол HTTP/1.1
								return this->_http1.send(bid, code, mess, headers, end);
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Выполняем сброс состояния HTTP-парсера
								options->http.reset();
								// Выполняем очистку заголовков
								options->http.clear(http_t::suite_t::HEADER);
								// Устанавливаем заголовоки запроса
								options->http.headers(headers);
								// Если сообщение ответа не установлено
								if(mess.empty())
									// Выполняем установку сообщения по умолчанию
									const_cast <string &> (mess) = options->http.message(code);
								// Формируем ответ на запрос клиента
								awh::web_t::res_t response(2.0f, code, mess);
								// Получаем заголовки ответа удалённому клиенту
								const auto & headers = options->http.process2(http_t::process_t::RESPONSE, response);
								// Если заголовки запроса получены
								if(!headers.empty()){
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										{
											// Выводим заголовок ответа
											cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
											// Получаем бинарные данные REST-ответа
											const auto & buffer = options->http.process(http_t::process_t::RESPONSE, response);
											// Если бинарные данные ответа получены
											if(!buffer.empty())
												// Выводим параметры ответа
												cout << string(buffer.begin(), buffer.end()) << endl << endl;
										}
									#endif
									// Флаг отправляемого фрейма
									awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
									// Если тело запроса не существует
									if(end && (code >= 200))
										// Устанавливаем флаг завершения потока
										flag = awh::http2_t::flag_t::END_STREAM;
									// Выполняем заголовки запроса на сервер
									return web2_t::send(id, bid, headers, flag);
								}
							} break;
						}
					}
				} break;
			}
		}
	}
	// Выводим значение по умолчанию
	return -1;
}
/**
 * send Метод отправки данных в бинарном виде клиенту
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 */
void awh::server::Http2::send(const uint64_t bid, const char * buffer, const size_t size) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0))
		// Выполняем отправку заголовков ответа клиенту
		const_cast <server::core_t *> (this->_core)->write(buffer, size, bid);
}
/**
 * send Метод отправки сообщения брокеру
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Http2::send(const uint64_t bid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выполняем передачу запроса на сервер HTTP/1.1
								this->_http1.send(bid, code, mess, entity, headers);
							break;
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Выполняем сброс состояния HTTP парсера
								options->http.reset();
								// Выполняем очистку данных тела
								options->http.clear(http_t::suite_t::BODY);
								// Выполняем очистку заголовков
								options->http.clear(http_t::suite_t::HEADER);
								// Устанавливаем полезную нагрузку
								options->http.body(entity);
								// Устанавливаем заголовки ответа
								options->http.headers(headers);
								// Если сообщение ответа не установлено
								if(mess.empty())
									// Выполняем установку сообщения по умолчанию
									const_cast <string &> (mess) = options->http.message(code);
								{
									// Формируем ответ на запрос клиента
									awh::web_t::res_t response(2.0f, code, mess);
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
												// Получаем бинарные данные REST-ответа
												const auto & buffer = options->http.process(http_t::process_t::RESPONSE, response);
												// Если бинарные данные ответа получены
												if(!buffer.empty())
													// Выводим параметры ответа
													cout << string(buffer.begin(), buffer.end()) << endl << endl;
											}
										#endif
										// Флаг отправляемого фрейма
										awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
										// Если тело запроса не существует
										if((code >= 200) && options->http.body().empty())
											// Устанавливаем флаг завершения потока
											flag = awh::http2_t::flag_t::END_STREAM;
										// Выполняем ответ подключившемуся клиенту
										int32_t sid = web2_t::send(options->sid, bid, headers, flag);
										// Если запрос не получилось отправить, выходим из функции
										if(sid < 0)
											// Выходим из функции
											return;
										// Устанавливаем идентификатор потока
										options->sid = (options->sid > -1 ? options->sid : sid);
										// Если тело запроса существует
										if((code >= 200) && !options->http.body().empty()){
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
												if((code >= 200) && options->http.body().empty() && (options->http.trailers() == 0))
													// Устанавливаем флаг завершения потока
													flag = awh::http2_t::flag_t::END_STREAM;
												// Выполняем отправку тела запроса на сервер
												if(!web2_t::send(options->sid, bid, entity.data(), entity.size(), flag))
													// Выходим из функции
													return;
											}
											// Если список трейлеров установлен
											if(options->http.trailers() > 0){
												// Выполняем извлечение трейлеров
												const auto & trailers = options->http.trailers2();
												/**
												 * Если включён режим отладки
												 */
												#if defined(DEBUG_MODE)
													// Название выводимого заголовка
													string name = "";
													// Выводим заголовок трейлеров
													cout << "<Trailers>" << endl << endl;
													// Выполняем перебор всего списка отправляемых трейлеров
													for(auto & trailer : trailers){
														// Получаем название заголовка
														name = trailer.first;
														// Переводим заголовок в нормальный режим
														this->_fmk->transform(name, fmk_t::transform_t::SMART);
														// Выводим сообщение о выводе чанка тела
														cout << this->_fmk->format("%s: %s", name.c_str(), trailer.second.c_str()) << endl;
													}
													// Выводим завершение вывода информации
													cout << endl << endl;
												#endif
												// Выполняем отправку трейлеров
												if(!web2_t::send(options->sid, bid, trailers))
													// Выходим из функции
													return;
											}
										}
									}
								}
							} break;
						}
					}
				} break;
			}
		}
	}
}
/**
 * shutdown2 Метод HTTP/2 отправки клиенту сообщения корректного завершения
 * @param bid идентификатор брокера
 * @return    результат выполнения операции
 */
bool awh::server::Http2::shutdown2(const uint64_t bid) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем тправки клиенту сообщения корректного завершения
							return web2_t::shutdown(bid);
					}
				}
			}
		}
	}
	// Выводим значение по умолчанию
	return false;
}
/**
 * reject2 Метод HTTP/2 выполнения сброса подключения
 * @param id    идентификатор потока
 * @param bid   идентификатор брокера
 * @param error код отправляемой ошибки
 * @return      результат отправки сообщения
 */
bool awh::server::Http2::reject2(const int32_t id, const uint64_t bid, const awh::http2_t::error_t error) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем сброс подключения
							return web2_t::reject(id, bid, error);
					}
				}
			}
		}
	}
	// Выводим значение по умолчанию
	return false;
}
/**
 * goaway2 Метод HTTP/2 отправки сообщения закрытия всех потоков
 * @param last   идентификатор последнего потока
 * @param bid    идентификатор брокера
 * @param error  код отправляемой ошибки
 * @param buffer буфер отправляемых данных если требуется
 * @param size   размер отправляемого буфера данных
 * @return       результат отправки данных фрейма
 */
bool awh::server::Http2::goaway2(const int32_t last, const uint64_t bid, const awh::http2_t::error_t error, const uint8_t * buffer, const size_t size) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем отправку сообщения закрытия всех потоков
							return web2_t::goaway(last, bid, error, buffer, size);
					}
				}
			}
		}
	}
	// Выводим значение по умолчанию
	return false;
}
/**
 * send2 Метод HTTP/2 отправки трейлеров
 * @param id      идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @return        результат отправки данных указанному клиенту
 */
bool awh::server::Http2::send2(const int32_t id, const uint64_t bid, const vector <pair <string, string>> & headers) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем отправку трейлеров
							return web2_t::send(id, bid, headers);
					}
				}
			}
		}
	}
	// Выводим значение по умолчанию
	return false;
}
/**
 * send2 Метод HTTP/2 отправки сообщения клиенту
 * @param id     идентификатор потока
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых
 * @param size   размер сообщения в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::Http2::send2(const int32_t id, const uint64_t bid, const char * buffer, const size_t size, const awh::http2_t::flag_t flag) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем отправку сообщения клиенту
							return web2_t::send(id, bid, buffer, size, flag);
					}
				}
			}
		}
	}
	// Выводим значение по умолчанию
	return false;
}
/**
 * send2 Метод HTTP/2 отправки заголовков
 * @param id      идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::server::Http2::send2(const int32_t id, const uint64_t bid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем отправку заголовков
							return web2_t::send(id, bid, headers, flag);
					}
				}
			}
		}
	}
	// Выводим значение по умолчанию
	return -1;
}
/**
 * push2 Метод HTTP/2 отправки push-уведомлений
 * @param id      идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::server::Http2::push2(const int32_t id, const uint64_t bid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP): {
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Выполняем инициализацию объекта HTTP-парсера
									http_t http(this->_fmk, this->_log);
									// Устанавливаем заголовки запроса
									http.headers2(headers);
									// Выводим заголовок запроса
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ PUSH ^^^^^^^^^\x1B[0m" << endl;
									// Получаем бинарные данные HTTP-запроса
									const auto & buffer = http.process(http_t::process_t::REQUEST, http.request());
									// Выводим параметры запроса
									cout << string(buffer.begin(), buffer.end()) << endl << endl;
								}
							#endif
							// Выполняем отправку push-уведомлений
							return web2_t::push(id, bid, headers, flag);
						}
					}
				}
			}
		}
	}
	// Выводим значение по умолчанию
	return -1;
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const uint64_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для извлечения пароля
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <string (const uint64_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <bool (const uint64_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации брокера на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const uint64_t, const vector <char> &, const bool)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода бинарных данных в сыром виде полученных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <bool (const uint64_t, const char *, const size_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функция обратного вызова при выполнении рукопожатия
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const agent_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const direct_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода запроса клиента к серверу
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Http2::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-сервера
	this->_ws2.on(callback);
	// Выполняем установку функции обратного вызова для HTTP-сервера
	this->_http1.on(callback);
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
u_int awh::server::Http2::port(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.port(bid);
}
/**
 * agent Метод извлечения агента клиента
 * @param bid идентификатор брокера
 * @return    агент к которому относится подключённый клиент
 */
awh::server::web_t::agent_t awh::server::Http2::agent(const uint64_t bid) const noexcept {
	// Получаем параметры активного клиента
	scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Определяем протокола подключения
		switch(static_cast <uint8_t> (options->proto)){
			// Если протокол подключения соответствует HTTP/1.1
			case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_http1._agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_http1._agents.end())
					// Выводим идентификатор агента
					return it->second;
			} break;
			// Если протокол подключения соответствует HTTP/2
			case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end())
					// Выводим идентификатор агента
					return it->second;
			} break;
		}
	}
	// Выводим сообщение, что ничего не найдено
	return agent_t::HTTP;
}
/**
 * ip Метод получения IP-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::Http2::ip(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::Http2::mac(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.mac(bid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Http2::stop() noexcept {
	// Выполняем остановку работы основного модуля
	web2_t::stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::Http2::start() noexcept {
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
void awh::server::Http2::close(const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены, устанавливаем флаг закрытия подключения
	if((this->_core != nullptr) && (options != nullptr)){
		// Определяем протокола подключения
		switch(static_cast <uint8_t> (options->proto)){
			// Если протокол подключения соответствует HTTP/1.1
			case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_http1._agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_http1._agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
						// Если протокол соответствует протоколу WebSocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выполняем закрытие подключения клиента HTTP/1.1
							this->_http1.close(bid);
						break;
					}
				}
			} break;
			// Если протокол подключения соответствует HTTP/2
			case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP): {
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
						} break;
						// Если протокол соответствует протоколу WebSocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET): {
							// Выполняем закрытие подключения клиента WebSocket
							this->_ws2.close(bid);
							// Выполняем поиск брокера в списке активных сессий
							auto it = this->_ws2._sessions.find(bid);
							// Если активная сессия найдена
							if(it != this->_ws2._sessions.end())
								// Выполняем закрытие подключения
								(* it->second.get()) = nullptr;
						} break;
					}
				}
			} break;
		}
	}
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::server::Http2::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку списка поддерживаемого сабпротокола для WebSocket-модуля
	this->_ws2.subprotocol(subprotocol);
	// Выполняем установку списка поддерживаемого сабпротокола для HTTP-модуля
	this->_http1.subprotocol(subprotocol);
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::server::Http2::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку списка поддерживаемых сабпротоколов для WebSocket-модуля
	this->_ws2.subprotocols(subprotocols);
	// Выполняем установку списка поддерживаемых сабпротоколов для HTTP-модуля
	this->_http1.subprotocols(subprotocols);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @param bid идентификатор брокера
 * @return    список выбранных сабпротоколов
 */
const set <string> & awh::server::Http2::subprotocols(const uint64_t bid) const noexcept {
	// Результат работы функции
	static const set <string> result;
	// Получаем параметры активного клиента
	scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Определяем протокола подключения
		switch(static_cast <uint8_t> (options->proto)){
			// Если протокол подключения соответствует HTTP/1.1
			case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_http1._agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_http1._agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует протоколу WebSocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выводим согласованный сабпротокол
							return this->_http1.subprotocols(bid);
					}
				}
			} break;
			// Если протокол подключения соответствует HTTP/2
			case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует протоколу WebSocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выводим согласованный сабпротокол
							return this->_ws2.subprotocols(bid);
					}
				}
			} break;
		}
	}
	// Выводим результат по умолчанию
	return result;
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::server::Http2::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка поддерживаемых расширений для WebSocket-модуля
	this->_ws2.extensions(extensions);
	// Выполняем установку списка поддерживаемых расширений для HTTP-модуля
	this->_http1.extensions(extensions);
}
/**
 * extensions Метод извлечения списка расширений
 * @param bid идентификатор брокера
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::Http2::extensions(const uint64_t bid) const noexcept {
	// Результат работы функции
	static const vector <vector <string>> result;
	// Получаем параметры активного клиента
	scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Определяем протокола подключения
		switch(static_cast <uint8_t> (options->proto)){
			// Если протокол подключения соответствует HTTP/1.1
			case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_http1._agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_http1._agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует протоколу WebSocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выполняем извлечение списка поддерживаемых расширений WebSocket
							return this->_http1.extensions(bid);
					}
				}
			} break;
			// Если протокол подключения соответствует HTTP/2
			case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
				// Выполняем поиск агента которому соответствует клиент
				auto it = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(it != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (it->second)){
						// Если протокол соответствует протоколу WebSocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выполняем извлечение списка поддерживаемых расширений WebSocket
							return this->_ws2.extensions(bid);
					}
				}
			} break;
		}
	}
	// Выводим результат по умолчанию
	return result;
}
/**
 * multiThreads Метод активации многопоточности
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::server::Http2::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Если многопоточность ещё не активированна
		if(!this->_ws2._thr.is())
			// Выполняем инициализацию пула потоков
			this->_ws2._thr.init(count);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->_ws2._thr.wait();
			// Выполняем инициализацию нового тредпула
			this->_ws2._thr.init(count);
		}
		// Если сетевое ядро установлено
		if(this->_core != nullptr)
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->_ws2._thr.wait();
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Http2::total(const u_short total) noexcept {
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr)
		// Устанавливаем максимальное количество одновременных подключений
		const_cast <server::core_t *> (this->_core)->total(this->_scheme.sid, total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::Http2::segmentSize(const size_t size) noexcept {
	// Выполняем установку размеров сегментов фрейма для WebSocket-сервера
	this->_ws2.segmentSize(size);
	// Выполняем установку размеров сегментов фрейма для HTTP-сервера
	this->_http1.segmentSize(size);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::Http2::clusterAutoRestart(const bool mode) noexcept {
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
void awh::server::Http2::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
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
void awh::server::Http2::compressors(const vector <http_t::compress_t> & compressors) noexcept {
	// Устанавливаем список компрессоров
	this->_scheme.compressors = compressors;
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::server::Http2::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для WebSocket-сервера
	this->_ws2.mode(flags);
	// Устанавливаем флаги настроек модуля для HTTP-сервера
	this->_http1.mode(flags);
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = (flags.count(flag_t::NOT_STOP) == 0);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.count(flag_t::ALIVE) > 0);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг разрешающий выполнять подключение к протоколу WebSocket
	this->_webSocket = (flags.count(flag_t::WEBSOCKET_ENABLE) > 0);
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
void awh::server::Http2::alive(const bool mode) noexcept {
	// Выполняем установку долгоживущего подключения
	web2_t::alive(mode);
	// Выполняем установку долгоживущего подключения для WebSocket-сервера
	this->_ws2.alive(mode);
	// Выполняем установку долгоживущего подключения для HTTP-сервера
	this->_http1.alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Http2::alive(const time_t time) noexcept {
	// Выполняем установку времени жизни подключения
	web2_t::alive(time);
	// Выполняем установку времени жизни подключения для WebSocket-сервера
	this->_ws2.alive(time);
	// Выполняем установку времени жизни подключения для HTTP-сервера
	this->_http1.alive(time);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param bid  идентификатор брокера
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Http2::alive(const uint64_t bid, const bool mode) noexcept {
	// Получаем параметры активного клиента
	scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr)
		// Устанавливаем флаг пдолгоживущего подключения
		options->alive = mode;
	// Выполняем установку долгоживущего подключения для HTTP-сервера
	this->_http1.alive(bid, mode);
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::server::Http2::core(const server::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
		// Добавляем схемы сети в сетевое ядро
		const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
		// Устанавливаем функцию активации ядра сервера
		const_cast <server::core_t *> (this->_core)->on(std::bind(&http2_t::eventsCallback, this, _1, _2));
		// Если многопоточность активированна
		if(this->_ws2._thr.is() || this->_ws2._ws1._thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_ws2._thr.is() || this->_ws2._ws1._thr.is()){
			// Выполняем завершение всех активных потоков
			this->_ws2._thr.wait();
			// Выполняем завершение всех активных потоков
			this->_ws2._ws1._thr.wait();
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
 * identity Метод установки идентичности протокола модуля
 * @param identity идентичность протокола модуля
 */
void awh::server::Http2::identity(const http_t::identity_t identity) noexcept {
	// Устанавливаем флаг идентичности протокола модуля
	this->_identity = identity;
	// Устанавливаем флаг идентичности протокола модуля для модуля HTTP/1.1
	this->_http1.identity(identity);
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Http2::waitTimeDetect(const time_t read, const time_t write) noexcept {
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
void awh::server::Http2::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
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
void awh::server::Http2::realm(const string & realm) noexcept {
	// Устанавливаем название сервера
	web2_t::realm(realm);
	// Устанавливаем название сервера для WebSocket-сервера
	this->_ws2.realm(realm);
	// Устанавливаем название сервера для HTTP-сервера
	this->_http1.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Http2::opaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	web2_t::opaque(opaque);
	// Устанавливаем временный ключ сессии сервера для WebSocket-сервера
	this->_ws2.opaque(opaque);
	// Устанавливаем временный ключ сессии сервера для HTTP-сервера
	this->_http1.opaque(opaque);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::Http2::chunk(const size_t size) noexcept {
	// Устанавливаем размера чанка
	web2_t::chunk(size);
	// Устанавливаем размера чанка для WebSocket-сервера
	this->_ws2.chunk(size);
	// Устанавливаем размера чанка для HTTP-сервера
	this->_http1.chunk(size);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Http2::maxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	web2_t::maxRequests(max);
	// Устанавливаем максимальное количество запросов для WebSocket-сервера
	this->_ws2.maxRequests(max);
	// Устанавливаем максимальное количество запросов для HTTP-сервера
	this->_http1.maxRequests(max);
}
/**
 * ident Метод установки идентификации сервера
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Http2::ident(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификацию сервера
	web2_t::ident(id, name, ver);
	// Устанавливаем идентификацию сервера для WebSocket-сервера
	this->_ws2.ident(id, name, ver);
	// Устанавливаем идентификацию сервера для HTTP-сервера
	this->_http1.ident(id, name, ver);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Http2::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	web2_t::authType(type, hash);
	// Устанавливаем тип авторизации для WebSocket-сервера
	this->_ws2.authType(type, hash);
	// Устанавливаем тип авторизации для HTTP-сервера
	this->_http1.authType(type, hash);
}
/**
 * crypted Метод получения флага шифрования
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::server::Http2::crypted(const uint64_t bid) const noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выводим установленный флаг шифрования
								return this->_http1.crypted(bid);
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выводим установленный флаг шифрования
								return options->crypted;
							// Если протокол соответствует протоколу WebSocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выводим установленный флаг шифрования
								return this->_ws2.crypted(bid);
						}
					}
				} break;
			}
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
void awh::server::Http2::encrypt(const uint64_t bid, const bool mode) noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Устанавливаем флаг шифрования для клиента
								this->_http1.encrypt(bid, mode);
							break;
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto it = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(it != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (it->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Устанавливаем флаг шифрования для клиента
								options->crypted = mode;
								// Устанавливаем флаг шифрования
								options->http.encryption(options->crypted);
							} break;
							// Если протокол соответствует протоколу WebSocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Устанавливаем флаг шифрования для клиента
								this->_ws2.encrypt(bid, mode);
							break;
						}
					}
				} break;
			}
		}
	}
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::server::Http2::encryption(const bool mode) noexcept {
	// Устанавливаем флага шифрования
	web2_t::encryption(mode);
	// Устанавливаем флага шифрования для WebSocket-сервера
	this->_ws2.encryption(mode);
	// Устанавливаем флага шифрования для HTTP-сервера
	this->_http1.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Http2::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования
	web2_t::encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для WebSocket-сервера
	this->_ws2.encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для HTTP-сервера
	this->_http1.encryption(pass, salt, cipher);
}
/**
 * Http2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Http2::Http2(const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(fmk, log), _webSocket(false), _identity(http_t::identity_t::HTTP), _ws2(fmk, log), _http1(fmk, log), _scheme(fmk, log) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&http2_t::openCallback, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&http2_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&http2_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&http2_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&http2_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта брокера
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&http2_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
/**
 * Http2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Http2::Http2(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(core, fmk, log), _webSocket(false), _identity(http_t::identity_t::HTTP), _ws2(fmk, log), _http1(fmk, log), _scheme(fmk, log) {
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&http2_t::openCallback, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&http2_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&http2_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&http2_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&http2_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта брокера
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&http2_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
/**
 * ~Http2 Деструктор
 */
awh::server::Http2::~Http2() noexcept {
	// Снимаем адрес сетевого ядра
	this->_ws2._core = nullptr;
}
