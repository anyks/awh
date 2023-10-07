/**
 * @file: ws2.cpp
 * @date: 2023-09-14
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

// Подключаем заголовочный файл
#include <client/web/ws2.hpp>

/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket2::connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ}, event_t::CONNECT)){
		// Выполняем сброс параметров запроса
		this->flush();
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Выполняем сброс состояния HTTP парсера
		this->_http.reset();
		// Выполняем очистку параметров HTTP запроса
		this->_http.clear();
		// Выполняем установку идентификатора объекта
		this->_http.id(aid);
		// Выполняем очистку функций обратного вызова
		this->_resultCallback.clear();
		// Если HTTP-заголовки установлены
		if(!this->_headers.empty())
			// Выполняем установку HTTP-заголовков
			this->_http.headers(this->_headers);
		// Устанавливаем метод сжатия
		this->_http.compress(this->_compress);
		// Разрешаем перехватывать контекст компрессии
		this->_hash.takeoverCompress(this->_client.takeover);
		// Разрешаем перехватывать контекст декомпрессии
		this->_hash.takeoverDecompress(this->_server.takeover);
		// Разрешаем перехватывать контекст для клиента
		this->_http.takeover(awh::web_t::hid_t::CLIENT, this->_client.takeover);
		// Разрешаем перехватывать контекст для сервера
		this->_http.takeover(awh::web_t::hid_t::SERVER, this->_server.takeover);
		// Выполняем инициализацию сессии HTTP/2
		web2_t::connectCallback(aid, sid, core);
		// Если флаг инициализации сессии HTTP2 установлен
		if(this->_sessionInitialized){
			// Выполняем переключение протокола интернета на HTTP/2
			this->_proto = engine_t::proto_t::HTTP2;
			// Создаём объек запроса
			awh::web_t::req_t query(2.0f, awh::web_t::method_t::CONNECT, this->_scheme.url);
			// Если метод CONNECT запрещён для прокси-сервера
			if(!this->_proxy.connect){
				// Получаем строку авторизации на проксе-сервере
				const string & auth = this->_scheme.proxy.http.getAuth(http_t::process_t::REQUEST, awh::web_t::method_t::CONNECT);
				// Если строка автоирации получена
				if(!auth.empty())
					// Выполняем добавление заголовка авторизации
					this->_http.header("Proxy-Authorization", auth);
			}
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим заголовок запроса
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
				// Получаем бинарные данные REST запроса
				const auto & buffer = this->_http.process(http_t::process_t::REQUEST, query);
				// Если бинарные данные запроса получены
				if(!buffer.empty())
					// Выводим параметры запроса
					cout << string(buffer.begin(), buffer.end()) << endl << endl;
			#endif
			// Выполняем запрос на получение заголовков
			const auto & headers = this->_http.process2(http_t::process_t::REQUEST, std::move(query));
			// Выполняем запрос на удалённый сервер	
			this->_sid = web2_t::send(-1, headers, false);
			// Если запрос не получилось отправить
			if(this->_sid < 0)
				// Выходим из функции
				return;
		// Если активирован режим работы с HTTP/1.1 протоколом
		} else {
			// Устанавливаем идентификатор потока
			this->_sid = 1;
			// Выполняем установку идентификатора объекта
			this->_ws1._http.id(aid);
			// Выполняем установку сетевого ядра
			this->_ws1._core = this->_core;
			// Устанавливаем метод сжатия
			this->_ws1._compress = this->_compress;
			// Выполняем установку данных URL-адреса
			this->_ws1._scheme.url = this->_scheme.url;
			// Если HTTP-заголовки установлены
			if(!this->_headers.empty())
				// Выполняем установку HTTP-заголовков
				this->_ws1.setHeaders(this->_headers);
			// Если функция обратного вызова, для вывода полученного чанка бинарных данных с сервера установлена
			if(this->_callback.is("chunks"))
				// Выполняем установку функции обратного вызова
				this->_ws1._callback.set <void (const int32_t, const vector <char> &)> ("chunks", this->_callback.get <void (const int32_t, const vector <char> &)> ("chunks"));
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
			this->_callback.call <const mode_t> ("active", mode_t::CONNECT);
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket2::disconnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Выполняем сброс идентификатора потока
	this->_sid = -1;
	// Если флаг инициализации сессии HTTP2 установлен
	if(this->_sessionInitialized){
		// Выполняем закрытие подключения
		this->_nghttp2.close();
		// Выполняем снятие флага инициализации сессии HTTP2
		this->_sessionInitialized = !this->_sessionInitialized;
	}
	// Выполняем редирект, если редирект выполнен
	if(this->redirect(aid, sid, core))
		// Выходим из функции
		return;
	// Если подключение является постоянным
	if(this->_scheme.alive){
		// Выполняем очистку оставшихся данных
		this->_buffer.clear();
		// Выполняем очистку оставшихся фрагментов
		this->_fragmes.clear();
	// Если подключение не является постоянным
	} else {
		// Выполняем сброс параметров запроса
		this->flush();
		// Выполняем зануление идентификатора адъютанта
		this->_aid = 0;
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Если завершить работу разрешено
		if(this->_unbind)
			// Завершаем работу
			dynamic_cast <client::core_t *> (core)->stop();
	}
	// Выполняем переключение протокола интернета обратно на HTTP/1.1
	this->_proto = engine_t::proto_t::HTTP1_1;
	// Если функция обратного вызова при подключении/отключении установлена
	if(this->_callback.is("active"))
		// Выводим функцию обратного вызова
		this->_callback.call <const mode_t> ("active", mode_t::DISCONNECT);
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::WebSocket2::readCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Если подключение закрыто
		if(this->_close){
			// Принудительно выполняем отключение лкиента
			dynamic_cast <client::core_t *> (core)->close(aid);
			// Выходим из функции
			return;
		}
		// Если протокол подключения является HTTP/2
		if(core->proto(aid) == engine_t::proto_t::HTTP2){
			// Если получение данных не разрешено
			if(!this->_allow.receive)
				// Выходим из функции
				return;
			// Выполняем извлечение полученного чанка данных из сокета
			ssize_t bytes = nghttp2_session_mem_recv(this->_nghttp2.session, (const uint8_t *) buffer, size);
			// Если данные не прочитаны, выводим ошибку и выходим
			if(bytes < 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, nghttp2_strerror(static_cast <int> (bytes)));
				// Выходим из функции
				return;
			}
			// Фиксируем полученный результат
			if((bytes = nghttp2_session_send(this->_nghttp2.session)) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(static_cast <int> (bytes)));
				// Выходим из функции
				return;
			}
		// Если активирован режим работы с HTTP/1.1 протоколом, выполняем переброс вызова чтения на клиент WebSocket
		} else this->_ws1.readCallback(buffer, size, aid, sid, core);
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
void awh::client::WebSocket2::writeCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Если переключение протокола на HTTP/2 не выполнено
		if(this->_proto != engine_t::proto_t::HTTP2)
			// Выполняем переброс вызова записи на клиент WebSocket
			this->_ws1.writeCallback(buffer, size, aid, sid, core);
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket2::persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Если переключение протокола на HTTP/2 не выполнено
		if(this->_proto != engine_t::proto_t::HTTP2)
			// Выполняем переброс персистентного вызова на клиент WebSocket
			this->_ws1.persistCallback(aid, sid, core);
		// Если переключение протокола на HTTP/2 выполнено
		else {
			// Получаем текущий штамп времени
			const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
			// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
			if(this->_close || ((stamp - this->_point) >= (PERSIST_INTERVAL * 5)))
				// Завершаем работу
				dynamic_cast <client::core_t *> (core)->close(aid);
			// Отправляем запрос адъютанту
			else this->ping(to_string(aid));
		}
	}
}
/**
 * chunkSignal Метод обратного вызова при получении чанка с сервера HTTP/2
 * @param sid    идентификатор потока
 * @param buffer буфер данных который содержит полученный чанк
 * @param size   размер полученного буфера данных чанка
 * @return       статус полученных данных
 */
