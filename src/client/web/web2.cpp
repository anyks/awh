/**
 * @file: web2.cpp
 * @date: 2023-09-12
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
#include <client/web/web.hpp>

/**
 * debug Функция обратного вызова при получении отладочной информации
 * @param format формат вывода отладочной информации
 * @param args   список аргументов отладочной информации
 */
void awh::client::Web2::debug(const char * format, va_list args) noexcept {
	// Буфер данных для логирования
	string buffer(1024, '\0');
	// Выполняем перебор всех аргументов
	while(true){
		// Создаем список аргументов
		va_list args2;
		// Копируем список аргументов
		va_copy(args2, args);
		// Выполняем запись в буфер данных
		size_t res = vsnprintf(buffer.data(), buffer.size(), format, args2);
		// Если результат получен
		if((res >= 0) && (res < buffer.size())){
			// Завершаем список аргументов
			va_end(args);
			// Завершаем список локальных аргументов
			va_end(args2);
			// Если результат не получен
			if(res == 0)
				// Выполняем сброс результата
				buffer.clear();
			// Выводим результат
			else buffer.assign(buffer.begin(), buffer.begin() + res);
			// Выходим из цикла
			break;
		}
		// Размер буфера данных
		size_t size = 0;
		// Если данные не получены, увеличиваем буфер в два раза
		if(res < 0)
			// Увеличиваем размер буфера в два раза
			size = (buffer.size() * 2);
		// Увеличиваем размер буфера на один байт
		else size = (res + 1);
		// Очищаем буфер данных
		buffer.clear();
		// Выделяем память для буфера
		buffer.resize(size);
		// Завершаем список локальных аргументов
		va_end(args2);
	}
	// Выводим отладочную информацию
	cout << " \x1B[36m\x1B[1mDebug:\x1B[0m " << buffer << endl;
}
/**
 * onFrame Функция обратного вызова при получении фрейма заголовков HTTP/2 с сервера
 * @param session объект сессии HTTP/2
 * @param frame   объект фрейма заголовков HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::Web2::onFrame(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) session;
	// Получаем объект HTTP2-клиента
	web2_t * web = reinterpret_cast <web2_t *> (ctx);
	// Если подключение производится через, прокси-сервер
	if(web->_scheme.isProxy())
		// Выполняем обработку полученных данных фрейма для прокси-сервера
		return web->signalFrameProxy(frame);
	// Выполняем обработку полученных данных фрейма
	return web->signalFrame(frame);
}
/**
 * onClose Функция закрытия подключения с сервером HTTP/2
 * @param session объект сессии HTTP/2
 * @param sid     идентификатор потока
 * @param error   флаг ошибки HTTP/2 если присутствует
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученного события
 */
