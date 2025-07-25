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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <client/web/ws1.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * connectEvent Метод обратного вызова при подключении к серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Websocket1::connectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::OPEN, event_t::READ, event_t::PROXY_READ}, event_t::CONNECT)){
		// Выполняем сброс параметров запроса
		this->flush();
		// Запоминаем идентификатор брокера
		this->_bid = bid;
		// Выполняем очистку параметров HTTP-запроса
		this->_http.clear();
		// Выполняем сброс состояния HTTP-парсера
		this->_http.reset();
		// Выполняем установку идентификатора объекта
		this->_http.id(bid);
		// Выполняем очистку функций обратного вызова
		this->_callback.clear();
		// Если HTTP-заголовки установлены
		if(!this->_headers.empty())
			// Выполняем установку HTTP-заголовков
			this->_http.headers(this->_headers);
		// Устанавливаем список поддерживаемых компрессоров
		this->_http.compressors(this->_compressors);
		// Разрешаем перехватывать контекст компрессии
		this->_hash.takeoverCompress(this->_client.takeover);
		// Разрешаем перехватывать контекст декомпрессии
		this->_hash.takeoverDecompress(this->_server.takeover);
		// Разрешаем перехватывать контекст для клиента
		this->_http.takeover(awh::web_t::hid_t::CLIENT, this->_client.takeover);
		// Разрешаем перехватывать контекст для сервера
		this->_http.takeover(awh::web_t::hid_t::SERVER, this->_server.takeover);
		// Если идентификатор потока не установлен
		if(this->_sid == -1)
			// Устанавливаем идентификатор потока
			this->_sid = 1;
		// Если идентификатор запроса не установлен
		if(this->_rid == 0)
			// Выполняем генерацию идентификатора запроса
			this->_rid = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::NANOSECONDS);
		// Создаём объек запроса
		awh::web_t::req_t request(awh::web_t::method_t::GET, this->_scheme.url);
		// Если активирован режим прокси-сервера
		if(this->_proxy.mode){
			// Активируем точную установку хоста
			this->_http.precise(!this->_proxy.connect);
			// Выполняем извлечение заголовка авторизации на прокси-сервера
			const string & header = this->_scheme.proxy.http.auth(http_t::process_t::REQUEST, request);
			// Если заголовок авторизации получен
			if(!header.empty())
				// Выполняем установки заголовка авторизации на прокси-сервере
				this->_http.header("Proxy-Authorization", header);
			// Если установлено постоянное подключение к прокси-серверу
			if(this->_scheme.proxy.http.is(http_t::state_t::ALIVE))
				// Устанавливаем постоянное подключение к прокси-серверу
				this->_http.header("Proxy-Connection", "keep-alive");
			// Устанавливаем закрытие подключения к прокси-серверу
			else this->_http.header("Proxy-Connection", "close");
		}
		// Получаем бинарные данные REST запроса
		const auto & buffer = this->_http.process(http_t::process_t::REQUEST, request);
		// Если бинарные данные запроса получены
		if(!buffer.empty()){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим заголовок запроса
				std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
				// Выводим параметры запроса
				std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
			#endif
			// Выполняем отправку сообщения на сервер
			const_cast <client::core_t *> (this->_core)->send(buffer.data(), buffer.size(), bid);
		}
		// Если функция обратного вызова при подключении/отключении установлена
		if(web_t::_callback.is("active"))
			// Выполняем функцию обратного вызова
			web_t::_callback.call <void (const mode_t)> ("active", mode_t::CONNECT);
		// Если установлена функция отлова завершения запроса
		if(web_t::_callback.is("end"))
			// Выполняем функцию обратного вызова
			web_t::_callback.call <void (const int32_t, const uint64_t, const direct_t)> ("end", this->_sid, this->_rid, direct_t::SEND);
	}
}
/**
 * disconnectEvent Метод обратного вызова при отключении от сервера
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Websocket1::disconnectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Выполняем редирект, если редирект выполнен
	if(this->redirect())
		// Выходим из функции
		return;
	// Выполняем сброс идентификатора запроса
	this->_rid = 0;
	// Выполняем сброс идентификатора потока
	this->_sid = -1;
	// Если подключение является постоянным
	if(this->_scheme.alive){
		// Выполняем очистку оставшихся данных
		this->_buffer.clear();
		// Выполняем очистку оставшихся фрагментов
		this->_fragments.clear();
		// Если размер выделенной памяти выше максимального размера буфера
		if(this->_fragments.capacity() > AWH_BUFFER_SIZE)
			// Выполняем очистку временного буфера данных
			vector <char> ().swap(this->_fragments);
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
	// Если функция обратного вызова при подключении/отключении установлена
	if(web_t::_callback.is("active"))
		// Выполняем функцию обратного вызова
		web_t::_callback.call <void (const mode_t)> ("active", mode_t::DISCONNECT);
}
/**
 * readEvent Метод обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::client::Websocket1::readEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Флаг выполнения обработки полученных данных
		bool process = false;
		// Если установлена функция обратного вызова для вывода данных в сыром виде
		if(!(process = !web_t::_callback.is("raw")))
			// Выполняем функцию обратного вызова
			process = web_t::_callback.call <bool (const char *, const size_t)> ("raw", buffer, size);
		// Если обработка полученных данных разрешена
		if(process){
			// Если подключение закрыто
			if(this->_close)
				// Выходим из функции
				return;
			// Создаём объект холдирования
			hold_t <event_t> hold(this->_events);
			// Если событие соответствует разрешённому
			if(hold.access({event_t::CONNECT}, event_t::READ)){
				// Если рукопожатие не выполнено
				if(!(this->_shake = reinterpret_cast <http_t &> (this->_http).is(http_t::state_t::HANDSHAKE))){
					// Добавляем полученные данные в буфер
					this->_buffer.push(buffer, size);
					// Выполняем парсинг полученных данных
					const size_t bytes = this->_http.parse(reinterpret_cast <const char *> (this->_buffer.get()), this->_buffer.size());
					// Есла данных передано больше чем обработано
					if(this->_buffer.size() > bytes)
						// Удаляем количество обработанных байт
						this->_buffer.erase(bytes);
					// Если данных в буфере больше нет, очищаем буфер собранных данных
					else this->_buffer.clear();
					// Если все данные получены
					if(this->_http.is(http_t::state_t::END)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
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
								if(!response.empty()){
									// Получаем размер тела сообщения
									const size_t bodySize = this->_http.body().size();
									// Выводим заголовок ответа
									std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
									// Выводим параметры ответа
									std::cout << string(response.begin(), response.end()) << std::endl << std::flush;
									// Если тело ответа существует
									if(bodySize > 0)
										// Выводим сообщение о выводе чанка тела
										std::cout << this->_fmk->format("<body %zu>", bodySize) << std::endl << std::endl << std::flush;
									// Иначе устанавливаем перенос строки
									else std::cout << std::endl << std::flush;
								}
							}
						#endif
						// Выполняем препарирование полученных данных
						switch(static_cast <uint8_t> (this->prepare(this->_sid, bid))){
							// Если необходимо выполнить остановку обработки
							case static_cast <uint8_t> (status_t::STOP): {
								// Выполняем сброс количества попыток
								this->_attempt = 0;
								// Если установлена функция отлова завершения запроса
								if(web_t::_callback.is("end"))
									// Выполняем функцию обратного вызова
									web_t::_callback.call <void (const int32_t, const uint64_t, const direct_t)> ("end", this->_sid, this->_rid, direct_t::RECV);
								// Завершаем работу
								const_cast <client::core_t *> (this->_core)->close(bid);
							} break;
						}
						// Если функция обратного вызова активности потока установлена
						if(this->_callback.is("stream"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (void)> ("stream");
						// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
						if(this->_callback.is("entity"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (void)> ("entity");
						// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
						if(this->_callback.is("complete"))
							// Выполняем функцию обратного вызова
							this->_callback.call <void (void)> ("complete");
						// Выполняем очистку функций обратного вызова
						this->_callback.clear();
					}
					// Завершаем работу
					return;
				// Если рукопожатие выполнено
				} else if(this->_allow.receive) {
					// Добавляем полученные данные в буфер
					this->_buffer.push(buffer, size);
					// Обнуляем время последнего ответа на пинг
					this->_respPong = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
					// Обновляем время отправленного пинга
					this->_sendPing = this->_respPong;
					// Выполняем препарирование полученных данных
					this->prepare(this->_sid, bid);
				}
				// Если функция обратного вызова активности потока установлена
				if(this->_callback.is("stream"))
					// Выполняем функцию обратного вызова
					this->_callback.call <void (void)> ("stream");
				// Выполняем очистку функций обратного вызова
				this->_callback.clear();
				// Если установлена функция отлова завершения запроса
				if(web_t::_callback.is("end"))
					// Выполняем функцию обратного вызова
					web_t::_callback.call <void (const int32_t, const uint64_t, const direct_t)> ("end", this->_sid, this->_rid, direct_t::RECV);
			}
		}
	}
}
/**
 * writeEvent Метод обратного вызова при записи сообщения на клиенте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::client::Websocket1::writeEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0)){
		// Если необходимо выполнить закрыть подключение
		if(!this->_close && this->_stopped){
			// Устанавливаем флаг закрытия подключения
			this->_close = !this->_close;
			// Принудительно выполняем отключение лкиента
			const_cast <client::core_t *> (this->_core)->close(bid);
			// Если установлена функция отлова завершения запроса
			if(web_t::_callback.is("end"))
				// Выполняем функцию обратного вызова
				web_t::_callback.call <void (const int32_t, const uint64_t, const direct_t)> ("end", sid, this->_rid, direct_t::SEND);
		}
	}
}
/**
 * callbackEvent Метод отлавливания событий контейнера функций обратного вызова
 * @param event событие контейнера функций обратного вызова
 * @param fid   идентификатор функции обратного вызова
 */
