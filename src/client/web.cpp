/**
 * @file: web.cpp
 * @date: 2022-11-14
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
#include <client/web.hpp>

/**
 * onFrameHttp2 Функция обратного вызова при получении фрейма заголовков HTTP/2 с сервера
 * @param session объект сессии HTTP/2
 * @param frame   объект фрейма заголовков HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::WEB::onFrameHttp2(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) session;
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Выполняем определение типа фрейма
	switch(frame->hd.type){
		// Если мы получили входящие данные тела ответа
		case NGHTTP2_DATA:
		// Если мы получили входящие данные заголовков ответа
		case NGHTTP2_HEADERS: {
			// Если сессия клиента совпадает с сессией полученных даных
			if((frame->hd.flags & NGHTTP2_FLAG_END_STREAM) && (web->_http2.id == frame->hd.stream_id)){
				// Выполняем коммит полученного результата
				web->_http.commit();
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					{
						// Получаем данные ответа
						const auto & response = web->_http.response(true);
						// Если параметры ответа получены
						if(!response.empty()){
							// Выводим параметры ответа
							cout << string(response.begin(), response.end()) << endl;
							// Если тело ответа существует
							if(!web->_http.body().empty())
								// Выводим сообщение о выводе чанка тела
								cout << web->_fmk->format("<body %u>", web->_http.body().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					}
				#endif
				// Результат работы функции
				res_t result;
				// Получаем параметры запроса
				auto query = web->_http.query();
				// Получаем объект запроса
				req_t & request = web->_requests.front();
				// Получаем объект ответа
				res_t & response = web->_responses.front();
				// Устанавливаем код ответа
				response.code = query.code;
				// Устанавливаем сообщение ответа
				response.message = query.message;
				// Получаем статус ответа
				awh::http_t::stath_t status = web->_http.getAuth();
				// Если выполнять редиректы запрещено
				if(!web->_redirects && (status == awh::http_t::stath_t::RETRY)){
					// Если нужно произвести запрос заново
					if((response.code == 201) || (response.code == 301) ||
					   (response.code == 302) || (response.code == 303) ||
					   (response.code == 307) || (response.code == 308))
							// Запрещаем выполнять редирект
							status = awh::http_t::stath_t::GOOD;
				}
				// Выполняем анализ результата авторизации
				switch(static_cast <uint8_t> (status)){
					// Если нужно попытаться ещё раз
					case static_cast <uint8_t> (awh::http_t::stath_t::RETRY): {
						// Если попытка повторить авторизацию ещё не проводилась
						if(request.attempt < web->_attempts){
							// Получаем новый адрес запроса
							const uri_t::url_t & url = web->_http.getUrl();
							// Если адрес запроса получен
							if(!url.empty()){
								// Увеличиваем количество попыток
								request.attempt++;
								// Устанавливаем новый адрес запроса
								web->_scheme.url = std::forward <const uri_t::url_t> (url);
								// Получаем параметры адреса запроса
								request.query = web->_uri.query(web->_scheme.url);
								// Если соединение является постоянным
								if(web->_http.isAlive())
									// Отправляем повторный запрос
									web->send();
								// Если нам необходимо отключиться
								else const_cast <client::core_t *> (web->_core)->close(web->_aid);
								// Завершаем работу
								return 0;
							}
						}
						// Устанавливаем флаг принудительной остановки
						web->_stopped = true;
					} break;
					// Если запрос выполнен удачно
					case static_cast <uint8_t> (awh::http_t::stath_t::GOOD): {
						// Получаем объект ответа
						result = response;
						// Выполняем сброс количества попыток
						request.attempt = 0;
						// Если функция обратного вызова установлена, выводим сообщение
						if(web->_callback.message != nullptr){
							// Получаем тело запроса
							const auto & entity = web->_http.body();
							// Получаем заголовки ответа
							const auto & headers = web->_http.headers();
							// Устанавливаем тело ответа
							result.entity.assign(entity.begin(), entity.end());
							// Устанавливаем заголовки ответа
							result.headers = std::forward <const unordered_multimap <string, string>> (headers);
						}
						// Устанавливаем размер стопбайт
						if(!web->_http.isAlive()){
							// Выполняем очистку оставшихся данных
							web->_buffer.clear();
							// Завершаем работу
							const_cast <client::core_t *> (web->_core)->close(web->_aid);
							// Выполняем завершение работы
							goto Stop;
						}
						// Выполняем сброс состояния HTTP парсера
						web->_http.reset();
						// Выполняем очистку параметров HTTP запроса
						web->_http.clear();
						// Если объект ещё не удалён
						if(!web->_requests.empty())
							// Выполняем удаление объекта запроса
							web->_requests.erase(web->_requests.begin());
						// Если объект ещё не удалён
						if(!web->_responses.empty())
							// Выполняем удаление объекта ответа
							web->_responses.erase(web->_responses.begin());
						// Выполняем завершение работы
						goto Stop;
					} break;
					// Если запрос неудачный
					case static_cast <uint8_t> (awh::http_t::stath_t::FAULT):
						// Устанавливаем флаг принудительной остановки
						web->_stopped = true;
					break;
				}
				// Если функция обратного вызова установлена, выводим сообщение
				if(web->_callback.message != nullptr){
					// Получаем объект ответа
					result = response;
					// Получаем тело запроса
					const auto & entity = web->_http.body();
					// Получаем заголовки ответа
					const auto & headers = web->_http.headers();
					// Устанавливаем тело ответа
					result.entity.assign(entity.begin(), entity.end());
					// Устанавливаем заголовки ответа
					result.headers = std::forward <const unordered_multimap <string, string>> (headers);
					// Завершаем работу
					const_cast <client::core_t *> (web->_core)->close(web->_aid);
				// Завершаем работу
				} else const_cast <client::core_t *> (web->_core)->close(web->_aid);
				// Устанавливаем метку завершения работы
				Stop:
				// Если функция обратного вызова установлена, выводим сообщение
				if(web->_callback.message != nullptr)
					// Выполняем функцию обратного вызова
					web->receive(std::move(result));
			}
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * onCloseHttp2 Метод закрытия подключения с сервером HTTP/2
 * @param session объект сессии HTTP/2
 * @param sid     идентификатор сессии HTTP/2
 * @param error   флаг ошибки HTTP/2 если присутствует
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученного события
 */
