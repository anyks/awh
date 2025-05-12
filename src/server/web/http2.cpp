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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <server/web/http2.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * connectEvents Метод обратного вызова при подключении к серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::server::Http2::connectEvents(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
		// Создаём брокера
		this->_scheme.set(bid);
		// Выполняем активацию HTTP/2 протокола
		web2_t::connectEvents(bid, sid);
		// Выполняем проверку инициализирован ли протокол HTTP/2 для текущего клиента
		auto i = this->_sessions.find(bid);
		// Если протокол HTTP/2 для клиента не инициализирован
		if(i == this->_sessions.end()){
			// Выполняем установку сетевого ядра
			this->_http1._core = this->_core;
			// Устанавливаем метод компрессии поддерживаемый сервером
			this->_http1._scheme.compressors = this->_scheme.compressors;
			// Выполняем переброс вызова коннекта на клиент Websocket
			this->_http1.connectEvents(bid, sid);
		}
		// Выполняем добавление агнета
		this->_agents.emplace(bid, agent_t::HTTP);
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
void awh::server::Http2::disconnectEvents(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
		// Выполняем поиск брокера в списке активных сессий
		auto i = this->_sessions.find(bid);
		// Если активная сессия найдена
		if(i != this->_sessions.end()){
			// Выполняем закрытие подключения
			i->second->close();
			// Выполняем поиск брокера в списке активных сессий
			auto j = this->_ws2._sessions.find(bid);
			// Если активная сессия найдена
			if(j != this->_ws2._sessions.end())
				// Выполняем удаление сессии подключения
				this->_ws2._sessions.erase(j);
		}
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
void awh::server::Http2::readEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
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
			scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Если подключение закрыто
				if(options->close){
					// Принудительно выполняем отключение лкиента
					const_cast <server::core_t *> (this->_core)->close(bid);
					// Выходим из функции
					return;
				}
				// Выполняем установку протокола подключения
				options->proto = this->_core->proto(bid);
				// Определяем протокола подключения
				switch(static_cast <uint8_t> (options->proto)){
					// Если протокол подключения соответствует HTTP/1.1
					case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
						// Выполняем поиск агента которому соответствует клиент
						auto i = this->_http1._agents.find(bid);
						// Если активный агент клиента установлен
						if(i != this->_http1._agents.end()){
							// Определяем тип активного протокола
							switch(static_cast <uint8_t> (i->second)){
								// Если протокол соответствует HTTP-протоколу
								case static_cast <uint8_t> (agent_t::HTTP):
								// Если протокол соответствует протоколу Websocket
								case static_cast <uint8_t> (agent_t::WEBSOCKET):
									// Выполняем переброс вызова чтения клиенту HTTP
									this->_http1.readEvents(buffer, size, bid, sid);
								break;
							}
						}
					} break;
					// Если протокол подключения соответствует HTTP/2
					case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
						// Выполняем поиск агента которому соответствует клиент
						auto i = this->_agents.find(bid);
						// Если активный агент клиента установлен
						if(i != this->_agents.end()){
							// Определяем тип активного протокола
							switch(static_cast <uint8_t> (i->second)){
								// Если протокол соответствует HTTP-протоколу
								case static_cast <uint8_t> (agent_t::HTTP): {
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
								} break;
								// Если протокол соответствует протоколу Websocket
								case static_cast <uint8_t> (agent_t::WEBSOCKET):
									// Выполняем переброс вызова чтения клиенту Websocket
									this->_ws2.readEvents(buffer, size, bid, sid);
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
 * writeEvents Метод обратного вызова при записи сообщение брокеру
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::server::Http2::writeEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
							// Если протокол соответствует протоколу Websocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем переброс вызова записи клиенту HTTP
								this->_http1.writeEvents(buffer, size, bid, sid);
							break;
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
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
							// Если протокол соответствует протоколу Websocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем переброс вызова записи клиенту Websocket
								this->_ws2.writeEvents(buffer, size, bid, sid);
							break;
						}
					}
				} break;
			}
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
void awh::server::Http2::callbacksEvents(const fn_t::event_t event, const uint64_t idw, const string & name, const fn_t::dump_t * dump) noexcept {
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
				if(!callbacks.empty()){
					// Выполняем установку функций обратного вызова для Websocket-сервера
					this->_ws2.callbacks(callbacks);
					// Выполняем установку функций обратного вызова для HTTP-сервера
					this->_http1.callbacks(callbacks);
				}
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
int32_t awh::server::Http2::beginSignal(const int32_t sid, const uint64_t bid) noexcept {
	// Выполняем открытие потока
	this->_scheme.openStream(sid, bid);
	// Извлекаем данные потока
	scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
	// Если поток получен удачно
	if(stream != nullptr){
		// Выполняем очистку HTTP-парсера
		stream->http.clear();
		// Выполняем сброс состояния HTTP-парсера
		stream->http.reset();
		// Выполняем установку идентификатора объекта
		stream->http.id(bid);
		// Устанавливаем размер чанка
		stream->http.chunk(this->_chunkSize);
		// Устанавливаем флаг идентичности протокола
		stream->http.identity(this->_identity);
		// Устанавливаем список компрессоров поддерживаемый сервером
		stream->http.compressors(this->_scheme.compressors);
		// Устанавливаем данные сервиса
		stream->http.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
		// Если функция обратного вызова на на вывод ошибок установлена
		if(this->_callbacks.is("error"))
			// Устанавливаем функцию обратного вызова для вывода ошибок
			stream->http.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_callbacks.get <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error"));
		// Если требуется установить параметры шифрования
		if(this->_encryption.mode){
			// Устанавливаем флаг шифрования
			stream->http.encryption(this->_encryption.mode);
			// Устанавливаем параметры шифрования
			stream->http.encryption(this->_encryption.pass, this->_encryption.salt, this->_encryption.cipher);
		}
		// Определяем тип авторизации
		switch(static_cast <uint8_t> (this->_service.type)){
			// Если тип авторизации Basic
			case static_cast <uint8_t> (auth_t::type_t::BASIC): {
				// Устанавливаем параметры авторизации
				stream->http.authType(this->_service.type);
				// Если функция обратного вызова для обработки чанков установлена
				if(this->_callbacks.is("checkPassword"))
					// Устанавливаем функцию проверки авторизации
					stream->http.authCallback(std::bind(this->_callbacks.get <bool (const uint64_t, const string &, const string &)> ("checkPassword"), bid, _1, _2));
			} break;
			// Если тип авторизации Digest
			case static_cast <uint8_t> (auth_t::type_t::DIGEST): {
				// Устанавливаем название сервера
				stream->http.realm(this->_service.realm);
				// Устанавливаем временный ключ сессии сервера
				stream->http.opaque(this->_service.opaque);
				// Устанавливаем параметры авторизации
				stream->http.authType(this->_service.type, this->_service.hash);
				// Если функция обратного вызова для обработки чанков установлена
				if(this->_callbacks.is("extractPassword"))
					// Устанавливаем функцию извлечения пароля
					stream->http.extractPassCallback(std::bind(this->_callbacks.get <string (const uint64_t, const string &)> ("extractPassword"), bid, _1));
			} break;
		}
	// Если поток не создан, выполняем закрытие подключения
	} else web2_t::close(bid);
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
int32_t awh::server::Http2::closedSignal(const int32_t sid, const uint64_t bid, const awh::http2_t::error_t error) noexcept {
	// Выполняем закрытие потока
	this->_scheme.closeStream(sid, bid);
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
int32_t awh::server::Http2::headerSignal(const int32_t sid, const uint64_t bid, const string & key, const string & val) noexcept {
	// Извлекаем данные потока
	scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
	// Если поток получен удачно
	if(stream != nullptr)
		// Устанавливаем полученные заголовки
		stream->http.header2(key, val);
	// Если функция обратного вызова на полученного заголовка с сервера установлена
	if(this->_callbacks.is("header"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const uint64_t, const string &, const string &)> ("header", sid, bid, key, val);
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
int32_t awh::server::Http2::chunkSignal(const int32_t sid, const uint64_t bid, const uint8_t * buffer, const size_t size) noexcept {
	// Получаем параметры активного клиента
	scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Если функция обратного вызова на перехват входящих чанков установлена
		if(this->_callbacks.is("chunking")){
			// Извлекаем данные потока
			scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
			// Если поток получен удачно
			if(stream != nullptr)
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", bid, vector <char> (buffer, buffer + size), &stream->http);
		// Если функция перехвата полученных чанков не установлена
		} else if(this->_core != nullptr) {
			// Если подключение закрыто
			if(options->close){
				// Принудительно выполняем отключение лкиента
				const_cast <server::core_t *> (this->_core)->close(bid);
				// Выходим из функции
				return 0;
			}
			// Извлекаем данные потока
			scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
			// Если поток получен удачно
			if(stream != nullptr){
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP): {
							// Добавляем полученный чанк в тело данных
							stream->http.payload(vector <char> (buffer, buffer + size));
							// Обновляем время отправленного пинга
							options->sendPing = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
						} break;
						// Если протокол соответствует протоколу Websocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выполняем передачу сигнала полученных чанков в модуль Websocket
							this->_ws2.chunkSignal(sid, bid, buffer, size);
						// Выводим результат
						return 0;
					}
				// Добавляем полученный чанк в тело данных
				} else stream->http.body(vector <char> (buffer, buffer + size));
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
int32_t awh::server::Http2::frameSignal(const int32_t sid, const uint64_t bid, const awh::http2_t::direct_t direct, const awh::http2_t::frame_t frame, const set <awh::http2_t::flag_t> & flags) noexcept {
	// Определяем направление передачи фрейма
	switch(static_cast <uint8_t> (direct)){
		// Если производится передача фрейма на сервер
		case static_cast <uint8_t> (awh::http2_t::direct_t::SEND): {
			// Если мы получили флаг завершения потока
			if(flags.find(awh::http2_t::flag_t::END_STREAM) != flags.end()){
				// Получаем параметры активного клиента
				scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
				// Если параметры активного клиента получены
				if((this->_core != nullptr) && (options != nullptr)){
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
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
							} break;
							// Если протокол соответствует протоколу Websocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET): {
								// Выполняем передачу фрейма клиенту Websocket
								this->_ws2.frameSignal(sid, bid, direct, frame, flags);
								// Выполняем поиск брокера в списке активных сессий
								auto i = this->_ws2._sessions.find(bid);
								// Если активная сессия найдена
								if(i != this->_ws2._sessions.end()){
									// Если сессия была удалена
									if(!i->second->is())
										// Выполняем копирование контекста сессии HTTP/2
										this->_sessions.at(bid) = i->second;
								}
							} break;
						}
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
		case static_cast <uint8_t> (awh::http2_t::direct_t::RECV): {
			// Получаем параметры активного клиента
			scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if((this->_core != nullptr) && (options != nullptr)){
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP): {
							// Выполняем определение типа фрейма
							switch(static_cast <uint8_t> (frame)){
								// Если мы получили входящие данные тела ответа
								case static_cast <uint8_t> (awh::http2_t::frame_t::DATA): {
									// Если мы получили неустановленный флаг или флаг завершения потока
									if(flags.find(awh::http2_t::flag_t::END_STREAM) != flags.end()){
										// Извлекаем данные потока
										scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
										// Если поток получен удачно
										if(stream != nullptr){
											// Выполняем коммит полученного результата
											stream->http.commit();
											// Выполняем обработку полученных данных
											this->prepare(sid, bid);
											// Если функция обратного вызова активности потока установлена
											if(this->_callbacks.is("stream"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, bid, mode_t::CLOSE);
											// Если установлена функция отлова завершения запроса
											if(this->_callbacks.is("end"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, bid, direct_t::RECV);
										}
									}
								} break;
								// Если мы получили входящие данные заголовков ответа
								case static_cast <uint8_t> (awh::http2_t::frame_t::HEADERS): {
									// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
									if(flags.find(awh::http2_t::flag_t::END_HEADERS) != flags.end()){
										// Извлекаем данные потока
										scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
										// Если поток получен удачно
										if(stream != nullptr){
											// Выполняем коммит полученного результата
											stream->http.commit();
											// Выполняем извлечение параметров запроса
											const auto & request = stream->http.request();
											// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
											if(this->_callbacks.is("request"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request", sid, bid, request.method, request.url);
											// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
											if(this->_callbacks.is("headers"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", sid, bid, request.method, request.url, const_cast <scheme::web2_t::stream_t *> (stream)->http.headers());
											// Если мы получили неустановленный флаг или флаг завершения потока
											if(flags.find(awh::http2_t::flag_t::END_STREAM) != flags.end()){
												// Выполняем обработку полученных данных
												this->prepare(sid, bid);
												// Если функция обратного вызова активности потока установлена
												if(this->_callbacks.is("stream"))
													// Выполняем функцию обратного вызова
													this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, bid, mode_t::CLOSE);
												// Если установлена функция отлова завершения запроса
												if(this->_callbacks.is("end"))
													// Выполняем функцию обратного вызова
													this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, bid, direct_t::RECV);
											// Если заголовок Websocket или прокси-сервер активирован
											} else if((stream->http.identity() == http_t::identity_t::WS) ||
											          (stream->http.identity() == http_t::identity_t::PROXY))
												// Выполняем обработку полученных данных
												this->prepare(sid, bid);
										}
									}
								} break;
							}
						} break;
						// Если протокол соответствует протоколу Websocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выполняем передачу фрейма клиенту Websocket
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
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 */
void awh::server::Http2::prepare(const int32_t sid, const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Если подключение не установлено как постоянное
		if(!this->_service.alive && !options->alive){
			// Если количество запросов ограничен
			if(this->_maxRequests > 0)
				// Увеличиваем количество выполненных запросов
				options->requests++;
			// Устанавливаем флаг закрытия подключения
			options->close = ((this->_maxRequests > 0) && (options->requests >= this->_maxRequests));
		// Выполняем сброс количества выполненных запросов
		} else options->requests = 0;
		// Извлекаем данные потока
		scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
		// Если поток получен удачно
		if(stream != nullptr){
			// Получаем флаг шифрованных данных
			stream->crypted = stream->http.crypted();
			// Получаем поддерживаемый метод компрессии
			stream->compressor = stream->http.compression();
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				{
					// Получаем объект работы с HTTP-запросами
					const http_t & http = reinterpret_cast <http_t &> (stream->http);
					// Получаем бинарные данные REST-ответа
					const auto & buffer = http.process(http_t::process_t::REQUEST, stream->http.request());
					// Если параметры ответа получены
					if(!buffer.empty())
						// Выводим параметры ответа
						std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
					// Если тело ответа существует
					if(!stream->http.empty(awh::http_t::suite_t::BODY))
						// Выводим сообщение о выводе чанка тела
						std::cout << this->_fmk->format("<body %u>", stream->http.body().size()) << std::endl << std::endl << std::flush;
					// Иначе устанавливаем перенос строки
					else std::cout << std::endl << std::flush;
				}
			#endif
			// Выполняем проверку авторизации
			switch(static_cast <uint8_t> (stream->http.auth())){
				// Если запрос выполнен удачно
				case static_cast <uint8_t> (http_t::status_t::GOOD): {
					// Если заголовок Websocket активирован
					if(stream->http.identity() == http_t::identity_t::WS){
						// Если запрашиваемый протокол соответствует Websocket
						if(this->_webSocket)
							// Выполняем инициализацию Websocket-сервера
							this->websocket(sid, bid);
						// Если протокол запрещён или не поддерживается
						else {
							// Выполняем очистку HTTP-парсера
							stream->http.clear();
							// Выполняем сброс состояния HTTP-парсера
							stream->http.reset();
							// Формируем ответ на запрос об авторизации
							const awh::web_t::res_t & response = awh::web_t::res_t(2.f, static_cast <uint32_t> (505), "Requested protocol is not supported by this server");
							// Получаем заголовки ответа удалённому клиенту
							const auto & headers = stream->http.reject2(response);
							// Если бинарные данные ответа получены
							if(!headers.empty()){
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									{
										// Выводим заголовок ответа
										std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
										// Получаем объект работы с HTTP-запросами
										const http_t & http = reinterpret_cast <http_t &> (stream->http);
										// Получаем бинарные данные REST-ответа
										const auto & buffer = http.process(http_t::process_t::RESPONSE, response);
										// Если бинарные данные ответа получены
										if(!buffer.empty())
											// Выводим параметры ответа
											std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
									}
								#endif
								// Флаг отправляемого фрейма
								awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
								// Если тело запроса не существует
								if(stream->http.empty(awh::http_t::suite_t::BODY))
									// Устанавливаем флаг завершения потока
									flag = awh::http2_t::flag_t::END_STREAM;
								// Если запрос не получилось отправить
								if(web2_t::send(sid, bid, headers, flag) < 0)
									// Выходим из функции
									return;
								// Если тело запроса существует
								if(!stream->http.empty(awh::http_t::suite_t::BODY)){
									// Тело HTTP-запроса
									vector <char> entity;
									// Получаем данные тела запроса
									while(!(entity = stream->http.payload()).empty()){
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											// Выводим сообщение о выводе чанка тела
											std::cout << this->_fmk->format("<chunk %zu>", entity.size()) << std::endl << std::endl << std::flush;
										#endif
										// Если нужно установить флаг закрытия потока
										if(stream->http.empty(awh::http_t::suite_t::BODY) && (stream->http.trailers() == 0))
											// Устанавливаем флаг завершения потока
											flag = awh::http2_t::flag_t::END_STREAM;
										// Выполняем отправку тела запроса на сервер
										if(!web2_t::send(sid, bid, entity.data(), entity.size(), flag))
											// Выходим из функции
											return;
									}
									// Если список трейлеров установлен
									if(stream->http.trailers() > 0){
										// Выполняем извлечение трейлеров
										const auto & trailers = stream->http.trailers2();
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											// Название выводимого заголовка
											string name = "";
											// Выводим заголовок трейлеров
											std::cout << "<Trailers>" << std::endl << std::endl << std::flush;
											// Выполняем перебор всего списка отправляемых трейлеров
											for(auto & trailer : trailers){
												// Получаем название заголовка
												name = trailer.first;
												// Переводим заголовок в нормальный режим
												this->_fmk->transform(name, fmk_t::transform_t::SMART);
												// Выводим сообщение о выводе чанка тела
												std::cout << this->_fmk->format("%s: %s", name.c_str(), trailer.second.c_str()) << std::endl << std::flush;
											}
											// Выводим завершение вывода информации
											std::cout << std::endl << std::endl << std::flush;
										#endif
										// Выполняем отправку трейлеров
										if(!web2_t::send(sid, bid, trailers))
											// Выходим из функции
											return;
									}
								}
							// Если сообщение о закрытии подключения не отправлено
							} else if(!web2_t::reject(sid, bid, awh::http2_t::error_t::PROTOCOL_ERROR))
								// Выполняем отключение брокера
								const_cast <server::core_t *> (this->_core)->close(bid);
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callbacks.is("error"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "Requested protocol is not supported by this server");
						}
						// Завершаем обработку
						return;
					}
					// Выполняем извлечение параметров запроса
					const auto & request = stream->http.request();
					// Если функция обратного вызова на получение удачного запроса установлена
					if(this->_callbacks.is("handshake"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const agent_t)> ("handshake", sid, bid, agent_t::HTTP);
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(!stream->http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", sid, bid, request.method, request.url, stream->http.body());
					// Если функция обратного вызова на вывод полученных данных запроса клиента установлена
					if(this->_callbacks.is("complete"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", sid, bid, request.method, request.url, stream->http.body(), stream->http.headers());
				} break;
				// Если запрос неудачный
				case static_cast <uint8_t> (http_t::status_t::FAULT): {
					// Выполняем очистку HTTP-парсера
					stream->http.clear();
					// Выполняем сброс состояния HTTP-парсера
					stream->http.reset();
					// Ответ на запрос об авторизации
					awh::web_t::res_t response;
					// Определяем идентичность сервера
					switch(static_cast <uint8_t> (this->_identity)){
						// Если сервер соответствует Websocket-серверу
						case static_cast <uint8_t> (http_t::identity_t::WS):
						// Если сервер соответствует HTTP-серверу
						case static_cast <uint8_t> (http_t::identity_t::HTTP):
							// Формируем ответ на запрос об авторизации
							response = awh::web_t::res_t(2.f, static_cast <uint32_t> (401));
						break;
						// Если сервер соответствует PROXY-серверу
						case static_cast <uint8_t> (http_t::identity_t::PROXY):
							// Формируем ответ на запрос об авторизации
							response = awh::web_t::res_t(2.f, static_cast <uint32_t> (407));
						break;
					}
					// Получаем заголовки ответа удалённому клиенту
					const auto & headers = stream->http.reject2(response);
					// Если бинарные данные ответа получены
					if(!headers.empty()){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							{
								// Выводим заголовок ответа
								std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
								// Получаем объект работы с HTTP-запросами
								const http_t & http = reinterpret_cast <http_t &> (stream->http);
								// Получаем бинарные данные REST-ответа
								const auto & buffer = http.process(http_t::process_t::RESPONSE, response);
								// Если бинарные данные ответа получены
								if(!buffer.empty())
									// Выводим параметры ответа
									std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
							}
						#endif
						// Флаг отправляемого фрейма
						awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
						// Если тело запроса не существует
						if(stream->http.empty(awh::http_t::suite_t::BODY))
							// Устанавливаем флаг завершения потока
							flag = awh::http2_t::flag_t::END_STREAM;
						// Если запрос не получилось отправить
						if(web2_t::send(sid, bid, headers, flag) < 0)
							// Выходим из функции
							return;
						// Если тело запроса существует
						if(!stream->http.empty(awh::http_t::suite_t::BODY)){
							// Тело HTTP-запроса
							vector <char> entity;
							// Получаем данные тела запроса
							while(!(entity = stream->http.payload()).empty()){
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Выводим сообщение о выводе чанка тела
									std::cout << this->_fmk->format("<chunk %zu>", entity.size()) << std::endl << std::endl << std::flush;
								#endif
								// Если нужно установить флаг закрытия потока
								if(stream->http.empty(awh::http_t::suite_t::BODY) && (stream->http.trailers() == 0))
									// Устанавливаем флаг завершения потока
									flag = awh::http2_t::flag_t::END_STREAM;
								// Выполняем отправку тела запроса на сервер
								if(!web2_t::send(sid, bid, entity.data(), entity.size(), flag))
									// Выходим из функции
									return;
							}
							// Если список трейлеров установлен
							if(stream->http.trailers() > 0){
								// Выполняем извлечение трейлеров
								const auto & trailers = stream->http.trailers2();
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Название выводимого заголовка
									string name = "";
									// Выводим заголовок трейлеров
									std::cout << "<Trailers>" << std::endl << std::endl << std::flush;
									// Выполняем перебор всего списка отправляемых трейлеров
									for(auto & trailer : trailers){
										// Получаем название заголовка
										name = trailer.first;
										// Переводим заголовок в нормальный режим
										this->_fmk->transform(name, fmk_t::transform_t::SMART);
										// Выводим сообщение о выводе чанка тела
										std::cout << this->_fmk->format("%s: %s", name.c_str(), trailer.second.c_str()) << std::endl << std::flush;
									}
									// Выводим завершение вывода информации
									std::cout << std::endl << std::endl << std::flush;
								#endif
								// Выполняем отправку трейлеров
								if(!web2_t::send(sid, bid, trailers))
									// Выходим из функции
									return;
							}
						}
					// Если сообщение о закрытии подключения не отправлено
					} else if(!web2_t::reject(sid, bid, awh::http2_t::error_t::PROTOCOL_ERROR))
						// Выполняем отключение брокера
						const_cast <server::core_t *> (this->_core)->close(bid);
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callbacks.is("error"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "authorization failed");
				}
			}
		}
	}
}
/**
 * websocket Метод инициализации Websocket протокола
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 */
void awh::server::Http2::websocket(const int32_t sid, const uint64_t bid) noexcept {
	// Если данные переданы верные
	if((sid > 0) && (bid > 0) && (this->_core != nullptr)){
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
			options->proto = this->_core->proto(bid);
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
			if(this->_callbacks.is("error"))
				// Устанавливаем функцию обратного вызова для вывода ошибок
				options->http.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", this->_callbacks.get <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error"));
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
			// Извлекаем данные потока
			const scheme::web2_t::stream_t * stream = this->_scheme.getStream(sid, bid);
			// Если поток получен удачно
			if(stream != nullptr){
				// Выполняем установку параметров запроса
				options->http.request(stream->http.request());
				// Выполняем установку полученных заголовков
				options->http.headers(stream->http.headers());
				// Выполняем коммит полученного результата
				options->http.commit();
				// Ответ клиенту по умолчанию успешный
				awh::web_t::res_t response(2.f, static_cast <uint32_t> (200));
				// Если рукопожатие выполнено
				if((options->shake = options->http.handshake(http_t::process_t::REQUEST))){
					// Проверяем версию протокола
					if(!options->http.check(ws_core_t::flag_t::VERSION)){
						// Получаем бинарные данные REST запроса
						response = awh::web_t::res_t(2.f, static_cast <uint32_t> (400), "Unsupported protocol version");
						// Завершаем работу
						goto End;
					}
					// Получаем флаг шифрованных данных
					options->crypted = options->http.crypted();
					// Выполняем очистку HTTP-парсера
					options->http.clear();
					// Если клиент согласился на шифрование данных
					if(this->_encryption.mode){
						// Устанавливаем размер шифрования
						options->cipher = this->_encryption.cipher;
						// Устанавливаем соль шифрования
						options->hash.salt(this->_encryption.salt);
						// Устанавливаем пароль шифрования
						options->hash.password(this->_encryption.pass);
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
								std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
								// Получаем объект работы с HTTP-запросами
								const http_t & http = reinterpret_cast <http_t &> (options->http);
								// Получаем бинарные данные REST-ответа
								const auto & buffer = http.process(http_t::process_t::RESPONSE, response);
								// Если бинарные данные ответа получены
								if(!buffer.empty())
									// Выводим параметры ответа
									std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
							}
						#endif
						// Выполняем замену активного агнета
						this->_agents.at(bid) = agent_t::WEBSOCKET;
						// Выполняем поиск брокера в списке активных сессий
						auto i = this->_sessions.find(bid);
						// Если активная сессия найдена
						if(i != this->_sessions.end()){
							// Выполняем создание нового объекта сессии HTTP/2
							auto ret = this->_ws2._sessions.emplace(bid, make_unique <awh::http2_t> (this->_fmk, this->_log));
							// Выполняем копирование контекста сессии HTTP/2
							ret.first->second = i->second;
						}
						// Выполняем установку сетевого ядра
						this->_ws2._core = this->_core;
						// Выполняем ответ подключившемуся клиенту
						if(web2_t::send(options->sid, bid, headers, awh::http2_t::flag_t::NONE) < 0)
							// Выходим из функции
							return;
						// Выполняем извлечение параметров запроса
						const auto & request = stream->http.request();
						// Если функция обратного вызова активности потока установлена
						if(this->_callbacks.is("stream"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, bid, mode_t::OPEN);
						// Если функция обратного вызова на получение удачного запроса установлена
						if(this->_callbacks.is("handshake"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const int32_t, const uint64_t, const agent_t)> ("handshake", sid, bid, agent_t::WEBSOCKET);
						// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
						if(!stream->http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", sid, bid, request.method, request.url, stream->http.body());
						// Если функция обратного вызова на вывод полученных данных запроса клиента установлена
						if(this->_callbacks.is("complete"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", sid, bid, request.method, request.url, stream->http.body(), stream->http.headers());
						// Завершаем работу
						return;
					// Формируем ответ, что произошла внутренняя ошибка сервера
					} else response = awh::web_t::res_t(2.f, static_cast <uint32_t> (500));
				// Формируем ответ, что страница не доступна
				} else response = awh::web_t::res_t(2.f, static_cast <uint32_t> (403), "Handshake failed");
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
							std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
							// Получаем объект работы с HTTP-запросами
							const http_t & http = reinterpret_cast <http_t &> (options->http);
							// Получаем бинарные данные REST-ответа
							const auto & buffer = http.process(http_t::process_t::RESPONSE, response);
							// Если бинарные данные ответа получены
							if(!buffer.empty())
								// Выводим параметры ответа
								std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
						}
					#endif
					// Флаг отправляемого фрейма
					awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
					// Если тело запроса не существует
					if(options->http.empty(awh::http_t::suite_t::BODY))
						// Устанавливаем флаг завершения потока
						flag = awh::http2_t::flag_t::END_STREAM;
					// Выполняем ответ подключившемуся клиенту
					if(web2_t::send(options->sid, bid, headers, flag) < 0)
						// Выходим из функции
						return;
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
								std::cout << this->_fmk->format("<chunk %zu>", entity.size()) << std::endl << std::endl << std::flush;
							#endif
							// Если нужно установить флаг закрытия потока
							if(options->http.empty(awh::http_t::suite_t::BODY) && (options->http.trailers() == 0))
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
								std::cout << "<Trailers>" << std::endl << std::endl << std::flush;
								// Выполняем перебор всего списка отправляемых трейлеров
								for(auto & trailer : trailers){
									// Получаем название заголовка
									name = trailer.first;
									// Переводим заголовок в нормальный режим
									this->_fmk->transform(name, fmk_t::transform_t::SMART);
									// Выводим сообщение о выводе чанка тела
									std::cout << this->_fmk->format("%s: %s", name.c_str(), trailer.second.c_str()) << std::endl << std::flush;
								}
								// Выводим завершение вывода информации
								std::cout << std::endl << std::endl << std::flush;
							#endif
							// Выполняем отправку трейлеров
							if(!web2_t::send(options->sid, bid, trailers))
								// Выходим из функции
								return;
						}
					}
					// Выполняем извлечение параметров запроса
					const auto & request = stream->http.request();
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(!stream->http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", sid, bid, request.method, request.url, stream->http.body());
					// Если функция обратного вызова на вывод полученных данных запроса клиента установлена
					if(this->_callbacks.is("complete"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", sid, bid, request.method, request.url, stream->http.body(), stream->http.headers());
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callbacks.is("error"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, response.message);
					// Выполняем очистку HTTP-парсера
					options->http.clear();
					// Выполняем сброс состояния HTTP-парсера
					options->http.reset();
					// Завершаем работу
					return;
				}
				// Выполняем закрытие подключения
				web2_t::close(bid);
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
			scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Устанавливаем флаг отключения
				options->close = true;
				// Определяем протокола подключения
				switch(static_cast <uint8_t> (options->proto)){
					// Если протокол подключения соответствует HTTP/1.1
					case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
						// Выполняем поиск агента которому соответствует клиент
						auto i = this->_http1._agents.find(bid);
						// Если активный агент клиента установлен
						if(i != this->_http1._agents.end()){
							// Определяем тип активного протокола
							switch(static_cast <uint8_t> (i->second)){
								// Если протокол соответствует HTTP-протоколу
								case static_cast <uint8_t> (agent_t::HTTP):
								// Если протокол соответствует протоколу Websocket
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
						auto i = this->_agents.find(bid);
						// Если активный агент клиента установлен
						if(i != this->_agents.end()){
							// Определяем тип активного протокола
							switch(static_cast <uint8_t> (i->second)){
								// Если протокол соответствует HTTP-протоколу
								case static_cast <uint8_t> (agent_t::HTTP):
									// Выполняем удаление созданной ранее сессии HTTP/2
									this->_sessions.erase(bid);
								break;
								// Если протокол соответствует протоколу Websocket
								case static_cast <uint8_t> (agent_t::WEBSOCKET): {
									// Выполняем поиск брокера в списке активных сессий
									auto i = this->_ws2._sessions.find(bid);
									// Если активная сессия найдена
									if(i != this->_ws2._sessions.end())
										// Выполняем удаление устаревшей сессии
										this->_ws2._sessions.erase(i);
									// Выполняем удаление созданной ранее сессии HTTP/2
									this->_sessions.erase(bid);
									// Выполняем удаление отключённого брокера Websocket-клиента
									this->_ws2.erase(bid);
								} break;
							}
							// Выполняем удаление активного агента
							this->_agents.erase(i);
						}
					} break;
				}
			}
			// Выполняем удаление параметров брокера
			this->_scheme.rm(bid);
		};
		// Получаем текущее значение времени
		const uint64_t date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
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
void awh::server::Http2::disconnect(const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Определяем протокола подключения
		switch(static_cast <uint8_t> (options->proto)){
			// Если протокол подключения соответствует HTTP/1.1
			case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_http1._agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_http1._agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
						// Если протокол соответствует протоколу Websocket
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
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует протоколу Websocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выполняем отключение брокера клиента Websocket
							this->_ws2.disconnect(bid);
						break;
					}
				}
			} break;
		}
		// Добавляем в очередь список отключившихся клиентов
		this->_disconected.emplace(bid, this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS));
	}
}
/**
 * pinging Метод таймера выполнения пинга клиента
 * @param tid идентификатор таймера
 */
void awh::server::Http2::pinging(const uint16_t tid) noexcept {
	// Если данные существуют и разрешено выполнять пинги
	if((tid > 0) && this->_pinging){
		// Выполняем перебор всех активных клиентов
		for(auto & item : this->_scheme.get()){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (item.second->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(item.first);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
							// Если протокол соответствует протоколу Websocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем переброс события пинга в модуль HTTP
								this->_http1.pinging(tid);
							break;
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_agents.find(item.first);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Получаем текущий штамп времени
								const uint64_t date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
								// Если время с предыдущего пинга прошло больше половины времени пинга
								if((this->_pingInterval > 0) && ((date - item.second->sendPing) > static_cast <uint64_t> (this->_pingInterval / 2))){
									// Если переключение протокола на HTTP/2 выполнено и пинг не прошёл
									if(!this->ping(item.first))
										// Выполняем закрытие подключения
										web2_t::close(item.first);
									// Обновляем время отправленного пинга
									else item.second->sendPing = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
								}
							} break;
							// Если протокол соответствует протоколу Websocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем переброс события пинга в модуль Websocket
								this->_ws2.pinging(tid);
							break;
						}
					}
				} break;
			}
		}
	}
}
/**
 * parser Метод извлечения объекта HTTP-парсера
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @return    объект HTTP-парсера
 */
const awh::http_t * awh::server::Http2::parser(const int32_t sid, const uint64_t bid) const noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
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
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Извлекаем данные потока
								const scheme::web2_t::stream_t * stream = this->_scheme.getStream(sid, bid);
								// Если поток получен удачно
								if(stream != nullptr)
									// Выполняем получение объекта HTTP-парсера
									return &stream->http;
							}
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
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @return    флаг запроса клиентом передачи трейлеров
 */
bool awh::server::Http2::trailers(const int32_t sid, const uint64_t bid) const noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
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
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Извлекаем данные потока
								const scheme::web2_t::stream_t * stream = this->_scheme.getStream(sid, bid);
								// Если поток получен удачно
								if(stream != nullptr)
									// Выполняем получение флага запроса клиента на передачу трейлеров
									return stream->http.is(http_t::state_t::TRAILERS);
							}
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
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @param key ключ заголовка
 * @param val значение заголовка
 */
void awh::server::Http2::trailer(const int32_t sid, const uint64_t bid, const string & key, const string & val) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
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
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Извлекаем данные потока
								scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
								// Если поток получен удачно
								if(stream != nullptr)
									// Выполняем установку трейлера
									stream->http.trailer(key, val);
							} break;
						}
					}
				} break;
			}
		}
	}
}
/**
 * init Метод инициализации Websocket-сервера
 * @param socket      unix-сокет для биндинга
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Http2::init(const string & socket, const vector <http_t::compressor_t> & compressors) noexcept {
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
void awh::server::Http2::init(const uint32_t port, const string & host, const vector <http_t::compressor_t> & compressors) noexcept {
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
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует протоколу Websocket
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
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует протоколу Websocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем отправку ошибки клиенту Websocket
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
 * @return        результат отправки сообщения
 */
