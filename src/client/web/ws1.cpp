/**
 * @file: ws1.cpp
 * @date: 2023-09-13
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
#include <client/web/ws1.hpp>

/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket1::connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
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
		// Создаём объек запроса
		awh::web_t::req_t query(awh::web_t::method_t::GET, this->_scheme.url);
		// Если метод CONNECT запрещён для прокси-сервера
		if(!this->_proxy.connect){
			// Выполняем извлечение заголовка авторизации на прокси-сервера
			const string & header = this->_scheme.proxy.http.getAuth(http_t::process_t::REQUEST, query);
			// Если заголовок авторизации получен
			if(!header.empty())
				// Выполняем установки заголовка авторизации на прокси-сервере
				this->_http.header("Proxy-Authorization", header);
		}
		// Получаем бинарные данные REST запроса		
		const auto & buffer = this->_http.process(http_t::process_t::REQUEST, std::move(query));
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
			// Выполняем отправку сообщения на сервер
			dynamic_cast <client::core_t *> (core)->write(buffer.data(), buffer.size(), aid);
		}
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выводим функцию обратного вызова
			this->_callback.call <const mode_t> ("active", mode_t::CONNECT);
		// Если установлена функция отлова завершения запроса
		if(this->_callback.is("end"))
			// Выводим функцию обратного вызова
			this->_callback.call <const int32_t, const direct_t> ("end", 1, direct_t::SEND);
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket1::disconnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Выполняем редирект, если редирект выполнен
	if(this->redirect())
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
	// Если функция обратного вызова при подключении/отключении установлена
	if(this->_callback.is("active"))
		// Выводим функцию обратного вызова
		this->_callback.call <const mode_t> ("active", mode_t::DISCONNECT);
	// Активируем асинхронный режим работы
	dynamic_cast <client::core_t *> (core)->mode(client::core_t::mode_t::SYNC);
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::client::WebSocket1::readCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Если подключение закрыто
		if(this->_close){
			// Принудительно выполняем отключение лкиента
			dynamic_cast <client::core_t *> (core)->close(aid);
			// Выходим из функции
			return;
		}
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::CONNECT}, event_t::READ)){
			// Если рукопожатие не выполнено
			if(!(this->_shake = reinterpret_cast <http_t &> (this->_http).isHandshake())){
				// Добавляем полученные данные в буфер
				this->_buffer.insert(this->_buffer.end(), buffer, buffer + size);
				// Выполняем парсинг полученных данных
				const size_t bytes = this->_http.parse(this->_buffer.data(), this->_buffer.size());
				// Если все данные получены
				if(this->_http.isEnd()){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						{
							// Получаем объект работы с HTTP-запросами
							const http_t & http = reinterpret_cast <http_t &> (this->_http);
							// Получаем данные ответа
							const auto & response = http.process(http_t::process_t::RESPONSE, http.response());
							// Если параметры ответа получены
							if(!response.empty()){
								// Выводим заголовок ответа
								cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
								// Выводим параметры ответа
								cout << string(response.begin(), response.end()) << endl;
								// Если тело ответа существует
								if(!http.body().empty())
									// Выводим сообщение о выводе чанка тела
									cout << this->_fmk->format("<body %u>", http.body().size()) << endl << endl;
								// Иначе устанавливаем перенос строки
								else cout << endl;
							}
						}
					#endif
					// Выполняем препарирование полученных данных
					switch(static_cast <uint8_t> (this->prepare(1, aid, dynamic_cast <client::core_t *> (core)))){
						// Если необходимо выполнить остановку обработки
						case static_cast <uint8_t> (status_t::STOP): {
							// Выполняем сброс количества попыток
							this->_attempt = 0;
							// Завершаем работу
							dynamic_cast <client::core_t *> (core)->close(aid);
							// Если установлена функция отлова завершения запроса
							if(this->_callback.is("end"))
								// Выводим функцию обратного вызова
								this->_callback.call <const int32_t, const direct_t> ("end", 1, direct_t::RECV);
						} break;
						// Если необходимо выполнить переход к следующему этапу обработки
						case static_cast <uint8_t> (status_t::NEXT): {
							// Есла данных передано больше чем обработано
							if(this->_buffer.size() > bytes)
								// Удаляем количество обработанных байт
								this->_buffer.assign(this->_buffer.begin() + bytes, this->_buffer.end());
								// vector <decltype(this->_buffer)::value_type> (this->_buffer.begin() + bytes, this->_buffer.end()).swap(this->_buffer);
							// Если данных в буфере больше нет, очищаем буфер собранных данных
							else this->_buffer.clear();
						} break;
					}
					// Если функция обратного вызова активности потока установлена
					if(this->_resultCallback.is("stream"))
						// Выводим функцию обратного вызова
						this->_resultCallback.bind  <const int32_t, const mode_t> ("stream");
					// Если функция обратного вызова установлена, выводим сообщение
					if(this->_resultCallback.is("entity"))
						// Выполняем функцию обратного вызова дисконнекта
						this->_resultCallback.bind <const int32_t, const u_int, const string, const vector <char>> ("entity");
					// Выполняем очистку функций обратного вызова
					this->_resultCallback.clear();
				}
				// Завершаем работу
				return;
			// Если рукопожатие выполнено
			} else if(this->_allow.receive) {
				// Добавляем полученные данные в буфер
				this->_buffer.insert(this->_buffer.end(), buffer, buffer + size);
				// Выполняем препарирование полученных данных
				switch(static_cast <uint8_t> (this->prepare(1, aid, dynamic_cast <client::core_t *> (core)))){
					// Если необходимо выполнить остановку обработки
					case static_cast <uint8_t> (status_t::STOP):
						// Выполняем завершение работы
						goto End;
					// Если необходимо выполнить переход к следующему этапу обработки
					case static_cast <uint8_t> (status_t::NEXT):
						// Выполняем реконнект
						goto Reconnect;
				}
			}
			// Устанавливаем метку реконнекта
			Reconnect:
			// Выполняем отправку сообщения об ошибке
			this->sendError(this->_mess);
			// Устанавливаем метку завершения работы
			End:
			// Если функция обратного вызова активности потока установлена
			if(this->_resultCallback.is("stream"))
				// Выводим функцию обратного вызова
				this->_resultCallback.bind  <const int32_t, const mode_t> ("stream");
			// Выполняем очистку функций обратного вызова
			this->_resultCallback.clear();
			// Если установлена функция отлова завершения запроса
			if(this->_callback.is("end"))
				// Выводим функцию обратного вызова
				this->_callback.call <const int32_t, const direct_t> ("end", 1, direct_t::RECV);
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
void awh::client::WebSocket1::writeCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Если необходимо выполнить закрыть подключение
		if(!this->_close && this->_stopped){
			// Устанавливаем флаг закрытия подключения
			this->_close = !this->_close;
			// Принудительно выполняем отключение лкиента
			dynamic_cast <client::core_t *> (core)->close(aid);
			// Если установлена функция отлова завершения запроса
			if(this->_callback.is("end"))
				// Выводим функцию обратного вызова
				this->_callback.call <const int32_t, const direct_t> ("end", sid, direct_t::SEND);
		}
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket1::persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
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
 * redirect Метод выполнения редиректа если требуется
 * @return результат выполнения редиректа
 */