void awh::client::Websocket1::callbackEvent(const callback_t::event_t event, const uint64_t fid, const callback_t::fn_t &) noexcept {
	// Определяем входящее событие контейнера функций обратного вызова
	switch(static_cast <uint8_t> (event)){
		// Если событием является установка функции обратного вызова
		case static_cast <uint8_t> (callback_t::event_t::SET): {
			// Если функция обратного вызова на перехват полученных чанков установлена
			if(fid == web_t::_callback.fid("chunking"))
				// Устанавливаем внешнюю функцию обратного вызова
				this->_http.on <void (const int32_t, const uint64_t, const vector <char> &)> ("chunking", web_t::_callback.get <void (const int32_t, const uint64_t, const vector <char> &)> ("chunking"));
		} break;
	}
}
/**
 * redirect Метод выполнения редиректа если требуется
 * @return результат выполнения редиректа
 */
bool awh::client::Websocket1::redirect() noexcept {
	// Результат работы функции
	bool result = false;
	// Если список ответов получен
	if(this->_redirects && (result = !this->_stopped)){
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
	// Выводим результат
	return result;
}
/**
 * response Метод получения ответа сервера
 * @param bid     идентификатор брокера
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 */
void awh::client::Websocket1::response([[maybe_unused]] const uint64_t bid, const uint32_t code, const string & message) noexcept {
	// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
	if(web_t::_callback.is("response"))
		// Выполняем функцию обратного вызова
		web_t::_callback.call <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", this->_sid, this->_rid, code, message);
}
/**
 * header Метод получения заголовка
 * @param bid   идентификатор брокера
 * @param key   ключ заголовка
 * @param value значение заголовка
 */
void awh::client::Websocket1::header([[maybe_unused]] const uint64_t bid, const string & key, const string & value) noexcept {
	// Если функция обратного вызова на полученного заголовка с сервера установлена
	if(web_t::_callback.is("header"))
		// Выполняем функцию обратного вызова
		web_t::_callback.call <void (const int32_t, const uint64_t, const string &, const string &)> ("header", this->_sid, this->_rid, key, value);
}
/**
 * headers Метод получения заголовков
 * @param bid     идентификатор брокера
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 * @param headers заголовки ответа сервера
 */
void awh::client::Websocket1::headers([[maybe_unused]] const uint64_t bid, const uint32_t code, const string & message, const std::unordered_multimap <string, string> & headers) noexcept {
	// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
	if(web_t::_callback.is("headers"))
		// Выполняем функцию обратного вызова
		web_t::_callback.call <void (const int32_t, const uint64_t, const uint32_t, const string &, const std::unordered_multimap <string, string> &)> ("headers", this->_sid, this->_rid, code, message, this->_http.headers());
}
/**
 * chunking Метод обработки получения чанков
 * @param bid   идентификатор брокера
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::client::Websocket1::chunking([[maybe_unused]] const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()){
		// Если функция обратного вызова на перехват входящих чанков установлена
		if(web_t::_callback.is("chunking"))
			// Выполняем функцию обратного вызова
			web_t::_callback.call <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", this->_rid, chunk, http);
		// Если функция перехвата полученных чанков не установлена
		else if(this->_core != nullptr) {
			// Выполняем добавление полученного чанка в тело ответа
			const_cast <awh::http_t *> (http)->body(chunk);
			// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
			if(web_t::_callback.is("chunks"))
				// Выполняем функцию обратного вызова
				web_t::_callback.call <void (const int32_t, const uint64_t, const vector <char> &)> ("chunks", this->_sid, this->_rid, chunk);
		}
	}
}
/**
 * flush Метод сброса параметров запроса
 */
void awh::client::Websocket1::flush() noexcept {
	// Снимаем флаг отключения
	this->_close = false;
	// Снимаем флаг принудительной остановки
	this->_stopped = false;
	// Устанавливаем флаг разрешающий обмен данных
	this->_allow = allow_t();
	// Выполняем очистку оставшихся данных
	this->_buffer.clear();
	// Выполняем очистку оставшихся фрагментов
	this->_fragments.clear();
	// Если размер выделенной памяти выше максимального размера буфера
	if(this->_fragments.capacity() > AWH_BUFFER_SIZE)
		// Выполняем очистку временного буфера данных
		vector <char> ().swap(this->_fragments);
}
/**
 * pinging Метод таймера выполнения пинга удалённого сервера
 * @param tid идентификатор таймера
 */
void awh::client::Websocket1::pinging(const uint16_t tid) noexcept {
	// Если данные существуют
	if((tid > 0) && (this->_core != nullptr)){
		// Если разрешено выполнять пинги
		if(this->_pinging && this->_shake && !this->_close){
			// Получаем текущий штамп времени
			const uint64_t date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
			// Если брокер не ответил на пинг больше двух интервалов, отключаем его
			if((this->_waitPong > 0) && ((date - this->_respPong) >= static_cast <uint64_t> (this->_waitPong))){
				// Создаём сообщение
				this->_mess = ws::mess_t(1005, "PING response not received");
				// Выполняем отправку сообщения об ошибке
				this->sendError(this->_mess);
			// Если время с предыдущего пинга прошло больше половины времени пинга
			} else if((this->_waitPong > 0) && (this->_pingInterval > 0) && ((date - this->_sendPing) > static_cast <uint64_t> (this->_pingInterval / 2))) {
				/**
				 * Создаём отдельную переменную для отправки,
				 * так-как в профессе формирования фрейма она будет изменена,
				 * нельзя отправлять идентификатор подключения в том виде, как он есть.
				 */
				const uint64_t message = this->_bid;
				// Отправляем запрос брокеру
				this->ping(&message, sizeof(message));
			}
		}
	}
}
/**
 * ping Метод проверки доступности сервера
 * @param buffer бинарный буфер отправляемого сообщения
 * @param size   размер бинарного буфера отправляемого сообщения
 */
void awh::client::Websocket1::ping(const void * buffer, const size_t size) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && this->_allow.send){
		// Если рукопожатие выполнено
		if(this->_http.handshake(http_t::process_t::RESPONSE) && (this->_bid > 0)){
			// Создаём фрейм для отправки
			const auto & frame = this->_frame.methods.ping(buffer, size, true);
			// Если фрейм для отправки получен
			if(!frame.empty()){
				// Выполняем отправку сообщения на сервер
				if(const_cast <client::core_t *> (this->_core)->send(frame.data(), frame.size(), this->_bid))
					// Обновляем время отправленного пинга
					this->_sendPing = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
			}
		}
	}
}
/**
 * pong Метод ответа на проверку о доступности сервера
 * @param buffer бинарный буфер отправляемого сообщения
 * @param size   размер бинарного буфера отправляемого сообщения
 */
