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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <server/web/http1.hpp>

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
void awh::server::Http1::connectEvents(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
		// Создаём брокера
		this->_scheme.set(bid);
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
			// Если функция обратного вызова для обработки чанков установлена
			if(!this->_callbacks.is("chunking"))
				// Устанавливаем функцию обработки вызова для получения чанков
				options->http.callback <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", std::bind(&http1_t::chunking, this, _1, _2, _3));
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
			// Выполняем добавление агнета
			this->_agents.emplace(bid, agent_t::HTTP);
			// Если функция обратного вызова при подключении/отключении установлена
			if(this->_callbacks.is("active"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const mode_t)> ("active", bid, mode_t::CONNECT);
		}
	}
}
/**
 * disconnectEvents Метод обратного вызова при отключении клиента
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::server::Http1::disconnectEvents(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
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
void awh::server::Http1::readEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
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
			// Выполняем поиск агента которому соответствует клиент
			auto i = this->_agents.find(bid);
			// Если агент соответствует Websocket-у
			if((i != this->_agents.end()) && (i->second == agent_t::WEBSOCKET))
				// Выполняем передачу данных клиенту Websocket
				this->_ws1.readEvents(buffer, size, bid, sid);
			// Иначе выполняем обработку входящих данных как Web-сервер
			else {
				// Получаем параметры активного клиента
				scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
				// Если параметры активного клиента получены
				if(options != nullptr){
					// Если подключение закрыто
					if(options->close){
						// Принудительно выполняем отключение лкиента
						const_cast <server::core_t *> (this->_core)->close(bid);
						// Выходим из функции
						return;
					}
					// Добавляем полученные данные в буфер
					options->buffer.push(buffer, size);
					// Если функция обратного вызова активности потока установлена
					if(!options->mode && (options->mode = this->_callbacks.is("stream")))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", 1, bid, mode_t::OPEN);
					// Выполняем обработку полученных данных
					while(!options->close){
						// Выполняем парсинг полученных данных
						const size_t bytes = options->http.parse(reinterpret_cast <const char *> (options->buffer.get()), options->buffer.size());
						// Если все данные получены
						if((bytes > 0) && options->http.is(http_t::state_t::END)){
							// Получаем флаг постоянного подключения
							const bool alive = options->http.is(http_t::state_t::ALIVE);
							// Если включён режим отладки
							#if defined(DEBUG_MODE)
								{
									// Получаем данные запроса
									const auto & request = options->http.process(http_t::process_t::REQUEST, options->http.request());
									// Если параметры запроса получены
									if(!request.empty()){
										// Выводим заголовок запроса
										std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
										// Выводим параметры запроса
										std::cout << string(request.begin(), request.end()) << std::endl << std::endl << std::flush;
										// Если тело запроса существует
										if(!options->http.empty(awh::http_t::suite_t::BODY))
											// Выводим сообщение о выводе чанка тела
											std::cout << this->_fmk->format("<body %u>", options->http.body().size()) << std::endl << std::endl << std::flush;
										// Иначе устанавливаем перенос строки
										else std::cout << std::endl << std::flush;
									}
								}
							#endif
							/**
							 * rejectFn Функция завершения подключения
							 * @param bid идентификатор брокера
							 */
							auto rejectFn = [alive, &options, this](const uint64_t bid) noexcept -> void {
								// Выполняем очистку HTTP-парсера
								options->http.clear();
								// Выполняем сброс состояния HTTP-парсера
								options->http.reset();
								// Выполняем очистку буфера полученных данных
								options->buffer.clear();
								// Если подключение установленно не постоянное
								if(!alive){
									// Определяем идентичность сервера
									switch(static_cast <uint8_t> (this->_identity)){
										// Если сервер соответствует HTTP-серверу
										case static_cast <uint8_t> (http_t::identity_t::HTTP):
											// Устанавливаем закрытие подключения
											options->http.header("Connection", "close");
										break;
										// Если сервер соответствует PROXY-серверу
										case static_cast <uint8_t> (http_t::identity_t::PROXY): {
											// Устанавливаем закрытие подключения
											options->http.header("Connection", "close");
											// Устанавливаем закрытие подключения
											options->http.header("Proxy-Connection", "close");
										} break;
									}
								}
								// Формируем запрос авторизации
								const auto & response = options->http.reject(awh::web_t::res_t(static_cast <uint32_t> (505), "Requested protocol is not supported by this server"));
								// Если ответ получен
								if(!response.empty()){
									// Тело полезной нагрузки
									vector <char> payload;
									/**
									 * Если включён режим отладки
									 */
									#if defined(DEBUG_MODE)
										// Выводим заголовок ответа
										std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
										// Выводим параметры ответа
										std::cout << string(response.begin(), response.end()) << std::endl << std::endl << std::flush;
									#endif
									// Отправляем ответ брокеру
									const_cast <server::core_t *> (this->_core)->send(response.data(), response.size(), bid);
									// Получаем тело полезной нагрузки ответа
									while(!(payload = options->http.payload()).empty()){
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											// Выводим сообщение о выводе чанка полезной нагрузки
											std::cout << this->_fmk->format("<chunk %zu>", payload.size()) << std::endl << std::endl << std::flush;
										#endif
										// Если тела данных для отправки больше не осталось
										if(options->http.empty(awh::http_t::suite_t::BODY))
											// Если подключение не установлено как постоянное, устанавливаем флаг завершения работы
											options->stopped = (!this->_service.alive && !options->alive && !options->http.is(http_t::state_t::ALIVE));
										// Выполняем отправку тела ответа клиенту
										const_cast <server::core_t *> (this->_core)->send(payload.data(), payload.size(), bid);
									}
								// Выполняем отключение брокера
								} else const_cast <server::core_t *> (this->_core)->close(bid);
								// Если функция обратного вызова активности потока установлена
								if(this->_callbacks.is("stream"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", 1, bid, mode_t::CLOSE);
								// Если функция обратного вызова на на вывод ошибок установлена
								if(this->_callbacks.is("error"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "Requested protocol is not supported by this server");
								// Если установлена функция отлова завершения запроса
								if(this->_callbacks.is("end"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", 1, bid, direct_t::RECV);
							};
							// Если метод CONNECT на сервере запрещён и в данный момент он выполняется
							if(!this->_methodConnect && (options->http.request().method == awh::web_t::method_t::CONNECT)){
								// Выполняем закрытие подключения
								rejectFn(bid);
								// Завершаем обработку
								goto Next;
							}
							// Если подключение не установлено как постоянное
							if(!this->_service.alive && !options->alive){
								// Если количество запросов ограничен
								if(this->_maxRequests > 0)
									// Увеличиваем количество выполненных запросов
									options->requests++;
								// Если количество выполненных запросов превышает максимальный
								if(!(options->close = ((this->_maxRequests > 0) && (options->requests >= this->_maxRequests))))
									// Получаем текущий штамп времени
									options->respPong = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
							// Выполняем сброс количества выполненных запросов
							} else options->requests = 0;
							// Получаем флаг шифрованных данных
							options->crypted = options->http.crypted();
							// Получаем поддерживаемый метод компрессии
							options->compressor = options->http.compression();
							// Выполняем проверку авторизации
							switch(static_cast <uint8_t> (options->http.auth())){
								// Если запрос выполнен удачно
								case static_cast <uint8_t> (http_t::status_t::GOOD): {
									// Если сервер соответствует HTTP-серверу
									if(this->_identity == http_t::identity_t::HTTP){
										// Если заголовок Upgrade установлен
										if(options->http.is(http_t::suite_t::HEADER, "upgrade")){
											// Выполняем извлечение заголовка Upgrade
											const string & header = options->http.header("upgrade");
											// Если запрашиваемый протокол соответствует Websocket
											if(this->_webSocket && this->_fmk->compare(header, "websocket"))
												// Выполняем инициализацию Websocket-сервера
												this->websocket(bid, sid);
											// Если протокол запрещён или не поддерживается, выполняем закрытие подключения
											else rejectFn(bid);
											// Завершаем обработку
											goto Next;
										}
									}
									// Выполняем извлечение параметров запроса
									const auto & request = options->http.request();
									// Если функция обратного вызова на получение удачного запроса установлена
									if(this->_callbacks.is("handshake"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const int32_t, const uint64_t, const agent_t)> ("handshake", 1, bid, agent_t::HTTP);
									// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
									if(!options->http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", 1, bid, request.method, request.url, options->http.body());
									// Если функция обратного вызова на вывод полученных данных запроса клиента установлена
									if(this->_callbacks.is("complete"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", 1, bid, request.method, request.url, options->http.body(), options->http.headers());
									// Если функция обратного вызова активности потока установлена
									if(this->_callbacks.is("stream"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", 1, bid, mode_t::CLOSE);
									// Если установлена функция отлова завершения запроса
									if(this->_callbacks.is("end"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", 1, bid, direct_t::RECV);
									// Завершаем обработку
									goto Next;
								} break;
								// Если запрос неудачный
								case static_cast <uint8_t> (http_t::status_t::FAULT): {
									// Ответ на запрос об авторизации
									vector <char> response;
									// Выполняем очистку HTTP-парсера
									options->http.clear();
									// Выполняем сброс состояния HTTP-парсера
									options->http.reset();
									// Выполняем очистку буфера полученных данных
									options->buffer.clear();
									// Определяем идентичность сервера
									switch(static_cast <uint8_t> (this->_identity)){
										// Если сервер соответствует HTTP-серверу
										case static_cast <uint8_t> (http_t::identity_t::HTTP): {
											// Если подключение установленно не постоянное
											if(!alive)
												// Устанавливаем закрытие подключения
												options->http.header("Connection", "close");
											// Формируем запрос авторизации
											response = options->http.reject(awh::web_t::res_t(static_cast <uint32_t> (401)));
										} break;
										// Если сервер соответствует PROXY-серверу
										case static_cast <uint8_t> (http_t::identity_t::PROXY): {
											// Если подключение установленно не постоянное
											if(!alive){
												// Устанавливаем закрытие подключения
												options->http.header("Connection", "close");
												// Устанавливаем закрытие подключения
												options->http.header("Proxy-Connection", "close");
											}
											// Формируем запрос авторизации
											response = options->http.reject(awh::web_t::res_t(static_cast <uint32_t> (407)));
										} break;
									}
									// Если ответ получен
									if(!response.empty()){
										// Тело полезной нагрузки
										vector <char> payload;
										/**
										 * Если включён режим отладки
										 */
										#if defined(DEBUG_MODE)
											// Выводим заголовок ответа
											std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
											// Выводим параметры ответа
											std::cout << string(response.begin(), response.end()) << std::endl << std::endl << std::flush;
										#endif
										// Отправляем ответ брокеру
										const_cast <server::core_t *> (this->_core)->send(response.data(), response.size(), bid);
										// Получаем данные полезной нагрузки ответа
										while(!(payload = options->http.payload()).empty()){
											/**
											 * Если включён режим отладки
											 */
											#if defined(DEBUG_MODE)
												// Выводим сообщение о выводе чанка полезной нагрузки
												std::cout << this->_fmk->format("<chunk %zu>", payload.size()) << std::endl << std::endl << std::flush;
											#endif
											// Если тела данных для отправки больше не осталось
											if(options->http.empty(awh::http_t::suite_t::BODY))
												// Если подключение не установлено как постоянное, устанавливаем флаг завершения работы
												options->stopped = (!this->_service.alive && !options->alive && !options->http.is(http_t::state_t::ALIVE));
											// Отправляем тело ответа клиенту
											const_cast <server::core_t *> (this->_core)->send(payload.data(), payload.size(), bid);
										}
									// Выполняем отключение брокера
									} else const_cast <server::core_t *> (this->_core)->close(bid);
									// Если функция обратного вызова активности потока установлена
									if(this->_callbacks.is("stream"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", 1, bid, mode_t::CLOSE);
									// Если функция обратного вызова на на вывод ошибок установлена
									if(this->_callbacks.is("error"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", bid, log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "authorization failed");
									// Если установлена функция отлова завершения запроса
									if(this->_callbacks.is("end"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", 1, bid, direct_t::RECV);
									// Выходим из функции
									return;
								}
							}
						}
						// Устанавливаем метку продолжения обработки пайплайна
						Next:
						// Если парсер обработал какое-то количество байт
						if((bytes > 0) && !options->buffer.empty()){
							// Если размер буфера больше количества удаляемых байт
							if(options->buffer.size() >= bytes)
								// Удаляем количество обработанных байт
								options->buffer.erase(bytes);
							// Если байт в буфере меньше, просто очищаем буфер
							else options->buffer.clear();
							// Если данных для обработки не осталось, выходим
							if(options->buffer.empty())
								// Выходим из цикла
								break;
						// Если данных для обработки недостаточно, выходим
						} else break;
					}
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
void awh::server::Http1::writeEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0)){
		// Выполняем поиск агента которому соответствует клиент
		auto i = this->_agents.find(bid);
		// Если агент соответствует Websocket-у
		if((i != this->_agents.end()) && (i->second == agent_t::WEBSOCKET))
			// Выполняем передачу данных клиенту Websocket
			this->_ws1.writeEvents(buffer, size, bid, sid);
		// Иначе выполняем обработку входящих данных как Web-сервер
		else if(this->_core != nullptr) {
			// Получаем параметры активного клиента
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Если необходимо выполнить закрыть подключение
				if(!options->close && options->stopped){
					// Устанавливаем флаг закрытия подключения
					options->close = !options->close;
					// Принудительно выполняем отключение лкиента
					const_cast <server::core_t *> (this->_core)->close(bid);
				}
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
void awh::server::Http1::callbacksEvents(const fn_t::event_t event, const uint64_t idw, const string & name, const fn_t::dump_t * dump) noexcept {
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
					this->_ws1.callbacks(callbacks);
			}
		} break;
	}
}
/**
 * websocket Метод инициализации Websocket протокола
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::server::Http1::websocket(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
		// Создаём брокера
		this->_ws1._scheme.set(bid);
		// Получаем параметры активного клиента
		scheme::ws_t::options_t * options = const_cast <scheme::ws_t::options_t *> (this->_ws1._scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если данные необходимо зашифровать
			if(this->_encryption.mode){
				// Устанавливаем размер шифрования
				options->cipher = this->_encryption.cipher;
				// Устанавливаем соль шифрования
				options->hash.salt(this->_encryption.salt);
				// Устанавливаем пароль шифрования
				options->hash.password(this->_encryption.pass);
			}
			// Выполняем установку идентификатора объекта
			options->http.id(bid);
			// Устанавливаем флаг перехвата контекста компрессии
			options->server.takeover = this->_ws1._server.takeover;
			// Устанавливаем флаг перехвата контекста декомпрессии
			options->client.takeover = this->_ws1._client.takeover;
			// Разрешаем перехватывать контекст компрессии
			options->hash.takeoverCompress(this->_ws1._server.takeover);
			// Разрешаем перехватывать контекст декомпрессии
			options->hash.takeoverDecompress(this->_ws1._client.takeover);
			// Разрешаем перехватывать контекст для клиента
			options->http.takeover(awh::web_t::hid_t::CLIENT, this->_ws1._client.takeover);
			// Разрешаем перехватывать контекст для сервера
			options->http.takeover(awh::web_t::hid_t::SERVER, this->_ws1._server.takeover);
			// Устанавливаем данные сервиса
			options->http.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
			// Если сабпротоколы установлены
			if(!this->_ws1._subprotocols.empty())
				// Устанавливаем поддерживаемые сабпротоколы
				options->http.subprotocols(this->_ws1._subprotocols);
			// Если список расширений установлены
			if(!this->_ws1._extensions.empty())
				// Устанавливаем список поддерживаемых расширений
				options->http.extensions(this->_ws1._extensions);
			// Если размер фрейма установлен
			if(this->_ws1._frameSize > 0)
				// Выполняем установку размера фрейма
				options->frame.size = this->_ws1._frameSize;
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
			// Получаем параметры активного клиента
			scheme::web_t::options_t * web = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(web != nullptr){
				// Буфер данных для записи в сокет
				vector <char> buffer;
				// Выполняем установку параметров запроса
				options->http.request(web->http.request());
				// Выполняем установку полученных заголовков
				options->http.headers(web->http.headers());
				// Выполняем коммит полученного результата
				options->http.commit();
				// Метод компрессии данных
				http_t::compressor_t compressor = http_t::compressor_t::NONE;
				// Если рукопожатие выполнено
				if(options->http.handshake(http_t::process_t::REQUEST)){
					// Получаем метод компрессии HTML данных
					compressor = options->http.compression();
					// Проверяем версию протокола
					if(!options->http.check(ws_core_t::flag_t::VERSION)){
						// Получаем бинарные данные REST ответа
						buffer = web->http.reject(awh::web_t::res_t(static_cast <uint32_t> (505), "Unsupported protocol version"));
						// Завершаем работу
						goto End;
					}
					// Проверяем ключ брокера
					if(!options->http.check(ws_core_t::flag_t::KEY)){
						// Получаем бинарные данные REST ответа
						buffer = web->http.reject(awh::web_t::res_t(static_cast <uint32_t> (400), "Wrong client key"));
						// Завершаем работу
						goto End;
					}
					// Выполняем очистку HTTP-парсера
					options->http.clear();
					// Получаем флаг шифрованных данных
					options->crypted = options->http.crypted();
					// Если клиент согласился на шифрование данных
					if(this->_encryption.mode)
						// Устанавливаем параметры шифрования
						options->http.encryption(this->_encryption.pass, this->_encryption.salt, this->_encryption.cipher);
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
					if(!this->_ws1._headers.empty())
						// Выполняем установку HTTP-заголовков
						options->http.headers(this->_ws1._headers);
					// Получаем бинарные данные REST-ответа клиенту
					buffer = options->http.process(http_t::process_t::RESPONSE, awh::web_t::res_t(static_cast <uint32_t> (101)));
					// Если бинарные данные ответа получены
					if(!buffer.empty()){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок ответа
							std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
							// Выводим параметры ответа
							std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
						#endif
						// Выполняем замену активного агнета
						this->_agents.at(bid) = agent_t::WEBSOCKET;
						// Выполняем установку сетевого ядра
						this->_ws1._core = this->_core;
						// Выполняем отправку данных брокеру
						const_cast <server::core_t *> (this->_core)->send(buffer.data(), buffer.size(), bid);
						// Выполняем извлечение параметров запроса
						const auto & request = options->http.request();
						// Если функция обратного вызова активности потока установлена
						if(this->_callbacks.is("stream"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", options->sid, bid, mode_t::OPEN);
						// Если функция обратного вызова на получение удачного запроса установлена
						if(this->_callbacks.is("handshake"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const int32_t, const uint64_t, const agent_t)> ("handshake", options->sid, bid, agent_t::WEBSOCKET);
						// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
						if(!options->http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", options->sid, bid, request.method, request.url, options->http.body());
						// Если функция обратного вызова на вывод полученных данных запроса клиента установлена
						if(this->_callbacks.is("complete"))
							// Выполняем функцию обратного вызова
							this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", options->sid, bid, request.method, request.url, options->http.body(), options->http.headers());
						// Завершаем работу
						return;
					// Формируем ответ, что страница не доступна
					} else buffer = web->http.reject(awh::web_t::res_t(static_cast <uint32_t> (500)));
				// Сообщаем, что рукопожатие не выполнено
				} else buffer = web->http.reject(awh::web_t::res_t(static_cast <uint32_t> (403), "Handshake failed"));
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
						std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
						// Выводим параметры ответа
						std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
					#endif
					// Устанавливаем метод компрессии данных ответа
					web->http.compression(compressor);
					// Выполняем извлечение параметров запроса
					const auto & request = options->http.request();
					// Получаем параметры ответа
					const auto response = options->http.response();
					// Выполняем отправку заголовков сообщения
					const_cast <server::core_t *> (this->_core)->send(buffer.data(), buffer.size(), bid);
					// Получаем данные тела ответа
					while(!(payload = web->http.payload()).empty()){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение о выводе чанка полезной нагрузки
							std::cout << this->_fmk->format("<chunk %zu>", payload.size()) << std::endl << std::endl << std::flush;
						#endif
						// Если тела данных для отправки больше не осталось
						if(web->http.empty(awh::http_t::suite_t::BODY))
							// Если подключение не установлено как постоянное, устанавливаем флаг завершения работы
							options->stopped = (!this->_service.alive && !web->alive && !web->http.is(http_t::state_t::ALIVE));
						// Выполняем отправку ответа клиенту
						const_cast <server::core_t *> (this->_core)->send(payload.data(), payload.size(), bid);
					}
					// Если получение данных нужно остановить
					if(options->stopped)
						// Выполняем запрет на получение входящих данных
						const_cast <server::core_t *> (this->_core)->events(bid, awh::scheme_t::mode_t::DISABLED, engine_t::method_t::READ);
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(!web->http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", options->sid, bid, request.method, request.url, web->http.body());
					// Если функция обратного вызова на вывод полученных данных запроса клиента установлена
					if(this->_callbacks.is("complete"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", options->sid, bid, request.method, request.url, web->http.body(), web->http.headers());
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
					// Выполняем очистку фрагментированных сообщений
					options->buffer.fragmes.clear();
					// Если размер выделенной памяти выше максимального размера буфера
					if(options->buffer.fragmes.capacity() > AWH_BUFFER_SIZE)
						// Выполняем очистку временного буфера данных
						vector <char> ().swap(options->buffer.fragmes);
					// Завершаем работу
					return;
				}
				// Завершаем работу
				const_cast <server::core_t *> (this->_core)->close(bid);
			}
		}
	}
}
/**
 * erase Метод удаления отключившихся брокеров
 * @param bid идентификатор брокера
 */
void awh::server::Http1::erase(const uint64_t bid) noexcept {
	// Если список отключившихся брокеров не пустой
	if(!this->_disconected.empty()){
		/**
		 * eraseFn Функция удаления отключившегося брокера
		 * @param bid идентификатор брокера
		 */
		auto eraseFn = [this](const uint64_t bid) noexcept -> void {
			// Выполняем поиск агента которому соответствует клиент
			auto i = this->_agents.find(bid);
			// Если агент найден в списке активных агентов
			if(i != this->_agents.end()){
				// Если агент соответствует серверу Websocket
				if(i->second == agent_t::WEBSOCKET)
					// Выполняем удаление отключённого брокера
					this->_ws1.erase(i->first);
				// Выполняем удаление активного агента
				this->_agents.erase(i);
			}
			// Получаем параметры активного клиента
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Устанавливаем флаг отключения
				options->close = true;
				// Выполняем очистку оставшихся данных
				options->buffer.clear();
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
 * pinging Метод таймера выполнения пинга клиента
 * @param tid идентификатор таймера
 */
void awh::server::Http1::pinging(const uint16_t tid) noexcept {
	// Если данные существуют
	if((tid > 0) && (this->_core != nullptr)){
		// Если разрешено выполнять пинги
		if(this->_pinging){
			// Выполняем перебор всех активных агентов
			for(auto & agent : this->_agents){
				// Определяем тип активного агента
				switch(static_cast <uint8_t> (agent.second)){
					// Если агент соответствует серверу HTTP
					case static_cast <uint8_t> (agent_t::HTTP): {
						// Получаем параметры активного клиента
						scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(agent.first));
						// Если параметры активного клиента получены
						if((options != nullptr) && ((!options->alive && !this->_service.alive) || options->close)){
							// Если брокер давно должен был быть отключён, отключаем его
							if(options->close || !options->http.is(http_t::state_t::ALIVE))
								// Выполняем отключение клиента от сервера
								const_cast <server::core_t *> (this->_core)->close(agent.first);
						}
					} break;
					// Если агент соответствует серверу Websocket
					case static_cast <uint8_t> (agent_t::WEBSOCKET):
						// Выполняем передачу данных клиенту Websocket
						this->_ws1.pinging(tid);
					break;
				}
			}
		}
	}
}
/**
 * parser Метод извлечения объекта HTTP-парсера
 * @param bid идентификатор брокера
 * @return    объект HTTP-парсера
 */
const awh::http_t * awh::server::Http1::parser(const uint64_t bid) const noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto i = this->_agents.find(bid);
		// Если агент соответствует HTTP-протоколу
		if((i == this->_agents.end()) || (i->second == agent_t::HTTP)){
			// Получаем параметры активного клиента
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr)
				// Выполняем получение объекта HTTP-парсера
				return &options->http;
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
bool awh::server::Http1::trailers(const uint64_t bid) const noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto i = this->_agents.find(bid);
		// Если агент соответствует HTTP-протоколу
		if((i == this->_agents.end()) || (i->second == agent_t::HTTP)){
			// Получаем параметры активного клиента
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr)
				// Выполняем получение флага запроса клиента на передачу трейлеров
				return options->http.is(http_t::state_t::TRAILERS);
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
void awh::server::Http1::trailer(const uint64_t bid, const string & key, const string & val) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto i = this->_agents.find(bid);
		// Если агент соответствует HTTP-протоколу
		if((i == this->_agents.end()) || (i->second == agent_t::HTTP)){
			// Получаем параметры активного клиента
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr)
				// Выполняем установку трейлера
				options->http.trailer(key, val);
		}
	}
}
/**
 * init Метод инициализации WEB-сервера
 * @param socket      unix-сокет для биндинга
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Http1::init(const string & socket, const vector <http_t::compressor_t> & compressors) noexcept {
	// Устанавливаем список поддерживаемых компрессоров
	this->_scheme.compressors = compressors;
	// Выполняем инициализацию родительского объекта
	web_t::init(socket, compressors);
}
/**
 * init Метод инициализации WEB-сервера
 * @param port        порт сервера
 * @param host        хост сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Http1::init(const uint32_t port, const string & host, const vector <http_t::compressor_t> & compressors) noexcept {
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
void awh::server::Http1::sendError(const uint64_t bid, const ws::mess_t & mess) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto i = this->_agents.find(bid);
		// Если агент соответствует Websocket-у
		if((i != this->_agents.end()) && (i->second == agent_t::WEBSOCKET))
			// Выполняем отправку ошибки клиенту Websocket
			this->_ws1.sendError(bid, mess);
	}
}
/**
 * sendMessage Метод отправки сообщения клиенту
 * @param bid     идентификатор брокера
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::server::Http1::sendMessage(const uint64_t bid, const vector <char> & message, const bool text) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto i = this->_agents.find(bid);
		// Если агент соответствует Websocket-у
		if((i != this->_agents.end()) && (i->second == agent_t::WEBSOCKET))
			// Выполняем передачу данных клиенту Websocket
			return this->_ws1.sendMessage(bid, message, text);
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
bool awh::server::Http1::sendMessage(const uint64_t bid, const char * message, const size_t size, const bool text) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto i = this->_agents.find(bid);
		// Если агент соответствует Websocket-у
		if((i != this->_agents.end()) && (i->second == agent_t::WEBSOCKET))
			// Выполняем передачу данных клиенту Websocket
			return this->_ws1.sendMessage(bid, message, size, text);
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
bool awh::server::Http1::send(const uint64_t bid, const char * buffer, const size_t size) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0))
		// Выполняем отправку заголовков ответа клиенту
		return const_cast <server::core_t *> (this->_core)->send(buffer, size, bid);
	// Сообщаем что ничего не найдено
	return false;
}
/**
 * send Метод отправки тела сообщения клиенту
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых клиенту
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::Http1::send(const uint64_t bid, const char * buffer, const size_t size, const bool end) noexcept {
	// Результат работы функции
	bool result = false;
	// Если данные переданы верные
	if((result = ((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0)))){
		// Выполняем поиск агента которому соответствует клиент
		auto i = this->_agents.find(bid);
		// Если агент соответствует HTTP-протоколу
		if((i == this->_agents.end()) || (i->second == agent_t::HTTP)){
			// Получаем параметры активного клиента
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Тело WEB сообщения
				vector <char> entity;
				// Выполняем очистку данных тела
				options->http.clear(http_t::suite_t::BODY);
				// Устанавливаем тело запроса
				options->http.body(vector <char> (buffer, buffer + size));
				// Получаем данные тела полезной нагрузки
				while(!(entity = options->http.payload()).empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка тела
						std::cout << this->_fmk->format("<chunk %zu>", entity.size()) << std::endl << std::endl << std::flush;
					#endif
					// Устанавливаем флаг закрытия подключения
					options->stopped = (end && options->http.empty(awh::http_t::suite_t::BODY) && (options->http.trailers() == 0));
					// Выполняем отправку ответа клиенту
					const_cast <server::core_t *> (this->_core)->send(entity.data(), entity.size(), bid);
				}
				// Если список трейлеров установлен
				if(options->http.trailers() > 0){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим заголовок трейлеров
						std::cout << "<Trailers>" << std::endl << std::endl << std::flush;
					#endif
					// Получаем отправляемые трейлеры
					while(!(entity = options->http.trailer()).empty()){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение о выводе чанка тела
							std::cout << this->_fmk->format("%s", string(entity.begin(), entity.end()).c_str()) << std::flush;
						#endif
						// Устанавливаем флаг закрытия подключения
						options->stopped = (end && (options->http.trailers() == 0));
						// Выполняем отправку трейлера клиенту
						const_cast <server::core_t *> (this->_core)->send(entity.data(), entity.size(), bid);
					}
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим завершение вывода информации
						std::cout << std::endl << std::endl << std::flush;
					#endif
				}
			}
		}
	}
	// Выводим значение по умолчанию
	return result;
}
/**
 * send Метод отправки заголовков клиенту
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param headers заголовки отправляемые клиенту
 * @param end     размер сообщения в байтах
 * @return        идентификатор нового запроса
 */
int32_t awh::server::Http1::send(const uint64_t bid, const uint32_t code, const string & mess, const unordered_multimap <string, string> & headers, const bool end) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если заголовки запроса переданы
	if((result = ((this->_core != nullptr) && this->_core->working() && !headers.empty()))){
		// Выполняем поиск агента которому соответствует клиент
		auto i = this->_agents.find(bid);
		// Если агент соответствует HTTP-протоколу
		if((i == this->_agents.end()) || (i->second == agent_t::HTTP)){
			// Получаем параметры активного клиента
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
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
				awh::web_t::res_t response(code, mess);
				// Получаем заголовки ответа удалённому клиенту
				const auto & headers = options->http.process(http_t::process_t::RESPONSE, response);
				// Если заголовки запроса получены
				if(!headers.empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим заголовок запроса
						std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
						// Выводим параметры запроса
						std::cout << string(headers.begin(), headers.end()) << std::endl << std::endl << std::flush;
					#endif
					// Устанавливаем флаг закрытия подключения
					options->stopped = end;
					// Выполняем отправку заголовков запроса на сервер
					const_cast <server::core_t *> (this->_core)->send(headers.data(), headers.size(), bid);
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
 * send Метод отправки сообщения брокеру
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param buffer  данные полезной нагрузки (тело сообщения)
 * @param size    размер данных полезной нагрузки (размер тела сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Http1::send(const uint64_t bid, const uint32_t code, const string & mess, const char * buffer, const size_t size, const unordered_multimap <string, string> & headers) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск агента которому соответствует клиент
		auto i = this->_agents.find(bid);
		// Если агент соответствует HTTP-протоколу
		if((i == this->_agents.end()) || (i->second == agent_t::HTTP)){
			// Получаем параметры активного клиента
			scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
			// Если параметры активного клиента получены
			if(options != nullptr){
				// Тело полезной нагрузки
				vector <char> payload;
				// Получаем флаг постоянного подключения
				const bool alive = options->http.is(http_t::state_t::ALIVE);
				// Выполняем сброс состояния HTTP-парсера
				options->http.reset();
				// Выполняем очистку буфера полученных данных
				options->buffer.clear();
				// Выполняем очистку данных тела
				options->http.clear(http_t::suite_t::BODY);
				// Выполняем очистку заголовков
				options->http.clear(http_t::suite_t::HEADER);
				// Устанавливаем заголовки ответа
				options->http.headers(headers);
				// Устанавливаем полезную нагрузку
				options->http.body(buffer, size);
				// Если подключение установленно не постоянное
				if(!alive){
					// Определяем идентичность сервера
					switch(static_cast <uint8_t> (this->_identity)){
						// Если сервер соответствует HTTP-серверу
						case static_cast <uint8_t> (http_t::identity_t::HTTP): {
							// Если заголовок подключения не переопределён
							if(!options->http.is(http_t::suite_t::HEADER, "Connection"))
								// Устанавливаем закрытие подключения
								options->http.header("Connection", "close");
						} break;
						// Если сервер соответствует PROXY-серверу
						case static_cast <uint8_t> (http_t::identity_t::PROXY): {
							// Если заголовок подключения не переопределён
							if(!options->http.is(http_t::suite_t::HEADER, "Proxy-Connection"))
								// Устанавливаем закрытие подключения
								options->http.header("Proxy-Connection", "close");
						} break;
					}
				}
				// Если подключение не установлено как постоянное, но подключение долгоживущее
				if((this->_scheme.timeouts.wait > 0) && !this->_service.alive && !options->alive && options->http.is(http_t::state_t::ALIVE))
					// Указываем сколько запросов разрешено выполнить за указанный интервал времени
					options->http.header("Keep-Alive", this->_fmk->format("timeout=%d, max=%d", this->_scheme.timeouts.wait, this->_maxRequests));
				// Если сообщение ответа не установлено
				if(mess.empty())
					// Выполняем установку сообщения по умолчанию
					const_cast <string &> (mess) = options->http.message(code);
				// Формируем запрос авторизации
				const auto & response = options->http.process(http_t::process_t::RESPONSE, awh::web_t::res_t(static_cast <uint32_t> (code), mess));
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
					// Выводим параметры ответа
					std::cout << string(response.begin(), response.end()) << std::endl << std::endl << std::flush;
				#endif
				// Если тело данных не установлено для отправки
				if(options->http.empty(awh::http_t::suite_t::BODY))
					// Если подключение не установлено как постоянное, устанавливаем флаг завершения работы
					options->stopped = (!this->_service.alive && !options->alive && !options->http.is(http_t::state_t::ALIVE));
				// Отправляем серверу сообщение
				const_cast <server::core_t *> (this->_core)->send(response.data(), response.size(), bid);
				// Если код ответа содержит тело ответа
				if((code >= 200) && !options->http.empty(awh::http_t::suite_t::BODY)){
					// Получаем данные тела полезной нагрузки
					while(!(payload = options->http.payload()).empty()){
						// Если включён режим отладки
						#if defined(DEBUG_MODE)
							// Выводим сообщение о выводе чанка полезной нагрузки
							std::cout << this->_fmk->format("<chunk %zu>", payload.size()) << std::endl << std::endl << std::flush;
						#endif
						// Если тела данных для отправки больше не осталось
						if(options->http.empty(awh::http_t::suite_t::BODY) && (options->http.trailers() == 0))
							// Если подключение не установлено как постоянное, устанавливаем флаг завершения работы
							options->stopped = (!this->_service.alive && !options->alive && !options->http.is(http_t::state_t::ALIVE));
						// Отправляем тело ответа клиенту
						const_cast <server::core_t *> (this->_core)->send(payload.data(), payload.size(), bid);
					}
					// Если список трейлеров установлен
					if(options->http.trailers() > 0){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок трейлеров
							std::cout << "<Trailers>" << std::endl << std::endl << std::flush;
						#endif
						// Получаем отправляемые трейлеры
						while(!(payload = options->http.trailer()).empty()){
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим сообщение о выводе чанка тела
								std::cout << this->_fmk->format("%s", string(payload.begin(), payload.end()).c_str()) << std::flush;
							#endif
							// Если все трейлеры были отправлены
							if(options->http.trailers() == 0)
								// Устанавливаем флаг закрытия подключения
								options->stopped = (!this->_service.alive && !options->alive && !options->http.is(http_t::state_t::ALIVE));
							// Выполняем отправку трейлера клиенту
							const_cast <server::core_t *> (this->_core)->send(payload.data(), payload.size(), bid);
						}
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим завершение вывода информации
							std::cout << std::endl << std::endl << std::flush;
						#endif
					}
					// Если установлена функция отлова завершения запроса
					if(this->_callbacks.is("end"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", 1, bid, direct_t::SEND);
				}
			}
		}
	}
}
/**
 * send Метод отправки сообщения брокеру
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Http1::send(const uint64_t bid, const uint32_t code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Если тело сообщения передано
		if(!entity.empty())
			// Выполняем отправку ответа с телом сообщения
			this->send(bid, code, mess, entity.data(), entity.size(), headers);
		// Выполняем отправку ответа без тела сообщения
		else this->send(bid, code, mess, nullptr, 0, headers);
	}
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::server::Http1::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем добавление функций обратного вызова в основноной модуль
	web_t::callbacks(callbacks);
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
		if(!callbacks.empty())
			// Выполняем установку функций обратного вызова для Websocket-сервера
			this->_ws1.callbacks(callbacks);
	}
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
uint32_t awh::server::Http1::port(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.port(bid);
}
/**
 * agent Метод извлечения агента клиента
 * @param bid идентификатор брокера
 * @return    агент к которому относится подключённый клиент
 */
awh::server::web_t::agent_t awh::server::Http1::agent(const uint64_t bid) const noexcept {
	// Выполняем поиск нужного нам агента
	auto i = this->_agents.find(bid);
	// Если агент клиента найден
	if(i != this->_agents.end())
		// Выводим идентификатор агента
		return i->second;
	// Выводим сообщение, что ничего не найдено
	return agent_t::HTTP;
}
/**
 * ip Метод получения IP-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::Http1::ip(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::Http1::mac(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.mac(bid);
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
void awh::server::Http1::close(const uint64_t bid) noexcept {
	// Выполняем поиск агента которому соответствует клиент
	auto i = this->_agents.find(bid);
	// Если агент соответствует Websocket-у
	if((i != this->_agents.end()) && (i->second == agent_t::WEBSOCKET))
		// Выполняем закрытие подключения клиента Websocket
		this->_ws1.close(bid);
	// Иначе выполняем обработку входящих данных как Web-сервер
	else if(this->_core != nullptr) {
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
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
 * waitPong Метод установки времени ожидания ответа WebSocket-клиента
 * @param sec время ожидания в секундах
 */
void awh::server::Http1::waitPong(const uint16_t sec) noexcept {
	// Выполняем установку времени ожидания
	this->_ws1.waitPong(sec);
}
/**
 * pingInterval Метод установки интервала времени выполнения пингов
 * @param sec интервал времени выполнения пингов в секундах
 */
void awh::server::Http1::pingInterval(const uint16_t sec) noexcept {
	// Выполняем установку интервала времени выполнения пингов в секундах
	this->_ws1.pingInterval(sec);
	// Выполняем установку интервала времени выполнения пингов в секундах
	this->_pingInterval = (static_cast <uint32_t> (sec) * 1000);
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
 * @param bid идентификатор брокера
 * @return    список выбранных сабпротоколов
 */
const set <string> & awh::server::Http1::subprotocols(const uint64_t bid) const noexcept {
	// Выполняем извлечение выбранных сабпротоколов
	return this->_ws1.subprotocols(bid);
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
 * @param bid идентификатор брокера
 * @return    список поддерживаемых расширений
 */
const vector <vector <string>> & awh::server::Http1::extensions(const uint64_t bid) const noexcept {
	// Выполняем извлечение списка поддерживаемых расширений
	return this->_ws1.extensions(bid);
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
			this->_ws1._thr.stop();
			// Выполняем инициализацию нового тредпула
			this->_ws1._thr.init(count);
		}
		// Если сетевое ядро установлено
		if(this->_core != nullptr)
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->_ws1._thr.stop();
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Http1::total(const uint16_t total) noexcept {
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr)
		// Устанавливаем максимальное количество одновременных подключений
		const_cast <server::core_t *> (this->_core)->total(this->_scheme.id, total);
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
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Http1::compressors(const vector <http_t::compressor_t> & compressors) noexcept {
	// Устанавливаем список поддерживаемых компрессоров
	this->_scheme.compressors = compressors;
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Http1::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
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
	// Устанавливаем флаги настроек модуля для Websocket-сервера
	this->_ws1.mode(flags);
	// Активируем выполнение пинга
	this->_pinging = (flags.find(flag_t::NOT_PING) == flags.end());
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_complete = (flags.find(flag_t::NOT_STOP) == flags.end());
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.find(flag_t::ALIVE) != flags.end());
	// Устанавливаем флаг разрешающий выполнять подключение к протоколу Websocket
	this->_webSocket = (flags.find(flag_t::WEBSOCKET_ENABLE) != flags.end());
	// Устанавливаем флаг разрешающий выполнять метод CONNECT для сервера
	this->_methodConnect = (flags.find(flag_t::CONNECT_METHOD_ENABLE) != flags.end());
	// Если сетевое ядро установлено
	if(this->_core != nullptr)
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <server::core_t *> (this->_core)->verbose(flags.find(flag_t::NOT_INFO) == flags.end());
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
 * alive Метод установки долгоживущего подключения
 * @param bid  идентификатор брокера
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Http1::alive(const uint64_t bid, const bool mode) noexcept {
	// Получаем параметры активного клиента
	scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены
	if(options != nullptr)
		// Устанавливаем флаг пдолгоживущего подключения
		options->alive = mode;
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::server::Http1::core(const server::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку сетевого ядра
		web_t::core(core);
		// Добавляем схемы сети в сетевое ядро
		const_cast <server::core_t *> (this->_core)->scheme(&this->_scheme);
		// Если многопоточность активированна
		if(this->_ws1._thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <server::core_t *> (this->_core)->easily(true);
		// Устанавливаем событие на запуск системы
		const_cast <server::core_t *> (this->_core)->callback <void (const uint16_t)> ("open", std::bind(&http1_t::openEvents, this, _1));
		// Устанавливаем событие подключения
		const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("connect", std::bind(&http1_t::connectEvents, this, _1, _2));
		// Устанавливаем событие отключения
		const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("disconnect", std::bind(&http1_t::disconnectEvents, this, _1, _2));
		// Устанавливаем функцию чтения данных
		const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", std::bind(&http1_t::readEvents, this, _1, _2, _3, _4));
		// Устанавливаем функцию записи данных
		const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&http1_t::writeEvents, this, _1, _2, _3, _4));
		// Добавляем событие аццепта брокера
		const_cast <server::core_t *> (this->_core)->callback <bool (const string &, const string &, const uint32_t, const uint64_t)> ("accept", std::bind(&http1_t::acceptEvents, this, _1, _2, _3, _4));
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_ws1._thr.is()){
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
 * identity Метод установки идентичности протокола модуля
 * @param identity идентичность протокола модуля
 */
void awh::server::Http1::identity(const http_t::identity_t identity) noexcept {
	// Устанавливаем флаг идентичности протокола модуля
	this->_identity = identity;
}
/**
 * waitMessage Метод ожидания входящих сообщений
 * @param sec интервал времени в секундах
 */
void awh::server::Http1::waitMessage(const uint16_t sec) noexcept {
	// Устанавливаем время ожидания получения данных
	this->_scheme.timeouts.wait = sec;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Http1::waitTimeDetect(const uint16_t read, const uint16_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Http1::maxRequests(const uint32_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	this->_maxRequests = max;
}
/**
 * crypted Метод получения флага шифрования
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::server::Http1::crypted(const uint64_t bid) const noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
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
void awh::server::Http1::encrypt(const uint64_t bid, const bool mode) noexcept {
	// Если активированно шифрование обмена сообщениями
	if(this->_encryption.mode){
		// Получаем параметры активного клиента
		scheme::web_t::options_t * options = const_cast <scheme::web_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Устанавливаем флаг шифрования для клиента
			options->crypted = mode;
			// Устанавливаем флаг шифрования
			options->http.encryption(options->crypted);
		}
	}
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::server::Http1::encryption(const bool mode) noexcept {
	// Устанавливаем флага шифрования
	web_t::encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Http1::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования
	web_t::encryption(pass, salt, cipher);
}
/**
 * Http1 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Http1::Http1(const fmk_t * fmk, const log_t * log) noexcept :
 web_t(fmk, log), _webSocket(false), _methodConnect(false),
 _maxRequests(SERVER_MAX_REQUESTS), _identity(http_t::identity_t::HTTP), _ws1(fmk, log), _scheme(fmk, log) {}
/**
 * Http1 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Http1::Http1(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web_t(core, fmk, log), _webSocket(false), _methodConnect(false),
 _maxRequests(SERVER_MAX_REQUESTS), _identity(http_t::identity_t::HTTP), _ws1(fmk, log), _scheme(fmk, log) {
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->scheme(&this->_scheme);
	// Устанавливаем событие на запуск системы
	const_cast <server::core_t *> (this->_core)->callback <void (const uint16_t)> ("open", std::bind(&http1_t::openEvents, this, _1));
	// Устанавливаем событие подключения
	const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("connect", std::bind(&http1_t::connectEvents, this, _1, _2));
	// Устанавливаем событие отключения
	const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("disconnect", std::bind(&http1_t::disconnectEvents, this, _1, _2));
	// Устанавливаем функцию чтения данных
	const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", std::bind(&http1_t::readEvents, this, _1, _2, _3, _4));
	// Устанавливаем функцию записи данных
	const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&http1_t::writeEvents, this, _1, _2, _3, _4));
	// Добавляем событие аццепта брокера
	const_cast <server::core_t *> (this->_core)->callback <bool (const string &, const string &, const uint32_t, const uint64_t)> ("accept", std::bind(&http1_t::acceptEvents, this, _1, _2, _3, _4));
}