bool awh::client::WebSocket1::redirect() noexcept {
	// Результат работы функции
	bool result = false;
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
	// Выводим результат
	return result;
}
/**
 * response Метод получения ответа сервера
 * @param aid     идентификатор адъютанта
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 */
void awh::client::WebSocket1::response(const uint64_t aid, const u_int code, const string & message) noexcept {
	// Выполняем неиспользуемую переменную
	(void) aid;
	// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
	if(this->_callback.is("response"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const u_int, const string &> ("response", 1, code, message);
}
/**
 * header Метод получения заголовка
 * @param aid   идентификатор адъютанта
 * @param key   ключ заголовка
 * @param value значение заголовка
 */
void awh::client::WebSocket1::header(const uint64_t aid, const string & key, const string & value) noexcept {
	// Выполняем неиспользуемую переменную
	(void) aid;
	// Если функция обратного вызова на полученного заголовка с сервера установлена
	if(this->_callback.is("header"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const string &, const string &> ("header", 1, key, value);
}
/**
 * headers Метод получения заголовков
 * @param aid     идентификатор адъютанта
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 * @param headers заголовки ответа сервера
 */
void awh::client::WebSocket1::headers(const uint64_t aid, const u_int code, const string & message, const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем неиспользуемую переменную
	(void) aid;
	// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
	if(this->_callback.is("headers"))
		// Выводим функцию обратного вызова
		this->_callback.call <const int32_t, const u_int, const string &, const unordered_multimap <string, string> &> ("headers", 1, code, message, this->_http.headers());
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::WebSocket1::flush() noexcept {
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
/**
 * ping Метод проверки доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket1::ping(const string & message) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_allow.send){
		// Если рукопожатие выполнено
		if(this->_http.isHandshake(http_t::process_t::RESPONSE) && (this->_aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->_frame.methods.ping(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				const_cast <client::core_t *> (this->_core)->write(buffer.data(), buffer.size(), this->_aid);
		}
	}
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param message сообщение для отправки
 */
void awh::client::WebSocket1::pong(const string & message) noexcept {
	// Если подключение выполнено
	if(this->_core->working() && this->_allow.send){
		// Если рукопожатие выполнено
		if(this->_http.isHandshake(http_t::process_t::RESPONSE) && (this->_aid > 0)){
			// Создаём буфер для отправки
			const auto & buffer = this->_frame.methods.pong(message, true);
			// Если бинарный буфер получен
			if(!buffer.empty())
				// Выполняем отправку сообщения на сервер
				const_cast <client::core_t *> (this->_core)->write(buffer.data(), buffer.size(), this->_aid);
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
awh::client::Web::status_t awh::client::WebSocket1::prepare(const int32_t sid, const uint64_t aid, client::core_t * core) noexcept {
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
					// Если функция обратного вызова на на вывод ошибок установлена
					if((response.code == 401) && this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const log_t::flag_t, const http::error_t, const string &> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "authorization failed");
					// Получаем новый адрес запроса
					const uri_t::url_t & url = this->_http.getUrl();
					// Если URL-адрес запроса получен
					if(!url.empty()){
						// Выполняем проверку соответствие протоколов
						const bool schema = (this->_fmk->compare(url.schema, this->_scheme.url.schema));
						// Если соединение является постоянным
						if(schema && this->_http.isAlive()){
							// Выполняем сброс параметров запроса
							this->flush();
							// Увеличиваем количество попыток
							this->_attempt++;
							// Устанавливаем новый адрес запроса
							this->_uri.combine(this->_scheme.url, url);
							// Если функция обратного вызова активности потока установлена
							if(this->_callback.is("stream"))
								// Устанавливаем полученную функцию обратного вызова
								this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
							// Выполняем попытку повторить запрос
							this->connectCallback(aid, sid, core);
						// Если подключение не постоянное, то завершаем работу
						} else dynamic_cast <client::core_t *> (core)->close(aid);
					// Если URL-адрес запроса не получен
					} else {
						// Если соединение является постоянным
						if(this->_http.isAlive()){
							// Выполняем сброс параметров запроса
							this->flush();
							// Увеличиваем количество попыток
							this->_attempt++;
							// Если функция обратного вызова активности потока установлена
							if(this->_callback.is("stream"))
								// Устанавливаем полученную функцию обратного вызова
								this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
							// Выполняем попытку повторить запрос
							this->connectCallback(aid, sid, core);
						// Если подключение не постоянное, то завершаем работу
						} else dynamic_cast <client::core_t *> (core)->close(aid);
					}
					// Завершаем работу
					return status_t::SKIP;
				}
				// Создаём сообщение
				this->_mess = ws::mess_t(response.code, this->_http.message(response.code));
				// Выводим сообщение
				this->error(this->_mess);
				// Если функция обратного вызова активности потока установлена
				if(this->_callback.is("stream"))
					// Устанавливаем полученную функцию обратного вызова
					this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
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
					// Активируем асинхронный режим работы
					dynamic_cast <client::core_t *> (core)->mode(client::core_t::mode_t::ASYNC);
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
						// Устанавливаем полученную функцию обратного вызова
						this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::OPEN);
					// Если функция обратного вызова на получение удачного ответа установлена
					if(this->_callback.is("handshake"))
						// Выполняем функцию обратного вызова
						this->_callback.call <const int32_t, const agent_t> ("handshake", sid, agent_t::WEBSOCKET);
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
					// Если функция обратного вызова активности потока установлена
					if(this->_callback.is("stream"))
						// Устанавливаем полученную функцию обратного вызова
						this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
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
				// Если функция обратного вызова активности потока установлена
				if(this->_callback.is("stream"))
					// Устанавливаем полученную функцию обратного вызова
					this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
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
					// Если функция обратного вызова активности потока установлена
					if(this->_callback.is("stream"))
						// Устанавливаем полученную функцию обратного вызова
						this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
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
					// Если функция обратного вызова активности потока установлена
					if(this->_callback.is("stream"))
						// Устанавливаем полученную функцию обратного вызова
						this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
					// Выполняем реконнект
					return status_t::NEXT;
				}
				// Если опкоды требуют финального фрейма
				if(!head.fin && (static_cast <uint8_t> (head.optcode) > 0x07) && (static_cast <uint8_t> (head.optcode) < 0x0b)){
					// Создаём сообщение
					this->_mess = ws::mess_t(1002, "FIN must be set");
					// Выводим сообщение
					this->error(this->_mess);
					// Если функция обратного вызова активности потока установлена
					if(this->_callback.is("stream"))
						// Устанавливаем полученную функцию обратного вызова
						this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
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
						this->_inflate = (head.rsv[0] && (this->_compress != http_t::compress_t::NONE));
						// Если сообщение замаскированно
						if(head.mask){
							// Создаём сообщение
							this->_mess = ws::mess_t(1002, "Masked frame from server");
							// Выводим сообщение
							this->error(this->_mess);
							// Если функция обратного вызова активности потока установлена
							if(this->_callback.is("stream"))
								// Устанавливаем полученную функцию обратного вызова
								this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
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
							// Если функция обратного вызова активности потока установлена
							if(this->_callback.is("stream"))
								// Устанавливаем полученную функцию обратного вызова
								this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
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
						// Если функция обратного вызова активности потока установлена
						if(this->_callback.is("stream"))
							// Устанавливаем полученную функцию обратного вызова
							this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
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
					this->_thr.push(std::bind(&ws1_t::extraction, this, buffer, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT)));
				// Если тредпул не активирован, выполняем извлечение полученных сообщений
				else this->extraction(buffer, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT));
				// Очищаем буфер полученного сообщения
				buffer.clear();
			}
			// Если данные мы все получили, выходим
			if(!receive || this->_buffer.empty()) break;
		}
		// Выполняем завершение работы
		return status_t::STOP;
	}
	// Если функция обратного вызова активности потока установлена
	if(this->_callback.is("stream"))
		// Устанавливаем полученную функцию обратного вызова
		this->_resultCallback.set <void (const int32_t, const mode_t)> ("stream", this->_callback.get <void (const int32_t, const mode_t)> ("stream"), sid, mode_t::CLOSE);
	// Выполняем завершение работы
	return status_t::STOP;
}
/**
 * error Метод вывода сообщений об ошибках работы клиента
 * @param message сообщение с описанием ошибки
 */
void awh::client::WebSocket1::error(const ws::mess_t & message) const noexcept {
	// Очищаем список буффер бинарных данных
	const_cast <ws1_t *> (this)->_buffer.clear();
	// Очищаем список фрагментированных сообщений
	const_cast <ws1_t *> (this)->_fragmes.clear();
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
			// Если функция обратного вызова при получении ошибки WebSocket установлена
			if(this->_callback.is("wserror"))
				// Выводим функцию обратного вызова
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
void awh::client::WebSocket1::extraction(const vector <char> & buffer, const bool text) noexcept {
	// Если буфер данных передан
	if(!buffer.empty() && !this->_freeze && this->_callback.is("message")){
		// Если данные пришли в сжатом виде
		if(this->_inflate && (this->_compress != http_t::compress_t::NONE)){
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
void awh::client::WebSocket1::sendError(const ws::mess_t & mess) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
		// Если подключение выполнено
		if((this->_core != nullptr) && this->_core->working() && this->_allow.send && (this->_aid > 0)){
			// Запрещаем получение данных
			this->_allow.receive = false;
			// Получаем объект биндинга ядра TCP/IP
			client::core_t * core = const_cast <client::core_t *> (this->_core);
			// Выполняем остановку получения данных
			core->disabled(engine_t::method_t::READ, this->_aid);
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
					core->write(buffer.data(), buffer.size(), this->_aid);
					// Выходим из функции
					return;
				}
			}
			// Завершаем работу
			core->close(this->_aid);
		}
	}
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 */
void awh::client::WebSocket1::sendMessage(const vector <char> & message, const bool text) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
		// Если подключение выполнено
		if((this->_core != nullptr) && this->_core->working() && this->_allow.send){
			// Выполняем блокировку отправки сообщения
			this->_allow.send = !this->_allow.send;
			// Если рукопожатие выполнено
			if(!message.empty() && this->_http.isHandshake(http_t::process_t::RESPONSE) && (this->_aid > 0)){
				/**
				 * Если включён режим отладки
				 */
				#if defined(DEBUG_MODE)
					// Выводим заголовок ответа
					cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
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
				if(this->_crypt){
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
				head.rsv[0] = ((message.size() >= 1024) && (this->_compress != http_t::compress_t::NONE));
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
							data = this->_hash.compress(message.data(), message.size(), hash_t::method_t::DEFLATE);
							// Удаляем хвост в полученных данных
							this->_hash.rmTail(data);
						} break;
						// Если метод компрессии выбран GZip
						case static_cast <uint8_t> (http_t::compress_t::GZIP):
							// Выполняем компрессию полученных данных
							data = this->_hash.compress(message.data(), message.size(), hash_t::method_t::GZIP);
						break;
						// Если метод компрессии выбран Brotli
						case static_cast <uint8_t> (http_t::compress_t::BROTLI):
							// Выполняем компрессию полученных данных
							data = this->_hash.compress(message.data(), message.size(), hash_t::method_t::BROTLI);
						break;
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
							// Отправляем серверу сообщение
							const_cast <client::core_t *> (this->_core)->write(payload.data(), payload.size(), this->_aid);
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
					const auto & payload = this->_frame.methods.set(head, message.data(), message.size());
					// Если бинарный буфер для отправки данных получен
					if(!payload.empty())
						// Отправляем серверу сообщение
						const_cast <client::core_t *> (this->_core)->write(payload.data(), payload.size(), this->_aid);
				}
			}
			// Выполняем разблокировку отправки сообщения
			this->_allow.send = !this->_allow.send;
		}
	}
}
/**
 * pause Метод установки на паузу клиента
 */
void awh::client::WebSocket1::pause() noexcept {
	// Ставим работу клиента на паузу
	this->_freeze = true;
}
/**
 * stop Метод остановки клиента
 */
void awh::client::WebSocket1::stop() noexcept {
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
void awh::client::WebSocket1::start() noexcept {
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
void awh::client::WebSocket1::on(function <void (const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибок
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const u_int, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих ошибок
	this->_callback.set <void (const u_int, const string &)> ("wserror", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const vector <char> &, const bool)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих сообщений
	this->_callback.set <void (const vector <char> &, const bool)> ("message", callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Если функция обратного вызова передана
	if(callback != nullptr)
		// Устанавливаем функцию обратного вызова для HTTP/1.1
		this->_http.on(callback);
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	else this->_http.on(std::bind(&ws1_t::chunking, this, _1, _2, _3));
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функция обратного вызова активности потока
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const int32_t, const mode_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функция обратного вызова при выполнении рукопожатия
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const int32_t, const agent_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции обратного вызова при завершении запроса
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const int32_t, const direct_t)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const int32_t, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const int32_t, const u_int, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_http.on((function <void (const uint64_t, const u_int, const string &)>) std::bind(&ws1_t::response, this, _1, _2, _3));
}
/**
 * on Метод установки функции вывода полученного заголовка с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const int32_t, const string &, const string &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_http.on((function <void (const uint64_t, const string &, const string &)>) std::bind(&ws1_t::header, this, _1, _2, _3));
}
/**
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const int32_t, const u_int, const string &, const vector <char> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::client::WebSocket1::on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Выполняем установку функции обратного вызова
	web_t::on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/1.1
	this->_http.on((function <void (const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)>) std::bind(&ws1_t::headers, this, _1, _2, _3, _4));
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::client::WebSocket1::subprotocol(const string & subprotocol) noexcept {
	// Если поддерживаемый сабпротокол передан
	if(!subprotocol.empty())
		// Устанавливаем поддерживаемый сабподпротокол
		this->_http.subprotocol(subprotocol);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @return список выбранных сабпротоколов
 */
const set <string> & awh::client::WebSocket1::subprotocols() const noexcept {
	// Выводим список выбранных сабпротоколов
	return this->_http.subprotocols();
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::client::WebSocket1::subprotocols(const set <string> & subprotocols) noexcept {
	// Если список поддерживаемых сабпротоколов получен
	if(!subprotocols.empty())
		// Устанавливаем список поддерживаемых сабпротоколов
		this->_http.subprotocols(subprotocols);
}
/**
 * extensions Метод извлечения списка расширений
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::client::WebSocket1::extensions() const noexcept {
	// Выводим список доступных расширений
	return this->_http.extensions();
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::client::WebSocket1::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка доступных расширений
	this->_http.extensions(extensions);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::WebSocket1::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->_http.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::client::WebSocket1::segmentSize(const size_t size) noexcept {
	// Если размер передан, устанавливаем
	if(size > 0) this->_frame.size = size;
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::WebSocket1::mode(const set <flag_t> & flags) noexcept {
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
	// Устанавливаем флаг запрещающий выполнять метод CONNECT для прокси-клиента
	this->_proxy.connect = (flags.count(flag_t::PROXY_NOCONNECT) == 0);
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_client.takeover = (flags.count(flag_t::TAKEOVER_CLIENT) > 0);
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_server.takeover = (flags.count(flag_t::TAKEOVER_SERVER) > 0);
	// Если сетевое ядро установлено
	if(this->_core != nullptr){
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <client::core_t *> (this->_core)->noInfo(flags.count(flag_t::NOT_INFO) > 0);
		// Выполняем установку флага проверки домена
		const_cast <client::core_t *> (this->_core)->verifySSL(flags.count(flag_t::VERIFY_SSL) > 0);
	}
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::client::WebSocket1::core(const client::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
		// Активируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(true);
		// Добавляем схемы сети в сетевое ядро
		const_cast <client::core_t *> (this->_core)->add(&this->_scheme);
		// Устанавливаем функцию активации ядра клиента
		const_cast <client::core_t *> (this->_core)->on(std::bind(&ws1_t::eventsCallback, this, _1, _2));
		// Если многопоточность активированна
		if(this->_thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <client::core_t *> (this->_core)->easily(true);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_thr.is()){
			// Выполняем завершение всех активных потоков
			this->_thr.wait();
			// Снимаем режим простого чтения базы событий
			const_cast <client::core_t *> (this->_core)->easily(false);
		}
		// Деактивируем персистентный запуск для работы пингов
		const_cast <client::core_t *> (this->_core)->persistEnable(false);
		// Удаляем схему сети из сетевого ядра
		const_cast <client::core_t *> (this->_core)->remove(this->_scheme.sid);
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
	}
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::WebSocket1::user(const string & login, const string & password) noexcept {
	// Устанавливаем логин и пароль пользователя
	this->_http.user(login, password);
}
/**
 * setHeaders Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::client::WebSocket1::setHeaders(const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку HTTP-заголовков для отправки на сервер
	this->_headers = headers;
}
/**
 * userAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::client::WebSocket1::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()){
		// Устанавливаем пользовательского агента у родительского класса
		web_t::userAgent(userAgent);
		// Устанавливаем пользовательского агента
		this->_http.userAgent(userAgent);
	}
}
/**
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::WebSocket1::ident(const string & id, const string & name, const string & ver) noexcept {
	// Если данные сервиса переданы
	if(!id.empty() && !name.empty() && !ver.empty()){
		// Выполняем установку данных сервиса у родительского класса
		web_t::ident(id, name, ver);
		// Устанавливаем данные сервиса
		this->_http.ident(id, name, ver);
	}
}
/**
 * multiThreads Метод активации многопоточности
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::client::WebSocket1::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Если многопоточность ещё не активированна
		if(!this->_thr.is())
			// Выполняем инициализацию пула потоков
			this->_thr.init(count);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->_thr.wait();
			// Выполняем инициализацию нового тредпула
			this->_thr.init(count);
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
void awh::client::WebSocket1::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку параметры прокси-сервера
	web_t::proxy(uri, family);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::WebSocket1::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем параметры авторизации для HTTP-клиента
	this->_http.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::WebSocket1::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации на проксе-сервере
	web_t::authTypeProxy(type, hash);
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::WebSocket1::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования для HTTP-клиента
	this->_http.crypto(pass, salt, cipher);
}
/**
 * WebSocket1 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::WebSocket1::WebSocket1(const fmk_t * fmk, const log_t * log) noexcept :
 web_t(fmk, log), _close(false), _crypt(false), _shake(false), _noinfo(false), _freeze(false), _inflate(false),
 _point(0), _http(fmk, log), _hash(log), _frame(fmk, log), _resultCallback(log) {
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&ws1_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws1_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.on(std::bind(&ws1_t::chunking, this, _1, _2, _3));
}
/**
 * WebSocket1 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::WebSocket1::WebSocket1(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web_t(core, fmk, log), _close(false), _crypt(false), _shake(false), _noinfo(false), _freeze(false), _inflate(false),
 _point(0), _http(fmk, log), _hash(log), _frame(fmk, log), _resultCallback(log) {
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("persist", std::bind(&ws1_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&ws1_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.on(std::bind(&ws1_t::chunking, this, _1, _2, _3));
	// Активируем персистентный запуск для работы пингов
	const_cast <client::core_t *> (this->_core)->persistEnable(true);
}
/**
 * ~WebSocket1 Деструктор
 */
awh::client::WebSocket1::~WebSocket1() noexcept {
	// Если многопоточность активированна
	if(this->_thr.is())
		// Выполняем завершение всех активных потоков
		this->_thr.wait();
}