int awh::client::WEB::onCloseHttp2(nghttp2_session * session, const int32_t sid, const uint32_t error, void * ctx) noexcept {
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Если идентификатор сессии клиента совпадает
	if(web->_http2.id == sid){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим заголовок ответа
			cout << "\x1B[33m\x1B[1m^^^^^^^^^ CLOSE SESSION HTTP2 ^^^^^^^^^\x1B[0m" << endl;
			// Определяем тип получаемой ошибки
			switch(error){
				// Если ошибка не получена
				case 0x0:
					// Выводим информацию о закрытии сессии
					cout << web->_fmk->format("Stream %d closed", sid) << endl << endl;
				break;
				// Если получена ошибка протокола
				case 0x1:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "PROTOCOL_ERROR") << endl << endl;
				break;
				// Если получена ошибка реализации
				case 0x2:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "INTERNAL_ERROR") << endl << endl;
				break;
				// Если получена ошибка превышения предела управления потоком
				case 0x3:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "FLOW_CONTROL_ERROR") << endl << endl;
				break;
				// Если установка не подтверждённа
				case 0x4:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "SETTINGS_TIMEOUT") << endl << endl;
				break;
				// Если получен кадр для завершения потока
				case 0x5:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "STREAM_CLOSED") << endl << endl;
				break;
				// Если размер кадра некорректен
				case 0x6:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "FRAME_SIZE_ERROR") << endl << endl;
				break;
				// Если поток не обработан
				case 0x7:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "REFUSED_STREAM") << endl << endl;
				break;
				// Если поток аннулирован
				case 0x8:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "CANCEL") << endl << endl;
				break;
				// Если состояние компрессии не обновлено
				case 0x9:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "COMPRESSION_ERROR") << endl << endl;
				break;
				// Если получена ошибка TCP-соединения для метода CONNECT
				case 0xa:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "CONNECT_ERROR") << endl << endl;
				break;
				// Если превышена емкость для обработки
				case 0xb:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "ENHANCE_YOUR_CALM") << endl << endl;
				break;
				// Если согласованные параметры TLS не приемлемы
				case 0xc:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "INADEQUATE_SECURITY") << endl << endl;
				break;
				// Если для запроса используется HTTP/1.1
				case 0xd:
					// Выводим информацию о закрытии сессии с ошибкой
					cout << web->_fmk->format("Stream %d closed with error=%s", sid, "HTTP_1_1_REQUIRED") << endl << endl;
				break;
			}
		#endif
		// Отключаем флаг HTTP/2 так-как сессия уже закрыта
		web->_http2.mode = false;
		// Если сессия HTTP/2 закрыта не удачно
		if(nghttp2_session_terminate_session(session, NGHTTP2_NO_ERROR) != 0)
			// Выводим сообщение об ошибке
			return NGHTTP2_ERR_CALLBACK_FAILURE;
	}
	// Выводим результат
	return 0;
}
/**
 * onChunkHttp2 Функция обратного вызова при получении чанка с сервера HTTP/2
 * @param session объект сессии HTTP/2
 * @param flags   флаги события для сессии HTTP/2
 * @param sid     идентификатор сессии HTTP/2
 * @param buffer  буфер данных который содержит полученный чанк
 * @param size    размер полученного буфера данных чанка
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::WEB::onChunkHttp2(nghttp2_session * session, const uint8_t flags, const int32_t sid, const uint8_t * buffer, const size_t size, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Если идентификатор сессии клиента совпадает
	if(web->_http2.id == sid)
		// Добавляем полученный чанк в тело данных
		web->_http.body(vector <char> (buffer, buffer + size));
	// Выводим результат
	return 0;
}
/**
 * onBeginHeadersHttp2 Функция начала получения фрейма заголовков HTTP/2
 * @param session объект сессии HTTP/2
 * @param frame   объект фрейма заголовков HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::WEB::onBeginHeadersHttp2(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) session;
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Выполняем определение типа фрейма
	switch(frame->hd.type){
		// Если мы получили входящие данные заголовков ответа
		case NGHTTP2_HEADERS:{
			// Если сессия клиента совпадает с сессией полученных даных
			if((frame->headers.cat == NGHTTP2_HCAT_RESPONSE) && (web->_http2.id == frame->hd.stream_id)){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим информацию об ошибке
					cout << web->_fmk->format("Stream ID=%d", frame->hd.stream_id) << endl << endl;
				#endif
			}
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * onHeaderHttp2 Функция обратного вызова при получении заголовка HTTP/2
 * @param session объект сессии HTTP/2
 * @param frame   объект фрейма заголовков HTTP/2
 * @param key     данные ключа заголовка
 * @param keySize размер ключа заголовка
 * @param val     данные значения заголовка
 * @param valSize размер значения заголовка
 * @param flags   флаги события для сессии HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::WEB::onHeaderHttp2(nghttp2_session * session, const nghttp2_frame * frame, const uint8_t * key, const size_t keySize, const uint8_t * val, const size_t valSize, const uint8_t flags, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Выполняем определение типа фрейма
	switch(frame->hd.type){
		// Если мы получили входящие данные заголовков ответа
		case NGHTTP2_HEADERS:{
			// Если сессия клиента совпадает с сессией полученных даных
			if((frame->headers.cat == NGHTTP2_HCAT_RESPONSE) && (web->_http2.id == frame->hd.stream_id))
				// Устанавливаем полученные заголовки
				web->_http.header2(string((const char *) key, keySize), string((const char *) val, valSize));
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * sendHttp2 Функция обратного вызова при подготовки данных для отправки на сервер
 * @param session объект сессии HTTP/2
 * @param buffer  буфер данных которые следует отправить
 * @param size    размер буфера данных для отправки
 * @param flags   флаги события для сессии HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        количество отправленных байт
 */
ssize_t awh::client::WEB::sendHttp2(nghttp2_session * session, const uint8_t * buffer, const size_t size, const int flags, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Получаем объект HTTP-клиента
	web_t * web = reinterpret_cast <web_t *> (ctx);
	// Выполняем отправку заголовков запроса на сервер
	const_cast <client::core_t *> (web->_core)->write((const char *) buffer, size, web->_aid);
	// Возвращаем количество отправленных байт
	return static_cast <ssize_t> (size);
}
/**
 * readHttp2 Функция чтения подготовленных данных для формирования буфера данных который необходимо отправить на HTTP/2 сервер
 * @param session объект сессии HTTP/2
 * @param sid     идентификатор сессии HTTP/2
 * @param buffer  буфер данных которые следует отправить
 * @param size    размер буфера данных для отправки
 * @param flags   флаги события для сессии HTTP/2
 * @param source  объект промежуточных данных локального подключения
 * @param ctx     передаваемый промежуточный контекст
 * @return        количество отправленных байт
 */
ssize_t awh::client::WEB::readHttp2(nghttp2_session * session, const int32_t sid, uint8_t * buffer, const size_t size, uint32_t * flags, nghttp2_data_source * source, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) sid;
	(void) ctx;
	(void) session;
	// Результат работы функции
	ssize_t result = -1;
	/**
	 * Методы только для OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		// Выполняем чтение данных из сокета в буфер данных
		while(((result = _read(source->fd, buffer, size)) == -1) && (errno == EINTR));
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		// Выполняем чтение данных из сокета в буфер данных
		while(((result = ::read(source->fd, buffer, size)) == -1) && (errno == EINTR));
	#endif
	// Если данные не прочитанны из сокета
	if(result < 0)
		// Выводим сообщение об ошибке
		return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
	// Если все данные прочитаны полностью
	else if(result == 0) {
		/**
		 * Методы только для OS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Выполняем закрытие подключения
			_close(source->fd);
		/**
		 * Для всех остальных операционных систем
		 */
		#else
			// Выполняем закрытие подключения
			::close(source->fd);
		#endif
		// Устанавливаем флаг, завершения чтения данных
		(* flags) |= NGHTTP2_DATA_FLAG_EOF;
	}
	// Выводим количество прочитанных байт
	return result;
}
/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::client::WEB::chunking(const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <awh::http_t *> (http)->body(chunk);
}
/**
 * ping Метод выполнения пинга сервера
 * @return результат работы пинга
 */
bool awh::client::WEB::ping() noexcept {
	// Если протокол подключения установлен как HTTP/2
	if(this->_http2.mode && (this->_http2.ctx != nullptr)){
		// Результат выполнения поерации
		int rv = -1;
		// Выполняем пинг удалённого сервера
		if((rv = nghttp2_submit_ping(this->_http2.ctx, 0, nullptr)) != 0){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Выходим из функции
			return false;
		}
		// Фиксируем отправленный результат
		if((rv = nghttp2_session_send(this->_http2.ctx)) != 0){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Выходим из функции
			return false;
		}
		// Выводим результат
		return true;
	}
	// Выводим результат
	return false;
}
/** 
 * submit Метод выполнения удалённого запроса на сервер
 * @param request объект запроса на удалённый сервер
 */