void awh::client::Websocket1::pong(const void * buffer, const size_t size) noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && this->_allow.send){
		// Если рукопожатие выполнено
		if(this->_http.handshake(http_t::process_t::RESPONSE) && (this->_bid > 0)){
			// Создаём фрейм для отправки
			const auto & frame = this->_frame.methods.pong(buffer, size, true);
			// Если фрейм для отправки получен
			if(!frame.empty())
				// Выполняем отправку сообщения на сервер
				const_cast <client::core_t *> (this->_core)->send(frame.data(), frame.size(), this->_bid);
		}
	}
}
/**
 * prepare Метод выполнения препарирования полученных данных
 * @param sid идентификатор запроса
 * @param bid идентификатор брокера
 * @return    результат препарирования
 */
awh::client::Web::status_t awh::client::Websocket1::prepare(const int32_t sid, const uint64_t bid) noexcept {
	// Если рукопожатие не выполнено
	if(!this->_shake){
		// Получаем параметры запроса
		auto response = this->_http.response();
		// Выполняем проверку авторизации
		switch(static_cast <uint8_t> (this->_http.auth())){
			// Если нужно попытаться ещё раз
			case static_cast <uint8_t> (http_t::status_t::RETRY): {
				// Если функция обратного вызова получения статуса ответа установлена
				if(web_t::_callback.is("answer"))
					// Выполняем функцию обратного вызова
					web_t::_callback.call <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", sid, this->_rid, http_t::status_t::RETRY);
				// Если попытка повторить авторизацию ещё не проводилась
				if(!(this->_stopped = (this->_attempt >= this->_attempts))){
					// Если функция обратного вызова на на вывод ошибок установлена
					if(((response.code == 401) || (response.code == 407)) && web_t::_callback.is("error"))
						// Выполняем функцию обратного вызова
						web_t::_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::CRITICAL, http::error_t::HTTP1_RECV, "authorization failed");
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
							// Выполняем сброс параметров запроса
							this->flush();
							// Увеличиваем количество попыток
							this->_attempt++;
							// Устанавливаем новый адрес запроса
							this->_uri.combine(this->_scheme.url, url);
							// Если функция обратного вызова активности потока установлена
							if(web_t::_callback.is("stream"))
								// Устанавливаем полученную функцию обратного вызова
								this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::CLOSE);
							// Выполняем попытку повторить запрос
							this->connectEvent(bid, sid);
						// Если подключение не постоянное, то завершаем работу
						} else const_cast <client::core_t *> (this->_core)->close(bid);
					// Если URL-адрес запроса не получен
					} else {
						// Если активирован режим работы прокси-сервера и требуется авторизация
						if((response.code == 407) && this->_proxy.mode){
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
						// Если соединение является постоянным
						if(this->_http.is(http_t::state_t::ALIVE)){
							// Выполняем сброс параметров запроса
							this->flush();
							// Увеличиваем количество попыток
							this->_attempt++;
							// Если функция обратного вызова активности потока установлена
							if(web_t::_callback.is("stream"))
								// Устанавливаем полученную функцию обратного вызова
								this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::CLOSE);
							// Выполняем попытку повторить запрос
							this->connectEvent(bid, sid);
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
				// Если функция обратного вызова активности потока установлена
				if(web_t::_callback.is("stream"))
					// Устанавливаем полученную функцию обратного вызова
					this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::CLOSE);
			} break;
			// Если запрос выполнен удачно
			case static_cast <uint8_t> (http_t::status_t::GOOD): {
				// Если функция обратного вызова получения статуса ответа установлена
				if(web_t::_callback.is("answer"))
					// Выполняем функцию обратного вызова
					web_t::_callback.call <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", sid, this->_rid, http_t::status_t::GOOD);
				// Если рукопожатие выполнено
				if((this->_shake = this->_http.handshake(http_t::process_t::RESPONSE))){
					// Выполняем сброс количества попыток
					this->_attempt = 0;
					// Очищаем список фрагментированных сообщений
					this->_fragments.clear();
					// Если размер выделенной памяти выше максимального размера буфера
					if(this->_fragments.capacity() > AWH_BUFFER_SIZE)
						// Выполняем очистку временного буфера данных
						vector <char> ().swap(this->_fragments);
					// Получаем флаг шифрованных данных
					this->_crypted = this->_http.crypted();
					// Получаем поддерживаемый метод компрессии
					this->_compressor = this->_http.compression();
					// Получаем размер скользящего окна сервера
					this->_server.wbit = this->_http.wbit(awh::web_t::hid_t::SERVER);
					// Получаем размер скользящего окна клиента
					this->_client.wbit = this->_http.wbit(awh::web_t::hid_t::CLIENT);
					// Обновляем контрольную точку времени получения данных
					this->_respPong = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
					// Если данные необходимо зашифровать
					if(this->_encryption.mode && this->_crypted){
						// Устанавливаем размер шифрования
						this->_cipher = this->_encryption.cipher;
						// Устанавливаем соль шифрования
						this->_hash.salt(this->_encryption.salt);
						// Устанавливаем пароль шифрования
						this->_hash.password(this->_encryption.pass);
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
					if(web_t::_callback.is("stream"))
						// Устанавливаем полученную функцию обратного вызова
						this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::OPEN);
					// Если функция обратного вызова на получение удачного ответа установлена
					if(web_t::_callback.is("handshake"))
						// Выполняем функцию обратного вызова
						web_t::_callback.call <void (const int32_t, const uint64_t, const agent_t)> ("handshake", sid, this->_rid, agent_t::WEBSOCKET);
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(!this->_http.empty(awh::http_t::suite_t::BODY) && web_t::_callback.is("entity"))
						// Устанавливаем полученную функцию обратного вызова
						this->_callback.on <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", web_t::_callback.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, this->_rid, response.code, response.message, this->_http.body());
					// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
					if(web_t::_callback.is("complete"))
						// Выполняем функцию обратного вызова
						this->_callback.on <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const std::unordered_multimap <string, string> &)> ("complete", web_t::_callback.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const std::unordered_multimap <string, string> &)> ("complete"), sid, this->_rid, response.code, response.message, this->_http.body(), this->_http.headers());
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
					if(web_t::_callback.is("stream"))
						// Устанавливаем полученную функцию обратного вызова
						this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::CLOSE);
					// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
					if(!this->_http.empty(awh::http_t::suite_t::BODY) && web_t::_callback.is("entity"))
						// Устанавливаем полученную функцию обратного вызова
						this->_callback.on <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", web_t::_callback.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, this->_rid, response.code, response.message, this->_http.body());
					// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
					if(web_t::_callback.is("complete"))
						// Выполняем функцию обратного вызова
						this->_callback.on <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const std::unordered_multimap <string, string> &)> ("complete", web_t::_callback.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const std::unordered_multimap <string, string> &)> ("complete"), sid, this->_rid, response.code, response.message, this->_http.body(), this->_http.headers());
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
				if(web_t::_callback.is("answer"))
					// Выполняем функцию обратного вызова
					web_t::_callback.call <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", sid, this->_rid, http_t::status_t::FAULT);
				// Если функция обратного вызова активности потока установлена
				if(web_t::_callback.is("stream"))
					// Устанавливаем полученную функцию обратного вызова
					this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::CLOSE);
				// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
				if(!this->_http.empty(awh::http_t::suite_t::BODY) && web_t::_callback.is("entity"))
					// Устанавливаем полученную функцию обратного вызова
					this->_callback.on <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity", web_t::_callback.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>)> ("entity"), sid, this->_rid, response.code, response.message, this->_http.body());
				// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
				if(web_t::_callback.is("complete"))
					// Выполняем функцию обратного вызова
					this->_callback.on <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const std::unordered_multimap <string, string> &)> ("complete", web_t::_callback.get <void (const int32_t, const uint64_t, const uint32_t, const string, const vector <char>, const std::unordered_multimap <string, string> &)> ("complete"), sid, this->_rid, response.code, response.message, this->_http.body(), this->_http.headers());
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
			const auto & payload = this->_frame.methods.get(head, reinterpret_cast <const char *> (this->_buffer.get()), this->_buffer.size());
			// Если буфер данных получен
			if(!payload.empty() || (head.optcode == ws::frame_t::opcode_t::PING) || (head.optcode == ws::frame_t::opcode_t::PONG) || (head.optcode == ws::frame_t::opcode_t::CLOSE)){
				// Определяем тип ответа
				switch(static_cast <uint8_t> (head.optcode)){
					// Если ответом является PING
					case static_cast <uint8_t> (ws::frame_t::opcode_t::PING): {
						// Если сообщение пинга пришло
						if(!payload.empty())
							// Отправляем ответ серверу
							this->pong(payload.data(), payload.size());
						// Отправляем ответ серверу
						else this->pong();
					} break;
					// Если ответом является PONG
					case static_cast <uint8_t> (ws::frame_t::opcode_t::PONG): {
						// Если сообщение понга пришло
						if(!payload.empty()){
							// Если полезная нагрузка получена в виде числа
							if(payload.size() == sizeof(bid)){
								// Значение понга для проверки
								uint64_t result = 0;
								// Извлекаем значение ответа
								::memcpy(&result, payload.data(), payload.size());
								// Если идентификатор брокера совпадает
								if(bid == result){
									// Обновляем контрольную точку
									this->_respPong = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
									// Выходим из условия
									break;
								}
								// Выводим сообщение об ошибке
								this->_log->print("Response to the PONG message is not valid", log_t::flag_t::WARNING);
							}
						// Если сообщение понга не пришло
						} else this->_log->print("Response to the PONG message is empty", log_t::flag_t::WARNING);
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
							// Если функция обратного вызова активности потока установлена
							if(web_t::_callback.is("stream"))
								// Устанавливаем полученную функцию обратного вызова
								this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::CLOSE);
							// Выполняем реконнект
							return status_t::STOP;
						// Если список фрагментированных сообщений существует
						} else if(!this->_fragments.empty()) {
							// Очищаем список фрагментированных сообщений
							this->_fragments.clear();
							// Если размер выделенной памяти выше максимального размера буфера
							if(this->_fragments.capacity() > AWH_BUFFER_SIZE)
								// Выполняем очистку временного буфера данных
								vector <char> ().swap(this->_fragments);
							// Создаём сообщение
							this->_mess = ws::mess_t(1002, "Opcode for subsequent fragmented messages should not be set");
							// Выводим сообщение
							this->error(this->_mess);
							// Если функция обратного вызова активности потока установлена
							if(web_t::_callback.is("stream"))
								// Устанавливаем полученную функцию обратного вызова
								this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::CLOSE);
							// Выполняем реконнект
							return status_t::STOP;
						// Если сообщение является не последнем
						} else if(!head.fin)
							// Заполняем фрагментированное сообщение
							this->_fragments.insert(this->_fragments.end(), payload.begin(), payload.end());
						// Если сообщение является последним
						else {
							// Если тредпул активирован
							if(this->_thr.is())
								// Добавляем в тредпул новую задачу на извлечение полученных сообщений
								this->_thr.push(std::bind(&ws1_t::extraction, this, payload, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT)));
							// Если тредпул не активирован, выполняем извлечение полученных сообщений
							else this->extraction(payload, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT));
						}
					} break;
					// Если ответом является CONTINUATION
					case static_cast <uint8_t> (ws::frame_t::opcode_t::CONTINUATION): {
						// Если фрагменты сообщения уже собраны
						if(!this->_fragments.empty()){
							// Заполняем фрагментированное сообщение
							this->_fragments.insert(this->_fragments.end(), payload.begin(), payload.end());
							// Если сообщение является последним
							if(head.fin){
								// Если тредпул активирован
								if(this->_thr.is())
									// Добавляем в тредпул новую задачу на извлечение полученных сообщений
									this->_thr.push(std::bind(&ws1_t::extraction, this, this->_fragments, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT)));
								// Если тредпул не активирован, выполняем извлечение полученных сообщений
								else this->extraction(this->_fragments, (this->_frame.opcode == ws::frame_t::opcode_t::TEXT));
								// Очищаем список фрагментированных сообщений
								this->_fragments.clear();
								// Если размер выделенной памяти выше максимального размера буфера
								if(this->_fragments.capacity() > AWH_BUFFER_SIZE)
									// Выполняем очистку временного буфера данных
									vector <char> ().swap(this->_fragments);
							}
						// Если фрагментированные сообщения не существуют
						} else {
							// Создаём сообщение
							this->_mess = ws::mess_t(1002, "Fragmented Message Transfer Protocol Failure");
							// Выводим сообщение
							this->error(this->_mess);
							// Если функция обратного вызова активности потока установлена
							if(web_t::_callback.is("stream"))
								// Устанавливаем полученную функцию обратного вызова
								this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::CLOSE);
							// Выполняем реконнект
							return status_t::STOP;
						}
					} break;
					// Если ответом является CLOSE
					case static_cast <uint8_t> (ws::frame_t::opcode_t::CLOSE): {
						// Если сообщение получено
						if(!payload.empty()){
							// Извлекаем сообщение
							this->_mess = this->_frame.methods.message(payload.data(), payload.size());
							// Выводим сообщение
							this->error(this->_mess);
						}
						// Выполняем закрытие подключения
						const_cast <client::core_t *> (this->_core)->close(bid);
						// Если функция обратного вызова активности потока установлена
						if(web_t::_callback.is("stream"))
							// Устанавливаем полученную функцию обратного вызова
							this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::CLOSE);
						// Выполняем реконнект
						return status_t::STOP;
					}
				}
				// Если парсер обработал какое-то количество байт
				if((head.frame > 0) && !this->_buffer.empty()){
					// Если размер буфера больше количества удаляемых байт
					if((receive = (this->_buffer.size() >= head.frame)))
						// Удаляем количество обработанных байт
						this->_buffer.erase(head.frame);
				}
			// Если мы получили ошибку получения фрейма
			} else if(head.state == ws::frame_t::state_t::BAD) {
				// Создаём сообщение
				this->_mess = this->_frame.methods.message(head, 1005, (this->_compressor != http_t::compressor_t::NONE));
				// Выводим сообщение
				this->error(this->_mess);
				// Если функция обратного вызова активности потока установлена
				if(web_t::_callback.is("stream"))
					// Устанавливаем полученную функцию обратного вызова
					this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::CLOSE);
				// Выполняем реконнект
				return status_t::STOP;
			}
			// Если данные мы все получили, выходим
			if(!receive || (payload.empty() &&
			  (head.optcode != ws::frame_t::opcode_t::PING) &&
			  (head.optcode != ws::frame_t::opcode_t::PONG)) || this->_buffer.empty())
				// Выходим из условия
				break;
		}
		// Выполняем завершение работы
		return status_t::STOP;
	}
	// Если функция обратного вызова активности потока установлена
	if(web_t::_callback.is("stream"))
		// Устанавливаем полученную функцию обратного вызова
		this->_callback.on <void (const int32_t, const uint64_t, const mode_t)> ("stream", web_t::_callback.get <void (const int32_t, const uint64_t, const mode_t)> ("stream"), sid, this->_rid, mode_t::CLOSE);
	// Выполняем завершение работы
	return status_t::STOP;
}
/**
 * error Метод вывода сообщений об ошибках работы клиента
 * @param message сообщение с описанием ошибки
 */
