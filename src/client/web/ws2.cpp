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
void awh::client::WebSocket2::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ}, event_t::CONNECT)){
		// Запоминаем идентификатор адъютанта
		this->_aid = aid;
		// Выполняем сброс параметров запроса
		this->flush();
		// Выполняем сброс состояния HTTP парсера
		this->_http.reset();
		// Выполняем очистку параметров HTTP запроса
		this->_http.clear();
		// Устанавливаем метод сжатия
		this->_http.compress(this->_compress);
		// Разрешаем перехватывать контекст для клиента
		this->_http.clientTakeover(this->_client.takeOver);
		// Разрешаем перехватывать контекст для сервера
		this->_http.serverTakeover(this->_server.takeOver);
		// Разрешаем перехватывать контекст компрессии
		this->_hash.takeoverCompress(this->_client.takeOver);
		// Разрешаем перехватывать контекст декомпрессии
		this->_hash.takeoverDecompress(this->_server.takeOver);
		// Выполняем очистку функций обратного вызова
		this->_resultCallback.clear();
		// Если протокол подключения является HTTP/2
		if(!this->_upgraded && (dynamic_cast <client::core_t *> (core)->proto(aid) == engine_t::proto_t::HTTP2)){
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выполняем установку функции для вывода отладочной информации
				nghttp2_set_debug_vprintf_callback(&ws2_t::debug);
			#endif
			// Создаём объект функций обратного вызова
			nghttp2_session_callbacks * callbacks;
			// Выполняем инициализацию сессию функций обратного вызова
			nghttp2_session_callbacks_new(&callbacks);
			// Выполняем установку функции обратного вызова при подготовки данных для отправки на сервер
			nghttp2_session_callbacks_set_send_callback(callbacks, &ws2_t::onSend);
			// Выполняем установку функции обратного вызова при получении заголовка HTTP/2
			nghttp2_session_callbacks_set_on_header_callback(callbacks, &ws2_t::onHeader);
			// Выполняем установку функции обратного вызова при получении фрейма заголовков HTTP/2 с сервера
			nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, &ws2_t::onFrame);
			// Выполняем установку функции обратного вызова закрытия подключения с сервером HTTP/2
			nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, &ws2_t::onClose);
			// Выполняем установку функции обратного вызова при получении чанка с сервера HTTP/2
			nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, &ws2_t::onChunk);
			// Выполняем установку функции обратного вызова начала получения фрейма заголовков HTTP/2
			nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, &ws2_t::onBeginHeaders);
			// Выполняем подключение котнекста сессии HTTP/2
			nghttp2_session_client_new(&this->_session, callbacks, this);
			// Выполняем удаление объекта функций обратного вызова
			nghttp2_session_callbacks_del(callbacks);
			// Создаём параметры сессии подключения с HTTP/2 сервером
			const vector <nghttp2_settings_entry> iv = {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, this->_attempts + 1}};
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
		// Если активирован режим работы с HTTP/2 протоколом
		if(this->_upgraded){
			// Список заголовков для запроса
			vector <nghttp2_nv> nva;
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим заголовок запроса
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
				// Получаем бинарные данные REST запроса
				const auto & buffer = this->_http.request(this->_scheme.url);
				// Если бинарные данные запроса получены
				if(!buffer.empty())
					// Выводим параметры запроса
					cout << string(buffer.begin(), buffer.end()) << endl << endl;
			#endif
			// Выполняем запрос на получение заголовков
			const auto & headers = this->_http.request2(this->_scheme.url);
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
			// Выполняем запрос на удалённый сервер
			const int32_t id = nghttp2_submit_request(this->_session, nullptr, nva.data(), nva.size(), nullptr, this);
			// Если запрос не получилось отправить
			if(id < 0){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(id));
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
					// Выходим из функции
					return;
				}
			}
		// Если активирован режим работы с HTTP/1.1 протоколом
		} else {
			/** +++++++++++++++++++++++++ **/
		}
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket2::disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Получаем параметры запроса
	const auto & query = this->_http.query();
	// Если нужно произвести запрос заново
	if(!this->_stopped && ((query.code == 301) || (query.code == 308) || (query.code == 401) || (query.code == 407))){
		// Если статус ответа требует произвести авторизацию или заголовок перенаправления указан
		if((query.code == 401) || (query.code == 407) || this->_http.isHeader("location")){
			// Получаем новый адрес запроса
			const uri_t::url_t & url = this->_http.getUrl();
			// Если адрес запроса получен
			if(!url.empty()){
				// Увеличиваем количество попыток
				this->_attempt++;
				// Заменяем адрес запроса в схеме клиента
				this->_scheme.url = std::forward <const uri_t::url_t> (url);
				// Выполняем очистку оставшихся данных
				this->_buffer.clear();
				// Выполняем очистку оставшихся фрагментов
				this->_fragmes.clear();
				// Выполняем установку следующего экшена на открытие подключения
				this->open();
				// Завершаем работу
				return;
			}
		}
	}
	// Выполняем удаление списка воркеров
	this->_workers.clear();
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
void awh::client::WebSocket2::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Если протокол подключения является HTTP/2
		if(core->proto(aid) == engine_t::proto_t::HTTP2){
			// Если подключение закрыто
			if(this->_close){
				// Принудительно выполняем отключение лкиента
				reinterpret_cast <client::core_t *> (core)->close(aid);
				// Выходим из функции
				return;
			}
			// Если получение данных не разрешено
			if(!this->_allow.receive)
				// Выходим из функции
				return;
			// Выполняем извлечение полученного чанка данных из сокета
			ssize_t bytes = nghttp2_session_mem_recv(this->_session, (const uint8_t *) buffer, size);
			// Если данные не прочитаны, выводим ошибку и выходим
			if(bytes < 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
				// Выходим из функции
				return;
			}
			// Фиксируем полученный результат
			if((bytes = nghttp2_session_send(this->_session)) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(static_cast <int> (bytes)));
				// Выходим из функции
				return;
			}
		// Если активирован режим работы с HTTP/1.1 протоколом
		} else {
			/** +++++++++++++++++++++++++ **/
		}
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
void awh::client::WebSocket2::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Если необходимо выполнить закрыть подключение
		if(!this->_close && this->_stopped){
			// Устанавливаем флаг закрытия подключения
			this->_close = !this->_close;
			// Принудительно выполняем отключение лкиента
			dynamic_cast <client::core_t *> (core)->close(aid);
		}
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket2::persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
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
/**
 * receivedFrame Метод обратного вызова при получении фрейма заголовков HTTP/2 с сервера
 * @param frame   объект фрейма заголовков HTTP/2
 * @return        статус полученных данных
 */
