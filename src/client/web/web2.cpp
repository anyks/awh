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
	// Выполняем обработку полученных данных фрейма
	return reinterpret_cast <web2_t *> (ctx)->receivedFrame(frame);
}
/**
 * onClose Функция закрытия подключения с сервером HTTP/2
 * @param session объект сессии HTTP/2
 * @param sid     идентификатор сессии HTTP/2
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
	/**
	 * Если включён рабочий режим
	 */
	#else
		// Выполняем блокировку неиспользуемой переменной
		(void) sid;
	#endif
	// Отключаем флаг HTTP/2 так-как сессия уже закрыта
	web->_upgraded = false;
	// Если сессия HTTP/2 закрыта не удачно
	if(nghttp2_session_terminate_session(session, NGHTTP2_NO_ERROR) != 0)
		// Выводим сообщение об ошибке
		return NGHTTP2_ERR_CALLBACK_FAILURE;
	// Выводим результат
	return 0;
}
/**
 * onChunk Функция обратного вызова при получении чанка с сервера HTTP/2
 * @param session объект сессии HTTP/2
 * @param flags   флаги события для сессии HTTP/2
 * @param sid     идентификатор сессии HTTP/2
 * @param buffer  буфер данных который содержит полученный чанк
 * @param size    размер полученного буфера данных чанка
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::client::Web2::onChunk(nghttp2_session * session, const uint8_t flags, const int32_t sid, const uint8_t * buffer, const size_t size, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Выполняем обработку полученных данных чанка
	return reinterpret_cast <web2_t *> (ctx)->receivedChunk(sid, buffer, size);
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
				// Выполняем обработку сигнала начала получения заголовков
				return web->receivedBeginHeaders(frame->hd.stream_id);
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
			if(frame->headers.cat == NGHTTP2_HCAT_RESPONSE)
				// Выполняем обработку полученных заголовков
				return reinterpret_cast <web2_t *> (ctx)->receivedHeader(frame->hd.stream_id, string((const char *) key, keySize), string((const char *) val, valSize));
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
 * @param sid     идентификатор сессии HTTP/2
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
			dynamic_cast <client::core_t *> (core)->close(aid);
			// Выводим сообщение об ошибке
			this->_log->print("Could not submit SETTINGS: %s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
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
	// Если протокол подключения установлен как HTTP/2
	if(this->_upgraded && (this->_session != nullptr)){
		// Результат выполнения поерации
		int rv = -1;
		// Выполняем пинг удалённого сервера
		if((rv = nghttp2_submit_ping(this->_session, 0, nullptr)) != 0){
			// Выводим сообщение об полученной ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			// Выходим из функции
			return false;
		}
		// Фиксируем отправленный результат
		if((rv = nghttp2_session_send(this->_session)) != 0){
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
 * send Метод отправки сообщения на сервер
 * @param id      идентификатор потока HTTP/2
 * @param message сообщение передаваемое на сервер
 * @param size    размер сообщения в байтах
 * @param end     флаг последнего сообщения после которого поток закрывается
 */
void awh::client::Web2::send(const int32_t id, const char * message, const size_t size, const bool end) noexcept {
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
				// Выходим из функции
				return;
			}
			// Фиксируем отправленный результат
			if((rv = nghttp2_session_send(this->_session)) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Выходим из функции
				return;
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
