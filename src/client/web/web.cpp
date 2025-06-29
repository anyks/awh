/**
 * @file: web.cpp
 * @date: 2023-09-11
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
#include <client/web/web.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * Оператор [=] перемещения параметров запроса
 * @param request объект параметров запроса
 * @return        объект текущего запроса
 */
awh::client::Web::Request & awh::client::Web::Request::operator = (request_t && request) noexcept {
	// Выполняем установку идентификатора запроса
	this->id = request.id;
	// Выполняем установку агента воркера выполняющего запрос
	this->agent = request.agent;
	// Выполняем установку метода запроса
	this->method = request.method;
	// Выполняем перемещение URL-адреса
	this->url = ::move(request.url);
	// Выполняем перемещение тела запроса
	this->entity = ::move(request.entity);
	// Выполняем перемещение заголовков запроса
	this->headers = ::move(request.headers);
	// Выполняем перемещение списка компрессоров
	this->compressors = ::move(request.compressors);
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор [=] присванивания параметров запроса
 * @param request объект параметров запроса
 * @return        объект текущего запроса
 */
awh::client::Web::Request & awh::client::Web::Request::operator = (const request_t & request) noexcept {
	// Выполняем установку идентификатора запроса
	this->id = request.id;
	// Выполняем установку агента воркера выполняющего запрос
	this->agent = request.agent;
	// Выполняем установку метода запроса
	this->method = request.method;
	// Выполняем копирование URL-адреса
	this->url = request.url;
	// Выполняем копирование тела запроса
	this->entity = request.entity;
	// Выполняем копирование заголовков запроса
	this->headers = request.headers;
	// Выполняем копирование списка компрессоров
	this->compressors = request.compressors;
	// Выводим текущий объект
	return (* this);
}
/**
 * Оператор сравнения
 * @param request объект параметров запроса
 * @return        результат сравнения
 */
bool awh::client::Web::Request::operator == (const request_t & request) noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем сравнение основных параметров
	result = (
		(this->id == request.id) &&
		(this->agent == request.agent) &&
		(this->method == request.method) &&
		(this->url == request.url)
	);
	// Если параметры совпадают
	if(result){
		// Выполняем справнение тела запроса
		if((result = (this->entity.size() == request.entity.size())) && !this->entity.empty()){
			// Выполняем проверку соответствия содержимого
			result = (::memcmp(this->entity.data(), request.entity.data(), this->entity.size()) == 0);
			// Если проверка не пройдена
			if(!result)
				// Выходим из функции
				return result;
		}
		// Если проверка тела запроса пройдена
		if(result){
			// Если количество компрессоров совпадает
			if((result = (this->compressors.size() == request.compressors.size()))){
				// Выполняем перебор всех компрессоров
				for(size_t i = 0; i < this->compressors.size(); i++){
					// Выполняем сравнение компрессоров
					result = (this->compressors.at(i) == request.compressors.at(i));
					// Если компрессоры не совпадают
					if(!result)
						// Выходим из функции
						return result;
				}
			}
			// Если проверка тела запроса пройдена
			if(result){
				// Если список заголовков совпадает
				if((result = (this->headers.size() == request.headers.size()))){
					// Выполняем перебор всего списка заголовков
					for(auto & header : this->headers){
						// Выполняем поиск заголовка
						auto i = request.headers.find(header.first);
						// Если заголовок найден
						if((result = (i != request.headers.end()))){
							// Выполняем сравнение содержимого заголовков
							result = (header.second.compare(i->second) == 0);
							// Если компрессоры не совпадают
							if(!result)
								// Выходим из функции
								return result;
						// Выходим из функции
						} else return result;
					}
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * Request Конструктор перемещения
 * @param request объект параметров запроса
 */
awh::client::Web::Request::Request(request_t && request) noexcept {
	// Выполняем установку идентификатора запроса
	this->id = request.id;
	// Выполняем установку агента воркера выполняющего запрос
	this->agent = request.agent;
	// Выполняем установку метода запроса
	this->method = request.method;
	// Выполняем перемещение URL-адреса
	this->url = ::move(request.url);
	// Выполняем перемещение тела запроса
	this->entity = ::move(request.entity);
	// Выполняем перемещение заголовков запроса
	this->headers = ::move(request.headers);
	// Выполняем перемещение списка компрессоров
	this->compressors = ::move(request.compressors);
}
/**
 * Request Конструктор копирования
 * @param request объект параметров запроса
 */
awh::client::Web::Request::Request(const request_t & request) noexcept {
	// Выполняем установку идентификатора запроса
	this->id = request.id;
	// Выполняем установку агента воркера выполняющего запрос
	this->agent = request.agent;
	// Выполняем установку метода запроса
	this->method = request.method;
	// Выполняем копирование URL-адреса
	this->url = request.url;
	// Выполняем копирование тела запроса
	this->entity = request.entity;
	// Выполняем копирование заголовков запроса
	this->headers = request.headers;
	// Выполняем копирование списка компрессоров
	this->compressors = request.compressors;
}
/**
 * Request Конструктор
 */
awh::client::Web::Request::Request() noexcept : id(0), agent(agent_t::HTTP), method(awh::web_t::method_t::NONE) {}
/**
 * openEvent Метод обратного вызова при запуске работы
 * @param sid идентификатор схемы сети
 */
void awh::client::Web::openEvent(const uint16_t sid) noexcept {
	// Если данные переданы верные
	if(sid > 0){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::READ, event_t::CONNECT}, event_t::OPEN)){
			// Если подключение уже выполнено
			if(this->_scheme.status.real == scheme_t::mode_t::CONNECT){
				// Если подключение производится через, прокси-сервер
				if(this->_scheme.isProxy())
					// Выполняем запуск функции подключения для прокси-сервера
					this->proxyConnectEvent(this->_bid, sid);
				// Выполняем запуск функции подключения
				else this->connectEvent(this->_bid, sid);
			// Если биндинг уже запущен, выполняем запрос на сервер
			} else const_cast <client::core_t *> (this->_core)->open(sid);
		}
	}
}
/**
 * statusEvent Метод обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 */
void awh::client::Web::statusEvent(const awh::core_t::status_t status) noexcept {
	// Определяем статус активности сетевого ядра
	switch(static_cast <uint8_t> (status)){
		// Если система запущена
		case static_cast <uint8_t> (awh::core_t::status_t::START): {
			// Если разрешено выполнять пинги
			if(this->_pinging){
				// Выполняем биндинг ядра локального таймера выполнения пинга
				const_cast <client::core_t *> (this->_core)->bind(&this->_timer);
				// Устанавливаем интервал времени на выполнения пинга удалённого сервера
				const uint16_t tid = this->_timer.interval(this->_pingInterval);
				// Выполняем добавление функции обратного вызова
				this->_timer.on(tid, &web_t::pinging, this, tid);
			}
		} break;
		// Если система остановлена
		case static_cast <uint8_t> (awh::core_t::status_t::STOP): {
			// Если разрешено выполнять пинги
			if(this->_pinging){
				// Останавливаем все установленные таймеры
				this->_timer.clear();
				// Выполняем анбиндинг ядра локального таймера выполнения пинга
				const_cast <client::core_t *> (this->_core)->unbind(&this->_timer);
			}
		} break;
	}
	// Если функция получения событий запуска и остановки сетевого ядра установлена
	if(this->_callback.is("status"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const awh::core_t::status_t)> ("status", status);
}
/**
 * proxyConnectEvent Метод обратного вызова при подключении к прокси-серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::client::Web::proxyConnectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::OPEN, event_t::PROXY_READ, event_t::PROXY_CONNECT}, event_t::PROXY_CONNECT)){
			// Запоминаем идентификатор брокера
			this->_bid = bid;
			// Определяем тип прокси-сервера
			switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
				// Если прокси-сервер является Socks5
				case static_cast <uint8_t> (client::proxy_t::type_t::SOCKS5): {
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
						const_cast <client::core_t *> (this->_core)->send(buffer.data(), buffer.size(), bid);
				} break;
				// Если прокси-сервер является HTTP
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTP):
				// Если прокси-сервер является HTTPS
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTPS): {
					// Если протокол активирован HTTPS или WSS защищённый поверх SSL
					if(this->_proxy.connect){
						// Выполняем сброс состояния HTTP-парсера
						this->_scheme.proxy.http.reset();
						// Выполняем очистку параметров HTTP-запроса
						this->_scheme.proxy.http.clear();
						// Создаём объек запроса
						awh::web_t::req_t request(awh::web_t::method_t::CONNECT, this->_scheme.url);
						// Получаем бинарные данные WEB запроса
						const auto & buffer = this->_scheme.proxy.http.proxy(request);
						// Если бинарные данные запроса получены
						if(!buffer.empty()){
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								// Выводим заголовок запроса
								std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST PROXY ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
								// Выводим параметры запроса
								std::cout << string(buffer.begin(), buffer.end()) << std::endl << std::endl << std::flush;
							#endif
							// Выполняем отправку сообщения на сервер
							const_cast <client::core_t *> (this->_core)->send(buffer.data(), buffer.size(), bid);
						}
					// Если протокол подключения не является защищённым подключением
					} else {
						// Выполняем очистку буфера данных
						this->_buffer.clear();
						// Если защищённое подключение уже активированно
						if(this->_scheme.proxy.type == client::proxy_t::type_t::HTTPS){
							// Выполняем переключение на работу с сервером
							this->_scheme.switchConnect();
							// Выполняем запуск работы основного модуля
							this->connectEvent(bid, sid);
						// Выполняем переключение на работу с сервером
						} else const_cast <client::core_t *> (this->_core)->switchProxy(bid);
					}
				} break;
				// Иначе завершаем работу
				default: const_cast <client::core_t *> (this->_core)->close(bid);
			}
		}
	}
}
/**
 * proxyReadEvent Метод обратного вызова при чтении сообщения с прокси-сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::client::Web::proxyReadEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Создаём объект холдирования
		hold_t <event_t> hold(this->_events);
		// Если событие соответствует разрешённому
		if(hold.access({event_t::PROXY_CONNECT, event_t::PROXY_READ}, event_t::PROXY_READ)){
			// Добавляем полученные данные в буфер
			this->_buffer.push(buffer, size);
			// Определяем тип прокси-сервера
			switch(static_cast <uint8_t> (this->_scheme.proxy.type)){
				// Если прокси-сервер является Socks5
				case static_cast <uint8_t> (client::proxy_t::type_t::SOCKS5): {
					// Если данные не получены
					if(!this->_scheme.proxy.socks5.is(socks5_t::state_t::END)){
						// Выполняем парсинг входящих данных
						this->_scheme.proxy.socks5.parse(reinterpret_cast <const char *> (this->_buffer.get()), this->_buffer.size());
						// Получаем данные запроса
						const auto & buffer = this->_scheme.proxy.socks5.get();
						// Если данные получены
						if(!buffer.empty()){
							// Выполняем очистку буфера данных
							this->_buffer.clear();
							// Выполняем отправку запроса на сервер
							const_cast <client::core_t *> (this->_core)->send(buffer.data(), buffer.size(), bid);
							// Завершаем работу
							return;
						// Если данные все получены
						} else if(this->_scheme.proxy.socks5.is(socks5_t::state_t::END)) {
							// Выполняем очистку буфера данных
							this->_buffer.clear();
							// Если рукопожатие выполнено
							if(this->_scheme.proxy.socks5.is(socks5_t::state_t::HANDSHAKE)){
								// Выполняем переключение на работу с сервером
								const_cast <client::core_t *> (this->_core)->switchProxy(bid);
								// Выполняем запуск работы основного модуля
								this->connectEvent(bid, sid);
								// Завершаем работу
								return;
							// Если рукопожатие не выполнено
							} else {
								// Устанавливаем код ответа
								const uint32_t code = this->_scheme.proxy.socks5.code();
								// Устанавливаем сообщение ответа
								const string & message = this->_scheme.proxy.socks5.message(code);
								/**
								 * Если включён режим отладки
								 */
								#if defined(DEBUG_MODE)
									// Если заголовки получены
									if(!message.empty()){
										// Данные WEB ответа
										const string & answer = this->_fmk->format("SOCKS5 %u %s\r\n", code, message.c_str());
										// Выводим заголовок ответа
										std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
										// Выводим параметры ответа
										std::cout << string(answer.begin(), answer.end()) << std::endl << std::endl << std::flush;
									}
								#endif
								// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
								if(this->_callback.is("response"))
									// Выполняем функцию обратного вызова
									this->_callback.call <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", 1, 0, code, message);
								// Завершаем работу
								const_cast <client::core_t *> (this->_core)->close(bid);
								// Завершаем работу
								return;
							}
						}
					}
				} break;
				// Если прокси-сервер является HTTP
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTP):
				// Если прокси-сервер является HTTPS
				case static_cast <uint8_t> (client::proxy_t::type_t::HTTPS): {
					// Выполняем обработку полученных данных
					while(this->_reading){
						// Выполняем парсинг полученных данных
						const size_t bytes = this->_scheme.proxy.http.parse(reinterpret_cast <const char *> (this->_buffer.get()), this->_buffer.size());
						// Если все данные получены
						if((bytes > 0) && this->_scheme.proxy.http.is(http_t::state_t::END)){
							// Выполняем очистку буфера данных
							this->_buffer.clear();
							/**
							 * Если включён режим отладки
							 */
							#if defined(DEBUG_MODE)
								{
									// Получаем данные ответа
									const auto & response = this->_scheme.proxy.http.process(http_t::process_t::RESPONSE, this->_scheme.proxy.http.response());
									// Если параметры ответа получены
									if(!response.empty()){
										// Выводим заголовок ответа
										std::cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE PROXY ^^^^^^^^^\x1B[0m" << std::endl << std::flush;
										// Выводим параметры ответа
										std::cout << string(response.begin(), response.end()) << std::endl << std::endl << std::flush;
										// Если тело ответа существует
										if(!this->_scheme.proxy.http.empty(awh::http_t::suite_t::BODY))
											// Выводим сообщение о выводе чанка тела
											std::cout << this->_fmk->format("<body %u>", this->_scheme.proxy.http.body().size()) << std::endl << std::endl << std::flush;
									}
								}
							#endif
							// Получаем параметры запроса
							const auto & response = this->_scheme.proxy.http.response();
							// Получаем статус ответа
							awh::http_t::status_t status = this->_scheme.proxy.http.auth();
							// Устанавливаем ответ прокси-сервера
							this->_proxy.answer = response.code;
							// Если выполнять редиректы запрещено
							if(!this->_redirects && (status == awh::http_t::status_t::RETRY)){
								// Если ответом сервера является положительным
								if(response.code == 200)
									// Запрещаем выполнять редирект
									status = awh::http_t::status_t::GOOD;
							}
							// Если функция обратного вызова получения статуса ответа установлена
							if(this->_callback.is("answer"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const int32_t, const uint64_t, const awh::http_t::status_t)> ("answer", 1, 0, status);
							// Выполняем проверку авторизации
							switch(static_cast <uint8_t> (status)){
								// Если нужно попытаться ещё раз
								case static_cast <uint8_t> (awh::http_t::status_t::RETRY): {
									// Если попытки повторить переадресацию ещё не закончились
									if(!(this->_stopped = (this->_attempt >= this->_attempts))){
										// Если адрес запроса получен
										if(!this->_scheme.proxy.url.empty()){
											// Если соединение является постоянным
											if(this->_scheme.proxy.http.is(http_t::state_t::ALIVE)){
												// Увеличиваем количество попыток
												this->_attempt++;
												// Устанавливаем новый экшен выполнения
												this->proxyConnectEvent(bid, sid);
											// Если соединение должно быть закрыто
											} else const_cast <client::core_t *> (this->_core)->close(bid);
											// Завершаем работу
											return;
										}
									}
								} break;
								// Если запрос выполнен удачно
								case static_cast <uint8_t> (awh::http_t::status_t::GOOD): {
									// Если защищённое подключение уже активированно
									if(this->_scheme.proxy.type == client::proxy_t::type_t::HTTPS)
										// Выполняем переключение на работу с сервером
										this->_scheme.switchConnect();
									// Выполняем переключение на работу с сервером
									else const_cast <client::core_t *> (this->_core)->switchProxy(bid);
									// Выполняем запуск работы основного модуля
									this->connectEvent(bid, sid);
									// Завершаем работу
									return;
								} break;
								// Если запрос неудачный
								case static_cast <uint8_t> (awh::http_t::status_t::FAULT):
									// Устанавливаем флаг принудительной остановки
									this->_stopped = true;
								break;
							}
							// Если функция обратного вызова активности потока установлена
							if(this->_callback.is("stream"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const int32_t, const uint64_t, const mode_t)> ("stream", 1, 0, mode_t::CLOSE);
							// Если функция обратного вызова на вывод ответа сервера на ранее выполненный запрос установлена
							if(this->_callback.is("response"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", 1, 0, response.code, response.message);
							// Если функция обратного вызова на вывод полученных заголовков с сервера установлена
							if(this->_callback.is("headers"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const int32_t, const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headers", 1, 0, response.code, response.message, this->_scheme.proxy.http.headers());
							// Если функция обратного вызова на вывод полученного тела сообщения с сервера установлена
							if(!this->_scheme.proxy.http.empty(awh::http_t::suite_t::BODY) && this->_callback.is("entity"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &)> ("entity", 1, 0, response.code, response.message, this->_scheme.proxy.http.body());
							// Если функция обратного вызова на вывод полученных данных ответа сервера установлена
							if(this->_callback.is("complete"))
								// Выполняем функцию обратного вызова
								this->_callback.call <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", 1, 0, response.code, response.message, this->_scheme.proxy.http.body(), this->_scheme.proxy.http.headers());
							// Завершаем работу
							const_cast <client::core_t *> (this->_core)->close(bid);
							// Завершаем работу
							return;
						}
						// Если парсер обработал какое-то количество байт
						if((bytes > 0) && !this->_buffer.empty()){
							// Если размер буфера больше количества удаляемых байт
							if(this->_buffer.size() >= bytes)
								// Удаляем количество обработанных байт
								this->_buffer.erase(bytes);
						}
						// Если данные мы все получили, выходим
						if(this->_buffer.empty())
							// Выходим из цикла
							break;
					}
				} break;
				// Иначе завершаем работу
				default: const_cast <client::core_t *> (this->_core)->close(bid);
			}
		}
	}
}
/**
 * enableSSLEvent Метод активации зашифрованного канала SSL
 * @param url адрес сервера для которого выполняется активация зашифрованного канала SSL
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 * @return    результат активации зашифрованного канала SSL
 */
bool awh::client::Web::enableSSLEvent(const uri_t::url_t & url, [[maybe_unused]] const uint64_t bid, [[maybe_unused]] const uint16_t sid) noexcept {
	// Выполняем проверку, выполняется подключение к серверу в защищённом рижеме или нет
	return (!this->_nossl && (!url.empty() && (this->_fmk->compare(url.schema, "https") || this->_fmk->compare(url.schema, "wss"))));
}
/**
 * chunking Метод обработки получения чанков
 * @param bid   идентификатор брокера
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::client::Web::chunking([[maybe_unused]] const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()){
		// Выполняем добавление полученного чанка в тело ответа
		const_cast <awh::http_t *> (http)->body(chunk);
		// Если функция обратного вызова на вывода полученного чанка бинарных данных с сервера установлена
		if(this->_callback.is("chunks"))
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const int32_t, const uint64_t, const vector <char> &)> ("chunks", 1, 0, chunk);
	}
}
/**
 * errors Метод вывода полученных ошибок протокола
 * @param bid     идентификатор брокера
 * @param flag    флаг типа сообщения
 * @param error   тип полученной ошибки
 * @param message сообщение полученной ошибки
 */
void awh::client::Web::errors([[maybe_unused]] const uint64_t bid, const log_t::flag_t flag, const awh::http::error_t error, const string & message) noexcept {
	// Если функция обратного вызова на на вывод ошибок установлена
	if(this->_callback.is("error"))
		// Выполняем функцию обратного вызова
		this->_callback.call <void (const log_t::flag_t, const http::error_t, const string &)> ("error", flag, error, message);
}
/**
 * init Метод инициализации WEB клиента
 * @param dest        адрес назначения удалённого сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::client::Web::init(const string & dest, const vector <awh::http_t::compressor_t> & compressors) noexcept {
	// Если unix-сокет установлен
	if((this->_core != nullptr) && (this->_core->family() == scheme_t::family_t::NIX)){
		// Выполняем очистку схемы сети
		this->_scheme.clear();
		// Устанавливаем unix-сокет адрес в файловой системе
		this->_scheme.url = this->_uri.parse(this->_fmk->format("unix:%s.sock", dest.c_str()));
		/**
		 * Для операционной системы не являющейся OS Windows
		 */
		#if !defined(_WIN32) && !defined(_WIN64)
			// Выполняем установку unix-сокета 
			const_cast <client::core_t *> (this->_core)->sockname(dest);
		#endif
	// Выполняем установку unix-сокет
	} else {
		// Если адрес сервера передан
		if(!dest.empty()){
			// Выполняем очистку схемы сети
			this->_scheme.clear();
			// Устанавливаем URL-адрес запроса
			this->_scheme.url = this->_uri.parse(dest);
		}
	}
	// Если список компрессоров передан
	if(!compressors.empty())
		// Устанавливаем список поддерживаемых компрессоров
		this->_compressors = compressors;
}
/**
 * open Метод открытия подключения
 */
void awh::client::Web::open() noexcept {
	// Если сетевое ядро инициализировано
	if(this->_core != nullptr)
		// Выполняем открытие подключения на удалённом сервере
		this->openEvent(this->_scheme.id);
}
/**
 * reset Метод принудительного сброса подключения
 */
void awh::client::Web::reset() noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working())
		// Отправляем сигнал принудительного таймаута
		const_cast <client::core_t *> (this->_core)->reset(this->_bid);
}
/**
 * stop Метод остановки клиента
 */