void awh::client::WEB::submit(const req_t & request) noexcept {
	// Если подключение выполнено
	if(this->_aid > 0){
		// Выполняем сброс состояния HTTP парсера
		this->_http.reset();
		// Выполняем очистку параметров HTTP запроса
		this->_http.clear();
		// Устанавливаем метод компрессии
		this->_http.compress(this->_compress);
		// Если список заголовков получен
		if(!request.headers.empty())
			// Устанавливаем заголовоки запроса
			this->_http.headers(request.headers);
		// Если тело запроса существует
		if(!request.entity.empty())
			// Устанавливаем тело запроса
			this->_http.body(request.entity);
		// Получаем объект биндинга ядра TCP/IP
		client::core_t * core = const_cast <client::core_t *> (this->_core);
		// Если активирован режим работы с HTTP/2 протоколом
		if(this->_http2.mode){
			// Список заголовков для запроса
			vector <nghttp2_nv> nva;
			// Получаем URL ссылку для выполнения запроса
			this->_uri.append(this->_scheme.url, request.query);
			// Выполняем установку параметры ответа сервера
			this->_http.query(awh::web_t::query_t(2.0f, request.method));
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим заголовок запроса
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
				// Получаем бинарные данные WEB запроса
				const auto & buffer = this->_http.request(this->_scheme.url, request.method);
				// Выводим параметры запроса
				cout << string(buffer.begin(), buffer.end()) << endl;
			#endif
			// Выполняем запрос на получение заголовков
			const auto & headers = this->_http.request2(this->_scheme.url, request.method);
			// Выполняем перебор всех заголовков HTTP/2 запроса
			for(auto & header : headers){
				// Выполняем добавление метода запроса
				nva.push_back({
					(uint8_t *) header.first.c_str(),
					(uint8_t *) header.second.c_str(),
					header.first.size(),
					header.second.size(),
					NGHTTP2_NV_FLAG_NONE
				});
			}
			// Если тело запроса существует
			if(!request.entity.empty()){
				// Список файловых дескрипторов
				int fds[2];
				// Тело WEB сообщения
				vector <char> entity;
				/**
				 * Методы только для OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
					const int rv = _pipe(fds, 4096, O_BINARY);
				/**
				 * Для всех остальных операционных систем
				 */
				#else
					// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
					const int rv = ::pipe(fds);
				#endif
				// Выполняем подписку на основной канал передачи данных
				if(rv != 0){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					// Выполняем закрытие подключения
					core->close(this->_aid);
					// Выходим из функции
					return;
				}
				// Получаем данные тела запроса
				while(!(entity = this->_http.payload()).empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка тела
						cout << this->_fmk->format("<chunk %u>", entity.size()) << endl << endl;
					#endif
					/**
					 * Методы только для OS Windows
					 */
					#if defined(_WIN32) || defined(_WIN64)
						// Если данные небыли записаны в сокет
						if(static_cast <int> (_write(fds[1], entity.data(), entity.size())) != static_cast <int> (entity.size())){
							// Выполняем закрытие сокета для чтения
							_close(fds[0]);
							// Выполняем закрытие сокета для записи
							_close(fds[1]);
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выполняем закрытие подключения
							core->close(this->_aid);
							// Выходим из функции
							return;
						}
					/**
					 * Для всех остальных операционных систем
					 */
					#else
						// Если данные небыли записаны в сокет
						if(static_cast <int> (::write(fds[1], entity.data(), entity.size())) != static_cast <int> (entity.size())){
							// Выполняем закрытие сокета для чтения
							::close(fds[0]);
							// Выполняем закрытие сокета для записи
							::close(fds[1]);
							// Выводим в лог сообщение
							this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
							// Выполняем закрытие подключения
							core->close(this->_aid);
							// Выходим из функции
							return;
						}
					#endif
				}
				/**
				 * Методы только для OS Windows
				 */
				#if defined(_WIN32) || defined(_WIN64)
					// Выполняем закрытие подключения
					_close(fds[1]);
				/**
				 * Для всех остальных операционных систем
				 */
				#else
					// Выполняем закрытие подключения
					::close(fds[1]);
				#endif
				// Создаём объект передачи данных тела полезной нагрузки
				nghttp2_data_provider data;
				// Зануляем передаваемый контекст
				data.source.ptr = nullptr;
				// Устанавливаем файловый дескриптор
				data.source.fd = fds[0];
				// Устанавливаем функцию обратного вызова
				data.read_callback = &web_t::readHttp2;
				// Выполняем запрос на удалённый сервер
				this->_http2.id = nghttp2_submit_request(this->_http2.ctx, nullptr, nva.data(), nva.size(), &data, this);
			// Если тело запроса не существует, выполняем установленный запрос
			} else this->_http2.id = nghttp2_submit_request(this->_http2.ctx, nullptr, nva.data(), nva.size(), nullptr, this);
			// Если запрос не получилось отправить
			if(this->_http2.id < 0){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(this->_http2.id));
				// Выполняем закрытие подключения
				core->close(this->_aid);
				// Выходим из функции
				return;
			}{
				// Результат фиксации сессии
				int rv = -1;
				// Фиксируем отправленный результат
				if((rv = nghttp2_session_send(this->_http2.ctx)) != 0){
					// Выводим сообщение об полученной ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
					// Выходим из функции
					return;
				}
			}
		// Если активирован режим работы с HTTP/1.1 протоколом
		} else {
			// Получаем URL ссылку для выполнения запроса
			this->_uri.append(this->_scheme.url, request.query);
			// Получаем бинарные данные WEB запроса
			const auto & buffer = this->_http.request(this->_scheme.url, request.method);
			// Если бинарные данные запроса получены
			if(!buffer.empty()){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок запроса
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры запроса
					cout << string(buffer.begin(), buffer.end()) << endl << endl;
				#endif
				// Тело WEB сообщения
				vector <char> entity;
				// Выполняем отправку заголовков запроса на сервер
				core->write(buffer.data(), buffer.size(), this->_aid);
				// Получаем данные тела запроса
				while(!(entity = this->_http.payload()).empty()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение о выводе чанка тела
						cout << this->_fmk->format("<chunk %u>", entity.size()) << endl;
					#endif
					// Выполняем отправку тела запроса на сервер
					core->write(entity.data(), entity.size(), this->_aid);
				}
			}	
		}
	}
}
/**
 * receive Метод получения результата запроса
 * @param response объект ответа полученного с удалённого сервера
 */
void awh::client::WEB::receive(const res_t & response) noexcept {
	// Если функция обратного вызова установлена, выводим сообщение
	if(this->_callback.message != nullptr)
		// Выполняем функцию обратного вызова
		this->_callback.message(response, this);
	// Если подключение выполнено и список запросов не пустой
	if((this->_aid > 0) && !this->_requests.empty())
		// Выполняем запрос на удалённый сервер
		this->submit(this->_requests.front());
}
/**
 * openCallback Метод обратного вызова при запуске работы
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WEB::openCallback(const size_t sid, awh::core_t * core) noexcept {
	// Если дисконнекта ещё не произошло
	if(this->_action == action_t::NONE){
		// Устанавливаем экшен выполнения
		this->_action = action_t::OPEN;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::client::WEB::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Если система была остановлена
		if(status == awh::core_t::status_t::STOP){
			// Если контекст сессии HTTP/2 создан
			if(this->_http2.mode && (this->_http2.ctx != nullptr))
				// Выполняем удаление сессии
				nghttp2_session_del(this->_http2.ctx);
			// Деактивируем флаг работы с протоколом HTTP/2
			this->_http2.mode = false;
		}
		// Если функция обратного вызова установлена
		if(this->_callback.events != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.events(status, core);
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WEB::persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Если сервер уже отключился
		if(!this->ping())
			// Завершаем работу
			reinterpret_cast <client::core_t *> (core)->close(aid);
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WEB::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Устанавливаем экшен выполнения
		this->_action = action_t::CONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WEB::disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((sid > 0) && (core != nullptr)){
		// Устанавливаем экшен выполнения
		this->_action = action_t::DISCONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::WEB::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Если протокол подключения является HTTP/2
		if(core->proto(aid) == engine_t::proto_t::HTTP2){
			// Выполняем извлечение полученного чанка данных из сокета
			ssize_t bytes = nghttp2_session_mem_recv(this->_http2.ctx, (const uint8_t *) buffer, size);
			// Если данные не прочитаны, выводим ошибку и выходим
			if(bytes < 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
				// Выходим из функции
				return;
			}
			// Фиксируем полученный результат
			if((bytes = nghttp2_session_send(this->_http2.ctx)) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
				// Выходим из функции
				return;
			}
		// Если активирован режим работы с HTTP/1.1 протоколом
		} else {
			// Если дисконнекта ещё не произошло
			if((this->_action == action_t::NONE) || (this->_action == action_t::READ)){
				// Устанавливаем экшен выполнения
				this->_action = action_t::READ;
				// Добавляем полученные данные в буфер
				this->_buffer.insert(this->_buffer.end(), buffer, buffer + size);
				// Выполняем запуск обработчика событий
				this->handler();
			}
		}
	}
}
/**
 * enableTLSCallback Метод активации зашифрованного канала TLS
 * @param url  адрес сервера для которого выполняется активация зашифрованного канала TLS
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат активации зашифрованного канала TLS
 */