int awh::client::Web2::onClose(nghttp2_session * session, const int32_t sid, const uint32_t error, void * ctx) noexcept {
	// Получаем объект HTTP2-клиента
	web2_t * web = reinterpret_cast <web2_t *> (ctx);
	/**
	 * Если включён режим отладки
	 */
	#if defined(DEBUG_MODE)
		// Выводим заголовок ответа
		cout << "\x1B[33m\x1B[1m^^^^^^^^^ CLOSE SESSION HTTP2 ^^^^^^^^^\x1B[0m" << endl;
		// Если ошибка не была получена
		if(error == 0x0)
			// Выводим информацию о закрытии сессии
			cout << web->_fmk->format("Stream %d closed", sid) << endl << endl;
	#endif
	// Выполняем передачу сигнала
	return web->signalStreamClosed(sid, error);
}
/**
 * onChunk Функция обратного вызова при получении чанка с сервера HTTP/2
 * @param session объект сессии HTTP/2
 * @param flags   флаги события для сессии HTTP/2
 * @param sid     идентификатор потока
 * @param buffer  буфер данных который содержит полученный чанк
 * @param size    размер полученного буфера данных чанка
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::Web2::onChunk(nghttp2_session * session, const uint8_t flags, const int32_t sid, const uint8_t * buffer, const size_t size, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Получаем объект HTTP2-клиента
	web2_t * web = reinterpret_cast <web2_t *> (ctx);
	// Если подключение производится через, прокси-сервер
	if(web->_scheme.isProxy())
		// Выполняем обработку полученных данных чанка для прокси-сервера
		return web->signalChunkProxy(sid, buffer, size);
	// Выполняем обработку полученных данных чанка
	return web->signalChunk(sid, buffer, size);
}
/**
 * onBeginHeaders Функция начала получения фрейма заголовков HTTP/2
 * @param session объект сессии HTTP/2
 * @param frame   объект фрейма заголовков HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::Web2::onBeginHeaders(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) session;
	// Получаем объект HTTP2-клиента
	web2_t * web = reinterpret_cast <web2_t *> (ctx);
	// Выполняем определение типа фрейма
	switch(frame->hd.type){
		// Если мы получили входящие данные заголовков ответа
		case NGHTTP2_HEADERS:{
			// Если сессия клиента совпадает с сессией полученных даных
			if(frame->headers.cat == NGHTTP2_HCAT_RESPONSE){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
					// Выводим информацию об ошибке
					cout << web->_fmk->format("Stream ID=%d", frame->hd.stream_id) << endl << endl;
				#endif
				// Если подключение производится через, прокси-сервер
				if(web->_scheme.isProxy())
					// Выполняем обработку сигнала начала получения заголовков для прокси-сервера
					return web->signalBeginHeadersProxy(frame->hd.stream_id);
				// Выполняем обработку сигнала начала получения заголовков
				return web->signalBeginHeaders(frame->hd.stream_id);
			}
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * onHeader Функция обратного вызова при получении заголовка HTTP/2
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
int awh::client::Web2::onHeader(nghttp2_session * session, const nghttp2_frame * frame, const uint8_t * key, const size_t keySize, const uint8_t * val, const size_t valSize, const uint8_t flags, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Выполняем определение типа фрейма
	switch(frame->hd.type){
		// Если мы получили входящие данные заголовков ответа
		case NGHTTP2_HEADERS:{
			// Если сессия клиента совпадает с сессией полученных даных
			if(frame->headers.cat == NGHTTP2_HCAT_RESPONSE){
				// Получаем объект HTTP2-клиента
				web2_t * web = reinterpret_cast <web2_t *> (ctx);
				// Если подключение производится через, прокси-сервер
				if(web->_scheme.isProxy())
					// Выполняем обработку полученных заголовков для прокси-сервера
					return web->signalHeaderProxy(frame->hd.stream_id, string((const char *) key, keySize), string((const char *) val, valSize));
				// Выполняем обработку полученных заголовков
				return web->signalHeader(frame->hd.stream_id, string((const char *) key, keySize), string((const char *) val, valSize));
			}
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * onSend Функция обратного вызова при подготовки данных для отправки на сервер
 * @param session объект сессии HTTP/2
 * @param buffer  буфер данных которые следует отправить
 * @param size    размер буфера данных для отправки
 * @param flags   флаги события для сессии HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        количество отправленных байт
 */
ssize_t awh::client::Web2::onSend(nghttp2_session * session, const uint8_t * buffer, const size_t size, const int flags, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Получаем объект HTTP2-клиента
	web2_t * web = reinterpret_cast <web2_t *> (ctx);
	// Выполняем отправку заголовков запроса на сервер
	const_cast <client::core_t *> (web->_core)->write((const char *) buffer, size, web->_aid);
	// Возвращаем количество отправленных байт
	return static_cast <ssize_t> (size);
}
/**
 * onRead Функция чтения подготовленных данных для формирования буфера данных который необходимо отправить на HTTP/2 сервер
 * @param session объект сессии HTTP/2
 * @param sid     идентификатор потока
 * @param buffer  буфер данных которые следует отправить
 * @param size    размер буфера данных для отправки
 * @param flags   флаги события для сессии HTTP/2
 * @param source  объект промежуточных данных локального подключения
 * @param ctx     передаваемый промежуточный контекст
 * @return        количество отправленных байт
 */