void awh::client::Websocket1::error(const ws::mess_t & message) const noexcept {
	// Очищаем список буффер бинарных данных
	const_cast <ws1_t *> (this)->_buffer.clear();
	// Очищаем список фрагментированных сообщений
	const_cast <ws1_t *> (this)->_fragments.clear();
	// Если размер выделенной памяти выше максимального размера буфера
	if(this->_fragments.capacity() > AWH_BUFFER_SIZE)
		// Выполняем очистку временного буфера данных
		vector <char> ().swap(const_cast <ws1_t *> (this)->_fragments);
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
			// Если функция обратного вызова при получении ошибки Websocket установлена
			if(web_t::_callback.is("errorWebsocket"))
				// Выполняем функцию обратного вызова
				web_t::_callback.call <void (const uint32_t, const string &)> ("errorWebsocket", message.code, message.text);
			// Если функция обратного вызова на на вывод ошибок установлена
			if(web_t::_callback.is("error"))
				// Выполняем функцию обратного вызова
				web_t::_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", log_t::flag_t::WARNING, http::error_t::WEBSOCKET, this->_fmk->format("%s [%u]", message.code, message.text.c_str()));
		}
	}
}
/**
 * extraction Метод извлечения полученных данных
 * @param buffer данные в чистом виде полученные с сервера
 * @param text   данные передаются в текстовом виде
 */