bool awh::server::Http2::sendMessage(const uint64_t bid, const vector <char> & message, const bool text) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует протоколу Websocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем передачу данных клиенту
								return this->_http1.sendMessage(bid, message, text);
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует протоколу Websocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем передачу данных клиенту Websocket
								return this->_ws2.sendMessage(bid, message, text);
						}
					}
				} break;
			}
		}
	}
	// Сообщаем что ничего не найдено
	return false;
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param bid     идентификатор брокера
 * @param message передаваемое сообщения в бинарном виде
 * @param size    размер передаваемого сообещния
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::server::Http2::sendMessage(const uint64_t bid, const char * message, const size_t size, const bool text) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует протоколу Websocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем передачу данных клиенту
								return this->_http1.sendMessage(bid, message, size, text);
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует протоколу Websocket
							case static_cast <uint8_t> (agent_t::WEBSOCKET):
								// Выполняем передачу данных клиенту Websocket
								return this->_ws2.sendMessage(bid, message, size, text);
						}
					}
				} break;
			}
		}
	}
	// Сообщаем что ничего не найдено
	return false;
}
/**
 * send Метод отправки данных в бинарном виде клиенту
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 * @return       результат отправки сообщения
 */
bool awh::server::Http2::send(const uint64_t bid, const char * buffer, const size_t size) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0))
		// Выполняем отправку заголовков ответа клиенту
		return const_cast <server::core_t *> (this->_core)->send(buffer, size, bid);
	// Сообщаем что ничего не найдено
	return false;
}
/**
 * send Метод отправки трейлеров
 * @param sid     идентификатор потока HTTP/2
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @return        результат отправки данных указанному клиенту
 */