void awh::client::Web::stop() noexcept {
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
void awh::client::Web::start() noexcept {
	// Разрешаем чтение данных из буфера
	this->_reading = true;
	// Выполняем очистку буфера данных
	this->_buffer.clear();
	// Если адрес URL запроса передан
	if((this->_core != nullptr) && !this->_scheme.url.empty()){
		// Если биндинг не запущен
		if(!this->_core->working())
			// Выполняем запуск биндинга
			const_cast <client::core_t *> (this->_core)->start();
		// Если биндинг уже запущен, выполняем запрос на сервер
		else this->open();
	}
}
/**
 * callback Метод установки функций обратного вызова
 * @param callback функции обратного вызова
 */
void awh::client::Web::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова для вывода бинарных данных в сыром виде полученных с сервера
	this->_callback.set("raw", callback);
	// Выполняем установку функции обратного вызова при завершении запроса
	this->_callback.set("end", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибки
	this->_callback.set("error", callback);
	// Выполняем установку функции обратного вызова на событие запуска или остановки подключения
	this->_callback.set("active", callback);
	// Выполняем установку функции обратного вызова для получения событий запуска и остановки сетевого ядра
	this->_callback.set("status", callback);
	// Выполняем установку функции обратного вызова завершения запроса
	this->_callback.set("result", callback);
	// Выполняем установку функции обратного вызова активности потока
	this->_callback.set("stream", callback);
	// Выполняем установку функции обратного вызова при получении источника подключения
	this->_callback.set("origin", callback);
	// Выполняем установку функции обратного вызова при получении альтернативных сервисов
	this->_callback.set("altsvc", callback);
	// Выполняем установку функции обратного вызова при получении ответа сервера
	this->_callback.set("answer", callback);
	// Выполняем установку функции обратного вызова для вывода полученного чанка бинарных данных с сервера
	this->_callback.set("chunks", callback);
	// Выполняем установку функции обратного вызова для вывода полученного заголовка с сервера
	this->_callback.set("header", callback);
	// Выполняем установку функции обратного вызова для вывода полученного тела данных с сервера
	this->_callback.set("entity", callback);
	// Выполняем установку функции обратного вызова для вывода полученных заголовков с сервера
	this->_callback.set("headers", callback);
	// Выполняем установку функции обратного вызова для вывода ответа сервера на ранее выполненный запрос
	this->_callback.set("response", callback);
	// Выполняем установку функции обратного вызова для перехвата полученных чанков
	this->_callback.set("chunking", callback);
	// Выполняем установку функции завершения выполнения запроса
	this->_callback.set("complete", callback);
	// Выполняем установку функции обратного вызова при выполнении рукопожатия
	this->_callback.set("handshake", callback);
	// Выполняем установку функции обратного вызова на событие получения ошибок Websocket
	this->_callback.set("errorWebsocket", callback);
	// Выполняем установку функции обратного вызова на событие получения сообщений Websocket
	this->_callback.set("messageWebsocket", callback);
}
/**
 * proto Метод извлечения поддерживаемого протокола подключения
 * @return поддерживаемый протокол подключения (HTTP1_1, HTTP2)
 */
awh::engine_t::proto_t awh::client::Web::proto() const noexcept {
	// Если сетевое ядро установлено
	if(this->_core != nullptr)
		// Выводим идентификатор активного HTTP-протокола
		return this->_core->proto(this->_bid);
	// Выводим протокол по умолчанию
	return engine_t::proto_t::NONE;
}
/**
 * cork Метод отключения/включения алгоритма TCP/CORK
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::client::Web::cork(const engine_t::mode_t mode) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr)
		// Выполняем отключение/включение алгоритма TCP/CORK
		return const_cast <client::core_t *> (this->_core)->cork(this->_bid, mode);
	// Сообщаем, что ничего не установлено
	return false;
}
/**
 * nodelay Метод отключения/включения алгоритма Нейгла
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::client::Web::nodelay(const engine_t::mode_t mode) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr)
		// Выполняем отключение/включение алгоритма Нейгла
		return const_cast <client::core_t *> (this->_core)->nodelay(this->_bid, mode);
	// Сообщаем, что ничего не установлено
	return false;
}
/**
 * bandwidth Метод установки пропускной способности сети
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::client::Web::bandwidth(const string & read, const string & write) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr)
		// Выполняем установку пропускной способности сети
		const_cast <client::core_t *> (this->_core)->bandwidth(this->_bid, read, write);
}
/**
 * waitMessage Метод ожидания входящих сообщений
 * @param sec интервал времени в секундах
 */
void awh::client::Web::waitMessage(const uint16_t sec) noexcept {
	// Если объект сетевого ядра установлен
	if((this->_core != nullptr) && (this->_bid > 0))
		// Устанавливаем время ожидания получения данных
		const_cast <client::core_t *> (this->_core)->waitMessage(this->_bid, sec);
	// Устанавливаем время ожидания получения данных
	else this->_scheme.timeouts.wait = sec;
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::Web::waitTimeDetect(const uint16_t read, const uint16_t write, const uint16_t connect) noexcept {
	// Если объект сетевого ядра установлен
	if((this->_core != nullptr) && (this->_bid > 0))
		// Выполняем установку детекции сообщений по количеству секунд
		const_cast <client::core_t *> (this->_core)->waitTimeDetect(this->_bid, read, write, connect);
	// Если подключение ещё не установлено
	else {
		// Устанавливаем количество секунд на чтение
		this->_scheme.timeouts.read = read;
		// Устанавливаем количество секунд на запись
		this->_scheme.timeouts.write = write;
		// Устанавливаем количество секунд на подключение
		this->_scheme.timeouts.connect = connect;
	}
}
/**
 * proxy Метод активации/деактивации прокси-склиента
 * @param work флаг активации/деактивации прокси-клиента
 */
void awh::client::Web::proxy(const client::scheme_t::work_t work) noexcept {
	// Если прокси-сервер установлен
	if(this->_scheme.proxy.type != client::proxy_t::type_t::NONE)
		// Выполняем активацию прокси-сервера
		this->_scheme.activateProxy(work);
	// Снимаем флаг активации прокси-клиента
	else this->_scheme.proxy.mode = false;
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::Web::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Если URI параметры переданы
	if((this->_proxy.mode = !uri.empty())){
		// Устанавливаем семейство интернет протоколов
		this->_scheme.proxy.family = family;
		// Устанавливаем параметры прокси-сервера
		this->_scheme.proxy.url = this->_uri.parse(uri);
		// Если данные параметров прокси-сервера получены
		if(!this->_scheme.proxy.url.empty()){
			// Если протокол подключения SOCKS5
			if(this->_fmk->compare(this->_scheme.proxy.url.schema, "socks5")){
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = client::proxy_t::type_t::SOCKS5;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.socks5.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			// Если протокол подключения HTTP/1.1
			} else if(this->_fmk->compare(this->_scheme.proxy.url.schema, "http")) {
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = client::proxy_t::type_t::HTTP;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.http.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			// Если протокол подключения HTTP/2
			} else if(this->_fmk->compare(this->_scheme.proxy.url.schema, "https")) {
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = client::proxy_t::type_t::HTTPS;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.http.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			}
		}
	}
}
/**
 * attempts Метод установки общего количества попыток
 * @param attempts общее количество попыток
 */
void awh::client::Web::attempts(const uint8_t attempts) noexcept {
	// Если количество попыток передано, устанавливаем его
	if(attempts > 0)
		// Устанавливаем количество попыток
		this->_attempts = attempts;
}
/**
 * core Метод установки сетевого ядра
 * @param core объект сетевого ядра
 */
void awh::client::Web::core(const client::core_t * core) noexcept {
	// Если объект сетевого ядра передан
	if(core != nullptr){
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
		// Добавляем схемы сети в сетевое ядро
		const_cast <client::core_t *> (this->_core)->scheme(&this->_scheme);
		// Устанавливаем событие на запуск системы
		const_cast <client::core_t *> (this->_core)->on <void (const uint16_t)> ("open", &web_t::openEvent, this, _1);
		// Выполняем установку функций обратного вызова для HTTP-клиента
		const_cast <client::core_t *> (this->_core)->on <void (const awh::core_t::status_t)> ("status", &web_t::statusEvent, this, _1);
		// Устанавливаем событие подключения
		const_cast <client::core_t *> (this->_core)->on <void (const uint64_t, const uint16_t)> ("connect", &web_t::connectEvent, this, _1, _2);
		// Устанавливаем событие отключения
		const_cast <client::core_t *> (this->_core)->on <void (const uint64_t, const uint16_t)> ("disconnect", &web_t::disconnectEvent, this, _1, _2);
		// Устанавливаем событие на подключение к прокси-серверу
		const_cast <client::core_t *> (this->_core)->on <void (const uint64_t, const uint16_t)> ("connectProxy", &web_t::proxyConnectEvent, this, _1, _2);
		// Устанавливаем событие на активацию шифрованного SSL канала
		const_cast <client::core_t *> (this->_core)->on <bool (const uri_t::url_t &, const uint64_t, const uint16_t)> ("ssl", &web_t::enableSSLEvent, this, _1, _2, _3);
		// Устанавливаем функцию чтения данных
		const_cast <client::core_t *> (this->_core)->on <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", &web_t::readEvent, this, _1, _2, _3, _4);
		// Устанавливаем событие на чтение данных с прокси-сервера
		const_cast <client::core_t *> (this->_core)->on <void (const char *, const size_t, const uint64_t, const uint16_t)> ("readProxy", &web_t::proxyReadEvent, this, _1, _2, _3, _4);
	// Если объект сетевого ядра не передан но ранее оно было добавлено
	} else if(this->_core != nullptr) {
		// Удаляем схему сети из сетевого ядра
		const_cast <client::core_t *> (this->_core)->remove(this->_scheme.id);
		// Выполняем установку объекта сетевого ядра
		this->_core = core;
	}
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors список поддерживаемых компрессоров
 */
void awh::client::Web::compressors(const vector <awh::http_t::compressor_t> & compressors) noexcept {
	// Устанавливаем список поддерживаемых компрессоров
	this->_compressors = compressors;
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::Web::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::client::Web::userAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty())
		// Устанавливаем пользовательского агента для прокси-сервера
		this->_scheme.proxy.http.userAgent(userAgent);
}
/**
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::Web::ident(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем данные сервиса для прокси-сервера
	this->_scheme.proxy.http.ident(id, name, ver);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::Web::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Если объект авторизации создан
	this->_scheme.proxy.http.authType(type, hash);
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::client::Web::encryption(const bool mode) noexcept {
	// Устанавливаем флаг шифрования
	this->_encryption.mode = mode;
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::Web::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Если пароль для шифрования передаваемых данных получен
	if(!pass.empty())
		// Выполняем установку пароля шифрования передаваемых данных
		this->_encryption.pass = pass;
	// Если соль шифрования переданных данных получен
	if(!salt.empty())
		// Выполняем установку соли шифрования передаваемых данных
		this->_encryption.salt = salt;
	// Выполняем установку размера шифрования передаваемых данных
	this->_encryption.cipher = cipher;
}
/**
 * Web Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::Web::Web(const fmk_t * fmk, const log_t * log) noexcept :
 _bid(0), _uri(fmk, log), _scheme(fmk, log), _callback(log), _nossl(false),
 _reading(false), _stopped(false), _pinging(true), _complete(true), _redirects(false),
 _attempt(0), _attempts(15), _sendPing(0), _pingInterval(PING_INTERVAL), _buffer(log),
 _timer(fmk, log), _fmk(fmk), _log(log), _core(nullptr) {
	// Выполняем отключение информационных сообщений сетевого ядра пинга
	this->_timer.verbose(false);
	// Выполняем активацию ловушки событий контейнера функций обратного вызова
	this->_callback.on(std::bind(&web_t::callbackEvent, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_scheme.proxy.http.on <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", &web_t::chunking, this, _1, _2, _3);
}
/**
 * Web Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::Web::Web(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _bid(0), _uri(fmk, log), _scheme(fmk, log), _callback(log), _nossl(false),
 _reading(false), _stopped(false), _pinging(true), _complete(true), _redirects(false),
 _attempt(0), _attempts(15), _sendPing(0), _pingInterval(PING_INTERVAL), _buffer(log),
 _timer(fmk, log), _fmk(fmk), _log(log), _core(core) {
	// Выполняем отключение информационных сообщений сетевого ядра таймера
	this->_timer.verbose(false);
	// Выполняем активацию ловушки событий контейнера функций обратного вызова
	this->_callback.on(std::bind(&web_t::callbackEvent, this, _1, _2, _3));
	// Устанавливаем функцию обработки вызова для получения чанков для HTTP-клиента
	this->_scheme.proxy.http.on <void (const uint64_t, const vector <char> &, const awh::http_t *)> ("chunking", &web_t::chunking, this, _1, _2, _3);
	// Добавляем схемы сети в сетевое ядро
	const_cast <client::core_t *> (this->_core)->scheme(&this->_scheme);
	// Устанавливаем событие на запуск системы
	const_cast <client::core_t *> (this->_core)->on <void (const uint16_t)> ("open", &web_t::openEvent, this, _1);
	// Выполняем установку функций обратного вызова для HTTP-клиента
	const_cast <client::core_t *> (this->_core)->on <void (const awh::core_t::status_t)> ("status", &web_t::statusEvent, this, _1);
	// Устанавливаем событие подключения
	const_cast <client::core_t *> (this->_core)->on <void (const uint64_t, const uint16_t)> ("connect", &web_t::connectEvent, this, _1, _2);
	// Устанавливаем событие отключения
	const_cast <client::core_t *> (this->_core)->on <void (const uint64_t, const uint16_t)> ("disconnect", &web_t::disconnectEvent, this, _1, _2);
	// Устанавливаем событие на подключение к прокси-серверу
	const_cast <client::core_t *> (this->_core)->on <void (const uint64_t, const uint16_t)> ("connectProxy", &web_t::proxyConnectEvent, this, _1, _2);
	// Устанавливаем событие на активацию шифрованного SSL канала
	const_cast <client::core_t *> (this->_core)->on <bool (const uri_t::url_t &, const uint64_t, const uint16_t)> ("ssl", &web_t::enableSSLEvent, this, _1, _2, _3);
	// Устанавливаем функцию чтения данных
	const_cast <client::core_t *> (this->_core)->on <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", &web_t::readEvent, this, _1, _2, _3, _4);
	// Устанавливаем событие на чтение данных с прокси-сервера
	const_cast <client::core_t *> (this->_core)->on <void (const char *, const size_t, const uint64_t, const uint16_t)> ("readProxy", &web_t::proxyReadEvent, this, _1, _2, _3, _4);
}