ssize_t awh::client::Web2::onRead(nghttp2_session * session, const int32_t sid, uint8_t * buffer, const size_t size, uint32_t * flags, nghttp2_data_source * source, void * ctx) noexcept {
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
 * signalFrameProxy Метод обратного вызова при получении фрейма заголовков HTTP/2 с сервера-сервера
 * @param frame   объект фрейма заголовков HTTP/2
 * @return        статус полученных данных
 */
int awh::client::Web2::signalFrameProxy(const nghttp2_frame * frame) noexcept {
	// Если идентификатор сессии клиента совпадает
	if(this->_proxy.sid == frame->hd.stream_id){
		// Выполняем определение типа фрейма
		switch(frame->hd.type){
			// Если мы получили входящие данные тела ответа
			case NGHTTP2_DATA: {
				// Если мы получили флаг завершения потока
				if(frame->hd.flags & NGHTTP2_FLAG_END_STREAM){
					// Выполняем коммит полученного результата
					this->_scheme.proxy.http.commit();
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						{
							// Если тело ответа существует
							if(!this->_scheme.proxy.http.body().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->_fmk->format("<body %u>", this->_scheme.proxy.http.body().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					#endif
					// Получаем параметры запроса
					const auto & response = this->_scheme.proxy.http.response();
					// Если функция обратного вызова активности потока установлена
					if(this->_callback.is("stream"))
						// Выводим функцию обратного вызова
						this->_callback.call <const int32_t, const mode_t> ("stream", frame->hd.stream_id, mode_t::CLOSE);
					// Если функция обратного вызова установлена, выводим сообщение
					if(this->_callback.is("entity"))
						// Выполняем функцию обратного вызова дисконнекта
						this->_callback.call <const int32_t, const u_int, const string, const vector <char>> ("entity", frame->hd.stream_id, response.code, response.message, this->_scheme.proxy.http.body());
					// Завершаем работу
					const_cast <client::core_t *> (this->_core)->close(this->_aid);
				}
			} break;
			// Если мы получили входящие данные заголовков ответа
			case NGHTTP2_HEADERS: {
				// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
				if(frame->hd.flags & NGHTTP2_FLAG_END_HEADERS){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						{
							// Получаем данные ответа
							const auto & response = this->_scheme.proxy.http.process(http_t::process_t::RESPONSE, true);
							// Если параметры ответа получены
							if(!response.empty())
								// Выводим параметры ответа
								cout << string(response.begin(), response.end()) << endl;
						}
					#endif
					// Получаем параметры запроса
					const auto & response = this->_scheme.proxy.http.response();
					// Получаем статус ответа
					awh::http_t::stath_t status = this->_scheme.proxy.http.getAuth();
					// Устанавливаем ответ прокси-сервера
					this->_proxy.answer = response.code;
					// Если выполнять редиректы запрещено
					if(!this->_redirects && (status == awh::http_t::stath_t::RETRY)){
						// Если ответом сервера не является запросом авторизации
						if(response.code != 407)
							// Запрещаем выполнять редирект
							status = awh::http_t::stath_t::GOOD;
					}
					// Выполняем проверку авторизации
					switch(static_cast <uint8_t> (status)){
						// Если нужно попытаться ещё раз
						case static_cast <uint8_t> (awh::http_t::stath_t::RETRY): {
							// Если попытки повторить переадресацию ещё не закончились
							if(!(this->_stopped = (this->_attempt >= this->_attempts))){
								// Если адрес запроса получен
								if(!this->_scheme.proxy.url.empty()){
									// Если соединение является постоянным
									if(this->_scheme.proxy.http.isAlive()){
										// Увеличиваем количество попыток
										this->_attempt++;
										// Устанавливаем новый экшен выполнения
										this->proxyConnectCallback(this->_aid, this->_scheme.sid, const_cast <client::core_t *> (this->_core));
									// Если соединение должно быть закрыто
									} else const_cast <client::core_t *> (this->_core)->close(this->_aid);
									// Завершаем работу
									return 0;
								}
							}
						} break;
						// Если запрос выполнен удачно
						case static_cast <uint8_t> (awh::http_t::stath_t::GOOD): {
							// Выполняем переключение на работу с сервером
							this->_scheme.switchConnect();
							// Выполняем запуск работы основного модуля
							this->connectCallback(this->_aid, this->_scheme.sid, const_cast <client::core_t *> (this->_core));
							// Завершаем работу
							return 0;
						} break;
						// Если запрос неудачный
						case static_cast <uint8_t> (awh::http_t::stath_t::FAULT):
							// Устанавливаем флаг принудительной остановки
							this->_stopped = true;
						break;
					}
					// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
					if(this->_callback.is("response"))
						// Выводим функцию обратного вызова
						this->_callback.call <const int32_t, const u_int, const string &> ("response", frame->hd.stream_id, response.code, response.message);
					// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
					if(this->_callback.is("headers"))
						// Выводим функцию обратного вызова
						this->_callback.call <const int32_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", frame->hd.stream_id, response.code, response.message, this->_scheme.proxy.http.headers());
					// Завершаем работу
					const_cast <client::core_t *> (this->_core)->close(this->_aid);
				}
			} break;
		}
	}
	// Выводим результат
	return 0;
}
/**
 * signalChunkProxy Метод обратного вызова при получении чанка с прокси-сервера HTTP/2
 * @param sid    идентификатор потока
 * @param buffer буфер данных который содержит полученный чанк
 * @param size   размер полученного буфера данных чанка
 * @return       статус полученных данных
 */
int awh::client::Web2::signalChunkProxy(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept {
	// Если идентификатор сессии клиента совпадает
	if(this->_proxy.sid == sid)
		// Добавляем полученный чанк в тело данных
		this->_scheme.proxy.http.body(vector <char> (buffer, buffer + size));
	// Выводим результат
	return 0;
}
/**
 * signalBeginHeadersProxy Метод начала получения фрейма заголовков HTTP/2 прокси-сервера
 * @param sid идентификатор потока
 * @return    статус полученных данных
 */
int awh::client::Web2::signalBeginHeadersProxy(const int32_t sid) noexcept {
	// Если идентификатор сессии клиента совпадает
	if(this->_proxy.sid == sid)
		// Выполняем очистку параметров HTTP запроса
		this->_scheme.proxy.http.clear();
	// Выводим результат
	return 0;
}
/**
 * signalHeaderProxy Метод обратного вызова при получении заголовка HTTP/2 прокси-сервера
 * @param sid идентификатор потока
 * @param key данные ключа заголовка
 * @param val данные значения заголовка
 * @return    статус полученных данных
 */
int awh::client::Web2::signalHeaderProxy(const int32_t sid, const string & key, const string & val) noexcept {
	// Если идентификатор сессии клиента совпадает
	if(this->_proxy.sid == sid)
		// Устанавливаем полученные заголовки
		this->_scheme.proxy.http.header2(key, val);
	// Выводим результат
	return 0;
}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::client::Web2::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Если система была остановлена
		if(status == awh::core_t::status_t::STOP){
			// Если контекст сессии HTTP/2 создан
			if(this->_upgraded && (this->_session != nullptr))
				// Выполняем удаление сессии
				nghttp2_session_del(this->_session);
			// Деактивируем флаг работы с протоколом HTTP/2
			this->_upgraded = false;
		}
		// Если функция получения событий запуска и остановки сетевого ядра установлена
		if(this->_callback.is("events"))
			// Выводим функцию обратного вызова
			this->_callback.call <const awh::core_t::status_t, awh::core_t *> ("events", status, core);
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Web2::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ, event_t::CONNECT}, event_t::CONNECT))
		// Выполняем инициализацию сессии HTTP/2
		this->implementation(aid, dynamic_cast <client::core_t *> (core));
}
/**
 * proxyConnectCallback Метод обратного вызова при подключении к прокси-серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Web2::proxyConnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::OPEN, event_t::PROXY_READ}, event_t::PROXY_CONNECT)){
			// Определяем тип прокси-сервера
			switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
				// Если прокси-сервер является HTTP/1.1
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTP):
				// Если прокси-сервер является Socks5
				case static_cast <uint8_t> (client::proxy_t::type_t::SOCKS5):
					// Выполняем передачу сигнала родительскому модулю
					web_t::proxyConnectCallback(aid, sid, core);
				break;
				// Если прокси-сервер является HTTP/2
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTPS): {
					// Если протокол подключения является HTTP/2
					if(core->proto(aid) == engine_t::proto_t::HTTP2){
						// Запоминаем идентификатор адъютанта
						this->_aid = aid;
						// Если протокол активирован HTTPS или WSS защищённый поверх TLS
						if(this->_proxy.connect){
							// Выполняем сброс состояния HTTP парсера
							this->_scheme.proxy.http.reset();
							// Выполняем очистку параметров HTTP запроса
							this->_scheme.proxy.http.clear();
							// Выполняем инициализацию сессии HTTP/2
							this->implementation(aid, dynamic_cast <client::core_t *> (core));
							// Если активирован режим работы с HTTP/2 протоколом
							if(this->_upgraded){
								// Список заголовков для запроса
								vector <nghttp2_nv> nva;
								// Создаём объек запроса
								awh::web_t::req_t query(awh::web_t::method_t::CONNECT, this->_scheme.url);
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Выводим заголовок запроса
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST PROXY ^^^^^^^^^\x1B[0m" << endl;
									// Получаем бинарные данные WEB запроса
									const auto & buffer = this->_scheme.proxy.http.proxy(query);
									// Выводим параметры запроса
									cout << string(buffer.begin(), buffer.end()) << endl;
								#endif
								// Выполняем запрос на получение заголовков
								const auto & headers = this->_scheme.proxy.http.proxy2(std::move(query));
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
								// Если тело запроса не существует, выполняем установленный запрос
								this->_proxy.sid = nghttp2_submit_request(this->_session, nullptr, nva.data(), nva.size(), nullptr, this);
								// Если запрос не получилось отправить
								if(this->_proxy.sid < 0){
									// Выводим в лог сообщение
									this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(this->_proxy.sid));
									// Если функция обратного вызова на на вывод ошибок установлена
									if(this->_callback.is("error"))
										// Выводим функцию обратного вызова
										this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::PROXY_HTTP2_SUBMIT, nghttp2_strerror(this->_proxy.sid));
									// Выполняем закрытие подключения
									dynamic_cast <client::core_t *> (core)->close(aid);
									// Выходим из функции
									return;
								}{
									// Результат фиксации сессии
									int rv = -1;
									// Фиксируем отправленный результат
									if((rv = nghttp2_session_send(this->_session)) != 0){
										// Выводим сообщение об полученной ошибке
										this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
										// Если функция обратного вызова на на вывод ошибок установлена
										if(this->_callback.is("error"))
											// Выводим функцию обратного вызова
											this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::PROXY_HTTP2_SEND, nghttp2_strerror(rv));
										// Выполняем закрытие подключения
										dynamic_cast <client::core_t *> (core)->close(aid);
										// Выходим из функции
										return;
									}
								}
							// Если инициализировать сессию HTTP/2 не удалось
							} else {
								// Выводим сообщение об ошибке подключения к прокси-серверу
								this->_log->print("Proxy server does not support the HTTP/2 protocol", log_t::flag_t::CRITICAL);
								// Если функция обратного вызова на на вывод ошибок установлена
								if(this->_callback.is("error"))
									// Выводим функцию обратного вызова
									this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::PROXY_HTTP2_NO_INIT, "Proxy server does not support the HTTP/2 protocol");
								// Выполняем закрытие подключения
								dynamic_cast <client::core_t *> (core)->close(aid);
								// Выполняем завершение работы приложения
								return;
							}
						// Если протокол подключения не является защищённым подключением
						} else {
							// Выполняем переключение на работу с сервером
							this->_scheme.switchConnect();
							// Выполняем запуск работы основного модуля
							this->connectCallback(aid, sid, core);
						}
					// Выполняем передачу сигнала родительскому модулю
					} else web_t::proxyConnectCallback(aid, sid, core);
				} break;
				// Иначе завершаем работу
				default: dynamic_cast <client::core_t *> (core)->close(aid);
			}
		}
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
void awh::client::Web2::proxyReadCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::PROXY_CONNECT}, event_t::PROXY_READ)){
			// Определяем тип прокси-сервера
			switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
				// Если прокси-сервер является HTTP
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTP):
				// Если прокси-сервер является Socks5
				case static_cast <uint8_t> (client::proxy_t::type_t::SOCKS5):
					// Выполняем передачу сигнала родительскому модулю
					web_t::proxyReadCallback(buffer, size, aid, sid, core);
				break;
				// Если прокси-сервер является HTTP/2
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTPS): {
					// Если протокол подключения является HTTP/2
					if(core->proto(aid) == engine_t::proto_t::HTTP2){
						// Выполняем извлечение полученного чанка данных из сокета
						ssize_t bytes = nghttp2_session_mem_recv(this->_session, (const uint8_t *) buffer, size);
						// Если данные не прочитаны, выводим ошибку и выходим
						if(bytes < 0){
							// Выводим сообщение об полученной ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выводим функцию обратного вызова
								this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::PROXY_HTTP2_RECV, nghttp2_strerror(static_cast <int> (bytes)));
							// Выходим из функции
							return;
						}
						// Фиксируем полученный результат
						if((bytes = nghttp2_session_send(this->_session)) != 0){
							// Выводим сообщение об полученной ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
							// Если функция обратного вызова на на вывод ошибок установлена
							if(this->_callback.is("error"))
								// Выводим функцию обратного вызова
								this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::PROXY_HTTP2_SEND, nghttp2_strerror(static_cast <int> (bytes)));
							// Выходим из функции
							return;
						}
					// Если активирован режим работы с HTTP/1.1 протоколом
					} else web_t::proxyReadCallback(buffer, size, aid, sid, core);
				} break;
				// Иначе завершаем работу
				default: dynamic_cast <client::core_t *> (core)->close(aid);
			}
		}
	}
}
/**
 * implementation Метод выполнения активации сессии HTTP/2
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 */
void awh::client::Web2::implementation(const size_t aid, client::core_t * core) noexcept {
	// Если протокол подключения является HTTP/2
	if(!this->_upgraded && (core->proto(aid) == engine_t::proto_t::HTTP2)){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выполняем установку функции для вывода отладочной информации
			nghttp2_set_debug_vprintf_callback(&web2_t::debug);
		#endif
		// Создаём объект функций обратного вызова
		nghttp2_session_callbacks * callbacks;
		// Выполняем инициализацию сессию функций обратного вызова
		nghttp2_session_callbacks_new(&callbacks);
		// Выполняем установку функции обратного вызова при подготовки данных для отправки на сервер
		nghttp2_session_callbacks_set_send_callback(callbacks, &web2_t::onSend);
		// Выполняем установку функции обратного вызова при получении заголовка HTTP/2
		nghttp2_session_callbacks_set_on_header_callback(callbacks, &web2_t::onHeader);
		// Выполняем установку функции обратного вызова при получении фрейма заголовков HTTP/2 с сервера
		nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, &web2_t::onFrame);
		// Выполняем установку функции обратного вызова закрытия подключения с сервером HTTP/2
		nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, &web2_t::onClose);
		// Выполняем установку функции обратного вызова при получении чанка с сервера HTTP/2
		nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, &web2_t::onChunk);
		// Выполняем установку функции обратного вызова начала получения фрейма заголовков HTTP/2
		nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, &web2_t::onBeginHeaders);
		// Выполняем подключение котнекста сессии HTTP/2
		nghttp2_session_client_new(&this->_session, callbacks, this);
		// Выполняем удаление объекта функций обратного вызова
		nghttp2_session_callbacks_del(callbacks);
		// Создаём параметры сессии подключения с HTTP/2 сервером
		const vector <nghttp2_settings_entry> iv = {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 128}};
		// Клиентская 24-байтовая магическая строка будет отправлена библиотекой nghttp2
		const int rv = nghttp2_submit_settings(this->_session, NGHTTP2_FLAG_NONE, iv.data(), iv.size());
		// Если настройки для сессии установить не удалось
		if(rv != 0){
			// Выполняем закрытие подключения
			core->close(aid);
			// Выводим сообщение об ошибке
			this->_log->print("Could not submit SETTINGS: %s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::HTTP2_SETTINGS, nghttp2_strerror(rv));
			// Если сессия HTTP/2 создана удачно
			if(this->_session != nullptr)
				// Выполняем удаление сессии
				nghttp2_session_del(this->_session);
			// Выходим из функции
			return;
		}
		// Выполняем активацию работы с протоколом HTTP/2
		this->_upgraded = !this->_upgraded;
	}
}
/**
 * ping Метод выполнения пинга сервера
 * @return результат работы пинга
 */