int awh::client::WebSocket2::chunkSignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept {
	// Если подключение производится через, прокси-сервер
	if(this->_scheme.isProxy())
		// Выполняем обработку полученных данных чанка для прокси-сервера
		return this->chunkProxySignal(sid, buffer, size);
	// Если мы работаем с сервером напрямую
	else {
		// Если идентификатор сессии клиента совпадает
		if(this->_sid == sid){
			// Если функция обратного вызова на перехват входящих чанков установлена
			if(this->_callback.is("chunking"))
				// Выводим функцию обратного вызова
				this->_callback.call <const vector <char> &, const awh::http_t *> ("chunking", vector <char> (buffer, buffer + size), &this->_http);
			// Если функция перехвата полученных чанков не установлена
			else {
				// Если подключение закрыто
				if(this->_close){
					// Принудительно выполняем отключение лкиента
					const_cast <client::core_t *> (this->_core)->close(this->_aid);
					// Выходим из функции
					return 0;
				}
				// Если рукопожатие не выполнено
				if(!this->_shake)
					// Добавляем полученный чанк в тело данных
					this->_http.body(vector <char> (buffer, buffer + size));
				// Если рукопожатие выполнено
				else if(this->_allow.receive)
					// Добавляем полученные данные в буфер
					this->_buffer.insert(this->_buffer.end(), buffer, buffer + size);
				// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
				if(this->_callback.is("chunks"))
					// Выводим функцию обратного вызова
					this->_callback.call <const int32_t, const vector <char> &> ("chunks", sid, vector <char> (buffer, buffer + size));
			}
		}
	}
	// Выводим результат
	return 0;
}
/**
 * frameSignal Метод обратного вызова при получении фрейма заголовков сервера HTTP/2
 * @param sid    идентификатор потока
 * @param direct направление передачи фрейма
 * @param type   тип полученного фрейма
 * @param flags  флаг полученного фрейма
 * @return       статус полученных данных
 */
int awh::client::WebSocket2::frameSignal(const int32_t sid, const nghttp2_t::direct_t direct, const uint8_t type, const uint8_t flags) noexcept {
	// Определяем направление передачи фрейма
	switch(static_cast <uint8_t> (direct)){
		// Если производится передача фрейма на сервер
		case static_cast <uint8_t> (nghttp2_t::direct_t::SEND): {
			// Если мы получили флаг завершения потока
			if(flags & NGHTTP2_FLAG_END_STREAM){
				// Если необходимо выполнить закрыть подключение
				if(!this->_close && this->_stopped){
					// Устанавливаем флаг закрытия подключения
					this->_close = !this->_close;
					// Если флаг инициализации сессии HTTP2 установлен
					if(this->_sessionInitialized){
						// Выполняем закрытие подключения
						this->_nghttp2.close();
						// Выполняем снятие флага инициализации сессии HTTP2
						this->_sessionInitialized = !this->_sessionInitialized;
					}
					// Принудительно выполняем отключение лкиента
					const_cast <client::core_t *> (this->_core)->close(this->_aid);
				}
				// Если установлена функция отлова завершения запроса
				if(this->_callback.is("end"))
					// Выводим функцию обратного вызова
					this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::SEND);
			}
		} break;
		// Если производится получения фрейма с сервера
		case static_cast <uint8_t> (nghttp2_t::direct_t::RECV): {
			// Если подключение производится через, прокси-сервер
			if(this->_scheme.isProxy())
				// Выполняем обработку полученных данных фрейма для прокси-сервера
				return this->frameProxySignal(sid, direct, type, flags);
			// Если мы работаем с сервером напрямую
			else {
				// Выполняем определение типа фрейма
				switch(type){
					// Если мы получили входящие данные тела ответа
					case NGHTTP2_DATA: {
						// Если рукопожатие не выполнено
						if(!this->_shake){
							// Если мы получили флаг завершения потока
							if(flags & NGHTTP2_FLAG_END_STREAM){
								// Выполняем коммит полученного результата
								this->_http.commit();
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									{
										// Получаем объект работы с HTTP-запросами
										const http_t & http = reinterpret_cast <http_t &> (this->_http);
										// Если тело ответа существует
										if(!http.body().empty())
											// Выводим сообщение о выводе чанка тела
											cout << this->_fmk->format("<body %u>", http.body().size()) << endl << endl;
										// Иначе устанавливаем перенос строки
										else cout << endl;
									}
								#endif
								// Выполняем анализ результата авторизации
								switch(static_cast <uint8_t> (this->_http.getAuth())){
									// Если нужно попытаться ещё раз
									case static_cast <uint8_t> (awh::http_t::stath_t::RETRY): {
										// Если функция обратного вызова на на вывод ошибок установлена
										if((this->_http.response().code == 401) && this->_callback.is("error"))
											// Выводим функцию обратного вызова
											this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, "authorization failed");
										// Если попытки повторить переадресацию закончились
										if((this->_stopped = (this->_attempt >= this->_attempts)))
											// Завершаем работу
											const_cast <client::core_t *> (this->_core)->close(this->_aid);
									} break;
									// Если запрос неудачный
									case static_cast <uint8_t> (awh::http_t::stath_t::FAULT): {
										// Если функция обратного вызова на на вывод ошибок установлена
										if(this->_callback.is("error"))
											// Выводим функцию обратного вызова
											this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_RECV, this->_http.message(this->_http.response().code).c_str());
										// Завершаем работу
										const_cast <client::core_t *> (this->_core)->close(this->_aid);
									} break;
								}
								// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
								if(this->_resultCallback.is("entity"))
									// Выполняем функцию обратного вызова дисконнекта
									this->_resultCallback.bind <const int32_t, const u_int, const string, const vector <char>> ("entity");
								// Выполняем очистку функций обратного вызова
								this->_resultCallback.clear();
								// Если установлена функция отлова завершения запроса
								if(this->_callback.is("end"))
									// Выводим функцию обратного вызова
									this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::RECV);
								// Завершаем работу
								return 0;
							}
						// Если рукопожатие выполнено
						} else if(this->_allow.receive) {
							// Если мы получили неустановленный флаг или флаг завершения потока
							if((flags == NGHTTP2_FLAG_NONE) || (flags & NGHTTP2_FLAG_END_STREAM)){
								// Выполняем препарирование полученных данных
								switch(static_cast <uint8_t> (this->prepare(sid, this->_aid, const_cast <client::core_t *> (this->_core)))){
									// Если необходимо выполнить остановку обработки
									case static_cast <uint8_t> (status_t::STOP):
										// Выполняем завершение работы
										goto End;
									// Если необходимо выполнить переход к следующему этапу обработки
									case static_cast <uint8_t> (status_t::NEXT): {
										// Если поток небыл закрыт
										if(flags & NGHTTP2_FLAG_NONE)
											// Выполняем отправку сообщения об ошибке
											this->sendError(this->_mess);
									} break;
								}
								// Устанавливаем метку завершения работы
								End:
								// Если мы получили флаг завершения потока
								if(flags & NGHTTP2_FLAG_END_STREAM){
									// Если установлена функция отлова завершения запроса
									if(this->_callback.is("end"))
										// Выводим функцию обратного вызова
										this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::RECV);
								}
								// Выходим из функции
								return 0;
							}
						}
					} break;
					// Если мы получили входящие данные заголовков ответа
					case NGHTTP2_HEADERS: {
						// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
						if(flags & NGHTTP2_FLAG_END_HEADERS){
							// Выполняем коммит полученного результата
							this->_http.commit();
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Получаем объект работы с HTTP-запросами
									const http_t & http = reinterpret_cast <http_t &> (this->_http);
									// Получаем данные ответа
									const auto & response = http.process(http_t::process_t::RESPONSE, true);
									// Если параметры ответа получены
									if(!response.empty())
										// Выводим параметры ответа
										cout << string(response.begin(), response.end()) << endl;
								}
							#endif
							// Получаем параметры запроса
							const auto & response = this->_http.response();
							// Получаем объект биндинга ядра TCP/IP
							client::core_t * core = const_cast <client::core_t *> (this->_core);
							// Выполняем препарирование полученных данных
							switch(static_cast <uint8_t> (this->prepare(sid, this->_aid, core))){
								// Если необходимо выполнить остановку обработки
								case static_cast <uint8_t> (status_t::STOP): {
									// Выполняем сброс количества попыток
									this->_attempt = 0;
									// Завершаем работу
									core->close(this->_aid);
								} break;
								// Если необходимо выполнить переход к следующему этапу обработки
								case static_cast <uint8_t> (status_t::NEXT): {
									// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
									if(this->_callback.is("response"))
										// Выводим функцию обратного вызова
										this->_callback.call <const int32_t, const u_int, const string &> ("response", sid, response.code, response.message);
									// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
									if(this->_callback.is("headers"))
										// Выводим функцию обратного вызова
										this->_callback.call <const int32_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", sid, response.code, response.message, this->_http.headers());
									// Очищаем буфер собранных данных
									this->_buffer.clear();
									// Если мы получили флаг завершения потока
									if(flags & NGHTTP2_FLAG_END_STREAM){
										// Если установлена функция отлова завершения запроса
										if(this->_callback.is("end"))
											// Выводим функцию обратного вызова
											this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::RECV);
									}
									// Завершаем работу
									return 0;
								}
								// Если необходимо выполнить пропуск обработки данных
								case static_cast <uint8_t> (status_t::SKIP): {
									// Если мы получили флаг завершения потока
									if(flags & NGHTTP2_FLAG_END_STREAM){
										// Если установлена функция отлова завершения запроса
										if(this->_callback.is("end"))
											// Выводим функцию обратного вызова
											this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::RECV);
									}
									// Завершаем работу
									return 0;
								}
							}
							// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
							if(this->_callback.is("response"))
								// Выводим функцию обратного вызова
								this->_callback.call <const int32_t, const u_int, const string &> ("response", sid, response.code, response.message);
							// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
							if(this->_callback.is("headers"))
								// Выводим функцию обратного вызова
								this->_callback.call <const int32_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", sid, response.code, response.message, this->_http.headers());
							// Если мы получили флаг завершения потока
							if(flags & NGHTTP2_FLAG_END_STREAM){
								// Если установлена функция отлова завершения запроса
								if(this->_callback.is("end"))
									// Выводим функцию обратного вызова
									this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::RECV);
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
 * beginSignal Метод начала получения фрейма заголовков HTTP/2 сервера
 * @param sid идентификатор потока
 * @return    статус полученных данных
 */
