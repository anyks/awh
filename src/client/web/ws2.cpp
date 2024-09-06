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
 * send Метод отправки запроса на удалённый сервер
 * @param bid идентификатор брокера
 */
void awh::client::Websocket2::send(const uint64_t bid) noexcept {
	// Выполняем сброс параметров запроса
	this->flush();
	// Выполняем сброс состояния HTTP-парсера
	this->_http.clear();
	// Выполняем сброс состояния HTTP-парсера
	this->_http.reset();
	// Выполняем установку идентификатора объекта
	this->_http.id(bid);
	// Выполняем очистку функций обратного вызова
	this->_resultCallback.clear();
	// Если HTTP-заголовки установлены
	if(!this->_headers.empty())
		// Выполняем установку HTTP-заголовков
		this->_http.headers(this->_headers);
	// Устанавливаем список поддерживаемых компрессоров
	this->_http.compressors(this->_compressors);
	// Разрешаем перехватывать контекст для клиента
	this->_http.takeover(awh::web_t::hid_t::CLIENT, this->_client.takeover);
	// Разрешаем перехватывать контекст для сервера
	this->_http.takeover(awh::web_t::hid_t::SERVER, this->_server.takeover);
	// Создаём объек запроса
	awh::web_t::req_t query(2.0f, awh::web_t::method_t::CONNECT, this->_scheme.url);
	// Если активирован режим прокси-сервера
	if(this->_proxy.mode){
		// Активируем точную установку хоста
		this->_http.precise(!this->_proxy.connect);
		// Выполняем извлечение заголовка авторизации на прокси-сервера
		const string & header = this->_scheme.proxy.http.auth(http_t::process_t::REQUEST, query);
		// Если заголовок авторизации получен
		if(!header.empty())
			// Выполняем установки заголовка авторизации на прокси-сервере
			this->_http.header("Proxy-Authorization", header);
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
	// Запоминаем предыдущее значение идентификатора потока
	const int32_t sid = this->_sid;
	// Выполняем запрос на получение заголовков
	const auto & headers = this->_http.process2(http_t::process_t::REQUEST, std::move(query));
	// Выполняем запрос на удалённый сервер	
	this->_sid = web2_t::send(-1, headers, http2_t::flag_t::NONE);
	// Если запрос не получилось отправить
	if(this->_sid < 0)
		// Выполняем отключение от сервера
		const_cast <client::core_t *> (this->_core)->close(bid);
	// Если функция обратного вызова на вывод редиректа потоков установлена
	else if((sid > -1) && (sid != this->_sid) && this->_callbacks.is("redirect"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const int32_t)> ("redirect", sid, this->_sid);
}
/**
 * connectEvent Метод обратного вызова при подключении к серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Websocket2::connectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ, event_t::PROXY_CONNECT}, event_t::CONNECT)){
		// Запоминаем идентификатор брокера
		this->_bid = bid;
		// Разрешаем перехватывать контекст компрессии
		this->_hash.takeoverCompress(this->_client.takeover);
		// Разрешаем перехватывать контекст декомпрессии
		this->_hash.takeoverDecompress(this->_server.takeover);
		// Выполняем инициализацию сессии HTTP/2
		web2_t::connectEvent(bid, sid);
		// Если флаг инициализации сессии HTTP/2 установлен
		if(this->_http2.is()){
			// Устанавливаем режим работы буфера
			this->_buffer = buffer_t::mode_t::COPY;
			// Выполняем переключение протокола интернета на HTTP/2
			this->_proto = engine_t::proto_t::HTTP2;
			// Выполняем отправку запроса на удалённый сервер
			this->send(bid);
			// Если запрос не получилось отправить
			if(this->_sid < 0)
				// Выходим из функции
				return;
		// Если активирован режим работы с HTTP/1.1 протоколом
		} else {
			// Выполняем сброс параметров запроса
			this->flush();
			// Если идентификатор потока не установлен
			if(this->_sid == -1){
				// Устанавливаем идентификатор потока
				this->_sid = 1;
				// Устанавливаем идентификатор потока у протокола HTTP/1.1
				this->_ws1._sid = 1;
			}
			// Выполняем установку идентификатора объекта
			this->_ws1._http.id(bid);
			// Выполняем установку сетевого ядра
			this->_ws1._core = this->_core;
			// Выполняем установку данных URL-адреса
			this->_ws1._scheme.url = this->_scheme.url;
			// Устанавливаем список поддерживаемых компрессоров
			this->_ws1._compressors = this->_compressors;
			// Если идентификатор запроса не установлен
			if(this->_rid == 0)
				// Выполняем генерацию идентификатора запроса
				this->_rid = this->_fmk->timestamp(fmk_t::stamp_t::NANOSECONDS);
			// Устанавливаем идентификатор запроса
			this->_ws1._rid = this->_rid;
			// Если HTTP-заголовки установлены
			if(!this->_headers.empty())
				// Выполняем установку HTTP-заголовков
				this->_ws1.setHeaders(this->_headers);
			// Если многопоточность активированна
			if(this->_thr.is()){
				// Выполняем завершение всех активных потоков
				this->_thr.stop();
				// Выполняем инициализацию нового тредпула
				this->_ws1.multiThreads(this->_threads);
			}
			// Если активирован режим прокси-сервера
			if(this->_proxy.mode)
				// Выполняем сброс заголовков прокси-сервера
				this->_ws1._scheme.proxy.http.dataAuth(this->_scheme.proxy.http.dataAuth());
			// Выполняем переброс вызова коннекта на клиент Websocket
			this->_ws1.connectEvent(bid, sid);
		}
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callbacks.is("active"))
			// Выполняем функцию обратного вызова
			this->_callbacks.call <void (const mode_t)> ("active", mode_t::CONNECT);
	}
}
/**
 * disconnectEvent Метод обратного вызова при отключении от сервера
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Websocket2::disconnectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Выполняем удаление подключения
	this->_http2.close();
	// Выполняем редирект, если редирект выполнен
	if(this->redirect(bid, sid))
		// Выходим из функции
		return;
	// Выполняем сброс идентификатора потока
	this->_sid = -1;
	// Выполняем сброс идентификатора запроса
	this->_rid = 0;
	// Выполняем передачу сигнала отключения от сервера на Websocket-клиент
	this->_ws1.disconnectEvent(bid, sid);
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
		// Выполняем зануление идентификатора брокера
		this->_bid = 0;
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Если завершить работу разрешено
		if(this->_complete && (this->_core != nullptr))
			// Завершаем работу
			const_cast <client::core_t *> (this->_core)->stop();
	}
	// Выполняем сброс флага рукопожатия
	this->_shake = false;
	// Выполняем очистку параметров HTTP-запроса
	this->_http.clear();
	// Выполняем сброс состояния HTTP-парсера
	this->_http.reset();
	// Очищаем буфер собранных данных
	this->_buffer.clear();
	// Выполняем очистку оставшихся фрагментов
	this->_fragmes.clear();
	// Выполняем переключение протокола интернета обратно на HTTP/1.1
	this->_proto = engine_t::proto_t::HTTP1_1;
	// Если функция обратного вызова при подключении/отключении установлена
	if(this->_callbacks.is("active"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const mode_t)> ("active", mode_t::DISCONNECT);
}
/**
 * readEvent Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::client::Websocket2::readEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Флаг выполнения обработки полученных данных
		bool process = false;
		// Если установлена функция обратного вызова для вывода данных в сыром виде
		if(!(process = !this->_callbacks.is("raw")))
			// Выполняем функцию обратного вызова
			process = this->_callbacks.call <bool (const char *, const size_t)> ("raw", buffer, size);
		// Если обработка полученных данных разрешена
		if(process){
			// Если подключение закрыто
			if(this->_close)
				// Выходим из функции
				return;
			// Если протокол подключения является HTTP/2
			if(this->_core->proto(bid) == engine_t::proto_t::HTTP2){
				// Если получение данных не разрешено
				if(!this->_allow.receive)
					// Выходим из функции
					return;
				// Если прочитать данные фрейма не удалось, выходим из функции
				if(!this->_http2.frame(reinterpret_cast <const uint8_t *> (buffer), size)){
					// Выполняем закрытие подключения
					web2_t::close(bid);
					// Выходим из функции
					return;
				}
			// Если активирован режим работы с HTTP/1.1 протоколом, выполняем переброс вызова чтения на клиент Websocket
			} else this->_ws1.readEvent(buffer, size, bid, sid);
		}
	}
}
/**
 * writeCallback Метод обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::client::Websocket2::writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0)){
		// Если переключение протокола на HTTP/2 не выполнено
		if(this->_proto != engine_t::proto_t::HTTP2)
			// Выполняем переброс вызова записи на клиент Websocket
			this->_ws1.writeCallback(buffer, size, bid, sid);
	}
}
/**
 * chunkSignal Метод обратного вызова при получении чанка с сервера HTTP/2
 * @param sid    идентификатор потока
 * @param buffer буфер данных который содержит полученный чанк
 * @param size   размер полученного буфера данных чанка
 * @return       статус полученных данных
 */
int32_t awh::client::Websocket2::chunkSignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept {
	// Если подключение производится через, прокси-сервер
	if(this->_scheme.isProxy())
		// Выполняем обработку полученных данных чанка для прокси-сервера
		return this->chunkProxySignal(sid, buffer, size);
	// Если мы работаем с сервером напрямую
	else {
		// Если идентификатор сессии клиента совпадает
		if(this->_sid == sid){
			// Если функция обратного вызова на перехват входящих чанков установлена
			if(this->_callbacks.is("chunking"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", this->_rid, vector <char> (buffer, buffer + size), &this->_http);
			// Если функция перехвата полученных чанков не установлена
			else if(this->_core != nullptr) {
				// Если подключение закрыто
				if(this->_close)
					// Выходим из функции
					return 0;
				// Если рукопожатие не выполнено
				if(!this->_shake)
					// Добавляем полученный чанк в тело данных
					this->_http.payload(vector <char> (buffer, buffer + size));
				// Если рукопожатие выполнено
				else if(this->_allow.receive) {
					// Обнуляем время последнего ответа на пинг
					this->_point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					// Обновляем время отправленного пинга
					this->_sendPing = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					// Добавляем полученные данные в буфер
					this->_buffer.emplace(reinterpret_cast <const char *> (buffer), size);
				}
				// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
				if(this->_callbacks.is("chunks"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const int32_t, const uint64_t, const vector <char> &)> ("chunks", sid, this->_rid, vector <char> (buffer, buffer + size));
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
 * @param flags  флаги полученного фрейма
 * @return       статус полученных данных
 */
int32_t awh::client::Websocket2::frameSignal(const int32_t sid, const http2_t::direct_t direct, const http2_t::frame_t frame, const set <http2_t::flag_t> & flags) noexcept {
	// Определяем направление передачи фрейма
	switch(static_cast <uint8_t> (direct)){
		// Если производится передача фрейма на сервер
		case static_cast <uint8_t> (http2_t::direct_t::SEND): {
			// Если мы получили флаг завершения потока
			if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
				// Если необходимо выполнить закрыть подключение
				if((this->_core != nullptr) && !this->_close && this->_stopped){
					// Устанавливаем флаг закрытия подключения
					this->_close = !this->_close;
					// Выполняем закрытие подключения
					this->_http2.close();
					// Выполняем закрытие подключения
					web2_t::close(this->_bid);
				}
				// Если установлена функция отлова завершения запроса
				if(this->_callbacks.is("end"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, this->_rid, direct_t::SEND);
			}
		} break;
		// Если производится получения фрейма с сервера
		case static_cast <uint8_t> (http2_t::direct_t::RECV): {
			// Если подключение производится через, прокси-сервер
			if(this->_scheme.isProxy())
				// Выполняем обработку полученных данных фрейма для прокси-сервера
				return this->frameProxySignal(sid, direct, frame, flags);
			// Если мы работаем с сервером напрямую
			else if(this->_core != nullptr) {
				// Выполняем определение типа фрейма
				switch(static_cast <uint8_t> (frame)){
					// Если мы получили входящие данные тела ответа
					case static_cast <uint8_t> (http2_t::frame_t::DATA): {
						// Если рукопожатие не выполнено
						if(!this->_shake){
							// Если мы получили флаг завершения потока
							if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
								// Выполняем фиксацию полученного результата
								this->_http.commit();
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									{
										// Получаем объект работы с HTTP-запросами
										const http_t & http = reinterpret_cast <http_t &> (this->_http);
										// Если тело ответа существует
										if(!http.empty(awh::http_t::suite_t::BODY))
											// Выводим сообщение о выводе чанка тела
											cout << this->_fmk->format("<body %zu>", http.body().size()) << endl << endl;
										// Иначе устанавливаем перенос строки
										else cout << endl;
									}
								#endif
								// Получаем объект биндинга ядра TCP/IP
								client::core_t * core = const_cast <client::core_t *> (this->_core);
								// Выполняем препарирование полученных данных
								switch(static_cast <uint8_t> (this->prepare(sid, this->_bid))){
									// Если необходимо выполнить остановку обработки
									case static_cast <uint8_t> (status_t::STOP): {
										// Выполняем сброс количества попыток
										this->_attempt = 0;
										// Завершаем работу
										core->close(this->_bid);
									} break;
								}
								// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
								if(this->_resultCallback.is("entity"))
									// Выполняем функцию обратного вызова дисконнекта
									this->_resultCallback.bind("entity");
								// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
								if(this->_resultCallback.is("complete"))
									// Выполняем функцию обратного вызова
									this->_resultCallback.bind("complete");
								// Очищаем буфер собранных данных
								this->_buffer.clear();
								// Выполняем очистку функций обратного вызова
								this->_resultCallback.clear();
								// Если установлена функция отлова завершения запроса
								if(this->_callbacks.is("end"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, this->_rid, direct_t::RECV);
								// Завершаем работу
								return 0;
							}
						// Если рукопожатие выполнено
						} else if(this->_allow.receive) {
							// Если мы получили неустановленный флаг или флаг завершения потока
							if(flags.empty() || (flags.find(http2_t::flag_t::END_STREAM) != flags.end())){
								// Выполняем препарирование полученных данных
								switch(static_cast <uint8_t> (this->prepare(sid, this->_bid))){
									// Если необходимо выполнить остановку обработки
									case static_cast <uint8_t> (status_t::STOP):
										// Выполняем завершение работы
										goto End;
									// Если необходимо выполнить переход к следующему этапу обработки
									case static_cast <uint8_t> (status_t::NEXT): {
										// Если поток небыл закрыт
										if(flags.empty())
											// Выполняем отправку сообщения об ошибке
											this->sendError(this->_mess);
									} break;
								}
								// Устанавливаем метку завершения работы
								End:
								// Если мы получили флаг завершения потока
								if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
									// Если установлена функция отлова завершения запроса
									if(this->_callbacks.is("end"))
										// Выполняем функцию обратного вызова
										this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, this->_rid, direct_t::RECV);
								}
								// Выходим из функции
								return 0;
							}
						}
					} break;
					// Если мы получили входящие данные заголовков ответа
					case static_cast <uint8_t> (http2_t::frame_t::HEADERS): {
						// Если сессия клиента совпадает с сессией полученных даных и передача заголовков завершена
						if(flags.find(http2_t::flag_t::END_HEADERS) != flags.end()){
							// Выполняем фиксацию полученного результата
							this->_http.commit();
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Выполняем создание объекта для вывода HTTP-ответа
									http_t http(this->_fmk, this->_log);
									// Устанавливаем заголовки ответа
									http.headers(this->_http.headers());
									// Устанавливаем параметры ответа
									http.response(this->_http.response());
									// Устанавливаем компрессор ответа
									http.compression(this->_http.compression());
									// Выполняем коммит полученного результата
									http.commit();
									// Получаем данные ответа
									const auto & response = http.process(http_t::process_t::RESPONSE, http.response());
									// Если параметры ответа получены
									if(!response.empty())
										// Выводим параметры ответа
										cout << string(response.begin(), response.end()) << endl;
								}
							#endif
							// Получаем параметры запроса
							const auto & response = this->_http.response();
							// Если метод CONNECT запрещён для прокси-сервера
							if(this->_proxy.mode && !this->_proxy.connect){
								// Выполняем сброс заголовков прокси-сервера
								this->_scheme.proxy.http.clear();
								// Выполняем перебор всех полученных заголовков
								for(auto & item : this->_http.headers()){
									// Если заголовок соответствует прокси-серверу
									if(this->_fmk->exists("proxy-", item.first))
										// Выполняем добавление заголовков прокси-сервера
										this->_scheme.proxy.http.header(item.first, item.second);
								}
								// Устанавливаем статус ответа прокси-серверу
								this->_scheme.proxy.http.response(response);
								// Выполняем фиксацию полученного результата
								this->_scheme.proxy.http.commit();
							}
							// Если ответ пришел успешный или фрейм закрыт
							if((response.code == 200) || (flags.find(http2_t::flag_t::END_STREAM) != flags.end())){
								// Получаем объект биндинга ядра TCP/IP
								client::core_t * core = const_cast <client::core_t *> (this->_core);
								// Выполняем препарирование полученных данных
								switch(static_cast <uint8_t> (this->prepare(sid, this->_bid))){
									// Если необходимо выполнить остановку обработки
									case static_cast <uint8_t> (status_t::STOP): {
										// Выполняем сброс количества попыток
										this->_attempt = 0;
										// Завершаем работу
										core->close(this->_bid);
									} break;
									// Если необходимо выполнить переход к следующему этапу обработки
									case static_cast <uint8_t> (status_t::NEXT):
										// Очищаем буфер собранных данных
										this->_buffer.clear();
									break;
									// Если необходимо выполнить пропуск обработки данных
									case static_cast <uint8_t> (status_t::SKIP): {
										// Если мы получили флаг завершения потока
										if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
											// Очищаем буфер собранных данных
											this->_buffer.clear();
											// Если установлена функция отлова завершения запроса
											if(this->_callbacks.is("end"))
												// Выполняем функцию обратного вызова
												this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, this->_rid, direct_t::RECV);
										}
										// Завершаем работу
										return 0;
									}
								}
							}
							// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
							if(this->_callbacks.is("response"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", sid, this->_rid, response.code, response.message);
							// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
							if(this->_callbacks.is("headers"))
								// Выполняем функцию обратного вызова
								this->_callbacks.call <void (const int32_t, const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headers", sid, this->_rid, response.code, response.message, this->_http.headers());
							// Если мы получили флаг завершения потока
							if(flags.find(http2_t::flag_t::END_STREAM) != flags.end()){
								// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
								if(this->_callbacks.is("complete"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", sid, this->_rid, response.code, response.message, this->_http.body(), this->_http.headers());
								// Очищаем буфер собранных данных
								this->_buffer.clear();
								// Если установлена функция отлова завершения запроса
								if(this->_callbacks.is("end"))
									// Выполняем функцию обратного вызова
									this->_callbacks.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, this->_rid, direct_t::RECV);
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
 * closedSignal Метод завершения работы потока
 * @param sid   идентификатор потока
 * @param error флаг ошибки если присутствует
 * @return      статус полученных данных
 */
int32_t awh::client::Websocket2::closedSignal(const int32_t sid, const http2_t::error_t error) noexcept {
	// Если флаг инициализации сессии HTTP/2 установлен
	if((this->_core != nullptr) && (error != http2_t::error_t::NONE) && this->_http2.is())
		// Выполняем закрытие подключения
		web2_t::close(this->_bid);
	// Если функция обратного вызова активности потока установлена
	if(this->_callbacks.is("stream"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, this->_rid, mode_t::CLOSE);
	// Выводим результат
	return 0;
}
/**
 * beginSignal Метод начала получения фрейма заголовков HTTP/2 сервера
 * @param sid идентификатор потока
 * @return    статус полученных данных
 */
int32_t awh::client::Websocket2::beginSignal(const int32_t sid) noexcept {
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
			// Выполняем очистку параметров HTTP-запроса
			this->_http.clear();
			// Выполняем сброс состояния HTTP-парсера
			this->_http.reset();
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
 * headerSignal Метод обратного вызова при получении заголовка HTTP/2 сервера
 * @param sid идентификатор потока
 * @param key данные ключа заголовка
 * @param val данные значения заголовка
 * @return    статус полученных данных
 */
int32_t awh::client::Websocket2::headerSignal(const int32_t sid, const string & key, const string & val) noexcept {
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
			if(this->_callbacks.is("header"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const int32_t, const uint64_t, const string &, const string &)> ("header", sid, this->_rid, key, val);
		}
	}
	// Выводим результат
	return 0;
}
/**
 * answer Метод получение статуса ответа сервера
 * @param sid    идентификатор потока
 * @param rid    идентификатор запроса
 * @param status статус ответа сервера
 */
void awh::client::Websocket2::answer(const int32_t sid, const uint64_t rid, const awh::http_t::status_t status) noexcept {
	// Если статус входящего сообщения является положительным
	if(status == awh::http_t::status_t::GOOD)
		// Выполняем сброс количества попыток
		this->_attempt = 0;
	// Если функция обратного вызова получения статуса ответа установлена
	if(this->_callbacks.is("answer"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", sid, rid, status);
}
/**
 * redirect Метод выполнения редиректа если требуется
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 * @return    результат выполнения редиректа
 */
bool awh::client::Websocket2::redirect(const uint64_t bid, const uint16_t sid) noexcept {
	// Результат работы функции
	bool result = false;
	// Если редиректы разрешены
	if(this->_redirects){
		// Если переключение протокола на HTTP/2 не выполнено
		if(this->_proto != engine_t::proto_t::HTTP2){
			// Если список ответов получен
			if((result = !this->_ws1._stopped)){
				// Получаем параметры запроса
				const auto & response = this->_ws1._http.response();
				// Если необходимо выполнить ещё одну попытку выполнения авторизации
				if((result = (this->_proxy.answer == 407) || (response.code == 401) || (response.code == 407))){
					// Выполняем переброс вызова дисконнекта на клиент Websocket
					this->_ws1.disconnectEvent(bid, sid);
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
				if((result = this->_ws1._http.is(http_t::suite_t::HEADER, "location"))){
					// Выполняем очистку оставшихся данных
					this->_ws1._buffer.clear();
					// Получаем новый адрес запроса
					const uri_t::url_t & url = this->_ws1._http.url();
					// Если адрес запроса получен
					if((result = !url.empty())){
						// Выполняем переброс вызова дисконнекта на клиент Websocket
						this->_ws1.disconnectEvent(bid, sid);
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
				if((result = this->_http.is(http_t::suite_t::HEADER, "location"))){
					// Выполняем очистку оставшихся данных
					this->_buffer.clear();
					// Получаем новый адрес запроса
					const uri_t::url_t & url = this->_http.url();
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
	}
	// Выводим результат
	return result;
}
/**
 * eventCallback Метод отлавливания событий контейнера функций обратного вызова
 * @param event событие контейнера функций обратного вызова
 * @param idw   идентификатор функции обратного вызова
 * @param name  название функции обратного вызова
 * @param dump  дамп данных функции обратного вызова
 */
void awh::client::Websocket2::eventCallback(const fn_t::event_t event, const uint64_t idw, const string & name, const fn_t::dump_t * dump) noexcept {
	// Определяем входящее событие контейнера функций обратного вызова
	switch(static_cast <uint8_t> (event)){
		// Если событием является установка функции обратного вызова
		case static_cast <uint8_t> (fn_t::event_t::SET): {
			// Если переменная не является редиректом и не является событием подключения
			if((dump != nullptr) && !this->_fmk->compare(name, "redirect") && !this->_fmk->compare(name, "active")){
				// Создаём локальный контейнер функций обратного вызова
				fn_t callbacks(this->_log);
				// Выполняем установку функции обратного вызова
				callbacks.dump(idw, * dump);
				// Если функции обратного вызова установлены
				if(!callbacks.empty())
					// Выполняем установку функций обратного вызова для Websocket-клиента
					this->_ws1.callbacks(std::move(callbacks));
			}
		} break;
	}
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::Websocket2::flush() noexcept {
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2)
		// Выполняем сброс параметров запроса на клиенте Websocket
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
 * pinging Метод таймера выполнения пинга удалённого сервера
 * @param tid идентификатор таймера
 */
void awh::client::Websocket2::pinging(const uint16_t tid) noexcept {
	// Если данные существуют
	if((tid > 0) && (this->_core != nullptr)){
		// Если разрешено выполнять пинги
		if(this->_pinging && !this->_close){
			// Если переключение протокола на HTTP/2 не выполнено
			if(this->_proto != engine_t::proto_t::HTTP2)
				// Выполняем переброс персистентного вызова на клиент Websocket
				this->_ws1.pinging(tid);
			// Если переключение протокола на HTTP/2 выполнено
			else if(this->_allow.receive) {
				// Если рукопожатие выполнено
				if(this->_shake){
					// Получаем текущий штамп времени
					const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					// Если брокер не ответил на пинг больше двух интервалов, отключаем его
					if((stamp - this->_point) >= this->_waitPong){
						// Создаём сообщение
						this->_mess = ws::mess_t(1005, "PING response not received");
						// Выполняем отправку сообщения об ошибке
						this->sendError(this->_mess);
					// Если время с предыдущего пинга прошло больше половины времени пинга
					} else if((stamp - this->_sendPing) > (PING_INTERVAL / 2))
						// Отправляем запрос брокеру
						this->ping(::to_string(this->_bid));
				// Если рукопожатие уже выполнено и пинг не прошёл
				} else if(!web2_t::ping())
					// Выполняем закрытие подключения
					web2_t::close(this->_bid);
			}
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::Websocket2::ping(const string & message) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && this->_allow.send){
		// Если рукопожатие выполнено
		if(this->_http.handshake(http_t::process_t::RESPONSE) && (this->_bid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->_frame.methods.ping(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty()){
				// Выполняем отправку сообщения на сервер
				if(web2_t::send(this->_sid, buffer.data(), buffer.size(), http2_t::flag_t::NONE))
					// Обновляем время отправленного пинга
					this->_sendPing = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
			}
		}
	}
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::Websocket2::pong(const string & message) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && this->_allow.send){
		// Если рукопожатие выполнено
		if(this->_http.handshake(http_t::process_t::RESPONSE) && (this->_bid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->_frame.methods.pong(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				web2_t::send(this->_sid, buffer.data(), buffer.size(), http2_t::flag_t::NONE);
		}
	}
}
/**
 * prepare Метод выполнения препарирования полученных данных
 * @param sid идентификатор запроса
 * @param bid идентификатор брокера
 * @return    результат препарирования
 */
awh::client::Web::status_t awh::client::Websocket2::prepare(const int32_t sid, const uint64_t bid) noexcept {
	// Результат работы функции
	status_t result = status_t::STOP;
	// Если рукопожатие не выполнено
	if(!this->_shake){
		// Получаем параметры запроса
		auto response = this->_http.response();
		// Выполняем проверку авторизации
		switch(static_cast <uint8_t> (this->_http.auth())){
			// Если нужно попытаться ещё раз
			case static_cast <uint8_t> (http_t::status_t::RETRY): {
				// Если функция обратного вызова получения статуса ответа установлена
				if(this->_callbacks.is("answer"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", sid, this->_rid, http_t::status_t::RETRY);
				// Если попытка повторить авторизацию ещё не проводилась
				if(!(this->_stopped = (this->_attempt >= this->_attempts))){
					// Увеличиваем количество попыток
					this->_attempt++;
					// Если функция обратного вызова на на вывод ошибок установлена
					if((response.code == 401) && this->_callbacks.is("error"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "authorization failed");
					// Получаем новый адрес запроса
					const uri_t::url_t & url = this->_http.url();
					// Если URL-адрес запроса получен
					if(!url.empty()){
						// Выполняем проверку соответствие протоколов
						const bool schema = (
							(this->_fmk->compare(url.host, this->_scheme.url.host)) &&
							(this->_fmk->compare(url.schema, this->_scheme.url.schema))
						);
						// Если соединение является постоянным
						if(schema && this->_http.is(http_t::state_t::ALIVE)){
							// Устанавливаем новый адрес запроса
							this->_uri.combine(this->_scheme.url, url);
							// Выполняем отправку запроса на удалённый сервер
							this->send(bid);
							// Если запрос не получилось отправить
							if(this->_sid < 0)
								// Выполняем отключение от сервера
								const_cast <client::core_t *> (this->_core)->close(bid);
							// Завершаем работу
							return status_t::SKIP;
						// Если подключение не постоянное, то завершаем работу
						} else const_cast <client::core_t *> (this->_core)->close(bid);
					// Если URL-адрес запроса не получен
					} else {
						// Если соединение является постоянным
						if(this->_http.is(http_t::state_t::ALIVE)){
							// Выполняем отправку запроса на удалённый сервер
							this->send(bid);
							// Если запрос не получилось отправить
							if(this->_sid < 0)
								// Выполняем отключение от сервера
								const_cast <client::core_t *> (this->_core)->close(bid);
							// Завершаем работу
							return status_t::SKIP;
						// Если подключение не постоянное, то завершаем работу
						} else const_cast <client::core_t *> (this->_core)->close(bid);
					}
					// Завершаем работу
					return status_t::SKIP;
				}
				// Создаём сообщение
				this->_mess = ws::mess_t(response.code, this->_http.message(response.code));
				// Выводим сообщение
				this->error(this->_mess);
			} break;
			// Если запрос выполнен удачно
			case static_cast <uint8_t> (http_t::status_t::GOOD): {
				// Если функция обратного вызова получения статуса ответа установлена
				if(this->_callbacks.is("answer"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", sid, this->_rid, http_t::status_t::GOOD);
				// Если рукопожатие выполнено
				if((this->_shake = this->_http.handshake(http_t::process_t::RESPONSE))){
					// Выполняем сброс количества попыток
					this->_attempt = 0;
					// Очищаем список фрагментированных сообщений
					this->_fragmes.clear();
					// Получаем флаг шифрованных данных
					this->_crypted = this->_http.crypted();
					// Получаем поддерживаемый метод компрессии
					this->_compressor = this->_http.compression();
					// Получаем размер скользящего окна сервера
					this->_server.wbit = this->_http.wbit(awh::web_t::hid_t::SERVER);
					// Получаем размер скользящего окна клиента
					this->_client.wbit = this->_http.wbit(awh::web_t::hid_t::CLIENT);
					// Обновляем контрольную точку времени получения данных
					this->_point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					// Если данные необходимо зашифровать
					if(this->_encryption.mode && this->_crypted){
						// Устанавливаем соль шифрования
						this->_hash.salt(this->_encryption.salt);
						// Устанавливаем пароль шифрования
						this->_hash.pass(this->_encryption.pass);
						// Устанавливаем размер шифрования
						this->_hash.cipher(this->_encryption.cipher);
					}
					// Разрешаем перехватывать контекст компрессии для клиента
					this->_hash.takeoverCompress(this->_http.takeover(awh::web_t::hid_t::CLIENT));
					// Разрешаем перехватывать контекст компрессии для сервера
					this->_hash.takeoverDecompress(this->_http.takeover(awh::web_t::hid_t::SERVER));
					// Если разрешено в лог выводим информационные сообщения
					if(this->_verb)
						// Выводим в лог сообщение об удачной авторизации не Websocket-сервере
						this->_log->print("Authorization on the Websocket-server was successful", log_t::flag_t::INFO);
					// Если функция обратного вызова активности потока установлена
					if(this->_callbacks.is("stream"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", sid, this->_rid, mode_t::OPEN);
					// Если функция обратного вызова на получение удачного ответа установлена
					if(this->_callbacks.is("handshake"))
						// Выполняем функцию обратного вызова
						this->_callbacks.call <void (const int32_t, const uint64_t, const agent_t)> ("handshake", sid, this->_rid, agent_t::WEBSOCKET);
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(!this->_http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
						// Устанавливаем полученную функцию обратного вызова
						this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, this->_rid, response.code, response.message, this->_http.body());
					// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
					if(this->_callbacks.is("complete"))
						// Выполняем функцию обратного вызова
						this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const unordered_multimap <string, string> &)> ("complete"), sid, this->_rid, response.code, response.message, this->_http.body(), this->_http.headers());
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
					if(!this->_http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
						// Устанавливаем полученную функцию обратного вызова
						this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, this->_rid, response.code, response.message, this->_http.body());
					// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
					if(this->_callbacks.is("complete"))
						// Выполняем функцию обратного вызова
						this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const unordered_multimap <string, string> &)> ("complete"), sid, this->_rid, response.code, response.message, this->_http.body(), this->_http.headers());
				}
			} break;
			// Если запрос неудачный
			case static_cast <uint8_t> (http_t::status_t::FAULT): {
				// Устанавливаем флаг принудительной остановки
				this->_stopped = true;
				// Создаём сообщение
				this->_mess = ws::mess_t(response.code, response.message);
				// Выводим сообщение
				this->error(this->_mess);
				// Если функция обратного вызова получения статуса ответа установлена
				if(this->_callbacks.is("answer"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", sid, this->_rid, http_t::status_t::FAULT);
				// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
				if(!this->_http.empty(awh::http_t::suite_t::BODY) && this->_callbacks.is("entity"))
					// Устанавливаем полученную функцию обратного вызова
					this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, this->_rid, response.code, response.message, this->_http.body());
				// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
				if(this->_callbacks.is("complete"))
					// Выполняем функцию обратного вызова
					this->_resultCallback.set <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", this->_callbacks.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const unordered_multimap <string, string> &)> ("complete"), sid, this->_rid, response.code, response.message, this->_http.body(), this->_http.headers());
			} break;
		}
		// Завершаем работу
		return status_t::STOP;
	// Если рукопожатие выполнено
	} else if(this->_allow.receive) {
		// Флаг удачного получения данных
		bool receive = false;
		// Создаём объект шапки фрейма
		ws::frame_t::head_t head;
		// Выполняем обработку полученных данных
		while(!this->_close && this->_allow.receive && !this->_buffer.empty()){
			// Выполняем чтение фрейма Websocket
			const auto & payload = this->_frame.methods.get(head, static_cast <buffer_t::data_t> (this->_buffer), static_cast <size_t> (this->_buffer));
			// Если буфер данных получен
			if(!payload.empty()){
				// Определяем тип ответа
				switch(static_cast <uint8_t> (head.optcode)){
					// Если ответом является PING
					case static_cast <uint8_t> (ws::frame_t::opcode_t::PING):
						// Отправляем ответ серверу
						this->pong(string(payload.begin(), payload.end()));
					break;
					// Если ответом является PONG
					case static_cast <uint8_t> (ws::frame_t::opcode_t::PONG): {
						// Если идентификатор брокера совпадает
						if(::memcmp(::to_string(bid).c_str(), payload.data(), payload.size()) == 0)
							// Обновляем контрольную точку
							this->_point = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					} break;
					// Если ответом является TEXT
					case static_cast <uint8_t> (ws::frame_t::opcode_t::TEXT):
					// Если ответом является BINARY
					case static_cast <uint8_t> (ws::frame_t::opcode_t::BINARY): {
						// Запоминаем полученный опкод
						this->_frame.opcode = head.optcode;
						// Запоминаем, что данные пришли сжатыми
						this->_inflate = (head.rsv[0] && (this->_compressor != http_t::compressor_t::NONE));
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
							this->_fragmes.insert(this->_fragmes.end(), payload.begin(), payload.end());
						// Если сообщение является последним
						else {
							// Если тредпул активирован
							if(this->_thr.is())
								// Добавляем в тредпул новую задачу на извлечение полученных сообщений
								this->_thr.push(std::bind(&ws2_t::extraction, this, payload, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT)));
							// Если тредпул не активирован, выполняем извлечение полученных сообщений
							else this->extraction(payload, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT));
						}
					} break;
					// Если ответом является CONTINUATION
					case static_cast <uint8_t> (ws::frame_t::opcode_t::CONTINUATION): {
						// Заполняем фрагментированное сообщение
						this->_fragmes.insert(this->_fragmes.end(), payload.begin(), payload.end());
						// Если сообщение является последним
						if(head.fin){
							// Если тредпул активирован
							if(this->_thr.is())
								// Добавляем в тредпул новую задачу на извлечение полученных сообщений
								this->_thr.push(std::bind(&ws2_t::extraction, this, this->_fragmes, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT)));
							// Если тредпул не активирован, выполняем извлечение полученных сообщений
							else this->extraction(this->_fragmes, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT));
							// Очищаем список фрагментированных сообщений
							this->_fragmes.clear();
						}
					} break;
					// Если ответом является CLOSE
					case static_cast <uint8_t> (ws::frame_t::opcode_t::CLOSE): {
						// Извлекаем сообщение
						this->_mess = this->_frame.methods.message(payload);
						// Выводим сообщение
						this->error(this->_mess);
						// Выполняем закрытие подключения
						web2_t::close(bid);
						// Выполняем реконнект
						return status_t::NEXT;
					}
				}
				// Если парсер обработал какое-то количество байт
				if((head.frame > 0) && !this->_buffer.empty()){
					// Если размер буфера больше количества удаляемых байт
					if((receive = (static_cast <size_t> (this->_buffer) >= head.frame)))
						// Удаляем количество обработанных байт
						this->_buffer.erase(head.frame);
				}
			// Если мы получили ошибку получения фрейма
			} else if(head.state == ws::frame_t::state_t::BAD) {
				// Создаём сообщение
				this->_mess = this->_frame.methods.message(head, (this->_compressor != http_t::compressor_t::NONE));
				// Выводим сообщение
				this->error(this->_mess);
				// Выполняем реконнект
				return status_t::NEXT;
			}
			// Если данные мы все получили, выходим
			if(!receive || payload.empty() || this->_buffer.empty())
				// Выходим из условия
				break;
		}
		// Фиксируем изменение в буфере
		this->_buffer.commit();
	}
	// Выполняем завершение работы
	return status_t::STOP;
}
/**
 * error Метод вывода сообщений об ошибках работы клиента
 * @param message сообщение с описанием ошибки
 */
void awh::client::Websocket2::error(const ws::mess_t & message) const noexcept {
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
			if(this->_callbacks.is("errorWebsocket"))
				// Если функция обратного вызова установлена, выводим полученное сообщение
				this->_callbacks.call <void (const uint32_t, const string &)> ("errorWebsocket", message.code, message.text);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callbacks.is("error"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::WEBSOCKET, this->_fmk->format("%s [%u]", message.code, message.text.c_str()));
		}
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param buffer данные в чистом виде полученные с сервера
 * @param text   данные передаются в текстовом виде
 */
void awh::client::Websocket2::extraction(const vector <char> & buffer, const bool text) noexcept {
	// Если буфер данных передан
	if(!buffer.empty() && !this->_freeze && this->_callbacks.is("messageWebsocket")){
		// Если данные пришли в сжатом виде
		if(this->_inflate && (this->_compressor != http_t::compressor_t::NONE)){
			// Декомпрессионные данные
			vector <char> data;
			// Определяем метод компрессии
			switch(static_cast <uint8_t> (this->_compressor)){
				// Если метод компрессии выбран LZ4
				case static_cast <uint8_t> (http_t::compressor_t::LZ4):
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::LZ4);
				break;
				// Если метод компрессии выбран Zstandard
				case static_cast <uint8_t> (http_t::compressor_t::ZSTD):
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::ZSTD);
				break;
				// Если метод компрессии выбран LZma
				case static_cast <uint8_t> (http_t::compressor_t::LZMA):
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::LZMA);
				break;
				// Если метод компрессии выбран Brotli
				case static_cast <uint8_t> (http_t::compressor_t::BROTLI):
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BROTLI);
				break;
				// Если метод компрессии выбран BZip2
				case static_cast <uint8_t> (http_t::compressor_t::BZIP2):
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::BZIP2);
				break;
				// Если метод компрессии выбран GZip
				case static_cast <uint8_t> (http_t::compressor_t::GZIP):
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::GZIP);
				break;
				// Если метод компрессии выбран Deflate
				case static_cast <uint8_t> (http_t::compressor_t::DEFLATE): {
					// Устанавливаем размер скользящего окна
					this->_hash.wbit(this->_server.wbit);
					// Добавляем хвост в полученные данные
					this->_hash.setTail(* const_cast <vector <char> *> (&buffer));
					// Выполняем декомпрессию полученных данных
					data = this->_hash.decompress(buffer.data(), buffer.size(), hash_t::method_t::DEFLATE);
				} break;
			}
			// Если данные получены
			if(!data.empty()){
				// Если нужно производить дешифрование
				if(this->_crypted){
					// Выполняем шифрование переданных данных
					const auto & res = this->_hash.decrypt(data.data(), data.size());
					// Если данные сообщения получилось удачно расшифровать
					if(!res.empty())
						// Выводим данные полученного сообщения
						this->_callbacks.call <void (const vector <char> &, const bool)> ("messageWebsocket", res, text);
					// Иначе выводим сообщение так - как оно пришло
					else this->_callbacks.call <void (const vector <char> &, const bool)> ("messageWebsocket", data, text);
				// Отправляем полученный результат
				} else this->_callbacks.call <void (const vector <char> &, const bool)> ("messageWebsocket", data, text);
			// Выводим сообщение об ошибке
			} else {
				// Иначе выводим сообщение так - как оно пришло
				this->_callbacks.call <void (const vector <char> &, const bool)> ("messageWebsocket", buffer, text);
				// Создаём сообщение
				this->_mess = ws::mess_t(1007, "Received data decompression error");
				// Выполняем отправку сообщения об ошибке
				this->sendError(this->_mess);
			}
		// Если функция обратного вызова установлена, выводим полученное сообщение
		} else {
			// Если нужно производить дешифрование
			if(this->_crypted){
				// Выполняем шифрование переданных данных
				const auto & res = this->_hash.decrypt(buffer.data(), buffer.size());
				// Если данные сообщения получилось удачно распаковать
				if(!res.empty())
					// Выводим данные полученного сообщения
					this->_callbacks.call <void (const vector <char> &, const bool)> ("messageWebsocket", res, text);
				// Иначе выводим сообщение так - как оно пришло
				else this->_callbacks.call <void (const vector <char> &, const bool)> ("messageWebsocket", buffer, text);
			// Отправляем полученный результат
			} else this->_callbacks.call <void (const vector <char> &, const bool)> ("messageWebsocket", buffer, text);
		}
	}
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::Websocket2::sendError(const ws::mess_t & mess) noexcept {
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2)
		// Выполняем отправку сообщение об ошибке на клиент Websocket
		this->_ws1.sendError(mess);
	// Если переключение протокола на HTTP/2 выполнено
	else {
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
			// Если подключение выполнено
			if((this->_core != nullptr) && this->_core->working() && this->_allow.send && (this->_bid > 0)){
				// Запрещаем получение данных
				this->_allow.receive = false;
				// Если код ошибки относится к Websocket
				if((mess.code >= 1000) && !this->_stopped){
					// Получаем буфер сообщения
					const auto & buffer = this->_frame.methods.message(mess);
					// Если данные сообщения получены
					if((this->_stopped = !buffer.empty())){
						// Выполняем отправку сообщения на сервер
						web2_t::send(this->_sid, buffer.data(), buffer.size(), http2_t::flag_t::END_STREAM);
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим заголовок ответа
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ SEND ERROR ^^^^^^^^^\x1B[0m" << endl;
							// Выводим отправляемое сообщение
							cout << this->_fmk->format("%s [%u]", mess.text.c_str(), mess.code) << endl << endl;
						#endif
						// Выводим сообщение об ошибке
						this->error(mess);
						// Выходим из функции
						return;
					}
				}
				// Завершаем работу
				const_cast <client::core_t *> (this->_core)->close(this->_bid);
			}
		}
	}
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::client::Websocket2::sendMessage(const vector <char> & message, const bool text) noexcept {
	// Результат работы функции
	bool result = false;
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2)
		// Выполняем отправку сообщения на клиент Websocket
		return this->_ws1.sendMessage(message, text);
	// Если переключение протокола на HTTP/2 выполнено
	else {
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
			// Если подключение выполнено
			if((this->_core != nullptr) && this->_core->working() && this->_allow.send){
				// Выполняем блокировку отправки сообщения
				this->_allow.send = !this->_allow.send;
				// Если рукопожатие выполнено
				if(!message.empty() && this->_http.handshake(http_t::process_t::RESPONSE) && (this->_bid > 0)){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим заголовок ответа
						cout << "\x1B[33m\x1B[1m^^^^^^^^^ SEND MESSAGE ^^^^^^^^^\x1B[0m" << endl;
						// Если отправляемое сообщение является текстом
						if(text)
							// Выводим параметры ответа
							cout << string(message.begin(), message.end()) << endl << endl;
						// Выводим сообщение о выводе чанка полезной нагрузки
						else cout << this->_fmk->format("<bytes %zu>", message.size()) << endl << endl;
					#endif
					// Создаём объект заголовка для отправки
					ws::frame_t::head_t head(true, true);
					// Если нужно производить шифрование
					if(this->_crypted){
						// Выполняем шифрование переданных данных
						const auto & payload = this->_hash.encrypt(message.data(), message.size());
						// Если данные зашифрованны
						if(!payload.empty())
							// Заменяем сообщение для передачи
							const_cast <vector <char> &> (message).assign(payload.begin(), payload.end());
					}
					// Устанавливаем опкод сообщения
					head.optcode = (text ? ws::frame_t::opcode_t::TEXT : ws::frame_t::opcode_t::BINARY);
					// Указываем, что сообщение передаётся в сжатом виде
					head.rsv[0] = ((message.size() >= 1024) && (this->_compressor != http_t::compressor_t::NONE));
					// Если необходимо сжимать сообщение перед отправкой
					if(head.rsv[0]){
						// Компрессионные данные
						vector <char> data;
						// Определяем метод компрессии
						switch(static_cast <uint8_t> (this->_compressor)){
							// Если метод компрессии выбран LZ4
							case static_cast <uint8_t> (http_t::compressor_t::LZ4):
								// Выполняем компрессию полученных данных
								data = this->_hash.compress(message.data(), message.size(), hash_t::method_t::LZ4);
							break;
							// Если метод компрессии выбран Zstandard
							case static_cast <uint8_t> (http_t::compressor_t::ZSTD):
								// Выполняем компрессию полученных данных
								data = this->_hash.compress(message.data(), message.size(), hash_t::method_t::ZSTD);
							break;
							// Если метод компрессии выбран LZma
							case static_cast <uint8_t> (http_t::compressor_t::LZMA):
								// Выполняем компрессию полученных данных
								data = this->_hash.compress(message.data(), message.size(), hash_t::method_t::LZMA);
							break;
							// Если метод компрессии выбран Brotli
							case static_cast <uint8_t> (http_t::compressor_t::BROTLI):
								// Выполняем компрессию полученных данных
								data = this->_hash.compress(message.data(), message.size(), hash_t::method_t::BROTLI);
							break;
							// Если метод компрессии выбран BZip2
							case static_cast <uint8_t> (http_t::compressor_t::BZIP2):
								// Выполняем компрессию полученных данных
								data = this->_hash.compress(message.data(), message.size(), hash_t::method_t::BZIP2);
							break;
							// Если метод компрессии выбран GZip
							case static_cast <uint8_t> (http_t::compressor_t::GZIP):
								// Выполняем компрессию полученных данных
								data = this->_hash.compress(message.data(), message.size(), hash_t::method_t::GZIP);
							break;
							// Если метод компрессии выбран Deflate
							case static_cast <uint8_t> (http_t::compressor_t::DEFLATE): {
								// Устанавливаем размер скользящего окна
								this->_hash.wbit(this->_client.wbit);
								// Выполняем компрессию полученных данных
								data = this->_hash.compress(message.data(), message.size(), hash_t::method_t::DEFLATE);
								// Удаляем хвост в полученных данных
								this->_hash.rmTail(data);
							} break;
						}
						// Если сжатие данных прошло удачно
						if(!data.empty())
							// Заменяем сообщение для передачи
							const_cast <vector <char> &> (message).assign(data.begin(), data.end());
						// Снимаем флаг сжатых данных
						else head.rsv[0] = !head.rsv[0];
					}
					// Если требуется фрагментация сообщения
					if(message.size() > this->_frame.size){
						// Бинарный буфер чанка данных
						vector <char> chunk(this->_frame.size);
						// Смещение в бинарном буфере
						size_t start = 0, stop = this->_frame.size;
						// Выполняем разбивку полезной нагрузки на сегменты
						while(stop < message.size()){
							// Увеличиваем длину чанка
							stop += this->_frame.size;
							// Если длина чанка слишком большая, компенсируем
							stop = (stop > message.size() ? message.size() : stop);
							// Устанавливаем флаг финального сообщения
							head.fin = (stop == message.size());
							// Формируем чанк бинарных данных
							chunk.assign(message.data() + start, message.data() + stop);
							// Создаём буфер для отправки
							const auto & payload = this->_frame.methods.set(head, chunk.data(), chunk.size());
							// Если бинарный буфер для отправки данных получен
							if(!payload.empty())
								// Выполняем отправку сообщения на сервер
								result = web2_t::send(this->_sid, payload.data(), payload.size(), http2_t::flag_t::NONE);
							// Выполняем сброс RSV1
							head.rsv[0] = false;
							// Устанавливаем опкод сообщения
							head.optcode = ws::frame_t::opcode_t::CONTINUATION;
							// Увеличиваем смещение в буфере
							start = stop;
							// Если запрос не отправлен
							if(!result)
								// Выходим из цикла
								break;
						}
					// Если фрагментация сообщения не требуется
					} else {
						// Создаём буфер для отправки
						const auto & payload = this->_frame.methods.set(head, message.data(), message.size());
						// Если бинарный буфер для отправки данных получен
						if(!payload.empty())
							// Выполняем отправку сообщения на сервер
							result = web2_t::send(this->_sid, payload.data(), payload.size(), http2_t::flag_t::NONE);
					}
				}
				// Выполняем разблокировку отправки сообщения
				this->_allow.send = !this->_allow.send;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * send Метод отправки данных в бинарном виде серверу
 * @param buffer буфер бинарных данных передаваемых серверу
 * @param size   размер сообщения в байтах
 * @return       результат отправки сообщения
 */
bool awh::client::Websocket2::send(const char * buffer, const size_t size) noexcept {
	// Если данные переданы верные
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0))
		// Выполняем отправку заголовков запроса серверу
		return const_cast <client::core_t *> (this->_core)->send(buffer, size, this->_bid);
	// Сообщаем что ничего не найдено
	return false;
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::Websocket2::pause() noexcept {
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
void awh::client::Websocket2::stop() noexcept {
	// Запрещаем чтение данных из буфера
	this->_reading = false;
	// Выполняем очистку буфера данных
	this->_buffer.clear();
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем сброс параметров запроса
		this->flush();
		// Очищаем адрес сервера
		this->_scheme.url.clear();
		// Если завершить работу разрешено
		if(this->_complete && (this->_core != nullptr))
			// Завершаем работу
			const_cast <client::core_t *> (this->_core)->stop();
		// Если завершать работу запрещено, просто отключаемся
		else {
			/**
			 * Если установлено постоянное подключение
			 * нам нужно заблокировать автоматический реконнект.
			 */
			const bool alive = this->_scheme.alive;
			// Выполняем отключение флага постоянного подключения
			this->_scheme.alive = false;
			// Выполняем отключение клиента
			const_cast <client::core_t *> (this->_core)->close(this->_bid);
			// Восстанавливаем предыдущее значение флага
			this->_scheme.alive = alive;
		}
	}
}
/**
 * start Метод запуска клиента
 */
void awh::client::Websocket2::start() noexcept {
	// Разрешаем чтение данных из буфера
	this->_reading = true;
	// Выполняем очистку буфера данных
	this->_buffer.clear();
	// Если адрес URL запроса передан
	if((this->_core != nullptr) && !this->_freeze && !this->_scheme.url.empty()){
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
 * waitPong Метод установки времени ожидания ответа WebSocket-сервера
 * @param time время ожидания в миллисекундах
 */
void awh::client::Websocket2::waitPong(const time_t time) noexcept {
	// Если время ожидания передано
	if(time > 0){
		// Выполняем установку времени ожидания
		this->_waitPong = time;
		// Выполняем установку времени ожидания для WebSocket/1.1
		this->_ws1.waitPong(time);
	}
}
/**
 * callback Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::client::Websocket2::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем добавление функций обратного вызова в основноной модуль
	web2_t::callbacks(callbacks);
	// Выполняем установку функции обратного вызова для выполнения редиректа с одного потока на другой (необходим для совместимости с HTTP/2)
	this->_callbacks.set("redirect", callbacks);
	// Выполняем установку функции обратного вызова на событие получения ошибок Websocket
	this->_callbacks.set("errorWebsocket", callbacks);
	// Выполняем установку функции обратного вызова на событие получения сообщений Websocket
	this->_callbacks.set("messageWebsocket", callbacks);
	{
		// Создаём локальный контейнер функций обратного вызова
		fn_t callbacks(this->_log);
		// Выполняем установку функции обратного вызова для вывода бинарных данных в сыром виде полученных с сервера
		callbacks.set("raw", this->_callbacks);
		// Выполняем установку функции обратного вызова при завершении запроса
		callbacks.set("end", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие получения ошибки
		callbacks.set("error", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного тела данных с сервера
		callbacks.set("entity", this->_callbacks);
		// Выполняем установку функции обратного вызова активности потока
		callbacks.set("stream", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного чанка бинарных данных с сервера
		callbacks.set("chunks", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученного заголовка с сервера
		callbacks.set("header", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода полученных заголовков с сервера
		callbacks.set("headers", this->_callbacks);
		// Выполняем установку функции обратного вызова для вывода ответа сервера на ранее выполненный запрос
		callbacks.set("response", this->_callbacks);
		// Выполняем установку функции обратного вызова для перехвата полученных чанков
		callbacks.set("chunking", this->_callbacks);
		// Выполняем установку функции завершения выполнения запроса
		callbacks.set("complete", this->_callbacks);
		// Выполняем установку функции обратного вызова при выполнении рукопожатия
		callbacks.set("handshake", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие получения ошибок
		callbacks.set("errorWebsocket", this->_callbacks);
		// Выполняем установку функции обратного вызова на событие получения сообщений
		callbacks.set("messageWebsocket", this->_callbacks);
		// Если функции обратного вызова установлены
		if(!callbacks.empty())
			// Выполняем установку функций обратного вызова для Websocket-клиента
			this->_ws1.callbacks(std::move(callbacks));
	}
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::client::Websocket2::subprotocol(const string & subprotocol) noexcept {
	// Если сабпротокол передан
	if(!subprotocol.empty()){
		// Устанавливаем поддерживаемый сабпротокол для Websocket-клиента
		this->_ws1.subprotocol(subprotocol);
		// Устанавливаем поддерживаемый сабпротокол для HTTP-клиента
		this->_http.subprotocol(subprotocol);
	}
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @return список выбранных сабпротоколов
 */
const set <string> & awh::client::Websocket2::subprotocols() const noexcept {
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2)
		// Выводим список выбранных сабпротоколов
		return this->_ws1.subprotocols();
	// Если переключение протокола на HTTP/2 выполнено
	else
		// Выводим список выбранных сабпротоколов
		return this->_http.subprotocols();
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::client::Websocket2::subprotocols(const set <string> & subprotocols) noexcept {
	// Если список поддерживаемых сабпротоколов получен
	if(!subprotocols.empty()){
		// Устанавливаем список поддерживаемых сабсабпротоколов для Websocket-клиента
		this->_ws1.subprotocols(subprotocols);
		// Устанавливаем список поддерживаемых сабсабпротоколов для HTTP-клиента
		this->_http.subprotocols(subprotocols);
	}
}
/**
 * extensions Метод извлечения списка расширений
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::client::Websocket2::extensions() const noexcept {
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
void awh::client::Websocket2::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка доступных расширений для Websocket-клиента
	this->_ws1.extensions(extensions);
	// Выполняем установку списка доступных расширений для HTTP-клиента
	this->_http.extensions(extensions);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::Websocket2::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка для Websocket-клиента
	this->_ws1.chunk(size);
	// Устанавливаем размер чанка для HTTP-клиента
	this->_http.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::client::Websocket2::segmentSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0){
		// Если максимальный размер фрейма больше самого максимального значения
		if(static_cast <uint32_t> (size) > http2_t::MAX_FRAME_SIZE_MAX)
			// Выполняем уменьшение размера сегмента
			const_cast <size_t &> (size) = static_cast <size_t> (http2_t::MAX_FRAME_SIZE_MAX);
		// Если максимальный размер фрейма меньше самого минимального значения
		else if(static_cast <uint32_t> (size) < http2_t::MAX_FRAME_SIZE_MIN)
			// Выполняем уменьшение размера сегмента
			const_cast <size_t &> (size) = static_cast <size_t> (http2_t::MAX_FRAME_SIZE_MIN);
		// Устанавливаем размер сегментов фрейма
		this->_frame.size = size;
		// Устанавливаем размер сегментов фрейма для Websocket-клиента
		this->_ws1.segmentSize(size);
		// Выполняем извлечение максимального размера фрейма
		auto i = this->_settings.find(http2_t::settings_t::FRAME_SIZE);
		// Если размер максимального размера фрейма уже установлен
		if(i != this->_settings.end())
			// Выполняем замену максимального размера фрейма
			i->second = static_cast <uint32_t> (this->_frame.size);
		// Если размер максимального размера фрейма ещё не установлен
		else this->_settings.emplace(http2_t::settings_t::FRAME_SIZE, static_cast <uint32_t> (this->_frame.size));
	}
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Websocket2::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаги настроек модуля для Websocket-клиента
	this->_ws1.mode(flags);
	// Устанавливаем флаг разрешающий вывод информационных сообщений
	this->_verb = (flags.find(flag_t::NOT_INFO) == flags.end());
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_client.takeover = (flags.find(flag_t::TAKEOVER_CLIENT) != flags.end());
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_server.takeover = (flags.find(flag_t::TAKEOVER_SERVER) != flags.end());
	// Выполняем установку флагов настроек модуля
	web2_t::mode(flags);
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::client::Websocket2::core(const client::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем передачу настроек сетевого ядра в родительский модуль
		web_t::core(core);
		// Если многопоточность активированна
		if(this->_thr.is() || this->_ws1._thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <client::core_t *> (this->_core)->easily(true);
		// Устанавливаем функцию записи данных
		const_cast <client::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&ws2_t::writeCallback, this, _1, _2, _3, _4));
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_thr.is() || this->_ws1._thr.is()){
			// Выполняем завершение всех активных потоков
			this->_thr.stop();
			// Выполняем завершение всех активных потоков
			this->_ws1._thr.stop();
			// Снимаем режим простого чтения базы событий
			const_cast <client::core_t *> (this->_core)->easily(false);
		}
		// Выполняем передачу настроек сетевого ядра в родительский модуль
		web_t::core(core);
	}
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Websocket2::user(const string & login, const string & password) noexcept {
	// Устанавливаем логин и пароль пользователя для Websocket-клиента
	this->_ws1.user(login, password);
	// Устанавливаем логин и пароль пользователя для HTTP-клиента
	this->_http.user(login, password);
}
/**
 * setHeaders Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::client::Websocket2::setHeaders(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку HTTP-заголовков для отправки на сервер
	this->_headers = headers;
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::client::Websocket2::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента у родительского класса
		web2_t::userAgent(userAgent);
		// Устанавливаем пользовательского агента для Websocket-клиента
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
void awh::client::Websocket2::ident(const string & id, const string & name, const string & ver) noexcept {
	// Если данные сервиса переданы
	if(!id.empty() && !name.empty() && !ver.empty()){
		// Выполняем установку данных сервиса у родительского класса
		web2_t::ident(id, name, ver);
		// Устанавливаем данные сервиса для Websocket-клиента
		this->_ws1.ident(id, name, ver);
		// Устанавливаем данные сервиса для HTTP-клиента
		this->_http.ident(id, name, ver);
	}
}
/**
 * multiThreads Метод активации многопоточности
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::client::Websocket2::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Выполняем установку количества активных ядер
		this->_threads = count;
		// Если многопоточность ещё не активированна
		if(!this->_thr.is())
			// Выполняем инициализацию пула потоков
			this->_thr.init(this->_threads);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->_thr.stop();
			// Выполняем инициализацию нового тредпула
			this->_thr.init(this->_threads);
		}
		// Если сетевое ядро установлено
		if(this->_core != nullptr)
			// Устанавливаем простое чтение базы событий
			const_cast <client::core_t *> (this->_core)->easily(true);
	// Выполняем завершение всех потоков
	} else this->_thr.stop();
}
/**
 * proxy Метод активации/деактивации прокси-склиента
 * @param work флаг активации/деактивации прокси-клиента
 */
void awh::client::Websocket2::proxy(const client::scheme_t::work_t work) noexcept {
	// Устанавливаем флаг активации/деактивации прокси-клиента
	web2_t::proxy(work);
	// Устанавливаем флаг активации/деактивации прокси-клиента для Websocket-клиента
	this->_ws1.proxy(work);
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::Websocket2::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку параметры прокси-сервера
	web2_t::proxy(uri, family);
	// Выполняем установку параметры прокси-сервера для Websocket-клиента
	this->_ws1.proxy(uri, family);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Websocket2::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем параметры авторизации для Websocket-клиента
	this->_ws1.authType(type, hash);
	// Устанавливаем параметры авторизации для HTTP-клиента
	this->_http.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Websocket2::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации на проксе-сервере
	web2_t::authTypeProxy(type, hash);
	// Устанавливаем тип авторизации на проксе-сервере для Websocket-клиента
	this->_ws1.authTypeProxy(type, hash);
}
/**
 * crypted Метод получения флага шифрования
 * @return результат проверки
 */
bool awh::client::Websocket2::crypted() const noexcept {
	// Если переключение протокола на HTTP/2 не выполнено
	if(this->_proto != engine_t::proto_t::HTTP2)
		// Выполняем получение флага шифрования
		return this->_ws1.crypted();
	// Если переключение протокола на HTTP/2 выполнено
	else
		// Выполняем получение флага шифрования
		return this->_http.crypted();
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::client::Websocket2::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования в родительском модуле
	web2_t::encryption(mode);
	// Устанавливаем флаг шифрования для Websocket-клиента
	this->_ws1.encryption(mode);
	// Устанавливаем флаг шифрования для HTTP-клиента
	this->_http.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::Websocket2::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования в родительском модуле
	web2_t::encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для Websocket-клиента
	this->_ws1.encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для HTTP-клиента
	this->_http.encryption(pass, salt, cipher);
}
/**
 * Websocket2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Websocket2::Websocket2(const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(fmk, log), _sid(-1), _rid(0), _verb(true), _close(false),
 _shake(false), _freeze(false), _crypted(false), _inflate(false),
 _point(0), _threads(0), _waitPong(PING_INTERVAL * 5), _ws1(fmk, log),
 _http(fmk, log), _hash(log), _frame(fmk, log), _resultCallback(log),
 _proto(engine_t::proto_t::HTTP1_1), _compressor(awh::http_t::compressor_t::NONE) {
	// Если размер фрейма не установлен
	if(this->_frame.size == 0)
		// Устанавливаем размер сегментов фрейма
		this->_frame.size = static_cast <size_t> (http2_t::MAX_FRAME_SIZE_MIN);
	// Выполняем установку перехвата событий получения статуса овтета сервера для Websocket-клиента
	this->_ws1.callback <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", std::bind(&ws2_t::answer, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова на событие получения ошибок
	this->_http.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", std::bind(&ws2_t::errors, this, _1, _2, _3, _4));
}
/**
 * Websocket2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Websocket2::Websocket2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web2_t(core, fmk, log), _sid(-1), _rid(0), _verb(true), _close(false),
 _shake(false), _freeze(false), _crypted(false), _inflate(false),
 _point(0), _threads(0), _waitPong(PING_INTERVAL * 5), _ws1(fmk, log),
 _http(fmk, log), _hash(log), _frame(fmk, log), _resultCallback(log),
 _proto(engine_t::proto_t::HTTP1_1), _compressor(awh::http_t::compressor_t::NONE) {
	// Если размер фрейма не установлен
	if(this->_frame.size == 0)
		// Устанавливаем размер сегментов фрейма
		this->_frame.size = static_cast <size_t> (http2_t::MAX_FRAME_SIZE_MIN);
	// Выполняем установку перехвата событий получения статуса овтета сервера для Websocket-клиента
	this->_ws1.callback <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", std::bind(&ws2_t::answer, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова на событие получения ошибок
	this->_http.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", std::bind(&ws2_t::errors, this, _1, _2, _3, _4));
	// Устанавливаем функцию записи данных
	const_cast <client::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&ws2_t::writeCallback, this, _1, _2, _3, _4));
}
/**
 * ~Websocket2 Деструктор
 */
awh::client::Websocket2::~Websocket2() noexcept {
	// Если многопоточность активированна
	if(this->_thr.is())
		// Выполняем завершение всех активных потоков
		this->_thr.stop();
	// Если многопоточность активированна у Websocket/1.1 клиента
	if(this->_ws1._thr.is())
		// Выполняем завершение всех активных потоков
		this->_ws1._thr.stop();
}