bool awh::client::Web2::ping() noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
		// Если протокол подключения установлен как HTTP/2
		if(this->_upgraded && (this->_session != nullptr)){
			// Результат выполнения поерации
			int rv = -1;
			// Выполняем пинг удалённого сервера
			if((rv = nghttp2_submit_ping(this->_session, 0, nullptr)) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::HTTP2_PING, nghttp2_strerror(rv));
				// Выходим из функции
				return false;
			}
			// Фиксируем отправленный результат
			if((rv = nghttp2_session_send(this->_session)) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::HTTP2_SEND, nghttp2_strerror(rv));
				// Выходим из функции
				return false;
			}
			// Выводим результат
			return true;
		}
	}
	// Выводим результат
	return false;
}
/**
 * send Метод отправки сообщения на сервер
 * @param id      идентификатор потока HTTP/2
 * @param message сообщение передаваемое на сервер
 * @param size    размер сообщения в байтах
 * @param end     флаг последнего сообщения после которого поток закрывается
 */
void awh::client::Web2::send(const int32_t id, const char * message, const size_t size, const bool end) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ, event_t::SEND}, event_t::SEND)){
		// Если протокол подключения установлен как HTTP/2 и подключение выполнено
		if(this->_upgraded && this->_core->working() && (this->_session != nullptr) && (message != nullptr) && (size > 0)){
			// Список файловых дескрипторов
			int fds[2];
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
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::HTTP2_PIPE_INIT, strerror(errno));
				// Выполняем закрытие подключения
				const_cast <client::core_t *> (this->_core)->close(this->_aid);
				// Выходим из функции
				return;
			}
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Если данные небыли записаны в сокет
				if(static_cast <int> (_write(fds[1], message, size)) != static_cast <int> (size)){
					// Выполняем закрытие сокета для чтения
					_close(fds[0]);
					// Выполняем закрытие сокета для записи
					_close(fds[1]);
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::HTTP2_PIPE_WRITE, strerror(errno));
					// Выполняем закрытие подключения
					const_cast <client::core_t *> (this->_core)->close(this->_aid);
					// Выходим из функции
					return;
				}
			/**
			 * Для всех остальных операционных систем
			 */
			#else
				// Если данные небыли записаны в сокет
				if(static_cast <int> (::write(fds[1], message, size)) != static_cast <int> (size)){
					// Выполняем закрытие сокета для чтения
					::close(fds[0]);
					// Выполняем закрытие сокета для записи
					::close(fds[1]);
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::HTTP2_PIPE_WRITE, strerror(errno));
					// Выполняем закрытие подключения
					const_cast <client::core_t *> (this->_core)->close(this->_aid);
					// Выходим из функции
					return;
				}
			#endif
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
			data.read_callback = &web2_t::onRead;
			{
				// Результат фиксации сессии
				int rv = -1;
				// Выполняем формирование данных фрейма для отправки
				if((rv = nghttp2_submit_data(this->_session, (end ? NGHTTP2_FLAG_END_STREAM : NGHTTP2_FLAG_NONE), id, &data)) != 0){
					// Выводим сообщение об полученной ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::HTTP2_SUBMIT, nghttp2_strerror(rv));
					// Выходим из функции
					return;
				}
				// Фиксируем отправленный результат
				if((rv = nghttp2_session_send(this->_session)) != 0){
					// Выводим сообщение об полученной ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const log_t::flag_t, const error_t, const string &> ("error", log_t::flag_t::CRITICAL, error_t::HTTP2_SEND, nghttp2_strerror(rv));
					// Выходим из функции
					return;
				}
			}
		}
	}
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::Web2::chunk(const size_t size) noexcept {
	// Если размер чанка передан
	if(size >= 100)
		// Выполняем установку размера чанка
		this->_chunkSize = size;
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Web2::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = (flags.count(flag_t::NOT_STOP) == 0);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.count(flag_t::ALIVE) > 0);
	// Устанавливаем флаг разрешающий выполнять редиректы
	this->_redirects = (flags.count(flag_t::REDIRECTS) > 0);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг запрещающий выполнять метод CONNECT для прокси-клиента
	this->_proxy.connect = (flags.count(flag_t::PROXY_NOCONNECT) == 0);
	// Если сетевое ядро установлено
	if(this->_core != nullptr){
		// Активируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(this->_scheme.alive);
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <client::core_t *> (this->_core)->noInfo(flags.count(flag_t::NOT_INFO) > 0);
		// Выполняем установку флага проверки домена
		const_cast <client::core_t *> (this->_core)->verifySSL(flags.count(flag_t::VERIFY_SSL) > 0);
	}
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::Web2::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента у родительского класса
		web_t::userAgent(userAgent);
		// Устанавливаем пользовательского агента
		this->_userAgent = userAgent;
	}
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Web2::user(const string & login, const string & password) noexcept {
	// Если логин пользователя передан
	if(!login.empty())
		// Выполняем установку логина пользователя
		this->_login = login;
	// Если пароль пользователя передан
	if(!password.empty())
		// Выполняем установку пароля пользователя
		this->_password = password;
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::Web2::serv(const string & id, const string & name, const string & ver) noexcept {
	// Если данные сервиса переданы
	if(!id.empty() && !name.empty() && !ver.empty()){
		// Выполняем установку данных сервиса у родительского класса
		web_t::serv(id, name, ver);
		// Запоминаем идентификатор сервиса
		this->_serv.id = id;
		// Запоминаем версию сервиса
		this->_serv.ver = ver;
		// Запоминаем название сервиса
		this->_serv.name = name;
	}
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Web2::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_authType = type;
	// Выполняем установку алгоритма шифрования для Digest-авторизации
	this->_authHash = hash;
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::Web2::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Если пароль для шифрования передаваемых данных получен
	if(!pass.empty())
		// Выполняем установку пароля шифрования передаваемых данных
		this->_crypto.pass = pass;
	// Если соль шифрования переданных данных получен
	if(!salt.empty())
		// Выполняем установку соли шифрования передаваемых данных
		this->_crypto.salt = salt;
	// Выполняем установку размера шифрования передаваемых данных
	this->_crypto.cipher = cipher;
}
/**
 * Web2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Web2::Web2(const fmk_t * fmk, const log_t * log) noexcept :
 web_t(fmk, log), _login{""}, _password{""}, _userAgent{""}, _chunkSize(BUFFER_CHUNK),
 _authType(auth_t::type_t::BASIC), _authHash(auth_t::hash_t::MD5), _upgraded(false), _session(nullptr) {
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("persist", std::bind(&web2_t::persistCallback, this, _1, _2, _3));
}
/**
 * Web2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Web2::Web2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web_t(core, fmk, log), _login{""}, _password{""}, _userAgent{""}, _chunkSize(BUFFER_CHUNK),
 _authType(auth_t::type_t::BASIC), _authHash(auth_t::hash_t::MD5), _upgraded(false), _session(nullptr) {
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("persist", std::bind(&web2_t::persistCallback, this, _1, _2, _3));
}