void awh::client::Websocket1::extraction(const vector <char> & buffer, const bool text) noexcept {
	// Если буфер данных передан
	if(!buffer.empty() && !this->_freeze && web_t::_callback.is("messageWebsocket")){
		// Декомпрессионные данные
		vector <char> result(buffer.begin(), buffer.end());
		// Если нужно производить дешифрование
		if(this->_crypted)
			// Выполняем дешифрование полезной нагрузки
			result = this->_hash.decode <vector <char>> (result.data(), result.size(), this->_cipher);
		// Если данные пришли в сжатом виде
		if(this->_inflate && (this->_compressor != http_t::compressor_t::NONE)){
			// Определяем метод компрессии
			switch(static_cast <uint8_t> (this->_compressor)){
				// Если метод компрессии выбран LZ4
				case static_cast <uint8_t> (http_t::compressor_t::LZ4):
					// Выполняем декомпрессию полученных данных
					result = this->_hash.decompress <vector <char>> (result.data(), result.size(), hash_t::method_t::LZ4);
				break;
				// Если метод компрессии выбран Zstandard
				case static_cast <uint8_t> (http_t::compressor_t::ZSTD):
					// Выполняем декомпрессию полученных данных
					result = this->_hash.decompress <vector <char>> (result.data(), result.size(), hash_t::method_t::ZSTD);
				break;
				// Если метод компрессии выбран LZma
				case static_cast <uint8_t> (http_t::compressor_t::LZMA):
					// Выполняем декомпрессию полученных данных
					result = this->_hash.decompress <vector <char>> (result.data(), result.size(), hash_t::method_t::LZMA);
				break;
				// Если метод компрессии выбран Brotli
				case static_cast <uint8_t> (http_t::compressor_t::BROTLI):
					// Выполняем декомпрессию полученных данных
					result = this->_hash.decompress <vector <char>> (result.data(), result.size(), hash_t::method_t::BROTLI);
				break;
				// Если метод компрессии выбран BZip2
				case static_cast <uint8_t> (http_t::compressor_t::BZIP2):
					// Выполняем декомпрессию полученных данных
					result = this->_hash.decompress <vector <char>> (result.data(), result.size(), hash_t::method_t::BZIP2);
				break;
				// Если метод компрессии выбран GZip
				case static_cast <uint8_t> (http_t::compressor_t::GZIP):
					// Выполняем декомпрессию полученных данных
					result = this->_hash.decompress <vector <char>> (result.data(), result.size(), hash_t::method_t::GZIP);
				break;
				// Если метод компрессии выбран Deflate
				case static_cast <uint8_t> (http_t::compressor_t::DEFLATE): {
					// Устанавливаем размер скользящего окна
					this->_hash.wbit(this->_server.wbit);
					// Добавляем хвост в полученные данные
					this->_hash.setTail(result);
					// Выполняем декомпрессию полученных данных
					result = this->_hash.decompress <vector <char>> (result.data(), result.size(), hash_t::method_t::DEFLATE);
				} break;
			}
		}
		// Если данные получены
		if(!result.empty())
			// Отправляем полученный результат
			web_t::_callback.call <void (const vector <char> &, const bool)> ("messageWebsocket", result, text);
		// Выводим сообщение об ошибке
		else {
			// Иначе выводим сообщение так - как оно пришло
			web_t::_callback.call <void (const vector <char> &, const bool)> ("messageWebsocket", buffer, text);
			// Создаём сообщение
			this->_mess = ws::mess_t(1007, "Received data decompression error");
			// Выполняем отправку сообщения об ошибке
			this->sendError(this->_mess);
		}
	}
}
/**
 * sendError Метод отправки сообщения об ошибке
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::Websocket1::sendError(const ws::mess_t & mess) noexcept {
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
		// Если подключение выполнено
		if((this->_core != nullptr) && this->_core->working() && this->_allow.send && (this->_bid > 0)){
			// Запрещаем получение данных
			this->_allow.receive = false;
			// Получаем объект биндинга ядра TCP/IP
			client::core_t * core = const_cast <client::core_t *> (this->_core);
			// Выполняем остановку получения данных
			core->events(this->_bid, awh::scheme_t::mode_t::DISABLED, engine_t::method_t::READ);
			// Если код ошибки относится к Websocket
			if((mess.code >= 1000) && !this->_stopped){
				// Получаем буфер сообщения
				const auto & buffer = this->_frame.methods.message(mess);
				// Если данные сообщения получены
				if((this->_stopped = !buffer.empty())){
					// Выводим сообщение об ошибке
					this->error(mess);
					// Выполняем отправку сообщения на сервер
					if(core->send(buffer.data(), buffer.size(), this->_bid)){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим заголовок ответа
							std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ SEND ERROR ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
							// Выводим отправляемое сообщение
							std::cout << this->_fmk->format("%s [%u]", mess.text.c_str(), mess.code) << std::endl << std::endl << std::flush;
						#endif
						// Выходим из функции
						return;
					}
				}
			}
			// Завершаем работу
			core->close(this->_bid);
		}
	}
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::client::Websocket1::sendMessage(const vector <char> & message, const bool text) noexcept {
	// Выполняем отправку сообщения на сервер
	return this->sendMessage(message.data(), message.size(), text);
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param size    размер передаваемого сообещния
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::client::Websocket1::sendMessage(const char * message, const size_t size, const bool text) noexcept {
	// Результат работы функции
	bool result = false;
	// Создаём объект холдирования
	hold_t <event_t> hold(this->_events);
	// Если событие соответствует разрешённому
	if(hold.access({event_t::CONNECT, event_t::READ}, event_t::SEND)){
		// Если подключение выполнено
		if((this->_core != nullptr) && this->_core->working() && this->_allow.send){
			// Выполняем блокировку отправки сообщения
			this->_allow.send = !this->_allow.send;
			// Если рукопожатие выполнено
			if((message != nullptr) && (size > 0) && this->_http.handshake(http_t::process_t::RESPONSE) && (this->_bid > 0)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим заголовок ответа
					std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ SEND MESSAGE ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
					// Если отправляемое сообщение является текстом
					if(text)
						// Выводим параметры ответа
						std::cout << string(message, size) << std::endl << std::endl << std::flush;
					// Выводим сообщение о выводе чанка полезной нагрузки
					else std::cout << this->_fmk->format("<bytes %zu>", size) << std::endl << std::endl << std::flush;
				#endif
				// Бинарный буфер для отправки
				vector <char> buffer;
				// Создаём объект заголовка для отправки
				ws::frame_t::head_t head(true, true);
				// Устанавливаем опкод сообщения
				head.optcode = (text ? ws::frame_t::opcode_t::TEXT : ws::frame_t::opcode_t::BINARY);
				// Указываем, что сообщение передаётся в сжатом виде
				head.rsv[0] = ((size >= 1024) && (this->_compressor != http_t::compressor_t::NONE));
				// Если необходимо сжимать сообщение перед отправкой
				if(head.rsv[0]){
					// Определяем метод компрессии
					switch(static_cast <uint8_t> (this->_compressor)){
						// Если метод компрессии выбран LZ4
						case static_cast <uint8_t> (http_t::compressor_t::LZ4):
							// Выполняем компрессию полученных данных
							this->_hash.compress(message, size, hash_t::method_t::LZ4, buffer);
						break;
						// Если метод компрессии выбран Zstandard
						case static_cast <uint8_t> (http_t::compressor_t::ZSTD):
							// Выполняем компрессию полученных данных
							this->_hash.compress(message, size, hash_t::method_t::ZSTD, buffer);
						break;
						// Если метод компрессии выбран LZma
						case static_cast <uint8_t> (http_t::compressor_t::LZMA):
							// Выполняем компрессию полученных данных
							this->_hash.compress(message, size, hash_t::method_t::LZMA, buffer);
						break;
						// Если метод компрессии выбран Brotli
						case static_cast <uint8_t> (http_t::compressor_t::BROTLI):
							// Выполняем компрессию полученных данных
							this->_hash.compress(message, size, hash_t::method_t::BROTLI, buffer);
						break;
						// Если метод компрессии выбран BZIP2
						case static_cast <uint8_t> (http_t::compressor_t::BZIP2):
							// Выполняем компрессию полученных данных
							this->_hash.compress(message, size, hash_t::method_t::BZIP2, buffer);
						break;
						// Если метод компрессии выбран GZip
						case static_cast <uint8_t> (http_t::compressor_t::GZIP):
							// Выполняем компрессию полученных данных
							this->_hash.compress(message, size, hash_t::method_t::GZIP, buffer);
						break;
						// Если метод компрессии выбран Deflate
						case static_cast <uint8_t> (http_t::compressor_t::DEFLATE): {
							// Устанавливаем размер скользящего окна
							this->_hash.wbit(this->_client.wbit);
							// Выполняем компрессию полученных данных
							this->_hash.compress(message, size, hash_t::method_t::DEFLATE, buffer);
							// Удаляем хвост в полученных данных
							this->_hash.rmTail(buffer);
						} break;
					}
					// Если сжатие данных не выполнено
					if(buffer.empty()){
						// Снимаем флаг сжатых данных
						head.rsv[0] = !head.rsv[0];
						// Заполняем бинарный буфер данными в том виде как они пришли
						buffer.assign(message, message + size);
					}
				// Заполняем бинарный буфер данными в том виде как они пришли
				} else buffer.assign(message, message + size);
				// Если нужно производить шифрование
				if(this->_crypted)
					// Выполняем шифрование полезной нагрузки
					buffer = this->_hash.encode <vector <char>> (buffer.data(), buffer.size(), this->_cipher);
				// Если требуется фрагментация сообщения
				if(buffer.size() > this->_frame.size){
					// Смещение в бинарном буфере и актуальный размер блока
					size_t offset = 0, actual = 0;
					// Выполняем разбивку полезной нагрузки на сегменты
					while(offset < buffer.size()){
						// Поулчаем количество оставшихся байт в буфере
						actual = (buffer.size() - offset);
						// Выполняем получение актуального размера отправляемых данных
						actual = ((actual > this->_frame.size) ? this->_frame.size : actual);
						// Устанавливаем флаг финального сообщения
						head.fin = ((offset + actual) == buffer.size());
						// Создаём буфер для отправки
						const auto & payload = this->_frame.methods.set(head, buffer.data() + offset, actual);
						// Увеличиваем смещение в буфере
						offset += actual;
						// Если бинарный буфер для отправки данных получен
						if(!payload.empty())
							// Выполняем отправку сообщения на сервер
							result = const_cast <client::core_t *> (this->_core)->send(payload.data(), payload.size(), this->_bid);
						// Выполняем сброс RSV1
						head.rsv[0] = false;
						// Устанавливаем опкод сообщения
						head.optcode = ws::frame_t::opcode_t::CONTINUATION;
						// Если запрос не отправлен
						if(!result)
							// Выходим из цикла
							break;
					}
				// Если фрагментация сообщения не требуется
				} else {
					// Создаём буфер для отправки
					const auto & payload = this->_frame.methods.set(head, buffer.data(), buffer.size());
					// Если бинарный буфер для отправки данных получен
					if(!payload.empty())
						// Отправляем серверу сообщение
						result = const_cast <client::core_t *> (this->_core)->send(payload.data(), payload.size(), this->_bid);
				}
			}
			// Выполняем разблокировку отправки сообщения
			this->_allow.send = !this->_allow.send;
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
bool awh::client::Websocket1::send(const char * buffer, const size_t size) noexcept {
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
void awh::client::Websocket1::pause() noexcept {
	// Ставим работу клиента на паузу
	this->_freeze = true;
}
/**
 * stop Метод остановки клиента
 */