bool awh::client::WEB::enableTLSCallback(const uri_t::url_t & url, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Блокируем переменные которые не используем
	(void) aid;
	(void) sid;
	(void) core;
	// Выводим результат активации
	return (!url.empty() && this->_fmk->compare(url.schema, "https"));
}
/**
 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WEB::proxyConnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Устанавливаем экшен выполнения
		this->_action = action_t::PROXY_CONNECT;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * proxyReadCallback Метод обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::WEB::proxyReadCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Если дисконнекта ещё не произошло
		if((this->_action == action_t::NONE) || (this->_action == action_t::PROXY_READ)){
			// Устанавливаем экшен выполнения
			this->_action = action_t::PROXY_READ;
			// Добавляем полученные данные в буфер
			this->_buffer.insert(this->_buffer.end(), buffer, buffer + size);
			// Выполняем запуск обработчика событий
			this->handler();
		}
	}
}
/**
 * handler Метод управления входящими методами
 */
void awh::client::WEB::handler() noexcept {
	// Если управляющий блокировщик не заблокирован
	if(!this->_locker.mode){
		// Выполняем блокировку потока
		const lock_guard <recursive_mutex> lock(this->_locker.mtx);
		// Флаг разрешающий циклический перебор экшенов
		bool loop = true;
		// Выполняем блокировку обработчика
		this->_locker.mode = true;
		// Выполняем обработку всех экшенов
		while(loop && (this->_action != action_t::NONE)){
			// Определяем обрабатываемый экшен
			switch(static_cast <uint8_t> (this->_action)){
				// Если необходимо запустить экшен открытия подключения
				case static_cast <uint8_t> (action_t::OPEN): this->actionOpen(); break;
				// Если необходимо запустить экшен обработки данных поступающих с сервера
				case static_cast <uint8_t> (action_t::READ): this->actionRead(); break;
				// Если необходимо запустить экшен обработки подключения к серверу
				case static_cast <uint8_t> (action_t::CONNECT): this->actionConnect(); break;
				// Если необходимо запустить экшен обработки отключения от сервера
				case static_cast <uint8_t> (action_t::DISCONNECT): this->actionDisconnect(); break;
				// Если необходимо запустить экшен обработки данных поступающих с прокси-сервера
				case static_cast <uint8_t> (action_t::PROXY_READ): this->actionProxyRead(); break;
				// Если необходимо запустить экшен обработки подключения к прокси-серверу
				case static_cast <uint8_t> (action_t::PROXY_CONNECT): this->actionProxyConnect(); break;
				// Если сработал неизвестный экшен, выходим
				default: loop = false;
			}
		}
		// Выполняем разблокировку обработчика
		this->_locker.mode = false;
	}
}
/**
 * actionOpen Метод обработки экшена открытия подключения
 */
void awh::client::WEB::actionOpen() noexcept {
	// Выполняем подключение
	this->open();
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::OPEN)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
}
/**
 * actionRead Метод обработки экшена чтения с сервера
 */
void awh::client::WEB::actionRead() noexcept {
	// Результат работы функции
	res_t result;
	// Флаг удачного получения данных
	bool receive = false;
	// Флаг завершения работы
	bool completed = false;
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->_core);
	// Выполняем обработку полученных данных
	while(!this->_active){
		// Получаем объект запроса
		req_t & request = this->_requests.front();
		// Получаем объект ответа
		res_t & response = this->_responses.front();
		// Выполняем парсинг полученных данных
		size_t bytes = this->_http.parse(this->_buffer.data(), this->_buffer.size());
		// Если все данные получены
		if((response.ok = completed = this->_http.isEnd())){
			// Получаем параметры запроса
			auto query = this->_http.query();
			// Устанавливаем код ответа
			response.code = query.code;
			// Устанавливаем сообщение ответа
			response.message = query.message;
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				{
					// Получаем данные ответа
					const auto & response = this->_http.response(true);
					// Если параметры ответа получены
					if(!response.empty()){
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
						// Выводим параметры ответа
						cout << string(response.begin(), response.end()) << endl;
						// Если тело ответа существует
						if(!this->_http.body().empty())
							// Выводим сообщение о выводе чанка тела
							cout << this->_fmk->format("<body %u>", this->_http.body().size()) << endl << endl;
						// Иначе устанавливаем перенос строки
						else cout << endl;
					}
				}
			#endif
			// Получаем статус ответа
			awh::http_t::stath_t status = this->_http.getAuth();
			// Если выполнять редиректы запрещено
			if(!this->_redirects && (status == awh::http_t::stath_t::RETRY)){
				// Если нужно произвести запрос заново
				if((response.code == 201) || (response.code == 301) ||
				   (response.code == 302) || (response.code == 303) ||
				   (response.code == 307) || (response.code == 308))
						// Запрещаем выполнять редирект
						status = awh::http_t::stath_t::GOOD;
			}
			// Выполняем анализ результата авторизации
			switch(static_cast <uint8_t> (status)){
				// Если нужно попытаться ещё раз
				case static_cast <uint8_t> (awh::http_t::stath_t::RETRY): {
					// Если попытка повторить авторизацию ещё не проводилась
					if(request.attempt < this->_attempts){
						// Получаем новый адрес запроса
						const uri_t::url_t & url = this->_http.getUrl();
						// Если адрес запроса получен
						if(!url.empty()){
							// Увеличиваем количество попыток
							request.attempt++;
							// Устанавливаем новый адрес запроса
							this->_scheme.url = std::forward <const uri_t::url_t> (url);
							// Получаем параметры адреса запроса
							request.query = this->_uri.query(this->_scheme.url);
							// Выполняем очистку оставшихся данных
							this->_buffer.clear();
							// Если экшен соответствует, выполняем его сброс
							if(this->_action == action_t::READ)
								// Выполняем сброс экшена
								this->_action = action_t::NONE;
							// Если соединение является постоянным
							if(this->_http.isAlive())
								// Отправляем повторный запрос
								this->send();
							// Если нам необходимо отключиться
							else core->close(this->_aid);
							// Завершаем работу
							return;
						}
					}
					// Устанавливаем флаг принудительной остановки
					this->_stopped = true;
				} break;
				// Если запрос выполнен удачно
				case static_cast <uint8_t> (awh::http_t::stath_t::GOOD): {
					// Получаем объект ответа
					result = response;
					// Выполняем сброс количества попыток
					request.attempt = 0;
					// Если функция обратного вызова установлена, выводим сообщение
					if(this->_callback.message != nullptr){
						// Получаем тело запроса
						const auto & entity = this->_http.body();
						// Получаем заголовки ответа
						const auto & headers = this->_http.headers();
						// Устанавливаем тело ответа
						result.entity.assign(entity.begin(), entity.end());
						// Устанавливаем заголовки ответа
						result.headers = std::forward <const unordered_multimap <string, string>> (headers);
					}
					// Устанавливаем размер стопбайт
					if(!this->_http.isAlive()){
						// Выполняем очистку оставшихся данных
						this->_buffer.clear();
						// Завершаем работу
						core->close(this->_aid);
						// Выполняем завершение работы
						goto Stop;
					}
					// Выполняем сброс состояния HTTP парсера
					this->_http.reset();
					// Выполняем очистку параметров HTTP запроса
					this->_http.clear();
					// Если объект ещё не удалён
					if(!this->_requests.empty())
						// Выполняем удаление объекта запроса
						this->_requests.erase(this->_requests.begin());
					// Если объект ещё не удалён
					if(!this->_responses.empty())
						// Выполняем удаление объекта ответа
						this->_responses.erase(this->_responses.begin());
					// Завершаем обработку
					goto Next;
				} break;
				// Если запрос неудачный
				case static_cast <uint8_t> (awh::http_t::stath_t::FAULT):
					// Устанавливаем флаг принудительной остановки
					this->_stopped = true;
				break;
			}
			// Выполняем очистку оставшихся данных
			this->_buffer.clear();
			// Если функция обратного вызова установлена, выводим сообщение
			if(this->_callback.message != nullptr){
				// Получаем объект ответа
				result = response;
				// Получаем тело запроса
				const auto & entity = this->_http.body();
				// Получаем заголовки ответа
				const auto & headers = this->_http.headers();
				// Устанавливаем тело ответа
				result.entity.assign(entity.begin(), entity.end());
				// Устанавливаем заголовки ответа
				result.headers = std::forward <const unordered_multimap <string, string>> (headers);
				// Завершаем работу
				core->close(this->_aid);
			// Завершаем работу
			} else core->close(this->_aid);
			// Выполняем завершение работы
			goto Stop;
		}
		// Устанавливаем метку продолжения обработки пайплайна
		Next:
		// Если парсер обработал какое-то количество байт
		if((receive = ((bytes > 0) && !this->_buffer.empty()))){
			// Если размер буфера больше количества удаляемых байт
			if((receive = (this->_buffer.size() >= bytes)))
				// Удаляем количество обработанных байт
				this->_buffer.assign(this->_buffer.begin() + bytes, this->_buffer.end());
				// vector <decltype(this->_buffer)::value_type> (this->_buffer.begin() + bytes, this->_buffer.end()).swap(this->_buffer);
		}
		// Если данные мы все получили, выходим
		if(!receive || this->_buffer.empty()) break;
	}
	// Устанавливаем метку завершения работы
	Stop:
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::READ)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
	// Если функция обратного вызова установлена, выводим сообщение
	if(completed && (this->_callback.message != nullptr))
		// Выполняем функцию обратного вызова
		this->receive(std::move(result));
}
/**
 * actionConnect Метод обработки экшена подключения к серверу
 */
