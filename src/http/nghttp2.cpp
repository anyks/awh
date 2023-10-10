/**
 * @file: nghttp2.cpp
 * @date: 2023-09-27
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
#include <http/nghttp2.hpp>

/**
 * debug Функция обратного вызова при получении отладочной информации
 * @param format формат вывода отладочной информации
 * @param args   список аргументов отладочной информации
 */
void awh::NgHttp2::debug(const char * format, va_list args) noexcept {
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
 * begin Функция начала получения фрейма заголовков HTTP/2
 * @param session объект сессии HTTP/2
 * @param frame   объект фрейма заголовков HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::NgHttp2::begin(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) session;
	// Получаем объект родительского объекта
	nghttp2_t * self = reinterpret_cast <nghttp2_t *> (ctx);
	// Выполняем определение типа фрейма
	switch(frame->hd.type){
		// Если мы получили входящие данные заголовков ответа
		case NGHTTP2_HEADERS:{
			// Получаем объект родительского объекта
			nghttp2_t * self = reinterpret_cast <nghttp2_t *> (ctx);
			// Определяем идентификатор сервиса
			switch(static_cast <uint8_t> (self->_mode)){
				// Если сервис идентифицирован как клиент
				case static_cast <uint8_t> (mode_t::CLIENT): {
					// Если мы получили ответ сервера
					if(frame->headers.cat == NGHTTP2_HCAT_RESPONSE){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
							// Выводим информацию об ошибке
							cout << self->_fmk->format("Stream ID=%d", frame->hd.stream_id) << endl << endl;
						#endif
						// Если функция обратного вызова установлена
						if(self->_callback.is("begin"))
							// Выводим функцию обратного вызова
							return self->_callback.apply <int, const int32_t> ("begin", frame->hd.stream_id);
					}
				} break;
				// Если сервис идентифицирован как сервер
				case static_cast <uint8_t> (mode_t::SERVER): {
					// Если мы получили запрос клиента
					if(frame->headers.cat == NGHTTP2_HCAT_REQUEST){
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
							// Выводим информацию об ошибке
							cout << self->_fmk->format("Stream ID=%d", frame->hd.stream_id) << endl << endl;
						#endif
						// Если функция обратного вызова установлена
						if(self->_callback.is("begin"))
							// Выводим функцию обратного вызова
							return self->_callback.apply <int, const int32_t> ("begin", frame->hd.stream_id);
					}
				} break;
			}
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * frameRecv Функция обратного вызова при получении фрейма
 * @param session объект сессии HTTP/2
 * @param frame   объект фрейма заголовков HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::NgHttp2::frameRecv(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) session;
	// Получаем объект родительского объекта
	nghttp2_t * self = reinterpret_cast <nghttp2_t *> (ctx);
	// Если функция обратного вызова установлена
	if(self->_callback.is("frame"))
		// Выводим функцию обратного вызова
		return self->_callback.apply <int, const int32_t, const direct_t, const uint8_t, const uint8_t> ("frame", frame->hd.stream_id, direct_t::RECV, frame->hd.type, frame->hd.flags);
	// Выводим результат
	return 0;
}
/**
 * frameSend Функция обратного вызова при отправки фрейма
 * @param session объект сессии HTTP/2
 * @param frame   объект фрейма заголовков HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::NgHttp2::frameSend(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) session;
	// Получаем объект родительского объекта
	nghttp2_t * self = reinterpret_cast <nghttp2_t *> (ctx);
	// Если функция обратного вызова установлена
	if(self->_callback.is("frame"))
		// Выводим функцию обратного вызова
		return self->_callback.apply <int, const int32_t, const direct_t, const uint8_t, const uint8_t> ("frame", frame->hd.stream_id, direct_t::SEND, frame->hd.type, frame->hd.flags);
	// Выводим результат
	return 0;
}
/**
 * close Функция закрытия подключения
 * @param session объект сессии HTTP/2
 * @param sid     идентификатор потока
 * @param error   флаг ошибки HTTP/2 если присутствует
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученного события
 */
int awh::NgHttp2::close(nghttp2_session * session, const int32_t sid, const uint32_t error, void * ctx) noexcept {
	// Получаем объект родительского объекта
	nghttp2_t * self = reinterpret_cast <nghttp2_t *> (ctx);
	/**
	 * Если включён режим отладки
	 */
	#if defined(DEBUG_MODE)
		// Выводим заголовок ответа
		cout << "\x1B[33m\x1B[1m^^^^^^^^^ CLOSE STREAM HTTP2 ^^^^^^^^^\x1B[0m" << endl;
		// Если ошибка не была получена
		if(error == 0x0)
			// Выводим информацию о закрытии сессии
			cout << self->_fmk->format("Stream %d closed", sid) << endl << endl;
	#endif
	// Если функция обратного вызова установлена
	if(self->_callback.is("close"))
		// Выводим функцию обратного вызова
		return self->_callback.apply <int, const int32_t, const uint32_t> ("close", sid, error);
	// Выводим значение по умолчанию
	return 0;
}
/**
 * chunk Функция обратного вызова при получении чанка
 * @param session объект сессии HTTP/2
 * @param flags   флаги события для сессии HTTP/2
 * @param sid     идентификатор потока
 * @param buffer  буфер данных который содержит полученный чанк
 * @param size    размер полученного буфера данных чанка
 * @param ctx     передаваемый промежуточный контекст
 * @return        статус полученных данных
 */
int awh::NgHttp2::chunk(nghttp2_session * session, const uint8_t flags, const int32_t sid, const uint8_t * buffer, const size_t size, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Получаем объект родительского объекта
	nghttp2_t * self = reinterpret_cast <nghttp2_t *> (ctx);
	// Если функция обратного вызова установлена
	if(self->_callback.is("chunk"))
		// Выводим функцию обратного вызова
		return self->_callback.apply <int, const int32_t, const uint8_t *, const size_t> ("chunk", sid, buffer, size);
	// Выводим значение по умолчанию
	return 0;
}
/**
 * header Функция обратного вызова при получении заголовка HTTP/2
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
int awh::NgHttp2::header(nghttp2_session * session, const nghttp2_frame * frame, const uint8_t * key, const size_t keySize, const uint8_t * val, const size_t valSize, const uint8_t flags, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Выполняем определение типа фрейма
	switch(frame->hd.type){
		// Если мы получили входящие данные заголовков ответа
		case NGHTTP2_HEADERS:{
			// Получаем объект родительского объекта
			nghttp2_t * self = reinterpret_cast <nghttp2_t *> (ctx);
			// Определяем идентификатор сервиса
			switch(static_cast <uint8_t> (self->_mode)){
				// Если сервис идентифицирован как клиент
				case static_cast <uint8_t> (mode_t::CLIENT): {
					// Если мы получили ответ сервера
					if(frame->headers.cat == NGHTTP2_HCAT_RESPONSE){
						// Если функция обратного вызова установлена
						if(self->_callback.is("header"))
							// Выводим функцию обратного вызова
							return self->_callback.apply <int, const int32_t, const string &, const string &> ("header", frame->hd.stream_id, string((const char *) key, keySize), string((const char *) val, valSize));
					}
				} break;
				// Если сервис идентифицирован как сервер
				case static_cast <uint8_t> (mode_t::SERVER): {
					// Если мы получили запрос клиента
					if(frame->headers.cat == NGHTTP2_HCAT_REQUEST){
						// Если функция обратного вызова установлена
						if(self->_callback.is("header"))
							// Выводим функцию обратного вызова
							return self->_callback.apply <int, const int32_t, const string &, const string &> ("header", frame->hd.stream_id, string((const char *) key, keySize), string((const char *) val, valSize));
					}
				} break;
			}
		} break;
	}
	// Выводим результат
	return 0;
}
/**
 * send Функция обратного вызова при подготовки данных для отправки
 * @param session объект сессии HTTP/2
 * @param buffer  буфер данных которые следует отправить
 * @param size    размер буфера данных для отправки
 * @param flags   флаги события для сессии HTTP/2
 * @param ctx     передаваемый промежуточный контекст
 * @return        количество отправленных байт
 */
ssize_t awh::NgHttp2::send(nghttp2_session * session, const uint8_t * buffer, const size_t size, const int flags, void * ctx) noexcept {
	// Выполняем блокировку неиспользуемой переменных
	(void) flags;
	(void) session;
	// Получаем объект родительского объекта
	nghttp2_t * self = reinterpret_cast <nghttp2_t *> (ctx);
	// Если функция обратного вызова установлена
	if(self->_callback.is("send"))
		// Выводим функцию обратного вызова
		self->_callback.call <const uint8_t *, const size_t> ("send", buffer, size);
	// Возвращаем количество отправленных байт
	return static_cast <ssize_t> (size);
}
/**
 * read Функция чтения подготовленных данных для формирования буфера данных который необходимо отправить
 * @param session объект сессии HTTP/2
 * @param sid     идентификатор потока
 * @param buffer  буфер данных которые следует отправить
 * @param size    размер буфера данных для отправки
 * @param flags   флаги события для сессии HTTP/2
 * @param source  объект промежуточных данных локального подключения
 * @param ctx     передаваемый промежуточный контекст
 * @return        количество отправленных байт
 */
ssize_t awh::NgHttp2::read(nghttp2_session * session, const int32_t sid, uint8_t * buffer, const size_t size, uint32_t * flags, nghttp2_data_source * source, void * ctx) noexcept {
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
 * free Метод очистки активной сессии
 */
void awh::NgHttp2::free() noexcept {
	// Если сессия HTTP/2 создана удачно
	if(this->session != nullptr){
		// Выполняем удаление сессии
		nghttp2_session_del(this->session);
		// Выполняем обнуление активной сессии
		this->session = nullptr;
	}
}
/**
 * close Метод закрытия подключения
 * @return результат закрытия подключения
 */
bool awh::NgHttp2::close() noexcept {
	// Если сессия HTTP/2 создана удачно
	if(this->session != nullptr)
		// Выполняем остановку активной сессии
		return (nghttp2_session_terminate_session(this->session, NGHTTP2_NO_ERROR) == 0);
	// Выводим результат по умолчанию
	return false;
}
/**
 * init Метод инициализации
 * @param mode     идентификатор сервиса
 * @param settings параметры настроек сессии
 * @return         результат выполнения инициализации
 */
bool awh::NgHttp2::init(const mode_t mode, const vector <nghttp2_settings_entry> & settings) noexcept {
	// Результат работы функции
	bool result = false;
	// Если параметры настроек переданы
	if(!settings.empty() && (mode != mode_t::NONE)){
		// Выполняем очистку предыдущей сессии
		this->free();
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выполняем установку функции для вывода отладочной информации
			nghttp2_set_debug_vprintf_callback(&nghttp2_t::debug);
		#endif
		// Выполняем установку идентификатора сессии
		this->_mode = mode;
		// Создаём объект функций обратного вызова
		nghttp2_session_callbacks * callbacks;
		// Выполняем инициализацию сессию функций обратного вызова
		nghttp2_session_callbacks_new(&callbacks);
		// Выполняем установку функции обратного вызова при подготовки данных для отправки
		nghttp2_session_callbacks_set_send_callback(callbacks, &nghttp2_t::send);
		// Выполняем установку функции обратного вызова при получении заголовка
		nghttp2_session_callbacks_set_on_header_callback(callbacks, &nghttp2_t::header);
		// Выполняем установку функции обратного вызова закрытия подключения
		nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, &nghttp2_t::close);
		// Выполняем установку функции обратного вызова начала получения фрейма заголовков
		nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, &nghttp2_t::begin);
		// Выполняем установку функции обратного вызова при получении фрейма заголовков
		nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, &nghttp2_t::frameRecv);
		// Выполняем установку функции обратного вызова при отправки фрейма заголовков
		nghttp2_session_callbacks_set_on_frame_send_callback(callbacks, &nghttp2_t::frameSend);
		// Выполняем установку функции обратного вызова при получении чанка
		nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, &nghttp2_t::chunk);
		// Определяем идентификатор сервиса
		switch(static_cast <uint8_t> (mode)){
			// Если сервис идентифицирован как клиент
			case static_cast <uint8_t> (mode_t::CLIENT):
				// Выполняем создание клиента HTTP/2
				nghttp2_session_client_new(&this->session, callbacks, this);
			break;
			// Если сервис идентифицирован как сервер
			case static_cast <uint8_t> (mode_t::SERVER):
				// Выполняем создание сервера HTTP/2
				nghttp2_session_server_new(&this->session, callbacks, this);
			break;
		}
		// Выполняем удаление объекта функций обратного вызова
		nghttp2_session_callbacks_del(callbacks);
		// Если список параметров настроек не пустой
		if(!settings.empty()){
			// Клиентская 24-байтовая магическая строка будет отправлена библиотекой nghttp2
			const int rv = nghttp2_submit_settings(this->session, NGHTTP2_FLAG_NONE, settings.data(), settings.size());
			// Если настройки для сессии установить не удалось
			if(!(result = (rv == 0))){
				// Выводим сообщение об ошибке
				this->_log->print("Could not submit SETTINGS: %s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SETTINGS, this->_fmk->format("Could not submit SETTINGS: %s", nghttp2_strerror(rv)));
				// Выполняем очистку предыдущей сессии
				this->free();
			}
		// Если список параметров настроек пустой
		} else {
			// Выводим сообщение об ошибке
			this->_log->print("SETTINGS list is empty: %s", log_t::flag_t::CRITICAL);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выводим функцию обратного вызова
				this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP2_SETTINGS, "SETTINGS list is empty");
			// Выполняем очистку предыдущей сессии
			this->free();
		}
	}
	// Выводим результат
	return result;
}
/**
 * on Метод установки функции обратного вызова начала открытии потока
 * @param callback функция обратного вызова
 */
