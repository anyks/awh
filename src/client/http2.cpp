/**
 * @file: http2.cpp
 * @date: 2023-08-28
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
#include <client/http2.hpp>

/**
 * openCallback Метод обратного вызова при запуске работы
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Http2::openCallback(const size_t sid, awh::core_t * core) noexcept {
	// Если дисконнекта ещё не произошло
	if(this->_action == action_t::NONE){
		// Устанавливаем экшен выполнения
		this->_action = action_t::OPEN;
		// Выполняем запуск обработчика событий
		this->handler();
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::Http2::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
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
void awh::client::Http2::disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
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
void awh::client::Http2::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
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
/**
 * enableTLSCallback Метод активации зашифрованного канала TLS
 * @param url  адрес сервера для которого выполняется активация зашифрованного канала TLS
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат активации зашифрованного канала TLS
 */
bool awh::client::Http2::enableTLSCallback(const uri_t::url_t & url, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Блокируем переменные которые не используем
	(void) aid;
	(void) sid;
	(void) core;
	// Выводим результат активации
	return (!url.empty() && this->_fmk->compare(url.schema, "https"));
}
/**
 * handler Метод управления входящими методами
 */
void awh::client::Http2::handler() noexcept {
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
void awh::client::Http2::actionOpen() noexcept {
	// Выполняем подключение
	const_cast <client::core_t *> (this->_core)->open(this->_scheme.sid);
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::OPEN)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
}
/**
 * actionRead Метод обработки экшена чтения с сервера
 */
void awh::client::Http2::actionRead() noexcept {
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::READ)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
	

	ssize_t readlen = nghttp2_session_mem_recv(this->_session.ctx, (const uint8_t *) this->_buffer.data(), this->_buffer.size());
	if (readlen < 0) {

		this->_log->print("Fatal error: %s", log_t::flag_t::CRITICAL, nghttp2_strerror((int) readlen));
		
		return;
	}

	this->_buffer.clear();

	
	int rv = nghttp2_session_send(this->_session.ctx);
	if (rv != 0) {
		this->_log->print("Fatal error: %s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
		exit(1);
	}
}

static void print_header(FILE * f, const uint8_t * name, size_t namelen, const uint8_t * value, size_t valuelen){
  fwrite(name, 1, namelen, f);
  fprintf(f, ": ");
  fwrite(value, 1, valuelen, f);
  fprintf(f, "\n");
}

static void print_headers(FILE *f, nghttp2_nv *nva, size_t nvlen){
  size_t i;
  for (i = 0; i < nvlen; ++i) {
    print_header(f, nva[i].name, nva[i].namelen, nva[i].value, nva[i].valuelen);
  }
  fprintf(f, "\n");
}

static ssize_t send_callback(nghttp2_session * session, const uint8_t * data, size_t length, int flags, void * ctx){
  
  (void) session;
  (void) flags;

  // cout << " +++++++++++++++++++ SEND DATA " << length << endl;

  reinterpret_cast <awh::client::http2_t *> (ctx)->send((const char *) data, length);
  
  return (ssize_t) length;
}