void awh::client::WEB::actionConnect() noexcept {
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::CONNECT)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
	// Если протокол подключения является HTTP/2
	if(!this->_http2.mode && (this->_core->proto(this->_aid) == engine_t::proto_t::HTTP2)){
		// Создаём объект функций обратного вызова
		nghttp2_session_callbacks * callbacks;
		// Выполняем инициализацию сессию функций обратного вызова
		nghttp2_session_callbacks_new(&callbacks);
		// Выполняем установку функции обратного вызова при подготовки данных для отправки на сервер
		nghttp2_session_callbacks_set_send_callback(callbacks, &web_t::sendHttp2);
		// Выполняем установку функции обратного вызова при получении заголовка HTTP/2
		nghttp2_session_callbacks_set_on_header_callback(callbacks, &web_t::onHeaderHttp2);
		// Выполняем установку функции обратного вызова при получении фрейма заголовков HTTP/2 с сервера
		nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, &web_t::onFrameHttp2);
		// Выполняем установку функции обратного вызова закрытия подключения с сервером HTTP/2
		nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, &web_t::onCloseHttp2);
		// Выполняем установку функции обратного вызова при получении чанка с сервера HTTP/2
		nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, &web_t::onChunkHttp2);
		// Выполняем установку функции обратного вызова начала получения фрейма заголовков HTTP/2
		nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, &web_t::onBeginHeadersHttp2);
		// Выполняем подключение котнекста сессии HTTP/2
		nghttp2_session_client_new(&this->_http2.ctx, callbacks, this);
		// Выполняем удаление объекта функций обратного вызова
		nghttp2_session_callbacks_del(callbacks);
		// Создаём параметры сессии подключения с HTTP/2 сервером
		const vector <nghttp2_settings_entry> iv = {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 128}};
		// Клиентская 24-байтовая магическая строка будет отправлена библиотекой nghttp2
		const int rv = nghttp2_submit_settings(this->_http2.ctx, NGHTTP2_FLAG_NONE, iv.data(), iv.size());
		// Если настройки для сессии установить не удалось
		if(rv != 0){
			// Выводим сообщение об ошибке
			this->_log->print("Could not submit SETTINGS: %s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Выполняем закрытие подключения
			const_cast <client::core_t *> (this->_core)->close(this->_aid);
			// Если сессия HTTP/2 создана удачно
			if(this->_http2.ctx != nullptr)
				// Выполняем удаление сессии
				nghttp2_session_del(this->_http2.ctx);
			// Выходим из функции
			return;
		}
		// Если список источников установлен
		if(!this->_origins.empty())
			// Выполняем отправку списка источников
			this->sendOrigin(this->_origins);
		// Выполняем активацию работы с протоколом HTTP/2
		this->_http2.mode = !this->_http2.mode;
	}
	// Если функция обратного вызова существует
	if(this->_callback.active != nullptr)
		// Выполняем функцию обратного вызова
		this->_callback.active(mode_t::CONNECT, this);
}
/**
 * actionDisconnect Метод обработки экшена отключения от сервера
 */