void awh::NgHttp2::on(function <int (const int32_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <int (const int32_t)> ("begin", callback);
}
/**
 * on Метод установки функции обратного вызова при закрытии потока
 * @param callback функция обратного вызова
 */
void awh::NgHttp2::on(function <int (const int32_t, const uint32_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <int (const int32_t, const uint32_t)> ("close", callback);
}
/**
 * on Метод установки функции обратного вызова при отправки сообщения
 * @param callback функция обратного вызова
 */
void awh::NgHttp2::on(function <void (const uint8_t *, const size_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint8_t *, const size_t)> ("send", callback);
}
/**
 * on Метод установки функции обратного вызова при получении чанка
 * @param callback функция обратного вызова
 */
void awh::NgHttp2::on(function <int (const int32_t, const uint8_t *, const size_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <int (const int32_t, const uint8_t *, const size_t)> ("chunk", callback);
}
/**
 * on Метод установки функции обратного вызова при получении данных заголовка
 * @param callback функция обратного вызова
 */
void awh::NgHttp2::on(function <int (const int32_t, const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <int (const int32_t, const string &, const string &)> ("header", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::NgHttp2::on(function <void (const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const log_t::flag_t, const http::error_t, const string &)> ("error", callback);
}
/**
 * on Метод установки функции обратного вызова при обмене фреймами
 * @param callback функция обратного вызова
 */
void awh::NgHttp2::on(function <int (const int32_t, const direct_t, const uint8_t, const uint8_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <int (const int32_t, const direct_t, const uint8_t, const uint8_t)> ("frame", callback);
}
/**
 * ~NgHttp2 Деструктор
 */
awh::NgHttp2::~NgHttp2() noexcept {
	// Выполняем удаление созданную ранее сессию
	this->free();
}