int awh::client::WebSocket2::beginSignal(const int32_t sid) noexcept {
	// Если подключение производится через, прокси-сервер
	if(this->_scheme.isProxy())
		// Выполняем обработку сигнала начала получения заголовков для прокси-сервера
		return this->beginProxySignal(sid);
	// Если мы работаем с сервером напрямую
	else {
		// Если идентификатор сессии клиента совпадает
		if(this->_sid == sid){
			// Выполняем сброс флага рукопожатия
			this->_shake = false;
			// Выполняем очистку параметров HTTP запроса
			this->_http.clear();
			// Очищаем буфер собранных данных
			this->_buffer.clear();
			// Выполняем очистку оставшихся фрагментов
			this->_fragmes.clear();
		}
	}
	// Выводим результат
	return 0;
}
/**
 * closedSignal Метод завершения работы потока
 * @param sid   идентификатор потока
 * @param error флаг ошибки HTTP/2 если присутствует
 * @return      статус полученных данных
 */
int awh::client::WebSocket2::closedSignal(const int32_t sid, const uint32_t error) noexcept {
	// Определяем тип получаемой ошибки
	switch(error){
		// Если получена ошибка протокола
		case 0x1: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "PROTOCOL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_PROTOCOL, this->_fmk->format("Stream %d closed with error=%s", sid, "PROTOCOL_ERROR"));
		} break;
		// Если получена ошибка реализации
		case 0x2: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "INTERNAL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_INTERNAL, this->_fmk->format("Stream %d closed with error=%s", sid, "INTERNAL_ERROR"));
		} break;
		// Если получена ошибка превышения предела управления потоком
		case 0x3: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "FLOW_CONTROL_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_FLOW_CONTROL, this->_fmk->format("Stream %d closed with error=%s", sid, "FLOW_CONTROL_ERROR"));
		} break;
		// Если установка не подтверждённа
		case 0x4: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "SETTINGS_TIMEOUT");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SETTINGS_TIMEOUT, this->_fmk->format("Stream %d closed with error=%s", sid, "SETTINGS_TIMEOUT"));
		} break;
		// Если получен кадр для завершения потока
		case 0x5: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "STREAM_CLOSED");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_STREAM_CLOSED, this->_fmk->format("Stream %d closed with error=%s", sid, "STREAM_CLOSED"));
		} break;
		// Если размер кадра некорректен
		case 0x6: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "FRAME_SIZE_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_FRAME_SIZE, this->_fmk->format("Stream %d closed with error=%s", sid, "FRAME_SIZE_ERROR"));
		} break;
		// Если поток не обработан
		case 0x7: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "REFUSED_STREAM");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_REFUSED_STREAM, this->_fmk->format("Stream %d closed with error=%s", sid, "REFUSED_STREAM"));
		} break;
		// Если поток аннулирован
		case 0x8: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "CANCEL");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_CANCEL, this->_fmk->format("Stream %d closed with error=%s", sid, "CANCEL"));
		} break;
		// Если состояние компрессии не обновлено
		case 0x9: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "COMPRESSION_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_COMPRESSION, this->_fmk->format("Stream %d closed with error=%s", sid, "COMPRESSION_ERROR"));
		} break;
		// Если получена ошибка TCP-соединения для метода CONNECT
		case 0xA: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "CONNECT_ERROR");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_CONNECT, this->_fmk->format("Stream %d closed with error=%s", sid, "CONNECT_ERROR"));
		} break;
		// Если превышена емкость для обработки
		case 0xB: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "ENHANCE_YOUR_CALM");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_ENHANCE_YOUR_CALM, this->_fmk->format("Stream %d closed with error=%s", sid, "ENHANCE_YOUR_CALM"));
		} break;
		// Если согласованные параметры TLS не приемлемы
		case 0xC: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "INADEQUATE_SECURITY");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_INADEQUATE_SECURITY, this->_fmk->format("Stream %d closed with error=%s", sid, "INADEQUATE_SECURITY"));
		} break;
		// Если для запроса используется HTTP/1.1
		case 0xD: {
			// Выводим информацию о закрытии сессии с ошибкой
			this->_log->print("Stream %d closed with error=%s", log_t::flag_t::CRITICAL, sid, "HTTP_1_1_REQUIRED");
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_HTTP_1_1_REQUIRED, this->_fmk->format("Stream %d closed with error=%s", sid, "HTTP_1_1_REQUIRED"));
		} break;
	}
	// Если функция обратного вызова активности потока установлена
	if(this->_callback.is("stream"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::CLOSE);
	// Если флаг инициализации сессии HTTP2 установлен
	if((error > 0x00) && this->_sessionInitialized){
		// Выполняем снятие флага инициализации сессии HTTP2
		this->_sessionInitialized = !this->_sessionInitialized;
		// Если закрытие подключения не выполнено
		if(!this->_nghttp2.close()){
			// Выполняем отключение от сервера
			const_cast <client::core_t *> (this->_core)->close(this->_aid);
			// Выводим сообщение об ошибке
			return NGHTTP2_ERR_CALLBACK_FAILURE;
		// Выполняем отключение от сервера
		} else const_cast <client::core_t *> (this->_core)->close(this->_aid);
	}
	// Выводим результат
	return 0;
}
/**
 * headerSignal Метод обратного вызова при получении заголовка HTTP/2 сервера
 * @param sid идентификатор потока
 * @param key данные ключа заголовка
 * @param val данные значения заголовка
 * @return    статус полученных данных
 */