int awh::client::WebSocket2::receivedFrame(const nghttp2_frame * frame) noexcept {

}
/**
 * receivedChunk Метод обратного вызова при получении чанка с сервера HTTP/2
 * @param sid    идентификатор сессии HTTP/2
 * @param buffer буфер данных который содержит полученный чанк
 * @param size   размер полученного буфера данных чанка
 * @return       статус полученных данных
 */
int awh::client::WebSocket2::receivedChunk(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept {

}
/**
 * receivedBeginHeaders Метод начала получения фрейма заголовков HTTP/2
 * @param sid идентификатор сессии HTTP/2
 * @return    статус полученных данных
 */
int awh::client::WebSocket2::receivedBeginHeaders(const int32_t sid) noexcept {

}
/**
 * receivedHeader Метод обратного вызова при получении заголовка HTTP/2
 * @param sid идентификатор сессии HTTP/2
 * @param key данные ключа заголовка
 * @param val данные значения заголовка
 * @return    статус полученных данных
 */
int awh::client::WebSocket2::receivedHeader(const int32_t sid, const string & key, const string & val) noexcept {

}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::client::WebSocket2::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {

}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket2::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {

}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket2::persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {

}
/**
 * response Метод получения ответа сервера
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 */
void awh::client::WebSocket2::response(const u_int code, const string & message) noexcept {

}
/**
 * header Метод получения заголовка
 * @param key   ключ заголовка
 * @param value значение заголовка
 */
void awh::client::WebSocket2::header(const string & key, const string & value) noexcept {

}
/**
 * headers Метод получения заголовков
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 * @param headers заголовки ответа сервера
 */
void awh::client::WebSocket2::headers(const u_int code, const string & message, const unordered_multimap <string, string> & headers) noexcept {

}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::WebSocket2::flush() noexcept {

}
/**
 * ping Метод проверки доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket2::ping(const string & message) noexcept {

}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket2::pong(const string & message) noexcept {

}
/**
 * prepare Метод выполнения препарирования полученных данных
 * @param id   идентификатор запроса
 * @param aid  идентификатор адъютанта
 * @param core объект сетевого ядра
 * @return     результат препарирования
 */
awh::client::Web::status_t awh::client::WebSocket2::prepare(const int32_t id, const size_t aid, client::core_t * core) noexcept {

}
/**
 * error Метод вывода сообщений об ошибках работы клиента
 * @param message сообщение с описанием ошибки
 */
void awh::client::WebSocket2::error(const ws::mess_t & message) const noexcept {

}
/**
 * extraction Метод извлечения полученных данных
 * @param buffer данные в чистом виде полученные с сервера
 * @param utf8   данные передаются в текстовом виде
 */
void awh::client::WebSocket2::extraction(const vector <char> & buffer, const bool utf8) noexcept {

}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::WebSocket2::sendError(const ws::mess_t & mess) noexcept {

}
/**
 * send Метод отправки сообщения на сервер
 * @param message буфер сообщения в бинарном виде
 * @param size    размер сообщения в байтах
 * @param utf8    данные передаются в текстовом виде
 */
void awh::client::WebSocket2::send(const char * message, const size_t size, const bool utf8) noexcept {

}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::WebSocket2::pause() noexcept {

}
/**
 * stop Метод остановки клиента
 */
void awh::client::WebSocket2::stop() noexcept {

}
/**
 * start Метод запуска клиента
 */
void awh::client::WebSocket2::start() noexcept {

}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const u_int, const string &)> callback) noexcept {

}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const vector <char> &, const bool)> callback) noexcept {

}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept {

}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const int32_t, const u_int, const string &)> callback) noexcept {

}
/**
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const int32_t, const string &, const string &)> callback) noexcept {

}
/**
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket2::on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {

}
/**
 * sub Метод получения выбранного сабпротокола
 * @return выбранный сабпротокол
 */