bool awh::server::Http2::send(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем отправку трейлеров
							return web2_t::send(sid, bid, headers);
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
 * @param sid    идентификатор потока HTTP
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::Http2::send(const int32_t sid, const uint64_t bid, const char * buffer, const size_t size, const bool end) noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0)){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
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
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Тело WEB сообщения
								vector <char> entity;
								// Извлекаем данные потока
								scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
								// Если поток получен удачно
								if(stream != nullptr){
									// Выполняем очистку данных тела
									stream->http.clear(http_t::suite_t::BODY);
									// Устанавливаем тело запроса
									stream->http.body(vector <char> (buffer, buffer + size));
									// Флаг отправляемого фрейма
									awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
									// Получаем данные тела запроса
									while(!(entity = stream->http.payload()).empty()){
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											// Выводим сообщение о выводе чанка тела
											std::cout << this->_fmk->format("<chunk %zu>", entity.size()) << std::endl << std::endl << std::flush;
										#endif
										// Если нужно установить флаг закрытия потока
										if(end && stream->http.empty(awh::http_t::suite_t::BODY) && (stream->http.trailers() == 0))
											// Устанавливаем флаг завершения потока
											flag = awh::http2_t::flag_t::END_STREAM;
										// Выполняем отправку данных на удалённый сервер
										result = web2_t::send(sid, bid, entity.data(), entity.size(), flag);
									}
									// Если список трейлеров установлен
									if(result && (stream->http.trailers() > 0)){
										// Выполняем извлечение трейлеров
										const auto & trailers = stream->http.trailers2();
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											// Название выводимого заголовка
											string name = "";
											// Выводим заголовок трейлеров
											std::cout << "<Trailers>" << std::endl << std::endl << std::flush;
											// Выполняем перебор всего списка отправляемых трейлеров
											for(auto & trailer : trailers){
												// Получаем название заголовка
												name = trailer.first;
												// Переводим заголовок в нормальный режим
												this->_fmk->transform(name, fmk_t::transform_t::SMART);
												// Выводим сообщение о выводе чанка тела
												std::cout << this->_fmk->format("%s: %s", name.c_str(), trailer.second.c_str()) << std::endl << std::flush;
											}
											// Выводим завершение вывода информации
											std::cout << std::endl << std::endl << std::flush;
										#endif
										// Выполняем отправку трейлеров
										if((result = !web2_t::send(sid, bid, trailers)))
											// Выходим из функции
											return result;
									}
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
 * @param sid     идентификатор потока HTTP
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param headers заголовки отправляемые клиенту
 * @param end     размер сообщения в байтах
 * @return        идентификатор нового запроса
 */
int32_t awh::server::Http2::send(const int32_t sid, const uint64_t bid, const uint32_t code, const string & mess, const unordered_multimap <string, string> & headers, const bool end) noexcept {
	// Если заголовки запроса переданы
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
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
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Извлекаем данные потока
								scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
								// Если поток получен удачно
								if(stream != nullptr){
									// Выполняем сброс состояния HTTP-парсера
									stream->http.reset();
									// Выполняем очистку заголовков
									stream->http.clear(http_t::suite_t::HEADER);
									// Устанавливаем заголовоки запроса
									stream->http.headers(headers);
									// Если сообщение ответа не установлено
									if(mess.empty())
										// Выполняем установку сообщения по умолчанию
										const_cast <string &> (mess) = stream->http.message(code);
									// Формируем ответ на запрос клиента
									awh::web_t::res_t response(2.f, code, mess);
									// Получаем заголовки ответа удалённому клиенту
									const auto & headers = stream->http.process2(http_t::process_t::RESPONSE, response);
									// Если заголовки запроса получены
									if(!headers.empty()){
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											{
												// Выводим заголовок ответа
												std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
												// Получаем бинарные данные REST-ответа
												const auto & buffer = stream->http.process(http_t::process_t::RESPONSE, response);
												// Если бинарные данные ответа получены
												if(!buffer.empty())
													// Выводим параметры ответа
													std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
											}
										#endif
										// Флаг отправляемого фрейма
										awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
										// Если тело запроса не существует
										if(end)
											// Устанавливаем флаг завершения потока
											flag = awh::http2_t::flag_t::END_STREAM;
										// Выполняем заголовки запроса на сервер
										return web2_t::send(sid, bid, headers, flag);
									}
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
 * send Метод отправки сообщения брокеру
 * @param sid     идентификатор потока HTTP
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param buffer  данные полезной нагрузки (тело сообщения)
 * @param size    размер данных полезной нагрузки (размер тела сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Http2::send(const int32_t sid, const uint64_t bid, const uint32_t code, const string & mess, const char * buffer, const size_t size, const unordered_multimap <string, string> & headers) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP):
								// Выполняем передачу запроса на сервер HTTP/1.1
								this->_http1.send(bid, code, mess, buffer, size, headers);
							break;
						}
					}
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Извлекаем данные потока
								scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
								// Если поток получен удачно
								if(stream != nullptr){
									// Выполняем сброс состояния HTTP парсера
									stream->http.reset();
									// Выполняем очистку данных тела
									stream->http.clear(http_t::suite_t::BODY);
									// Выполняем очистку заголовков
									stream->http.clear(http_t::suite_t::HEADER);
									// Устанавливаем заголовки ответа
									stream->http.headers(headers);
									// Устанавливаем полезную нагрузку
									stream->http.body(buffer, size);
									// Если сообщение ответа не установлено
									if(mess.empty())
										// Выполняем установку сообщения по умолчанию
										const_cast <string &> (mess) = stream->http.message(code);
									{
										// Формируем ответ на запрос клиента
										awh::web_t::res_t response(2.f, code, mess);
										// Получаем заголовки ответа удалённому клиенту
										const auto & headers = stream->http.process2(http_t::process_t::RESPONSE, response);
										// Если бинарные данные ответа получены
										if(!headers.empty()){
											/**
											 * Если включён режим отладки
											 */
											#if defined(DEBUG_MODE)
												{
													// Выводим заголовок ответа
													std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
													// Получаем бинарные данные REST-ответа
													const auto & buffer = stream->http.process(http_t::process_t::RESPONSE, response);
													// Если бинарные данные ответа получены
													if(!buffer.empty())
														// Выводим параметры ответа
														std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
												}
											#endif
											// Флаг отправляемого фрейма
											awh::http2_t::flag_t flag = awh::http2_t::flag_t::NONE;
											// Если тело запроса не существует
											if(stream->http.empty(awh::http_t::suite_t::BODY))
												// Устанавливаем флаг завершения потока
												flag = awh::http2_t::flag_t::END_STREAM;
											// Если запрос не получилось отправить, выходим из функции
											if(web2_t::send(sid, bid, headers, flag) < 0)
												// Выходим из функции
												return;
											// Если тело запроса существует
											if((code >= 200) && !stream->http.empty(awh::http_t::suite_t::BODY)){
												// Тело HTTP-запроса
												vector <char> entity;
												// Получаем данные тела запроса
												while(!(entity = stream->http.payload()).empty()){
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Выводим сообщение о выводе чанка тела
														std::cout << this->_fmk->format("<chunk %zu>", entity.size()) << std::endl << std::endl << std::flush;
													#endif
													// Если нужно установить флаг закрытия потока
													if(stream->http.empty(awh::http_t::suite_t::BODY) && (stream->http.trailers() == 0))
														// Устанавливаем флаг завершения потока
														flag = awh::http2_t::flag_t::END_STREAM;
													// Выполняем отправку тела запроса на сервер
													if(!web2_t::send(sid, bid, entity.data(), entity.size(), flag))
														// Выходим из функции
														return;
												}
												// Если список трейлеров установлен
												if(stream->http.trailers() > 0){
													// Выполняем извлечение трейлеров
													const auto & trailers = stream->http.trailers2();
													/**
													 * Если включён режим отладки
													 */
													#if defined(DEBUG_MODE)
														// Название выводимого заголовка
														string name = "";
														// Выводим заголовок трейлеров
														std::cout << "<Trailers>" << std::endl << std::endl << std::flush;
														// Выполняем перебор всего списка отправляемых трейлеров
														for(auto & trailer : trailers){
															// Получаем название заголовка
															name = trailer.first;
															// Переводим заголовок в нормальный режим
															this->_fmk->transform(name, fmk_t::transform_t::SMART);
															// Выводим сообщение о выводе чанка тела
															std::cout << this->_fmk->format("%s: %s", name.c_str(), trailer.second.c_str()) << std::endl << std::flush;
														}
														// Выводим завершение вывода информации
														std::cout << std::endl << std::endl << std::flush;
													#endif
													// Выполняем отправку трейлеров
													if(!web2_t::send(sid, bid, trailers))
														// Выходим из функции
														return;
												}
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
 * send Метод отправки сообщения брокеру
 * @param sid     идентификатор потока HTTP
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Http2::send(const int32_t sid, const uint64_t bid, const uint32_t code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Если тело сообщения передано
		if(!entity.empty())
			// Выполняем отправку ответа с телом сообщения
			this->send(sid, bid, code, mess, entity.data(), entity.size(), headers);
		// Выполняем отправку ответа без тела сообщения
		else this->send(sid, bid, code, mess, nullptr, 0, headers);
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
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
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
 * @param sid   идентификатор потока
 * @param bid   идентификатор брокера
 * @param error код отправляемой ошибки
 * @return      результат отправки сообщения
 */
bool awh::server::Http2::reject2(const int32_t sid, const uint64_t bid, const awh::http2_t::error_t error) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем сброс подключения
							return web2_t::reject(sid, bid, error);
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
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
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
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @return        результат отправки данных указанному клиенту
 */
bool awh::server::Http2::send2(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем отправку трейлеров
							return web2_t::send(sid, bid, headers);
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
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых
 * @param size   размер сообщения в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::Http2::send2(const int32_t sid, const uint64_t bid, const char * buffer, const size_t size, const awh::http2_t::flag_t flag) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем отправку сообщения клиенту
							return web2_t::send(sid, bid, buffer, size, flag);
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
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::server::Http2::send2(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
							// Выполняем отправку заголовков
							return web2_t::send(sid, bid, headers, flag);
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
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::server::Http2::push2(const int32_t sid, const uint64_t bid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если протокол подключения соответствует HTTP/2
			if(options->proto == engine_t::proto_t::HTTP2){
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
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
									std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ PUSH ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
									// Получаем бинарные данные HTTP-запроса
									const auto & buffer = http.process(http_t::process_t::REQUEST, http.request());
									// Выводим параметры запроса
									std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
								}
							#endif
							// Выполняем отправку push-уведомлений
							return web2_t::push(sid, bid, headers, flag);
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
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::server::Http2::callbacks(const fn_t & callbacks) noexcept {
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
		// Выполняем установку функции обратного вызова для выполнения события запуска сервера
		callbacks.set("launched", this->_callbacks);
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
		if(!callbacks.empty()){
			// Выполняем установку функций обратного вызова для Websocket-сервера
			this->_ws2.callbacks(callbacks);
			// Выполняем установку функций обратного вызова для HTTP-сервера
			this->_http1.callbacks(callbacks);
		}
	}
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
uint32_t awh::server::Http2::port(const uint64_t bid) const noexcept {
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
	scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Определяем протокола подключения
		switch(static_cast <uint8_t> (options->proto)){
			// Если протокол подключения соответствует HTTP/1.1
			case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_http1._agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_http1._agents.end())
					// Выводим идентификатор агента
					return i->second;
			} break;
			// Если протокол подключения соответствует HTTP/2
			case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end())
					// Выводим идентификатор агента
					return i->second;
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
		else this->openEvents(this->_scheme.id);
	}
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Http2::close(const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены, устанавливаем флаг закрытия подключения
	if((this->_core != nullptr) && (options != nullptr)){
		// Определяем протокола подключения
		switch(static_cast <uint8_t> (options->proto)){
			// Если протокол подключения соответствует HTTP/1.1
			case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_http1._agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_http1._agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP):
						// Если протокол соответствует протоколу Websocket
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
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует HTTP-протоколу
						case static_cast <uint8_t> (agent_t::HTTP): {
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
						} break;
						// Если протокол соответствует протоколу Websocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET): {
							// Выполняем закрытие подключения клиента Websocket
							this->_ws2.close(bid);
							// Выполняем поиск брокера в списке активных сессий
							auto i = this->_ws2._sessions.find(bid);
							// Если активная сессия найдена
							if(i != this->_ws2._sessions.end())
								// Выполняем удаление сессии HTTP/2
								this->_ws2._sessions.erase(i);
						} break;
					}
				}
			} break;
		}
	}
}
/**
 * waitPong Метод установки времени ожидания ответа WebSocket-клиента
 * @param sec время ожидания в секундах
 */
void awh::server::Http2::waitPong(const uint16_t sec) noexcept {
	// Выполняем установку времени ожидания
	this->_ws2.waitPong(sec);
}
/**
 * pingInterval Метод установки интервала времени выполнения пингов
 * @param sec интервал времени выполнения пингов в секундах
 */
void awh::server::Http2::pingInterval(const uint16_t sec) noexcept {
	// Выполняем установку интервала времени выполнения пингов в секундах
	this->_ws2.pingInterval(sec);
	// Выполняем установку интервала времени выполнения пингов в секундах
	this->_pingInterval = (static_cast <uint32_t> (sec) * 1000);
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::server::Http2::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку списка поддерживаемого сабпротокола для Websocket-модуля
	this->_ws2.subprotocol(subprotocol);
	// Выполняем установку списка поддерживаемого сабпротокола для HTTP-модуля
	this->_http1.subprotocol(subprotocol);
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::server::Http2::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку списка поддерживаемых сабпротоколов для Websocket-модуля
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
	scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Определяем протокола подключения
		switch(static_cast <uint8_t> (options->proto)){
			// Если протокол подключения соответствует HTTP/1.1
			case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_http1._agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_http1._agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует протоколу Websocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выводим согласованный сабпротокол
							return this->_http1.subprotocols(bid);
					}
				}
			} break;
			// Если протокол подключения соответствует HTTP/2
			case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует протоколу Websocket
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
	// Выполняем установку списка поддерживаемых расширений для Websocket-модуля
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
	scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr){
		// Определяем протокола подключения
		switch(static_cast <uint8_t> (options->proto)){
			// Если протокол подключения соответствует HTTP/1.1
			case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_http1._agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_http1._agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует протоколу Websocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выполняем извлечение списка поддерживаемых расширений Websocket
							return this->_http1.extensions(bid);
					}
				}
			} break;
			// Если протокол подключения соответствует HTTP/2
			case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
				// Выполняем поиск агента которому соответствует клиент
				auto i = this->_agents.find(bid);
				// Если активный агент клиента установлен
				if(i != this->_agents.end()){
					// Определяем тип активного протокола
					switch(static_cast <uint8_t> (i->second)){
						// Если протокол соответствует протоколу Websocket
						case static_cast <uint8_t> (agent_t::WEBSOCKET):
							// Выполняем извлечение списка поддерживаемых расширений Websocket
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
			this->_ws2._thr.stop();
			// Выполняем инициализацию нового тредпула
			this->_ws2._thr.init(count);
		}
		// Если сетевое ядро установлено
		if(this->_core != nullptr)
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->_ws2._thr.stop();
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Http2::total(const uint16_t total) noexcept {
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr)
		// Устанавливаем максимальное количество одновременных подключений
		const_cast <server::core_t *> (this->_core)->total(this->_scheme.id, total);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::server::Http2::segmentSize(const size_t size) noexcept {
	// Выполняем установку размеров сегментов фрейма для Websocket-сервера
	this->_ws2.segmentSize(size);
	// Выполняем установку размеров сегментов фрейма для HTTP-сервера
	this->_http1.segmentSize(size);
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Http2::compressors(const vector <http_t::compressor_t> & compressors) noexcept {
	// Устанавливаем список компрессоров
	this->_scheme.compressors = compressors;
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Http2::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
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
void awh::server::Http2::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для Websocket-сервера
	this->_ws2.mode(flags);
	// Устанавливаем флаги настроек модуля для HTTP-сервера
	this->_http1.mode(flags);
	// Активируем выполнение пинга
	this->_pinging = (flags.find(flag_t::NOT_PING) == flags.end());
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_complete = (flags.find(flag_t::NOT_STOP) == flags.end());
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.find(flag_t::ALIVE) != flags.end());
	// Устанавливаем флаг разрешающий выполнять подключение к протоколу Websocket
	this->_webSocket = (flags.find(flag_t::WEBSOCKET_ENABLE) != flags.end());
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_client.takeover = (flags.find(flag_t::TAKEOVER_CLIENT) != flags.end());
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_server.takeover = (flags.find(flag_t::TAKEOVER_SERVER) != flags.end());
	// Устанавливаем флаг разрешающий выполнять метод CONNECT для сервера
	if(flags.find(flag_t::CONNECT_METHOD_ENABLE) != flags.end())
		// Выполняем установку разрешения использования метода CONNECT
		this->_settings.emplace(awh::http2_t::settings_t::CONNECT, 1);
	// Если сетевое ядро установлено
	if(this->_core != nullptr)
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <server::core_t *> (this->_core)->verbose(flags.find(flag_t::NOT_INFO) == flags.end());
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Http2::alive(const bool mode) noexcept {
	// Выполняем установку долгоживущего подключения
	web2_t::alive(mode);
	// Выполняем установку долгоживущего подключения для Websocket-сервера
	this->_ws2.alive(mode);
	// Выполняем установку долгоживущего подключения для HTTP-сервера
	this->_http1.alive(mode);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param bid  идентификатор брокера
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Http2::alive(const uint64_t bid, const bool mode) noexcept {
	// Получаем параметры активного клиента
	scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
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
		// Выполняем установку сетевого ядра
		web_t::core(core);
		// Добавляем схемы сети в сетевое ядро
		const_cast <server::core_t *> (this->_core)->scheme(&this->_scheme);
		// Если многопоточность активированна
		if(this->_ws2._thr.is() || this->_ws2._ws1._thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
		// Устанавливаем событие на запуск системы
		const_cast <server::core_t *> (this->_core)->callback <void (const uint16_t)> ("open", std::bind(&http2_t::openEvents, this, _1));
		// Устанавливаем событие подключения
		const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("connect", std::bind(&http2_t::connectEvents, this, _1, _2));
		// Устанавливаем событие отключения
		const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("disconnect", std::bind(&http2_t::disconnectEvents, this, _1, _2));
		// Устанавливаем функцию чтения данных
		const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", std::bind(&http2_t::readEvents, this, _1, _2, _3, _4));
		// Устанавливаем функцию записи данных
		const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&http2_t::writeEvents, this, _1, _2, _3, _4));
		// Добавляем событие аццепта брокера
		const_cast <server::core_t *> (this->_core)->callback <bool (const string &, const string &, const uint32_t, const uint64_t)> ("accept", std::bind(&http2_t::acceptEvents, this, _1, _2, _3, _4));
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_ws2._thr.is() || this->_ws2._ws1._thr.is()){
			// Выполняем завершение всех активных потоков
			this->_ws2._thr.stop();
			// Выполняем завершение всех активных потоков
			this->_ws2._ws1._thr.stop();
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
 * waitMessage Метод ожидания входящих сообщений
 * @param sec интервал времени в секундах
 */
void awh::server::Http2::waitMessage(const uint16_t sec) noexcept {
	// Устанавливаем время ожидания получения данных
	this->_scheme.timeouts.wait = sec;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Http2::waitTimeDetect(const uint16_t read, const uint16_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Http2::realm(const string & realm) noexcept {
	// Устанавливаем название сервера
	web2_t::realm(realm);
	// Устанавливаем название сервера для Websocket-сервера
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
	// Устанавливаем временный ключ сессии сервера для Websocket-сервера
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
	// Устанавливаем размера чанка для Websocket-сервера
	this->_ws2.chunk(size);
	// Устанавливаем размера чанка для HTTP-сервера
	this->_http1.chunk(size);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Http2::maxRequests(const uint32_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	this->_maxRequests = max;
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
	// Устанавливаем идентификацию сервера для Websocket-сервера
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
	// Устанавливаем тип авторизации для Websocket-сервера
	this->_ws2.authType(type, hash);
	// Устанавливаем тип авторизации для HTTP-сервера
	this->_http1.authType(type, hash);
}
/**
 * crypted Метод получения флага шифрования
 * @param sid идентификатор потока HTTP
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::server::Http2::crypted(const int32_t sid, const uint64_t bid) const noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
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
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Извлекаем данные потока
								const scheme::web2_t::stream_t * stream = this->_scheme.getStream(sid, bid);
								// Если поток получен удачно
								if(stream != nullptr)
									// Выводим установленный флаг шифрования
									return stream->crypted;
							} break;
							// Если протокол соответствует протоколу Websocket
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
 * @param sid  идентификатор потока HTTP
 * @param bid  идентификатор брокера
 * @param mode флаг активации шифрования
 */
void awh::server::Http2::encrypt(const int32_t sid, const uint64_t bid, const bool mode) noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		scheme::web2_t::options_t * options = const_cast <scheme::web2_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (options->proto)){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Выполняем поиск агента которому соответствует клиент
					auto i = this->_http1._agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_http1._agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
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
					auto i = this->_agents.find(bid);
					// Если активный агент клиента установлен
					if(i != this->_agents.end()){
						// Определяем тип активного протокола
						switch(static_cast <uint8_t> (i->second)){
							// Если протокол соответствует HTTP-протоколу
							case static_cast <uint8_t> (agent_t::HTTP): {
								// Извлекаем данные потока
								scheme::web2_t::stream_t * stream = const_cast <scheme::web2_t::stream_t *> (this->_scheme.getStream(sid, bid));
								// Если поток получен удачно
								if(stream != nullptr){
									// Устанавливаем флаг шифрования для клиента
									stream->crypted = mode;
									// Устанавливаем флаг шифрования
									stream->http.encryption(stream->crypted);
								}
							} break;
							// Если протокол соответствует протоколу Websocket
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
	// Устанавливаем флага шифрования для Websocket-сервера
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
	// Устанавливаем параметры шифрования для Websocket-сервера
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
 web2_t(fmk, log), _webSocket(false), _maxRequests(SERVER_MAX_REQUESTS),
 _identity(http_t::identity_t::HTTP), _ws2(fmk, log), _http1(fmk, log), _scheme(fmk, log) {}
/**
 * Http2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Http2::Http2(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(core, fmk, log), _webSocket(false), _maxRequests(SERVER_MAX_REQUESTS),
 _identity(http_t::identity_t::HTTP), _ws2(fmk, log), _http1(fmk, log), _scheme(fmk, log) {
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->scheme(&this->_scheme);
	// Устанавливаем событие на запуск системы
	const_cast <server::core_t *> (this->_core)->callback <void (const uint16_t)> ("open", std::bind(&http2_t::openEvents, this, _1));
	// Устанавливаем событие подключения
	const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("connect", std::bind(&http2_t::connectEvents, this, _1, _2));
	// Устанавливаем событие отключения
	const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("disconnect", std::bind(&http2_t::disconnectEvents, this, _1, _2));
	// Устанавливаем функцию чтения данных
	const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", std::bind(&http2_t::readEvents, this, _1, _2, _3, _4));
	// Устанавливаем функцию записи данных
	const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&http2_t::writeEvents, this, _1, _2, _3, _4));
	// Добавляем событие аццепта брокера
	const_cast <server::core_t *> (this->_core)->callback <bool (const string &, const string &, const uint32_t, const uint64_t)> ("accept", std::bind(&http2_t::acceptEvents, this, _1, _2, _3, _4));
}
/**
 * ~Http2 Деструктор
 */
awh::server::Http2::~Http2() noexcept {
	// Снимаем адрес сетевого ядра
	this->_ws2._core = nullptr;
}