void awh::client::WEB::actionDisconnect() noexcept {
	// Если список ответов получен
	if(!this->_responses.empty() && !this->_requests.empty()){
		// Получаем объект ответа
		const res_t & response = this->_responses.front();
		// Если нужно произвести запрос заново
		if(!this->_stopped && ((response.code == 201) || (response.code == 301) ||
		   (response.code == 302) || (response.code == 303) || (response.code == 307) ||
		   (response.code == 308) || (response.code == 401) || (response.code == 407))){
			// Если статус ответа требует произвести авторизацию или заголовок перенаправления указан
			if((response.code == 401) || (response.code == 407) || this->_http.isHeader("location")){
				// Получаем новый адрес запроса
				const uri_t::url_t & url = this->_http.getUrl();
				// Если адрес запроса получен
				if(!url.empty()){
					// Получаем объект запроса
					req_t & request = this->_requests.front();
					// Увеличиваем количество попыток
					request.attempt++;
					// Устанавливаем новый адрес запроса
					this->_scheme.url = std::forward <const uri_t::url_t> (url);
					// Получаем параметры адреса запроса
					request.query = this->_uri.query(this->_scheme.url);
					// Выполняем очистку оставшихся данных
					this->_buffer.clear();
					// Выполняем установку следующего экшена на открытие подключения
					this->_action = action_t::OPEN;
					// Завершаем работу
					return;
				}
			}
		}
	}
	// Если подключение является постоянным
	if(this->_scheme.alive){
		// Если экшен соответствует, выполняем его сброс
		if(this->_action == action_t::DISCONNECT)
			// Выполняем сброс экшена
			this->_action = action_t::NONE;
		// Если функция обратного вызова установлена
		if(this->_callback.active != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.active(mode_t::DISCONNECT, this);
	// Если подключение не является постоянным
	} else {
		// Получаем объект ответа
		res_t response = (!this->_responses.empty() ? std::forward <res_t> (this->_responses.front()) : res_t());
		// Если список ответов не получен, значит он был выведен ранее
		if(this->_responses.empty())
			// Устанавливаем код сообщения по умолчанию
			response.code = 1;
		// Выполняем очистку списка запросов
		this->_requests.clear();
		// Выполняем очистку списка ответов
		this->_responses.clear();
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Выполняем сброс параметров запроса
		this->flush();
		// Выполняем зануление идентификатора адъютанта
		this->_aid = 0;
		// Завершаем работу
		if(this->_unbind) const_cast <client::core_t *> (this->_core)->stop();
		// Если экшен соответствует, выполняем его сброс
		if(this->_action == action_t::DISCONNECT)
			// Выполняем сброс экшена
			this->_action = action_t::NONE;
		// Если функция обратного вызова установлена, выводим сообщение
		if((response.code == 0) && (this->_callback.message != nullptr)){
			// Устанавливаем код ответа сервера
			response.code = 500;
			// Выполняем функцию обратного вызова
			this->receive(std::move(response));
		}
		// Если функция обратного вызова существует
		if(this->_callback.active != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.active(mode_t::DISCONNECT, this);
	}
}
/**
 * actionProxyRead Метод обработки экшена чтения с прокси-сервера
 */
void awh::client::WEB::actionProxyRead() noexcept {
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->_core);
	// Определяем тип прокси-сервера
	switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
		// Если прокси-сервер является Socks5
		case static_cast <uint8_t> (proxy_t::type_t::SOCKS5): {
			// Если данные не получены
			if(!this->_scheme.proxy.socks5.isEnd()){
				// Выполняем парсинг входящих данных
				this->_scheme.proxy.socks5.parse(this->_buffer.data(), this->_buffer.size());
				// Получаем данные запроса
				const auto & buffer = this->_scheme.proxy.socks5.get();
				// Если данные получены
				if(!buffer.empty()){
					// Выполняем очистку буфера данных
					this->_buffer.clear();
					// Выполняем отправку запроса на сервер
					core->write(buffer.data(), buffer.size(), this->_aid);
					// Если экшен соответствует, выполняем его сброс
					if(this->_action == action_t::PROXY_READ)
						// Выполняем сброс экшена
						this->_action = action_t::NONE;
					// Завершаем работу
					return;
				// Если данные все получены
				} else if(this->_scheme.proxy.socks5.isEnd()) {
					// Выполняем очистку буфера данных
					this->_buffer.clear();
					// Если рукопожатие выполнено
					if(this->_scheme.proxy.socks5.isHandshake()){
						// Выполняем переключение на работу с сервером
						core->switchProxy(this->_aid);
						// Если экшен соответствует, выполняем его сброс
						if(this->_action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->_action = action_t::NONE;
						// Завершаем работу
						return;
					// Если рукопожатие не выполнено
					} else {
						// Получаем объект ответа
						res_t & response = this->_responses.front();
						// Устанавливаем код ответа
						response.code = this->_scheme.proxy.socks5.code();
						// Устанавливаем сообщение ответа
						response.message = this->_scheme.proxy.socks5.message(response.code);
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Если заголовки получены
							if(!response.message.empty()){
								// Данные WEB ответа
								const string & message = this->_fmk->format("SOCKS5 %u %s\r\n", response.code, response.message.c_str());
								// Выводим заголовок ответа
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры ответа
								cout << string(message.begin(), message.end()) << endl;
							}
						#endif
						// Если экшен соответствует, выполняем его сброс
						if(this->_action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->_action = action_t::NONE;
						// Если функция обратного вызова установлена, выводим сообщение
						if(this->_callback.message != nullptr){
							// Получаем результат ответа
							res_t result = response;
							// Завершаем работу
							core->close(this->_aid);
							// Выполняем функцию обратного вызова
							this->receive(std::move(result));
						// Завершаем работу
						} else core->close(this->_aid);
						// Завершаем работу
						return;
					}
				}
			}
		} break;
		// Если прокси-сервер является HTTP
		case static_cast <uint8_t> (proxy_t::type_t::HTTP): {
			// Выполняем парсинг полученных данных
			this->_scheme.proxy.http.parse(this->_buffer.data(), this->_buffer.size());
			// Если все данные получены
			if(this->_scheme.proxy.http.isEnd()){
				// Выполняем очистку буфера данных
				this->_buffer.clear();
				// Получаем параметры запроса
				const auto & query = this->_scheme.proxy.http.query();
				// Получаем объект запроса
				req_t & request = this->_requests.front();
				// Получаем объект ответа
				res_t & response = this->_responses.front();
				// Устанавливаем код ответа
				response.code = query.code;
				// Устанавливаем сообщение ответа
				response.message = query.message;
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					{
						// Получаем данные ответа
						const auto & response = this->_scheme.proxy.http.response(true);
						// Если параметры ответа получены
						if(!response.empty()){
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры ответа
							cout << string(response.begin(), response.end()) << endl;
							// Если тело ответа существует
							if(!this->_scheme.proxy.http.body().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->_fmk->format("<body %u>", this->_scheme.proxy.http.body().size()) << endl;
						}
					}
				#endif
				// Получаем статус ответа
				awh::http_t::stath_t status = this->_scheme.proxy.http.getAuth();
				// Если выполнять редиректы запрещено
				if(!this->_redirects && (status == awh::http_t::stath_t::RETRY)){
					// Если нужно произвести запрос заново
					if((response.code == 201) || (response.code == 301) ||
					   (response.code == 302) || (response.code == 303) ||
					   (response.code == 307) || (response.code == 308))
						// Запрещаем выполнять редирект
						status = awh::http_t::stath_t::GOOD;
				}
				// Выполняем проверку авторизации
				switch(static_cast <uint8_t> (status)){
					// Если нужно попытаться ещё раз
					case static_cast <uint8_t> (awh::http_t::stath_t::RETRY): {
						// Если попытка повторить авторизацию ещё не проводилась
						if(request.attempt < this->_attempts){
							// Если адрес запроса получен
							if(!this->_scheme.proxy.url.empty()){
								// Увеличиваем количество попыток
								request.attempt++;
								// Если соединение является постоянным
								if(this->_scheme.proxy.http.isAlive())
									// Устанавливаем новый экшен выполнения
									this->_action = action_t::PROXY_CONNECT;
								// Если соединение должно быть закрыто
								else {
									// Если экшен соответствует, выполняем его сброс
									if(this->_action == action_t::PROXY_READ)
										// Выполняем сброс экшена
										this->_action = action_t::NONE;
									// Завершаем работу
									core->close(this->_aid);
								}
								// Завершаем работу
								return;
							}
						}
						// Устанавливаем флаг принудительной остановки
						this->_stopped = true;
					} break;
					// Если запрос выполнен удачно
					case static_cast <uint8_t> (awh::http_t::stath_t::GOOD): {
						// Выполняем сброс количества попыток
						request.attempt = 0;
						// Выполняем переключение на работу с сервером
						core->switchProxy(this->_aid);
						// Если экшен соответствует, выполняем его сброс
						if(this->_action == action_t::PROXY_READ)
							// Выполняем сброс экшена
							this->_action = action_t::NONE;
						// Завершаем работу
						return;
					} break;
					// Если запрос неудачный
					case static_cast <uint8_t> (awh::http_t::stath_t::FAULT): {
						// Устанавливаем флаг принудительной остановки
						this->_stopped = true;
						// Получаем тело запроса
						const auto & entity = this->_scheme.proxy.http.body();
						// Устанавливаем заголовки ответа
						response.headers = this->_scheme.proxy.http.headers();
						// Устанавливаем тело ответа
						response.entity.assign(entity.begin(), entity.end());
					} break;
				}
				// Если экшен соответствует, выполняем его сброс
				if(this->_action == action_t::PROXY_READ)
					// Выполняем сброс экшена
					this->_action = action_t::NONE;
				// Если функция обратного вызова установлена, выводим сообщение
				if(this->_callback.message != nullptr){
					// Получаем результат ответа
					res_t result = response;
					// Завершаем работу
					core->close(this->_aid);
					// Выполняем функцию обратного вызова
					this->receive(std::move(result));
				// Завершаем работу
				} else core->close(this->_aid);
				// Завершаем работу
				return;
			}
		} break;
		// Иначе завершаем работу
		default: {
			// Если экшен соответствует, выполняем его сброс
			if(this->_action == action_t::PROXY_READ)
				// Выполняем сброс экшена
				this->_action = action_t::NONE;
			// Завершаем работу
			core->close(this->_aid);
		}
	}
}
/**
 * actionProxyConnect Метод обработки экшена подключения к прокси-серверу
 */
void awh::client::WEB::actionProxyConnect() noexcept {
	// Получаем объект биндинга ядра TCP/IP
	client::core_t * core = const_cast <client::core_t *> (this->_core);
	// Определяем тип прокси-сервера
	switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
		// Если прокси-сервер является Socks5
		case static_cast <uint8_t> (proxy_t::type_t::SOCKS5): {
			// Выполняем сброс состояния Socks5 парсера
			this->_scheme.proxy.socks5.reset();
			// Устанавливаем URL адрес запроса
			this->_scheme.proxy.socks5.url(this->_scheme.url);
			// Выполняем создание буфера запроса
			this->_scheme.proxy.socks5.parse();
			// Получаем данные запроса
			const auto & buffer = this->_scheme.proxy.socks5.get();
			// Если данные получены
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				core->write(buffer.data(), buffer.size(), this->_aid);
		} break;
		// Если прокси-сервер является HTTP
		case static_cast <uint8_t> (proxy_t::type_t::HTTP): {
			// Выполняем сброс состояния HTTP парсера
			this->_scheme.proxy.http.reset();
			// Выполняем очистку параметров HTTP запроса
			this->_scheme.proxy.http.clear();
			// Получаем бинарные данные WEB запроса
			const auto & buffer = this->_scheme.proxy.http.proxy(this->_scheme.url);
			// Если бинарные данные запроса получены
			if(!buffer.empty()){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок запроса
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST PROXY ^^^^^^^^^\x1B[0m" << endl;
					// Выводим параметры запроса
					cout << string(buffer.begin(), buffer.end()) << endl;
				#endif
				// Выполняем отправку сообщения на сервер
				core->write(buffer.data(), buffer.size(), this->_aid);
			}
		} break;
		// Иначе завершаем работу
		default: core->close(this->_aid);
	}
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::PROXY_CONNECT)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
}
/**
 * GET Метод запроса в формате HTTP методом GET
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::GET, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * DEL Метод запроса в формате HTTP методом DEL
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::DEL(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::DEL, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PUT(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Получаем тело запроса
	const string body = entity.dump();
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PUT, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PUT(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result = std::forward <const vector <char>> (entity);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PUT, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Тело в формате X-WWW-Form-Urlencoded
	string body = "";
	// Переходим по всему списку тела запроса
	for(auto & param : entity){
		// Есди данные уже набраны
		if(!body.empty()) body.append("&");
		// Добавляем в список набор параметров
		body.append(this->_uri.encode(param.first));
		// Добавляем разделитель
		body.append("=");
		// Добавляем значение
		body.append(this->_uri.encode(param.second));
	}
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PUT, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::POST(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Получаем тело запроса
	const string body = entity.dump();
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::POST, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::POST(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result = std::forward <const vector <char>> (entity);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::POST, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::POST(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Тело в формате X-WWW-Form-Urlencoded
	string body = "";
	// Переходим по всему списку тела запроса
	for(auto & param : entity){
		// Есди данные уже набраны
		if(!body.empty()) body.append("&");
		// Добавляем в список набор параметров
		body.append(this->_uri.encode(param.first));
		// Добавляем разделитель
		body.append("=");
		// Добавляем значение
		body.append(this->_uri.encode(param.second));
	}
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::POST, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PATCH(const uri_t::url_t & url, const json & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Получаем тело запроса
	const string body = entity.dump();
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/json");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PATCH, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PATCH(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result = std::forward <const vector <char>> (entity);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PATCH, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::WEB::PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Тело в формате X-WWW-Form-Urlencoded
	string body = "";
	// Переходим по всему списку тела запроса
	for(auto & param : entity){
		// Есди данные уже набраны
		if(!body.empty()) body.append("&");
		// Добавляем в список набор параметров
		body.append(this->_uri.encode(param.first));
		// Добавляем разделитель
		body.append("=");
		// Добавляем значение
		body.append(this->_uri.encode(param.second));
	}
	// Устанавливаем тепло запроса
	vector <char> result(body.begin(), body.end());
	// Добавляем заголовок типа контента
	const_cast <unordered_multimap <string, string> *> (&headers)->emplace("Content-Type", "application/x-www-form-urlencoded");
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::PATCH, url, result, * const_cast <unordered_multimap <string, string> *> (&headers));
	// Выводим результат
	return result;
}
/**
 * HEAD Метод запроса в формате HTTP методом HEAD
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::WEB::HEAD(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> entity;
	// Результат работы функции
	unordered_multimap <string, string> result = std::forward <const unordered_multimap <string, string>> (headers);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::HEAD, url, entity, result);
	// Выводим результат
	return result;
}
/**
 * TRACE Метод запроса в формате HTTP методом TRACE
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::WEB::TRACE(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> entity;
	// Результат работы функции
	unordered_multimap <string, string> result = std::forward <const unordered_multimap <string, string>> (headers);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::TRACE, url, entity, result);
	// Выводим результат
	return result;
}
/**
 * OPTIONS Метод запроса в формате HTTP методом OPTIONS
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::WEB::OPTIONS(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> entity;
	// Результат работы функции
	unordered_multimap <string, string> result = std::forward <const unordered_multimap <string, string>> (headers);
	// Выполняем HTTP запрос на сервер
	this->REQUEST(awh::web_t::method_t::OPTIONS, url, entity, result);
	// Выводим результат
	return result;
}
/**
 * REQUEST Метод выполнения запроса HTTP
 * @param method  метод запроса
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 */
void awh::client::WEB::REQUEST(const awh::web_t::method_t method, const uri_t::url_t & url, vector <char> & entity, unordered_multimap <string, string> & headers) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		// Подписываемся на событие коннекта и дисконнекта клиента
		this->on([method, &url, &entity, &headers](const web_t::mode_t mode, web_t * web) noexcept {
			// Если подключение выполнено
			if(mode == client::web_t::mode_t::CONNECT){
				// Создаём объект запроса
				req_t req;
				// Устанавливаем метод запроса
				req.method = method;
				// Устанавливаем тепло запроса
				req.entity = std::forward <const vector <char>> (entity);
				// Устанавливаем параметры запроса
				req.query = std::forward <const string> (web->_uri.query(url));
				// Запоминаем переданные заголовки
				req.headers = std::forward <const unordered_multimap <string, string>> (headers);
				// Выполняем запрос на сервер
				web->send({std::forward <req_t> (req)});
			// Выполняем остановку работы модуля
			} else web->stop();
		});
		// Подписываемся на событие получения сообщения
		this->on([&entity, &headers](const res_t & res, web_t * web) noexcept {
			// Проверяем на наличие ошибок
			if(res.code >= 300)
				// Выводим сообщение о неудачном запросе
				web->_log->print("request failed: %u %s", log_t::flag_t::WARNING, res.code, res.message.c_str());
			// Если тело ответа получено
			if(!res.entity.empty()){
				// Формируем результат ответа
				entity = std::forward <const vector <char>> (res.entity);
				// Формируем список заголовков
				headers = std::forward <const unordered_multimap <string, string>> (res.headers);
			}
			// Выполняем остановку
			web->stop();
		});
		// Выполняем инициализацию подключения
		this->init(this->_uri.origin(url));
		// Выполняем запуск работы
		this->start();
	}
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::WEB::flush() noexcept {
	// Сбрасываем флаг принудительной остановки
	this->_active = false;
	// Снимаем флаг принудительной остановки
	this->_stopped = false;
	// Выполняем очистку оставшихся данных
	this->_buffer.clear();
}
/**
 * init Метод инициализации WEB клиента
 * @param url      адрес WEB сервера
 * @param compress метод компрессии передаваемых сообщений
 */
void awh::client::WEB::init(const string & url, const http_t::compress_t compress) noexcept {
	// Если unix-сокет установлен
	if(this->_core->family() == scheme_t::family_t::NIX){
		// Выполняем очистку схемы сети
		this->_scheme.clear();
		// Устанавливаем метод компрессии сообщений
		this->_compress = compress;
		// Устанавливаем URL адрес запроса (как заглушка)
		this->_scheme.url = this->_uri.parse("http://unixsocket");
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем установку unix-сокета 
			const_cast <client::core_t *> (this->_core)->unixSocket(url);
		#endif
	// Если адрес сервера передан
	} else if(!url.empty()) {
		// Выполняем очистку схемы сети
		this->_scheme.clear();
		// Устанавливаем URL адрес запроса
		this->_scheme.url = this->_uri.parse(url);
		/**
		 * Если операционной системой не является Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Удаляем unix-сокет ранее установленный
			const_cast <client::core_t *> (this->_core)->removeUnixSocket();
		#endif
	}
	// Устанавливаем метод компрессии сообщений
	this->_compress = compress;
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const mode_t, web_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.active = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const res_t &, web_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.message = callback;
}
/**
 * on Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_http.chunking(callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::client::WEB::on(function <void (const awh::core_t::status_t status, awh::core_t * core)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.events = callback;
}
/**
 * sendTimeout Метод отправки сигнала таймаута
 */
void awh::client::WEB::sendTimeout() noexcept {
	// Если подключение выполнено
	if(this->_core->working())
		// Отправляем сигнал принудительного таймаута
		const_cast <client::core_t *> (this->_core)->sendTimeout(this->_aid);
}
/**
 * send Метод отправки сообщения на сервер
 * @param reqs список запросов
 */
void awh::client::WEB::send(const vector <req_t> & reqs) noexcept {
	// Если список запросов установлен
	if(!reqs.empty() || !this->_requests.empty()){
		// Выполняем сброс параметров запроса
		this->flush();
		// Если объект списка запросов не существует
		if(this->_requests.empty()){
			// Добавляем объект ответа в список ответов
			this->_responses.assign(reqs.size(), res_t());
			// Выполняем получение списка запросов
			this->_requests = std::forward <const vector <req_t>> (reqs);
		}
		// Выполняем запрос на удалённый сервер
		this->submit(this->_requests.front());
	}
}
/**
 * setOrigin Метод установки списка разрешенных источников для HTTP/2
 * @param origins список разрешённых источников
 */
void awh::client::WEB::setOrigin(const vector <string> & origins) noexcept {
	// Выполняем установку списка источников
	this->_origins = origins;
}
/**
 * sendOrigin Метод отправки списка разрешенных источников для HTTP/2
 * @param origins список разрешённых источников
 */
void awh::client::WEB::sendOrigin(const vector <string> & origins) noexcept {
	// Если сессия HTTP/2 активна
	if(this->_http2.mode && (this->_http2.ctx != nullptr)){
		// Список источников для установки на сервере
		vector <nghttp2_origin_entry> ov;
		// Если список источников передан
		if(!origins.empty()){
			// Выполняем перебор списка источников
			for(auto & origin : origins)
				// Выполняем добавление источника в списку
				ov.push_back({(uint8_t *) origin.c_str(), origin.size()});
		}
		// Результат выполнения поерации
		int rv = -1;
		// Выполняем установку фрейма полученных источников
		if((rv = nghttp2_submit_origin(this->_http2.ctx, NGHTTP2_FLAG_NONE, (!ov.empty() ? ov.data() : nullptr), ov.size())) != 0){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Выходим из функции
			return;
		}
		// Фиксируем отправленный результат
		if((rv = nghttp2_session_send(this->_http2.ctx)) != 0){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Выходим из функции
			return;
		}
	}
}
/**
 * open Метод открытия подключения
 */
void awh::client::WEB::open() noexcept {
	// Если подключение уже выполнено
	if(this->_scheme.status.real == scheme_t::mode_t::CONNECT){
		// Если подключение производится через, прокси-сервер
		if(this->_scheme.isProxy())
			// Выполняем запуск функции подключения для прокси-сервера
			this->actionProxyConnect();
		// Выполняем запуск функции подключения
		else this->actionConnect();
	// Если биндинг уже запущен, выполняем запрос на сервер
	} else const_cast <client::core_t *> (this->_core)->open(this->_scheme.sid);
}
/**
 * stop Метод остановки клиента
 */
void awh::client::WEB::stop() noexcept {
	// Устанавливаем флаг принудительной остановки
	this->_active = true;
	// Если подключение выполнено
	if(this->_core->working()){
		// Выполняем очистку списка запросов
		this->_requests.clear();
		// Выполняем очистку списка ответов
		this->_responses.clear();
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Завершаем работу, если разрешено остановить
		if(this->_unbind) const_cast <client::core_t *> (this->_core)->stop();
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
void awh::client::WEB::start() noexcept {
	// Если адрес URL запроса передан
	if(!this->_scheme.url.empty()){
		// Если биндинг не запущен
		if(!this->_core->working())
			// Выполняем запуск биндинга
			const_cast <client::core_t *> (this->_core)->start();
		// Если биндинг уже запущен, выполняем запрос на сервер
		else this->open();
	}
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::client::WEB::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
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
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::WEB::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
	// Устанавливаем количество секунд на подключение
	this->_scheme.timeouts.connect = connect;
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::WEB::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Если URI параметры переданы
	if(!uri.empty()){
		// Устанавливаем семейство интернет протоколов
		this->_scheme.proxy.family = family;
		// Устанавливаем параметры прокси-сервера
		this->_scheme.proxy.url = this->_uri.parse(uri);
		// Если данные параметров прокси-сервера получены
		if(!this->_scheme.proxy.url.empty()){
			// Если протокол подключения SOCKS5
			if(this->_fmk->compare(this->_scheme.proxy.url.schema, "socks5")){
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = proxy_t::type_t::SOCKS5;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.socks5.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			// Если протокол подключения HTTP
			} else if(this->_fmk->compare(this->_scheme.proxy.url.schema, "http") || this->_fmk->compare(this->_scheme.proxy.url.schema, "https")) {
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = proxy_t::type_t::HTTP;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.http.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			}
		}
	}
}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::client::WEB::mode(const u_short flag) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = !(flag & static_cast <uint8_t> (flag_t::NOT_STOP));
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flag & static_cast <uint8_t> (flag_t::ALIVE));
	// Устанавливаем флаг разрешающий выполнять редиректы
	this->_redirects = (flag & static_cast <uint8_t> (flag_t::REDIRECTS));
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flag & static_cast <uint8_t> (flag_t::WAIT_MESS));
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->_core)->noInfo(flag & static_cast <uint8_t> (flag_t::NOT_INFO));
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->_core)->verifySSL(flag & static_cast <uint8_t> (flag_t::VERIFY_SSL));
	// Если протокол подключения желательно установить HTTP/2
	if(this->_core->proto() == engine_t::proto_t::HTTP2)
		// Активируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(this->_scheme.alive);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::WEB::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->_http.chunk(size);
}
/**
 * attempts Метод установки общего количества попыток
 * @param attempts общее количество попыток
 */