int awh::client::WebSocket2::headerSignal(const int32_t sid, const string & key, const string & val) noexcept {
	// Если подключение производится через, прокси-сервер
	if(this->_scheme.isProxy())
		// Выполняем обработку полученных заголовков для прокси-сервера
		return this->headerProxySignal(sid, key, val);
	// Если мы работаем с сервером напрямую
	else {
		// Если идентификатор сессии клиента совпадает
		if(this->_sid == sid){
			// Устанавливаем полученные заголовки
			this->_http.header2(key, val);
			// Если функция обратного вызова на полученного заголовка с сервера установлена
			if(this->_callback.is("header"))
				// Выводим функцию обратного вызова
				this->_callback.call <const int32_t, const string &, const string &> ("header", sid, key, val);
		}
	}
	// Выводим результат
	return 0;
}
/**
 * redirect Метод выполнения редиректа если требуется
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат выполнения редиректа
 */
bool awh::client::WebSocket2::redirect(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Результат работы функции
	bool result = false;
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2){
		// Выполняем переброс вызова дисконнекта на клиент WebSocket
		this->_ws1.disconnectCallback(aid, sid, core);
		// Если список ответов получен
		if((result = !this->_ws1._stopped)){
			// Получаем параметры запроса
			const auto & response = this->_ws1._http.response();
			// Если необходимо выполнить ещё одну попытку выполнения авторизации
			if((result = (this->_proxy.answer == 407) || (response.code == 401) || (response.code == 407))){
				// Выполняем очистку оставшихся данных
				this->_ws1._buffer.clear();
				// Получаем количество попыток
				this->_attempt = this->_ws1._attempt;
				// Выполняем установку следующего экшена на открытие подключения
				this->open();
				// Завершаем работу
				return result;
			}
			// Выполняем определение ответа сервера
			switch(response.code){
				// Если ответ сервера: Moved Permanently
				case 301:
				// Если ответ сервера: Permanent Redirect
				case 308: break;
				// Если мы получили любой другой ответ, выходим
				default: return result;
			}
			// Если адрес для выполнения переадресации указан
			if((result = this->_ws1._http.isHeader("location"))){
				// Выполняем очистку оставшихся данных
				this->_ws1._buffer.clear();
				// Получаем новый адрес запроса
				const uri_t::url_t & url = this->_ws1._http.getUrl();
				// Если адрес запроса получен
				if((result = !url.empty())){
					// Получаем количество попыток
					this->_attempt = this->_ws1._attempt;
					// Устанавливаем новый адрес запроса
					this->_uri.combine(this->_scheme.url, url);
					// Выполняем установку следующего экшена на открытие подключения
					this->open();
					// Завершаем работу
					return result;
				}
			}
		}
	// Если переключение протокола на HTTP/2 выполнено
	} else {
		// Выполняем переключение протокола интернета обратно на HTTP/1.1
		this->_proto = engine_t::proto_t::HTTP1_1;
		// Если список ответов получен
		if((result = !this->_stopped)){
			// Получаем параметры запроса
			const auto & response = this->_http.response();
			// Если необходимо выполнить ещё одну попытку выполнения авторизации
			if((result = (this->_proxy.answer == 407) || (response.code == 401) || (response.code == 407))){
				// Увеличиваем количество попыток
				this->_attempt++;
				// Выполняем очистку оставшихся данных
				this->_buffer.clear();
				// Выполняем установку следующего экшена на открытие подключения
				this->open();
				// Завершаем работу
				return result;
			}
			// Выполняем определение ответа сервера
			switch(response.code){
				// Если ответ сервера: Moved Permanently
				case 301:
				// Если ответ сервера: Permanent Redirect
				case 308: break;
				// Если мы получили любой другой ответ, выходим
				default: return result;
			}
			// Если адрес для выполнения переадресации указан
			if((result = this->_http.isHeader("location"))){
				// Выполняем очистку оставшихся данных
				this->_buffer.clear();
				// Получаем новый адрес запроса
				const uri_t::url_t & url = this->_http.getUrl();
				// Если адрес запроса получен
				if((result = !url.empty())){
					// Увеличиваем количество попыток
					this->_attempt++;
					// Устанавливаем новый адрес запроса
					this->_uri.combine(this->_scheme.url, url);
					// Выполняем установку следующего экшена на открытие подключения
					this->open();
					// Завершаем работу
					return result;
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::WebSocket2::flush() noexcept {
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2)
		// Выполняем сброс параметров запроса на клиенте WebSocket
		this->_ws1.flush();
	// Если переключение протокола на HTTP/2 выполнено
	else {
		// Снимаем флаг отключения
		this->_close = false;
		// Снимаем флаг принудительной остановки
		this->_stopped = false;
		// Выполняем очистку оставшихся данных
		this->_buffer.clear();
		// Выполняем очистку оставшихся фрагментов
		this->_fragmes.clear();
		// Устанавливаем флаг разрешающий обмен данных
		this->_allow = allow_t();
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket2::ping(const string & message) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_allow.send){
		// Если рукопожатие выполнено
		if(this->_http.isHandshake(http_t::process_t::RESPONSE) && (this->_aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->_frame.methods.ping(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				web2_t::send(this->_sid, buffer.data(), buffer.size(), false);
		}
	}
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket2::pong(const string & message) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_allow.send){
		// Если рукопожатие выполнено
		if(this->_http.isHandshake(http_t::process_t::RESPONSE) && (this->_aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->_frame.methods.pong(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				web2_t::send(this->_sid, buffer.data(), buffer.size(), false);
		}
	}
}
/**
 * prepare Метод выполнения препарирования полученных данных
 * @param sid  идентификатор запроса
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 * @return     результат препарирования
 */
awh::client::Web::status_t awh::client::WebSocket2::prepare(const int32_t sid, const uint64_t aid, client::core_t * core) noexcept {
	// Результат работы функции
	status_t result = status_t::STOP;
	// Если рукопожатие не выполнено
	if(!this->_shake){
		// Получаем параметры запроса
		auto response = this->_http.response();
		// Выполняем проверку авторизации
		switch(static_cast <uint8_t> (this->_http.getAuth())){
			// Если нужно попытаться ещё раз
			case static_cast <uint8_t> (http_t::stath_t::RETRY): {
				// Если попытка повторить авторизацию ещё не проводилась
				if(!(this->_stopped = (this->_attempt >= this->_attempts))){
					// Получаем новый адрес запроса
					const uri_t::url_t & url = this->_http.getUrl();
					// Если URL-адрес запроса получен
					if(!url.empty())
						// Устанавливаем новый адрес запроса
						this->_uri.combine(this->_scheme.url, url);
					// Выполняем сброс параметров запроса
					this->flush();
					// Увеличиваем количество попыток
					this->_attempt++;
					// Если подключение не постоянное, то завершаем работу
					dynamic_cast <client::core_t *> (core)->close(aid);
					// Завершаем работу
					return status_t::SKIP;
				}
				// Создаём сообщение
				this->_mess = ws::mess_t(response.code, this->_http.message(response.code));
				// Выводим сообщение
				this->error(this->_mess);
			} break;
			// Если запрос выполнен удачно
			case static_cast <uint8_t> (http_t::stath_t::GOOD): {
				// Если рукопожатие выполнено
				if((this->_shake = this->_http.isHandshake(http_t::process_t::RESPONSE))){
					// Выполняем сброс количества попыток
					this->_attempt = 0;
					// Очищаем список фрагментированных сообщений
					this->_fragmes.clear();
					// Получаем флаг шифрованных данных
					this->_crypt = this->_http.isCrypt();
					// Получаем поддерживаемый метод компрессии
					this->_compress = this->_http.compress();
					// Получаем размер скользящего окна сервера
					this->_server.wbit = this->_http.wbit(awh::web_t::hid_t::SERVER);
					// Получаем размер скользящего окна клиента
					this->_client.wbit = this->_http.wbit(awh::web_t::hid_t::CLIENT);
					// Обновляем контрольную точку времени получения данных
					this->_point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					// Разрешаем перехватывать контекст компрессии для клиента
					this->_hash.takeoverCompress(this->_http.takeover(awh::web_t::hid_t::CLIENT));
					// Разрешаем перехватывать контекст компрессии для сервера
					this->_hash.takeoverDecompress(this->_http.takeover(awh::web_t::hid_t::SERVER));
					// Если разрешено в лог выводим информационные сообщения
					if(!this->_noinfo)
						// Выводим в лог сообщение об удачной авторизации не WebSocket-сервере
						this->_log->print("Authorization on the WebSocket-server was successful", log_t::flag_t::INFO);
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(!this->_http.body().empty() && this->_callback.is("entity"))
						// Устанавливаем полученную функцию обратного вызова
						this->_resultCallback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, this->_http.body());
					// Если функция обратного вызова активности потока установлена
					if(this->_callback.is("stream"))
						// Выводим функцию обратного вызова
						this->_callback.call <const int32_t, const mode_t> ("stream", sid, mode_t::OPEN);
					// Если функция обратного вызова на получение удачного ответа установлена
					if(this->_callback.is("handshake"))
						// Выполняем функцию обратного вызова
						this->_callback.call <const int32_t> ("handshake", sid);
					// Завершаем работу
					return status_t::NEXT;
				// Сообщаем, что рукопожатие не выполнено
				} else {
					// Если код ответа не является отрицательным
					if(response.code < 400){
						// Устанавливаем код ответа
						response.code = 403;
						// Заменяем ответ сервера
						this->_http.response(response);
					}
					// Создаём сообщение
					this->_mess = ws::mess_t(response.code, this->_http.message(response.code));
					// Выводим сообщение
					this->error(this->_mess);
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(!this->_http.body().empty() && this->_callback.is("entity"))
						// Устанавливаем полученную функцию обратного вызова
						this->_resultCallback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, this->_http.body());
				}
			} break;
			// Если запрос неудачный
			case static_cast <uint8_t> (http_t::stath_t::FAULT): {
				// Устанавливаем флаг принудительной остановки
				this->_stopped = true;
				// Создаём сообщение
				this->_mess = ws::mess_t(response.code, response.message);
				// Выводим сообщение
				this->error(this->_mess);
				// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
				if(!this->_http.body().empty() && this->_callback.is("entity"))
					// Устанавливаем полученную функцию обратного вызова
					this->_resultCallback.set <void (const int32_t, const u_int, const string, const vector <char>)> ("entity", this->_callback.get <void (const int32_t, const u_int, const string, const vector <char>)> ("entity"), sid, response.code, response.message, this->_http.body());
			} break;
		}
		// Завершаем работу
		return status_t::STOP;
	// Если рукопожатие выполнено
	} else if(this->_allow.receive) {
		// Флаг удачного получения данных
		bool receive = false;
		// Создаём буфер сообщения
		vector <char> buffer;
		// Создаём объект шапки фрейма
		ws::frame_t::head_t head;
		// Выполняем обработку полученных данных
		while(!this->_close && this->_allow.receive){
			// Выполняем чтение фрейма WebSocket
			const auto & data = this->_frame.methods.get(head, this->_buffer.data(), this->_buffer.size());
			// Если буфер данных получен
			if(!data.empty()){
				// Проверяем состояние флагов RSV2 и RSV3
				if(head.rsv[1] || head.rsv[2]){
					// Создаём сообщение
					this->_mess = ws::mess_t(1002, "RSV2 and RSV3 must be clear");
					// Выводим сообщение
					this->error(this->_mess);
					// Выполняем реконнект
					return status_t::NEXT;
				}
				// Если флаг компресси включён а данные пришли не сжатые
				if(head.rsv[0] && ((this->_compress == http_t::compress_t::NONE) ||
				  (head.optcode == ws::frame_t::opcode_t::CONTINUATION) ||
				  ((static_cast <uint8_t> (head.optcode) > 0x07) && (static_cast <uint8_t> (head.optcode) < 0x0b)))){
					// Создаём сообщение
					this->_mess = ws::mess_t(1002, "RSV1 must be clear");
					// Выводим сообщение
					this->error(this->_mess);
					// Выполняем реконнект
					return status_t::NEXT;
				}
				// Если опкоды требуют финального фрейма
				if(!head.fin && (static_cast <uint8_t> (head.optcode) > 0x07) && (static_cast <uint8_t> (head.optcode) < 0x0b)){
					// Создаём сообщение
					this->_mess = ws::mess_t(1002, "FIN must be set");
					// Выводим сообщение
					this->error(this->_mess);
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
						if(::memcmp(::to_string(aid).c_str(), data.data(), data.size()) == 0)
							// Обновляем контрольную точку
							this->_point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					break;
					// Если ответом является TEXT
					case static_cast <uint8_t> (ws::frame_t::opcode_t::TEXT):
					// Если ответом является BINARY
					case static_cast <uint8_t> (ws::frame_t::opcode_t::BINARY): {
						// Запоминаем полученный опкод
						this->_frame.opcode = head.optcode;
						// Запоминаем, что данные пришли сжатыми
						this->_deflate = (head.rsv[0] && (this->_compress != http_t::compress_t::NONE));
						// Если сообщение замаскированно
						if(head.mask){
							// Создаём сообщение
							this->_mess = ws::mess_t(1002, "Masked frame from server");
							// Выводим сообщение
							this->error(this->_mess);
							// Выполняем реконнект
							return status_t::NEXT;
						// Если список фрагментированных сообщений существует
						} else if(!this->_fragmes.empty()) {
							// Очищаем список фрагментированных сообщений
							this->_fragmes.clear();
							// Создаём сообщение
							this->_mess = ws::mess_t(1002, "Opcode for subsequent fragmented messages should not be set");
							// Выводим сообщение
							this->error(this->_mess);
							// Выполняем реконнект
							return status_t::NEXT;
						// Если сообщение является не последнем
						} else if(!head.fin)
							// Заполняем фрагментированное сообщение
							this->_fragmes.insert(this->_fragmes.end(), data.begin(), data.end());
						// Если сообщение является последним
						else buffer = std::forward <const vector <char>> (data);
					} break;
					// Если ответом является CONTINUATION
					case static_cast <uint8_t> (ws::frame_t::opcode_t::CONTINUATION): {
						// Заполняем фрагментированное сообщение
						this->_fragmes.insert(this->_fragmes.end(), data.begin(), data.end());
						// Если сообщение является последним
						if(head.fin){
							// Выполняем копирование всех собранных сегментов
							buffer = std::forward <const vector <char>> (this->_fragmes);
							// Очищаем список фрагментированных сообщений
							this->_fragmes.clear();
						}
					} break;
					// Если ответом является CLOSE
					case static_cast <uint8_t> (ws::frame_t::opcode_t::CLOSE): {
						// Извлекаем сообщение
						this->_mess = this->_frame.methods.message(data);
						// Выводим сообщение
						this->error(this->_mess);
						// Выполняем реконнект
						return status_t::NEXT;
					} break;
				}
			}
			// Если парсер обработал какое-то количество байт
			if((receive = ((head.frame > 0) && !this->_buffer.empty()))){
				// Если размер буфера больше количества удаляемых байт
				if((receive = (this->_buffer.size() >= head.frame)))
					// Удаляем количество обработанных байт
					this->_buffer.assign(this->_buffer.begin() + head.frame, this->_buffer.end());
					// vector <decltype(this->_buffer)::value_type> (this->_buffer.begin() + head.frame, this->_buffer.end()).swap(this->_buffer);
			}
			// Если сообщения получены
			if(!buffer.empty()){
				// Если тредпул активирован
				if(this->_thr.is())
					// Добавляем в тредпул новую задачу на извлечение полученных сообщений
					this->_thr.push(std::bind(&ws2_t::extraction, this, buffer, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT)));
				// Если тредпул не активирован, выполняем извлечение полученных сообщений
				else this->extraction(buffer, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT));
				// Очищаем буфер полученного сообщения
				buffer.clear();
			}
			// Если данные мы все получили, выходим
			if(!receive || this->_buffer.empty()) break;
		}
	}
	// Выполняем завершение работы
	return status_t::STOP;
}
/**
 * error Метод вывода сообщений об ошибках работы клиента
 * @param message сообщение с описанием ошибки
 */
void awh::client::WebSocket2::error(const ws::mess_t & message) const noexcept {
	// Очищаем список буффер бинарных данных
	const_cast <ws2_t *> (this)->_buffer.clear();
	// Очищаем список фрагментированных сообщений
	const_cast <ws2_t *> (this)->_fragmes.clear();
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
			// Если функция обратного вызова при подключении/отключении установлена
			if(this->_callback.is("wserror"))
				// Если функция обратного вызова установлена, выводим полученное сообщение
				this->_callback.call <const u_int, const string &> ("wserror", message.code, message.text);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::WARNING, http::error_t::WEBSOCKET, this->_fmk->format("%s [%u]", message.code, message.text.c_str()));
		}
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param buffer данные в чистом виде полученные с сервера
 * @param text   данные передаются в текстовом виде
 */
void awh::client::WebSocket2::extraction(const vector <char> & buffer, const bool text) noexcept {
	// Если буфер данных передан
	if(!buffer.empty() && !this->_freeze && this->_callback.is("message")){
		// Если данные пришли в сжатом виде
		if(this->_deflate && (this->_compress != http_t::compress_t::NONE)){
			// Декомпрессионные данные
			vector <char> data;
			// Определяем метод компрессии
			switch(static_cast <uint8_t> (this->_compress)){
				// Если метод компрессии выбран Deflate
				case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
					// Устанавливаем размер скользящего окна
					this->_hash.wbit(this->_server.wbit);
					// Добавляем хвост в полученные данные
					this->_hash.setTail(* const_cast <vector <char> *> (&buffer));
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
				} break;
				// Если метод компрессии выбран GZip
				case static_cast <uint8_t> (http_t::compress_t::GZIP):
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::GZIP);
				break;
				// Если метод компрессии выбран Brotli
				case static_cast <uint8_t> (http_t::compress_t::BROTLI):
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BROTLI);
				break;
			}
			// Если данные получены
			if(!data.empty()){
				// Если нужно производить дешифрование
				if(this->_crypt){
					// Выполняем шифрование переданных данных
					const auto & res = this->_hash.decrypt(data.data(), data.size());
					// Если данные сообщения получилось удачно расшифровать
					if(!res.empty())
						// Выводим данные полученного сообщения
						this->_callback.call <const vector <char> &, const bool> ("message", res, text);
					// Иначе выводим сообщение так - как оно пришло
					else this->_callback.call <const vector <char> &, const bool> ("message", data, text);
				// Отправляем полученный результат
				} else this->_callback.call <const vector <char> &, const bool> ("message", data, text);
			// Выводим сообщение об ошибке
			} else {
				// Создаём сообщение
				this->_mess = ws::mess_t(1007, "Received data decompression error");
				// Выводим сообщение
				this->error(this->_mess);
				// Иначе выводим сообщение так - как оно пришло
				this->_callback.call <const vector <char> &, const bool> ("message", buffer, text);
				// Выполняем отправку сообщения об ошибке
				this->sendError(this->_mess);
			}
		// Если функция обратного вызова установлена, выводим полученное сообщение
		} else {
			// Если нужно производить дешифрование
			if(this->_crypt){
				// Выполняем шифрование переданных данных
				const auto & res = this->_hash.decrypt(buffer.data(), buffer.size());
				// Если данные сообщения получилось удачно распаковать
				if(!res.empty())
					// Выводим данные полученного сообщения
					this->_callback.call <const vector <char> &, const bool> ("message", res, text);
				// Иначе выводим сообщение так - как оно пришло
				else this->_callback.call <const vector <char> &, const bool> ("message", buffer, text);
			// Отправляем полученный результат
			} else this->_callback.call <const vector <char> &, const bool> ("message", buffer, text);
		}
	}
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::WebSocket2::sendError(const ws::mess_t & mess) noexcept {
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2)
		// Выполняем отправку сообщение об ошибке на клиент WebSocket
		this->_ws1.sendError(mess);
	// Если переключение протокола на HTTP/2 выполнено
	else {
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
			// Если подключение выполнено
			if(this->_core->working() && this->_allow.send && (this->_aid > 0)){
				// Запрещаем получение данных
				this->_allow.receive = false;
				// Если код ошибки относится к WebSocket
				if(mess.code >= 1000){
					// Получаем буфер сообщения
					const auto & buffer = this->_frame.methods.message(mess);
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
						// Выполняем отправку сообщения на сервер
						web2_t::send(this->_sid, buffer.data(), buffer.size(), true);
						// Выходим из функции
						return;
					}
				}
				// Завершаем работу
				const_cast <client::core_t *> (this->_core)->close(this->_aid);
			}
		}
	}
}
/**
 * send Метод отправки сообщения на сервер
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param text    данные передаются в текстовом виде
 */