const string & awh::client::WebSocket2::sub() const noexcept {

}
/**
 * sub Метод установки подпротокола поддерживаемого сервером
 * @param sub подпротокол для установки
 */
void awh::client::WebSocket2::sub(const string & sub) noexcept {

}
/**
 * subs Метод установки списка подпротоколов поддерживаемых сервером
 * @param subs подпротоколы для установки
 */
void awh::client::WebSocket2::subs(const vector <string> & subs) noexcept {

}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::WebSocket2::chunk(const size_t size) noexcept {

}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::client::WebSocket2::segmentSize(const size_t size) noexcept {

}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::WebSocket2::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_noinfo = (flags.count(flag_t::NOT_INFO) > 0);
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_unbind = (flags.count(flag_t::NOT_STOP) == 0);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.count(flag_t::ALIVE) > 0);
	// Устанавливаем флаг разрешающий выполнять редиректы
	this->_redirects = (flags.count(flag_t::REDIRECTS) > 0);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_client.takeOver = (flags.count(flag_t::TAKEOVER_CLIENT) > 0);
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_server.takeOver = (flags.count(flag_t::TAKEOVER_SERVER) > 0);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->_core)->noInfo(flags.count(flag_t::NOT_INFO) > 0);
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->_core)->verifySSL(flags.count(flag_t::VERIFY_SSL) > 0);
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket2::core(const client::core_t * core) noexcept {

}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::WebSocket2::user(const string & login, const string & password) noexcept {

}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::WebSocket2::userAgent(const string & userAgent) noexcept {

}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::WebSocket2::serv(const string & id, const string & name, const string & ver) noexcept {

}
/**
 * multiThreads Метод активации многопоточности
 * @param threads количество потоков для активации
 * @param mode    флаг активации/деактивации мультипоточности
 */
void awh::client::WebSocket2::multiThreads(const size_t threads, const bool mode) noexcept {

}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::WebSocket2::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {

}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::WebSocket2::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {

}
/**
 * WebSocket2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::WebSocket2::WebSocket2(const fmk_t * fmk, const log_t * log) noexcept {

}
/**
 * WebSocket2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::WebSocket2::WebSocket2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept {

}
/**
 * ~WebSocket2 Деструктор
 */
awh::client::WebSocket2::~WebSocket2() noexcept {

}
