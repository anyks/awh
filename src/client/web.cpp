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
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::client::WEB::chunking(const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <awh::http_t *> (http)->body(chunk);
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
bool awh::client::WEB::enableTLSCallback(const uri_t::url_t & url, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Блокируем переменные которые не используем
	(void) aid;
	(void) sid;
	(void) core;
	// Выводим результат активации
	return (!url.empty() && (url.schema.compare("https") == 0));
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
			switch((uint8_t) this->_action){
				// Если необходимо запустить экшен открытия подключения
				case (uint8_t) action_t::OPEN: this->actionOpen(); break;
				// Если необходимо запустить экшен обработки данных поступающих с сервера
				case (uint8_t) action_t::READ: this->actionRead(); break;
				// Если необходимо запустить экшен обработки подключения к серверу
				case (uint8_t) action_t::CONNECT: this->actionConnect(); break;
				// Если необходимо запустить экшен обработки отключения от сервера
				case (uint8_t) action_t::DISCONNECT: this->actionDisconnect(); break;
				// Если необходимо запустить экшен обработки данных поступающих с прокси-сервера
				case (uint8_t) action_t::PROXY_READ: this->actionProxyRead(); break;
				// Если необходимо запустить экшен обработки подключения к прокси-серверу
				case (uint8_t) action_t::PROXY_CONNECT: this->actionProxyConnect(); break;
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
	const_cast <client::core_t *> (this->_core)->open(this->_scheme.sid);
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
			switch((uint8_t) status){
				// Если нужно попытаться ещё раз
				case (uint8_t) awh::http_t::stath_t::RETRY: {
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
				case (uint8_t) awh::http_t::stath_t::GOOD: {
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
				case (uint8_t) awh::http_t::stath_t::FAULT:
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
		this->_callback.message(result, this);
}
/**
 * actionConnect Метод обработки экшена подключения к серверу
 */
void awh::client::WEB::actionConnect() noexcept {
	// Если экшен соответствует, выполняем его сброс
	if(this->_action == action_t::CONNECT)
		// Выполняем сброс экшена
		this->_action = action_t::NONE;
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
			this->_callback.message(response, this);
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
	switch((uint8_t) this->_scheme.proxy.type){
		// Если прокси-сервер является Socks5
		case (uint8_t) proxy_t::type_t::SOCKS5: {
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
							this->_callback.message(result, this);
						// Завершаем работу
						} else core->close(this->_aid);
						// Завершаем работу
						return;
					}
				}
			}
		} break;
		// Если прокси-сервер является HTTP
		case (uint8_t) proxy_t::type_t::HTTP: {
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
				switch((uint8_t) status){
					// Если нужно попытаться ещё раз
					case (uint8_t) awh::http_t::stath_t::RETRY: {
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
					case (uint8_t) awh::http_t::stath_t::GOOD: {
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
					case (uint8_t) awh::http_t::stath_t::FAULT: {
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
					this->_callback.message(result, this);
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
	switch((uint8_t) this->_scheme.proxy.type){
		// Если прокси-сервер является Socks5
		case (uint8_t) proxy_t::type_t::SOCKS5: {
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
		case (uint8_t) proxy_t::type_t::HTTP: {
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
	// Устанавливаем функцию обработки вызова для получения чанков
	this->_http.chunking(callback);
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
		// Получаем объект биндинга ядра TCP/IP
		client::core_t * core = const_cast <client::core_t *> (this->_core);
		// Выполняем перебор всех подключений
		for(auto & req : this->_requests){
			// Выполняем сброс состояния HTTP парсера
			this->_http.reset();
			// Выполняем очистку параметров HTTP запроса
			this->_http.clear();
			// Устанавливаем метод компрессии
			this->_http.compress(this->_compress);
			// Если список заголовков получен
			if(!req.headers.empty())
				// Устанавливаем заголовоки запроса
				this->_http.headers(req.headers);
			// Если тело запроса существует
			if(!req.entity.empty())
				// Устанавливаем тело запроса
				this->_http.body(req.entity);			
			// Получаем URL ссылку для выполнения запроса
			this->_uri.append(this->_scheme.url, req.query);
			// Получаем бинарные данные WEB запроса
			const auto & buffer = this->_http.request(this->_scheme.url, req.method);
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
		else const_cast <client::core_t *> (this->_core)->open(this->_scheme.sid);
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
			if(this->_scheme.proxy.url.schema.compare("socks5") == 0){
				// Устанавливаем тип прокси-сервера
				this->_scheme.proxy.type = proxy_t::type_t::SOCKS5;
				// Если требуется авторизация на прокси-сервере
				if(!this->_scheme.proxy.url.user.empty() && !this->_scheme.proxy.url.pass.empty())
					// Устанавливаем данные пользователя
					this->_scheme.proxy.socks5.user(this->_scheme.proxy.url.user, this->_scheme.proxy.url.pass);
			// Если протокол подключения HTTP
			} else if((this->_scheme.proxy.url.schema.compare("http") == 0) || (this->_scheme.proxy.url.schema.compare("https") == 0)) {
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
	this->_unbind = !(flag & (uint8_t) flag_t::NOTSTOP);
	// Устанавливаем флаг разрешающий выполнять редиректы
	this->_redirects = (flag & (uint8_t) flag_t::REDIRECTS);
	// Устанавливаем флаг поддержания автоматического подключения
	this->_scheme.alive = (flag & (uint8_t) flag_t::ALIVE);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <client::core_t *> (this->_core)->noInfo(flag & (uint8_t) flag_t::NOINFO);
	// Выполняем установку флага проверки домена
	const_cast <client::core_t *> (this->_core)->verifySSL(flag & (uint8_t) flag_t::VERIFYSSL);
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
 _nwk(fmk), _uri(fmk, &_nwk), _http(fmk, log, &_uri), _scheme(fmk, log), _action(action_t::NONE),
 _compress(awh::http_t::compress_t::NONE), _aid(0), _unbind(true), _active(false),
 _stopped(false), _redirects(false), _attempts(10), _fmk(fmk), _log(log), _core(core) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&web_t::openCallback, this, _1, _2));
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
}