void awh::client::WEB::attempts(const uint8_t attempts) noexcept {
	// Если количество попыток передано, устанавливаем его
	if(attempts > 0) this->_attempts = attempts;
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::WEB::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента
		this->_http.userAgent(userAgent);
		// Устанавливаем пользовательского агента для прокси-сервера
		this->_scheme.proxy.http.userAgent(userAgent);
	}
}
/**
 * compress Метод установки метода компрессии
 * @param compress метод компрессии сообщений
 */
void awh::client::WEB::compress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_compress = compress;
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::WEB::user(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty())
		// Устанавливаем логин и пароль пользователя
		this->_http.user(login, password);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::WEB::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::WEB::serv(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса
	this->_http.serv(id, name, ver);
	// Устанавливаем данные сервиса для прокси-сервера
	this->_scheme.proxy.http.serv(id, name, ver);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::WEB::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования
	this->_http.crypto(pass, salt, cipher);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WEB::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->_http.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WEB::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->_scheme.proxy.http.authType(type, hash);
}
/**
 * WEB Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::WEB::WEB(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _uri(fmk), _http(fmk, log, &_uri), _scheme(fmk, log), _action(action_t::NONE),
 _compress(awh::http_t::compress_t::NONE), _aid(0), _unbind(true), _active(false),
 _stopped(false), _redirects(false), _attempts(10), _fmk(fmk), _log(log), _core(core) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&web_t::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("persist", std::bind(&web_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&web_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&web_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем событие на подключение к прокси-серверу
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connectProxy", std::bind(&web_t::proxyConnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&web_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем событие на чтение данных с прокси-сервера
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("readProxy", std::bind(&web_t::proxyReadCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем событие на активацию шифрованного TLS канала
	this->_scheme.callback.set <bool (const uri_t::url_t &, const size_t, const size_t, awh::core_t *)> ("tls", std::bind(&web_t::enableTLSCallback, this, _1, _2, _3, _4));
	// Устанавливаем функцию обработки вызова для получения чанков
	this->_http.chunking(std::bind(&web_t::chunking, this, _1, _2));
	// Добавляем схемы сети в сетевое ядро
	const_cast <client::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем функцию активации ядра клиента
	const_cast <client::core_t *> (this->_core)->callback(std::bind(&web_t::eventsCallback, this, _1, _2));
}