void awh::client::Websocket1::stop() noexcept {
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
			// Считываем значение флага
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
void awh::client::Websocket1::start() noexcept {
	// Разрешаем чтение данных из буфера
	this->_reading = true;
	// Выполняем очистку буфера данных
	this->_buffer.clear();
	// Если адрес URL запроса передан
	if(!this->_freeze && !this->_scheme.url.empty()){
		// Если биндинг не запущен, выполняем запуск биндинга
		if((this->_core != nullptr) && !this->_core->working())
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
 * @param sec время ожидания в секундах
 */
void awh::client::Websocket1::waitPong(const uint16_t sec) noexcept {
	// Выполняем установку времени ожидания
	this->_waitPong = (static_cast <uint32_t> (sec) * 1000);
}
/**
 * pingInterval Метод установки интервала времени выполнения пингов
 * @param sec интервал времени выполнения пингов в секундах
 */
void awh::client::Websocket1::pingInterval(const uint16_t sec) noexcept {
	// Выполняем установку интервала времени выполнения пингов в секундах
	this->_pingInterval = (static_cast <uint32_t> (sec) * 1000);
}
/**
 * callback Метод установки функций обратного вызова
 * @param callback функции обратного вызова
 */
void awh::client::Websocket1::callback(const callback_t & callback) noexcept {
	// Выполняем добавление функций обратного вызова в основноной модуль
	web_t::callback(callback);
	// Если функция обратного вызова на перехват полученных чанков установлена
	if(web_t::_callback.is("chunking"))
		// Устанавливаем внешнюю функцию обратного вызова
		this->_http.on <void (const int32_t, const uint64_t, const vector <char> &)> ("chunking", web_t::_callback.get <void (const int32_t, const uint64_t, const vector <char> &)> ("chunking"));
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	else this->_http.on <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", &ws1_t::chunking, this, _1, _2, _3);
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::client::Websocket1::subprotocol(const string & subprotocol) noexcept {
	// Если поддерживаемый сабпротокол передан
	if(!subprotocol.empty())
		// Устанавливаем поддерживаемый сабподпротокол
		this->_http.subprotocol(subprotocol);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @return список выбранных сабпротоколов
 */
const std::unordered_set <string> & awh::client::Websocket1::subprotocols() const noexcept {
	// Выводим список выбранных сабпротоколов
	return this->_http.subprotocols();
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::client::Websocket1::subprotocols(const std::unordered_set <string> & subprotocols) noexcept {
	// Если список поддерживаемых сабпротоколов получен
	if(!subprotocols.empty())
		// Устанавливаем список поддерживаемых сабпротоколов
		this->_http.subprotocols(subprotocols);
}
/**
 * extensions Метод извлечения списка расширений
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::client::Websocket1::extensions() const noexcept {
	// Выводим список доступных расширений
	return this->_http.extensions();
}
/**
 * extensions Метод установки списка расширений
 * @param extensions список поддерживаемых расширений
 */
void awh::client::Websocket1::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка доступных расширений
	this->_http.extensions(extensions);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::Websocket1::chunk(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->_http.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма
 * @param size минимальный размер сегмента
 */
void awh::client::Websocket1::segmentSize(const size_t size) noexcept {
	// Если размер сегмента фрейма передан
	if(size > 0)
		// Устанавливаем размер одного сегмента фрейма
		this->_frame.size = size;
	// Иначе устанавливаем размер сегментов по умолчанию
	else this->_frame.size = AWH_CHUNK_SIZE;
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::client::Websocket1::core(const client::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем передачу настроек сетевого ядра в родительский модуль
		web_t::core(core);
		// Если многопоточность активированна
		if(this->_thr.is())
			// Устанавливаем простое чтение базы событий
			const_cast <client::core_t *> (this->_core)->easily(true);
		// Устанавливаем функцию записи данных
		const_cast <client::core_t *> (this->_core)->on <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", &ws1_t::writeEvent, this, _1, _2, _3, _4);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Если многопоточность активированна
		if(this->_thr.is()){
			// Выполняем завершение всех активных потоков
			this->_thr.stop();
			// Снимаем режим простого чтения базы событий
			const_cast <client::core_t *> (this->_core)->easily(false);
		}
		// Выполняем передачу настроек сетевого ядра в родительский модуль
		web_t::core(core);
	}
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::Websocket1::mode(const std::set <flag_t> & flags) noexcept {
	// Устанавливаем флаг разрешающий вывод информационных сообщений
	this->_verb = (flags.find(flag_t::NOT_INFO) == flags.end());
	// Активируем выполнение пинга
	this->_pinging = (flags.find(flag_t::NOT_PING) == flags.end());
	// Если установлен флаг запрещающий переключение контекста SSL
	this->_nossl = (flags.find(flag_t::NO_INIT_SSL) != flags.end());
	// Устанавливаем флаг анбиндинга ядра сетевого модуля
	this->_complete = (flags.find(flag_t::NOT_STOP) == flags.end());
	// Устанавливаем флаг разрешающий выполнять редиректы
	this->_redirects = (flags.find(flag_t::REDIRECTS) != flags.end());
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flags.find(flag_t::ALIVE) != flags.end());
	// Устанавливаем флаг перехвата контекста компрессии для клиента
	this->_client.takeover = (flags.find(flag_t::TAKEOVER_CLIENT) != flags.end());
	// Устанавливаем флаг перехвата контекста компрессии для сервера
	this->_server.takeover = (flags.find(flag_t::TAKEOVER_SERVER) != flags.end());
	// Устанавливаем флаг разрешающий выполнять метод CONNECT для прокси-клиента
	this->_proxy.connect = (flags.find(flag_t::CONNECT_METHOD_ENABLE) != flags.end());
	// Если сетевое ядро установлено
	if(this->_core != nullptr)
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <client::core_t *> (this->_core)->verbose(flags.find(flag_t::NOT_INFO) == flags.end());
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::Websocket1::user(const string & login, const string & password) noexcept {
	// Устанавливаем логин и пароль пользователя
	this->_http.user(login, password);
}
/**
 * setHeaders Метод установки списка заголовков
 * @param headers список заголовков для установки
 */
void awh::client::Websocket1::setHeaders(const std::unordered_multimap <string, string> & headers) noexcept {
	// Выполняем установку HTTP-заголовков для отправки на сервер
	this->_headers = headers;
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::client::Websocket1::userAgent(const string & userAgent) noexcept {
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
void awh::client::Websocket1::ident(const string & id, const string & name, const string & ver) noexcept {
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
void awh::client::Websocket1::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Если нужно активировать многопоточность
	if(mode){
		// Если многопоточность ещё не активированна
		if(!this->_thr.is())
			// Выполняем инициализацию пула потоков
			this->_thr.init(count);
		// Если многопоточность уже активированна
		else {
			// Выполняем завершение всех активных потоков
			this->_thr.stop();
			// Выполняем инициализацию нового тредпула
			this->_thr.init(count);
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
void awh::client::Websocket1::proxy(const client::scheme_t::work_t work) noexcept {
	// Устанавливаем флаг активации/деактивации прокси-клиента
	web_t::proxy(work);
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / IPC)
 */
void awh::client::Websocket1::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку параметры прокси-сервера
	web_t::proxy(uri, family);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Websocket1::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем параметры авторизации для HTTP-клиента
	this->_http.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Websocket1::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации на проксе-сервере
	web_t::authTypeProxy(type, hash);
}
/**
 * crypted Метод получения флага шифрования
 * @return результат проверки
 */
bool awh::client::Websocket1::crypted() const noexcept {
	// Выполняем получение флага шифрования
	return this->_http.crypted();
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::client::Websocket1::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования в родительском модуле
	web_t::encryption(mode);
	// Устанавливаем флаг шифрования для HTTP-клиента
	this->_http.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::Websocket1::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем параметры шифрования в родительском модуле
	web_t::encryption(pass, salt, cipher);
	// Устанавливаем параметры шифрования для HTTP-клиента
	this->_http.encryption(pass, salt, cipher);
}
/**
 * Websocket1 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Websocket1::Websocket1(const fmk_t * fmk, const log_t * log) noexcept :
 web_t(fmk, log), _sid(-1), _rid(0), _verb(true), _close(false),
 _shake(false), _freeze(false), _crypted(false), _inflate(false),
 _waitPong(_pingInterval * 2), _respPong(0), _http(fmk, log), _hash(log), _frame(fmk, log),
 _cipher(hash_t::cipher_t::AES128), _callback(log), _compressor(awh::http_t::compressor_t::NONE) {
	// Устанавливаем функцию обработки вызова для вывода полученного заголовка с сервера
	this->_http.on <void (const uint64_t, const string &, const string &)> ("header", &ws1_t::header, this, _1, _2, _3);
	// Устанавливаем функцию обработки вызова для вывода ответа сервера на ранее выполненный запрос
	this->_http.on <void (const uint64_t, const uint32_t, const string &)> ("response", &ws1_t::response, this, _1, _2, _3);
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.on <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", &ws1_t::chunking, this, _1, _2, _3);
	// Устанавливаем функцию обработки вызова на событие получения ошибок
	this->_http.on <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", &ws1_t::errors, this, _1, _2, _3, _4);
	// Устанавливаем функцию обработки вызова для вывода полученных заголовков с сервера
	this->_http.on <void (const uint64_t, const uint32_t, const string &, const std::unordered_multimap <string, string> &)> ("headersResponse", &ws1_t::headers, this, _1, _2, _3, _4);
}
/**
 * Websocket1 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Websocket1::Websocket1(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 web_t(core, fmk, log), _sid(-1), _rid(0), _verb(true), _close(false),
 _shake(false), _freeze(false), _crypted(false), _inflate(false),
 _waitPong(_pingInterval * 2), _respPong(0), _http(fmk, log), _hash(log), _frame(fmk, log),
 _callback(log), _cipher(hash_t::cipher_t::AES128), _compressor(awh::http_t::compressor_t::NONE) {
	// Устанавливаем функцию обработки вызова для вывода полученного заголовка с сервера
	this->_http.on <void (const uint64_t, const string &, const string &)> ("header", &ws1_t::header, this, _1, _2, _3);
	// Устанавливаем функцию обработки вызова для вывода ответа сервера на ранее выполненный запрос
	this->_http.on <void (const uint64_t, const uint32_t, const string &)> ("response", &ws1_t::response, this, _1, _2, _3);
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_http.on <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", &ws1_t::chunking, this, _1, _2, _3);
	// Устанавливаем функцию обработки вызова на событие получения ошибок
	this->_http.on <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", &ws1_t::errors, this, _1, _2, _3, _4);
	// Устанавливаем функцию обработки вызова для вывода полученных заголовков с сервера
	this->_http.on <void (const uint64_t, const uint32_t, const string &, const std::unordered_multimap <string, string> &)> ("headersResponse", &ws1_t::headers, this, _1, _2, _3, _4);
	// Устанавливаем функцию записи данных
	const_cast <client::core_t *> (this->_core)->on <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", &ws1_t::writeEvent, this, _1, _2, _3, _4);
}
/**
 * ~Websocket1 Деструктор
 */
awh::client::Websocket1::~Websocket1() noexcept {
	// Если многопоточность активированна
	if(this->_thr.is())
		// Выполняем завершение всех активных потоков
		this->_thr.stop();
}