void awh::client::WebSocket2::send(const char * message, const size_t size, const bool text) noexcept {
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2)
		// Выполняем отправку сообщения на клиент WebSocket
		this->_ws1.send(message, size, text);
	// Если переключение протокола на HTTP/2 выполнено
	else {
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
			// Если подключение выполнено
			if(this->_core->working() && this->_allow.send){
				// Выполняем блокировку отправки сообщения
				this->_allow.send = !this->_allow.send;
				// Если рукопожатие выполнено
				if((message != nullptr) && (size > 0) && this->_http.isHandshake(http_t::process_t::RESPONSE) && (this->_aid > 0)){
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
					ws::frame_t::head_t head(true, true);
					// Если нужно производить шифрование
					if(this->_crypt){
						// Выполняем шифрование переданных данных
						buffer = this->_hash.encrypt(message, size);
						// Если данные зашифрованны
						if(!buffer.empty()){
							// Заменяем сообщение для передачи
							message = buffer.data();
							// Заменяем размер сообщения
							(* const_cast <size_t *> (&size)) = buffer.size();
						}
					}
					// Указываем, что сообщение передаётся в сжатом виде
					head.rsv[0] = ((size >= 1024) && (this->_compress != http_t::compress_t::NONE));
					// Устанавливаем опкод сообщения
					head.optcode = (text ? ws::frame_t::opcode_t::TEXT : ws::frame_t::opcode_t::BINARY);
					// Если необходимо сжимать сообщение перед отправкой
					if(head.rsv[0]){
						// Компрессионные данные
						vector <char> data;
						// Определяем метод компрессии
						switch(static_cast <uint8_t> (this->_compress)){
							// Если метод компрессии выбран Deflate
							case static_cast <uint8_t> (http_t::compress_t::DEFLATE): {
								// Устанавливаем размер скользящего окна
								this->_hash.wbit(this->_client.wbit);
								// Выполняем компрессию полученных данных
								data = this->_hash.compress(message, size, hash_t::method_t::DEFLATE);
								// Удаляем хвост в полученных данных
								this->_hash.rmTail(data);
							} break;
							// Если метод компрессии выбран GZip
							case static_cast <uint8_t> (http_t::compress_t::GZIP):
								// Выполняем компрессию полученных данных
								data = this->_hash.compress(message, size, hash_t::method_t::GZIP);
							break;
							// Если метод компрессии выбран Brotli
							case static_cast <uint8_t> (http_t::compress_t::BROTLI):
								// Выполняем компрессию полученных данных
								data = this->_hash.compress(message, size, hash_t::method_t::BROTLI);
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
					if(size > this->_frame.size){
						// Бинарный буфер чанка данных
						vector <char> chunk(this->_frame.size);
						// Смещение в бинарном буфере
						size_t start = 0, stop = this->_frame.size;
						// Выполняем разбивку полезной нагрузки на сегменты
						while(stop < size){
							// Увеличиваем длину чанка
							stop += this->_frame.size;
							// Если длина чанка слишком большая, компенсируем
							stop = (stop > size ? size : stop);
							// Устанавливаем флаг финального сообщения
							head.fin = (stop == size);
							// Формируем чанк бинарных данных
							chunk.assign(message + start, message + stop);
							// Создаём буфер для отправки
							const auto & buffer = this->_frame.methods.set(head, chunk.data(), chunk.size());
							// Если бинарный буфер для отправки данных получен
							if(!buffer.empty())
								// Выполняем отправку сообщения на сервер
								web2_t::send(this->_sid, buffer.data(), buffer.size(), false);
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
						const auto & buffer = this->_frame.methods.set(head, message, size);
						// Если бинарный буфер для отправки данных получен
						if(!buffer.empty())
							// Выполняем отправку сообщения на сервер
							web2_t::send(this->_sid, buffer.data(), buffer.size(), false);
					}
				}
				// Выполняем разблокировку отправки сообщения
				this->_allow.send = !this->_allow.send;
			}
		}
	}
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::WebSocket2::pause() noexcept {
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2)
		// Ставим работу клиента на паузу
		this->_ws1.pause();
	// Если переключение протокола на HTTP/2 выполнено
	else
		// Ставим работу клиента на паузу
		this->_freeze = true;
}
/**
 * stop Метод остановки клиента
 */
void awh::client::WebSocket2::stop() noexcept {
	// Устанавливаем флаг принудительной остановки
	this->_active = true;
	// Если подключение выполнено
	if(this->_core->working()){
		// Выполняем сброс параметров запроса
		this->flush();
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Если завершить работу разрешено
		if(this->_unbind)
			// Завершаем работу
			const_cast <client::core_t *> (this->_core)->stop();
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
void awh::client::WebSocket2::start() noexcept {
	// Если адрес URL запроса передан
	if(!this->_freeze && !this->_scheme.url.empty()){
		// Если биндинг не запущен, выполняем запуск биндинга
		if(!this->_core->working())
			// Выполняем запуск биндинга
			const_cast <client::core_t *> (this->_core)->start();
		// Выполняем запрос на сервер
		else this->open();
	}
	// Снимаем с паузы клиент
	this->_freeze = false;
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const u_int, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих ошибок
	this->_callback.set <void (const u_int, const string &)> ("wserror", callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const vector <char> &, const bool)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих сообщений
	this->_callback.set <void (const vector <char> &, const bool)> ("message", callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on on Метод установки функция обратного вызова при выполнении рукопожатия
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const int32_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const int32_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const int32_t, const direct_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const int32_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const int32_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const int32_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const int32_t, const u_int, const string &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web2_t::on(callback);
	// Выполняем установку функции обратного вызова для WebSocket-клиента
	this->_ws1.on(callback);
}
/**
 * sub Метод установки поддерживаемого сабпротокола
 * @param sub сабпротокол для установки
 */
void awh::client::WebSocket2::sub(const string & sub) noexcept {
	// Если сабпротокол передан
	if(!sub.empty()){
		// Устанавливаем поддерживаемый сабпротокол для WebSocket-клиента
		this->_ws1.sub(sub);
		// Устанавливаем поддерживаемый сабпротокол для HTTP-клиента
		this->_http.sub(sub);
	}
}
/**
 * sub Метод получения списка выбранных сабпротоколов
 * @return список выбранных сабпротоколов
 */
const set <string> & awh::client::WebSocket2::subs() const noexcept {
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2)
		// Выводим список выбранных сабпротоколов
		return this->_ws1.subs();
	// Если переключение протокола на HTTP/2 выполнено
	else
		// Выводим список выбранных сабпротоколов
		return this->_http.subs();
}
/**
 * subs Метод установки списка поддерживаемых сабпротоколов
 * @param subs сабпротоколы для установки
 */
void awh::client::WebSocket2::subs(const set <string> & subs) noexcept {
	// Если список поддерживаемых сабпротоколов получен
	if(!subs.empty()){
		// Устанавливаем список поддерживаемых сабсабпротоколов для WebSocket-клиента
		this->_ws1.subs(subs);
		// Устанавливаем список поддерживаемых сабсабпротоколов для HTTP-клиента
		this->_http.subs(subs);
	}
}
/**
 * extensions Метод извлечения списка расширений
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::client::WebSocket2::extensions() const noexcept {
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2)
		// Выводим список доступных расширений
		return this->_ws1.extensions();
	// Если переключение протокола на HTTP/2 выполнено
	else
		// Выводим список доступных расширений
		return this->_http.extensions();
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::client::WebSocket2::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка доступных расширений для WebSocket-клиента
	this->_ws1.extensions(extensions);
	// Выполняем установку списка доступных расширений для HTTP-клиента
	this->_http.extensions(extensions);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::WebSocket2::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка для WebSocket-клиента
	this->_ws1.chunk(size);
	// Устанавливаем размер чанка для HTTP-клиента
	this->_http.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::client::WebSocket2::segmentSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0){
		// Устанавливаем размер сегментов фрейма
		this->_frame.size = size;
		// Устанавливаем размер сегментов фрейма для WebSocket-клиента
		this->_ws1.segmentSize(size);
	}
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::WebSocket2::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для WebSocket-клиента
	this->_ws1.mode(flags);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_noinfo = (flags.count(flag_t::NOT_INFO) > 0);
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_client.takeover = (flags.count(flag_t::TAKEOVER_CLIENT) > 0);
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_server.takeover = (flags.count(flag_t::TAKEOVER_SERVER) > 0);
	// Выполняем установку флагов настроек модуля
	web2_t::mode(flags);
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket2::core(const client::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
		// Добавляем схемы сети в сетевое ядро
		const_cast <client::core_t *> (this->_core)->add(&this->_scheme);
		// Активируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(true);
		// Активируем асинхронный режим работы
		const_cast <client::core_t *> (this->_core)->mode(client::core_t::mode_t::ASYNC);
		// Устанавливаем функцию активации ядра клиента
		const_cast <client::core_t *> (this->_core)->on(std::bind(&ws2_t::eventsCallback, this, _1, _2));
		// Если многопоточность активированна
		if(this->_thr.is() || this->_ws1._thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <client::core_t *> (this->_core)->easily(true);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_thr.is() || this->_ws1._thr.is()){
			// Выполняем завершение всех активных потоков
			this->_thr.wait();
			// Выполняем завершение всех активных потоков
			this->_ws1._thr.wait();
			// Снимаем режим простого чтения базы событий
			const_cast <client::core_t *> (this->_core)->easily(false);
		}
		// Деактивируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(false);
		// Удаляем схему сети из сетевого ядра
		const_cast <client::core_t *> (this->_core)->remove(this->_scheme.sid);
		// Активируем асинхронный режим работы
		const_cast <client::core_t *> (this->_core)->mode(client::core_t::mode_t::SYNC);
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
	}
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::WebSocket2::user(const string & login, const string & password) noexcept {
	// Устанавливаем логин и пароль пользователя для WebSocket-клиента
	this->_ws1.user(login, password);
	// Устанавливаем логин и пароль пользователя для HTTP-клиента
	this->_http.user(login, password);
}
/**
 * setHeaders Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::client::WebSocket2::setHeaders(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку HTTP-заголовков для отправки на сервер
	this->_headers = headers;
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::WebSocket2::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента у родительского класса
		web2_t::userAgent(userAgent);
		// Устанавливаем пользовательского агента для WebSocket-клиента
		this->_ws1.userAgent(userAgent);
		// Устанавливаем пользовательского агента для HTTP-клиента
		this->_http.userAgent(userAgent);
	}
}
/**
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::WebSocket2::ident(const string & id, const string & name, const string & ver) noexcept {
	// Если данные сервиса переданы
	if(!id.empty() && !name.empty() && !ver.empty()){
		// Выполняем установку данных сервиса у родительского класса
		web2_t::ident(id, name, ver);
		// Устанавливаем данные сервиса для WebSocket-клиента
		this->_ws1.ident(id, name, ver);
		// Устанавливаем данные сервиса для HTTP-клиента
		this->_http.ident(id, name, ver);
	}
}
/**
 * multiThreads Метод активации многопоточности
 * @param threads количество потоков для активации
 * @param mode    флаг активации/деактивации мультипоточности
 */
void awh::client::WebSocket2::multiThreads(const size_t threads, const bool mode) noexcept {
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
			const_cast <client::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->_thr.wait();
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::WebSocket2::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку параметры прокси-сервера
	web2_t::proxy(uri, family);
	// Выполняем установку параметры прокси-сервера для WebSocket-клиента
	this->_ws1.proxy(uri, family);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::WebSocket2::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем параметры авторизации для WebSocket-клиента
	this->_ws1.authType(type, hash);
	// Устанавливаем параметры авторизации для HTTP-клиента
	this->_http.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::WebSocket2::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации на проксе-сервере
	web2_t::authTypeProxy(type, hash);
	// Устанавливаем тип авторизации на проксе-сервере для WebSocket-клиента
	this->_ws1.authTypeProxy(type, hash);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::WebSocket2::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования для WebSocket-клиента
	this->_ws1.crypto(pass, salt, cipher);
	// Устанавливаем параметры шифрования для HTTP-клиента
	this->_http.crypto(pass, salt, cipher);
}
/**
 * WebSocket2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::WebSocket2::WebSocket2(const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(fmk, log), _sid(-1), _close(false), _crypt(false), _shake(false), _noinfo(false), _freeze(false), _deflate(false),
 _point(0), _threads(0), _ws1(fmk, log), _http(fmk, log), _hash(log), _frame(fmk, log), _proto(engine_t::proto_t::HTTP1_1), _resultCallback(log) {
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&ws2_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws2_t::writeCallback, this, _1, _2, _3, _4, _5));
}
/**
 * WebSocket2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::WebSocket2::WebSocket2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(core, fmk, log), _sid(-1), _close(false), _crypt(false), _shake(false), _noinfo(false), _freeze(false), _deflate(false),
 _point(0), _threads(0), _ws1(fmk, log), _http(fmk, log), _hash(log), _frame(fmk, log), _proto(engine_t::proto_t::HTTP1_1), _resultCallback(log) {
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&ws2_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws2_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Активируем персистентный запуск для работы пингов
	const_cast <client::core_t *> (this->_core)->persistEnable(true);
	// Активируем асинхронный режим работы
	const_cast <client::core_t *> (this->_core)->mode(client::core_t::mode_t::ASYNC);
}
/**
 * ~WebSocket2 Деструктор
 */
awh::client::WebSocket2::~WebSocket2() noexcept {
	// Если многопоточность активированна
	if(this->_thr.is())
		// Выполняем завершение всех активных потоков
		this->_thr.wait();
	// Если многопоточность активированна у WebSocket/1.1 клиента
	if(this->_ws1._thr.is())
		// Выполняем завершение всех активных потоков
		this->_ws1._thr.wait();
}