static int on_frame_recv_callback(nghttp2_session * session, const nghttp2_frame *frame, void * ctx){
  (void) session;

  switch (frame->hd.type){
	case NGHTTP2_DATA: {
		cout << " +++++++++++++++++++ ALL DATA " << (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) << endl;

		nghttp2_session_del(reinterpret_cast <awh::client::http2_t *> (ctx)->_session.ctx);
	} break;
  	case NGHTTP2_HEADERS: {
		if((frame->headers.cat == NGHTTP2_HCAT_RESPONSE) && (reinterpret_cast <awh::client::http2_t *> (ctx)->_session.id == frame->hd.stream_id))
			cout << " +++++++++++++++++++ All headers received " << (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) << endl;
    } break;
  }

  if(frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
	cout << " +++++++++++++++++++ STOP " << endl;

  return 0;
}

static int on_data_chunk_recv_callback(nghttp2_session * session, uint8_t flags,  int32_t stream_id, const uint8_t * data, size_t len, void * ctx){
  (void)session;
  (void)flags;

  // cout << " +++++++++++++++++++ GET CHUNCK " << len << endl;

  if(reinterpret_cast <awh::client::http2_t *> (ctx)->_session.id == stream_id){
	// Если функция обратного вызова установлена, выводим сообщение
	if(reinterpret_cast <awh::client::http2_t *> (ctx)->_callback.message != nullptr)
		// Выполняем функцию обратного вызова
		reinterpret_cast <awh::client::http2_t *> (ctx)->_callback.message(vector <char> (data, data + len), reinterpret_cast <awh::client::http2_t *> (ctx));
  }

  return 0;
}

static int on_stream_close_callback(nghttp2_session * session, int32_t stream_id, uint32_t error_code, void * ctx){
  int rv;

  if(reinterpret_cast <awh::client::http2_t *> (ctx)->_session.id == stream_id) {
    fprintf(stderr, "Stream %d closed with error_code=%u\n", stream_id, error_code);
    rv = nghttp2_session_terminate_session(session, NGHTTP2_NO_ERROR);
    if (rv != 0) {
      return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
  }
  return 0;
}

static int on_header_callback(nghttp2_session * session, const nghttp2_frame * frame, const uint8_t * name, size_t namelen, const uint8_t * value, size_t valuelen, uint8_t flags, void * ctx){
  (void) session;
  (void) flags;

  switch (frame->hd.type) {
  	case NGHTTP2_HEADERS: {
		if((frame->headers.cat == NGHTTP2_HCAT_RESPONSE) && (reinterpret_cast <awh::client::http2_t *> (ctx)->_session.id == frame->hd.stream_id)){
			
			// cout << " @@@@@@@@@@@@ HEADER " << endl;

			/* Print response headers for the initiated request. */
			print_header(stderr, name, namelen, value, valuelen);
			break;
		}
	} break;
  }
  return 0;
}

static int on_begin_headers_callback(nghttp2_session * session, const nghttp2_frame * frame, void * ctx){
  (void) session;

  switch (frame->hd.type) {
  	case NGHTTP2_HEADERS: {
    	if((frame->headers.cat == NGHTTP2_HCAT_RESPONSE) && (reinterpret_cast <awh::client::http2_t *> (ctx)->_session.id == frame->hd.stream_id)){
      		fprintf(stderr, "Response headers for stream ID=%d:\n", frame->hd.stream_id);
    	}
    } break;
  }
  return 0;
}

static ssize_t file_read_callback(nghttp2_session * session, int32_t stream_id, uint8_t * buf, size_t length, uint32_t * data_flags, nghttp2_data_source * source, void * ctx){

	int fd = source->fd;
	ssize_t r;
	(void)session;
	(void)stream_id;
	(void)ctx;

	/**
	 * Методы только для OS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		while ((r = _read(fd, buf, length)) == -1 && errno == EINTR)
		;
	/**
	 * Для всех остальных операционных систем
	 */
	#else
		while ((r = ::read(fd, buf, length)) == -1 && errno == EINTR)
		;
	#endif


	
	if (r == -1) {
		return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
	}
	if (r == 0) {
		*data_flags |= NGHTTP2_DATA_FLAG_EOF;

		::close(fd);
	}
	return r;

}

nghttp2_nv make_nv_internal(const string &name, const string &value, bool no_index, uint8_t nv_flags){

	uint8_t flags = (nv_flags | (no_index ? NGHTTP2_NV_FLAG_NO_INDEX : NGHTTP2_NV_FLAG_NONE));

	return {
		(uint8_t *) name.c_str(),
		(uint8_t *) value.c_str(),
		name.size(),
		value.size(),
		flags
	};
}

nghttp2_nv make_nv(const std::string & name, const std::string & value, bool no_index = false){
  return make_nv_internal(name, value, no_index, NGHTTP2_NV_FLAG_NONE);
}

nghttp2_nv make_nv_nocopy(const std::string & name, const std::string & value, bool no_index = false){
	return make_nv_internal(name, value, no_index, NGHTTP2_NV_FLAG_NO_COPY_NAME |  NGHTTP2_NV_FLAG_NO_COPY_VALUE);
}

/**
 * actionConnect Метод обработки экшена подключения к серверу
 */
void awh::client::Http2::actionConnect() noexcept {
	// Выполняем очистку оставшихся данных
	this->_buffer.clear();
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::CONNECT)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
	
	nghttp2_session_callbacks * callbacks;

	nghttp2_session_callbacks_new(&callbacks);

	nghttp2_session_callbacks_set_send_callback(callbacks, send_callback);

	nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);

	nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);

	nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback);

	nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);

	nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, on_begin_headers_callback);

	nghttp2_session_client_new(&this->_session.ctx, callbacks, this);

	nghttp2_session_callbacks_del(callbacks);



	vector <nghttp2_settings_entry> iv = {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};

	/* client 24 bytes magic string will be sent by nghttp2 library */
	int rv = nghttp2_submit_settings(this->_session.ctx, NGHTTP2_FLAG_NONE, iv.data(), iv.size());
	if (rv != 0) {
		this->_log->print("Could not submit SETTINGS: %s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
		exit(1);
	}
	
	int32_t stream_id = 0;
	
	/*
	// GET
	{
		vector <nghttp2_nv> nva = {
			make_nv(":method", "GET"),
			make_nv(":path", "/"),
			make_nv(":scheme", "https"),
			make_nv(":authority", "anyks.com"),
			make_nv("accept", "*//*"),
			make_nv("user-agent", "nghttp2/" NGHTTP2_VERSION)
		};

		fprintf(stderr, "Request headers:\n");
		
		print_headers(stderr, nva.data(), nva.size());

		stream_id = nghttp2_submit_request(this->_session.ctx, nullptr, nva.data(), nva.size(), nullptr, this);
	}
	*/

	
	// POST
	{
		
		int pipefd[2];

		enum PIPES { READ, WRITE };

		// Методы только для OS Windows
		#if defined(_WIN32) || defined(_WIN64)
			int rv = _pipe(pipefd, 4096, O_BINARY);
		// Для всех остальных операционных систем
		#else
			int rv = pipe(pipefd);
		#endif

		

		static const char ERROR_HTML[] = "<html><head><title>404</title></head>"
                                 "<body><h1>Hello World!!!</h1></body></html>";

		const string contentLength = to_string(strlen(ERROR_HTML));

		vector <nghttp2_nv> nva = {
			make_nv(":method", "POST"),
			make_nv(":path", "/test.php"),
			make_nv(":scheme", "https"),
			make_nv(":authority", "anyks.com"),
			make_nv("accept", "*/*"),
			make_nv("content-type", "application/x-www-form-urlencoded"),
			make_nv("content-length", contentLength.c_str()),
			make_nv("user-agent", "nghttp2/" NGHTTP2_VERSION)
		};

		fprintf(stderr, "Request headers:\n");
		
		print_headers(stderr, nva.data(), nva.size());
		
		
		// Методы только для OS Windows
		#if defined(_WIN32) || defined(_WIN64)
			ssize_t writelen = _write(pipefd[WRITE], ERROR_HTML, sizeof(ERROR_HTML) - 1);

			_close(pipefd[WRITE]);
		// Для всех остальных операционных систем
		#else
			
			ssize_t writelen = ::write(pipefd[WRITE], ERROR_HTML, sizeof(ERROR_HTML) - 1);
			
			::close(pipefd[WRITE]);
		#endif

		if (writelen != sizeof(ERROR_HTML) - 1) {
			// Методы только для OS Windows
			#if defined(_WIN32) || defined(_WIN64)
				_close(pipefd[READ]);
			// Для всех остальных операционных систем
			#else
				::close(pipefd[READ]);
			#endif
			return;
		}

		nghttp2_data_provider data_prd;
		data_prd.source.ptr = nullptr;
		data_prd.source.fd = pipefd[0];
		data_prd.read_callback = file_read_callback;

		stream_id = nghttp2_submit_request(this->_session.ctx, nullptr, nva.data(), nva.size(), &data_prd, this);

		// ::close(pipefd[READ]);

	}


	


	if(stream_id < 0){
		this->_log->print("Could not submit HTTP request: %s", log_t::flag_t::CRITICAL, nghttp2_strerror(stream_id));
		exit(1);
	}

	this->_session.id = stream_id;

	{

		int rv = nghttp2_session_send(this->_session.ctx);
		if (rv != 0) {
			this->_log->print("Fatal error: %s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
			exit(1);
		}
	}


	// Если функция обратного вызова существует
	if(this->_callback.active != nullptr)
		// Выполняем функцию обратного вызова
		this->_callback.active(mode_t::CONNECT, this);
}
/**
 * actionDisconnect Метод обработки экшена отключения от сервера
 */
void awh::client::Http2::actionDisconnect() noexcept {
	// Если подключение является постоянным
	if(this->_scheme.alive){
		// Если функция обратного вызова установлена
		if(this->_callback.active != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.active(mode_t::DISCONNECT, this);
	// Если подключение не является постоянным
	} else {
		// Выполняем очистку оставшихся данных
		this->_buffer.clear();
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Завершаем работу
		if(this->_unbind) const_cast <client::core_t *> (this->_core)->stop();
		// Если экшен соответствует, выполняем его сброс
		if(this->_action == action_t::DISCONNECT)
			// Выполняем сброс экшена
			this->_action = action_t::NONE;
		// Если функция обратного вызова существует
		if(this->_callback.active != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.active(mode_t::DISCONNECT, this);
	}
}
/**
 * stop Метод остановки клиента
 */
void awh::client::Http2::stop() noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Завершаем работу, если разрешено остановить
		const_cast <client::core_t *> (this->_core)->stop();
	}
}
/**
 * start Метод запуска клиента
 */
void awh::client::Http2::start() noexcept {
	// Создаём объект URI
	uri_t uri(this->_fmk);
	// Устанавливаем параметры адреса
	this->_scheme.url = uri.parse("https://anyks.com");
	// Устанавливаем IP адрес
	this->_scheme.url.ip = "193.42.110.185";
	// Если биндинг не запущен
	if(!this->_core->working())
		// Выполняем запуск биндинга
		const_cast <client::core_t *> (this->_core)->start();
	// Если биндинг уже запущен, выполняем запрос на сервер
	else const_cast <client::core_t *> (this->_core)->open(this->_scheme.sid);
}
/**
 * close Метод закрытия подключения клиента
 */
void awh::client::Http2::close() noexcept {
	// Если подключение выполнено
	if(this->_core->working())
		// Завершаем работу, если разрешено остановить
		const_cast <client::core_t *> (this->_core)->close(this->_aid);
}
/**
 * on Метод установки функции обратного вызова при подключении/отключении
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const mode_t, Http2 *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.active = callback;
}
/**
 * setMessageCallback Метод установки функции обратного вызова при получении сообщения
 * @param callback функция обратного вызова
 */
void awh::client::Http2::on(function <void (const vector <char> &, Http2 *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.message = callback;
}
/**
 * response Метод отправки сообщения адъютанту
 * @param buffer буфер бинарных данных для отправки
 * @param size   размер бинарных данных для отправки
 */
void awh::client::Http2::send(const char * buffer, const size_t size) const noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		/*
		// Если включён режим отладки
		#if defined(DEBUG_MODE)
			// Выводим заголовок ответа
			cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
			// Выводим параметры ответа
			cout << string(buffer, size) << endl;
		#endif
		*/
		// Отправляем тело на сервер
		((awh::core_t *) const_cast <client::core_t *> (this->_core))->write(buffer, size, this->_aid);
	}
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::client::Http2::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
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
void awh::client::Http2::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
	// Устанавливаем количество секунд на подключение
	this->_scheme.timeouts.connect = connect;
}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::client::Http2::mode(const u_short flag) noexcept {
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = !(flag & static_cast <uint8_t> (flag_t::NOT_STOP));
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flag & static_cast <uint8_t> (flag_t::ALIVE));
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flag & static_cast <uint8_t> (flag_t::WAIT_MESS));
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->_core)->noInfo(flag & static_cast <uint8_t> (flag_t::NOT_INFO));
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->_core)->verifySSL(flag & static_cast <uint8_t> (flag_t::VERIFY_SSL));
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::Http2::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * Http2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Http2::Http2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _scheme(fmk, log), _action(action_t::NONE),
 _aid(0), _unbind(true), _fmk(fmk), _log(log), _core(core) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&Http2::openCallback, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&Http2::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&Http2::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&Http2::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем событие на активацию шифрованного TLS канала
	this->_scheme.callback.set <bool (const uri_t::url_t &, const size_t, const size_t, awh::core_t *)> ("tls", std::bind(&Http2::enableTLSCallback, this, _1, _2, _3, _4));
	// Добавляем схему сети в сетевое ядро
	const_cast <client::core_t *> (this->_core)->add(&this->_scheme);
}
