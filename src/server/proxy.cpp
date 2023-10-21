/**
 * @file: proxy.cpp
 * @date: 2022-09-03
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
#include <server/proxy.hpp>

/**
 * chunking Метод обработки получения чанков
 * @param bid   идентификатор брокера
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::server::Proxy::chunking(const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Выполняем блокировку неиспользуемой переменной
	(void) bid;
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <awh::http_t *> (http)->body(chunk);
}
/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::server::Proxy::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Определяем статус активности сетевого ядра
		switch(static_cast <uint8_t> (status)){
			// Если система запущена
			case static_cast <uint8_t> (awh::core_t::status_t::START): {
				// Выполняем биндинг базы событий для таймера
				this->_core.server.bind(reinterpret_cast <awh::core_t *> (&this->_core.timer));
				// Выполняем биндинг базы событий для клиента
				this->_core.server.bind(reinterpret_cast <awh::core_t *> (&this->_core.client));
				// Устанавливаем таймаут времени на удаление мусорных брокеров раз в 10 секунд
				this->_core.timer.setTimeout(10000, (function <void (const u_short, awh::core_t *)>) std::bind(&proxy_t::garbage, this, _1, _2));
			} break;
			// Если система остановлена
			case static_cast <uint8_t> (awh::core_t::status_t::STOP): {
				// Выполняем анбиндинг базы событий таймера
				this->_core.server.unbind(reinterpret_cast <awh::core_t *> (&this->_core.timer));
				// Выполняем анбиндинг базы событий клиента
				this->_core.server.unbind(reinterpret_cast <awh::core_t *> (&this->_core.client));
			} break;
		}
		// Если функция обратного вызова установлена
		if(this->_callback.events != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.events(status, core);
	}
}
/**
 * persistServerCallback Функция персистентного вызова
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::persistCallback(const size_t bid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры активного клиента
		proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if((options != nullptr) && ((options->method != web_t::method_t::CONNECT) || options->close) && ((!options->alive && !this->_alive) || options->close)){
			// Если брокер давно должен был быть отключён, отключаем его
			if(options->close || !options->srv.is(http_t::state_t::ALIVE))
				// Выполняем закрытие подключение клиента
				reinterpret_cast <server::core_t *> (core)->close(bid);
			// Иначе проверяем прошедшее время
			else {
				// Получаем текущий штамп времени
				const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
				// Если брокер не ответил на пинг больше двух интервалов, отключаем его
				if((stamp - options->checkPoint) >= this->_timeAlive)
					// Завершаем работу
					reinterpret_cast <server::core_t *> (core)->close(bid);
			}
		}
	}
}
/**
 * openServerCallback Функция обратного вызова при запуске работы
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::openServerCallback(const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((sid > 0) && (core != nullptr)){
		// Устанавливаем хост сервера
		reinterpret_cast <server::core_t *> (core)->init(sid, this->_port, this->_host);
		// Выполняем запуск сервера
		reinterpret_cast <server::core_t *> (core)->run(sid);
	}
}
/**
 * connectClientCallback Функция обратного вызова при подключении к серверу
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::connectClientCallback(const size_t bid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Ищем идентификатор брокера пары
		auto it = this->_scheme.pairs.find(sid);
		// Если брокер получен
		if(it != this->_scheme.pairs.end()){
			// Получаем параметры активного клиента
			proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(it->second));
			// Если подключение не выполнено
			if((options != nullptr) && !options->connect){
				// Разрешаем обработки данных
				options->locked = false;
				// Запоминаем, что подключение выполнено
				options->connect = true;
				// Выполняем сброс состояния HTTP парсера
				options->srv.clear();
				// Выполняем сброс состояния HTTP парсера
				options->srv.reset();
				// Если метод подключения CONNECT
				if(options->method == web_t::method_t::CONNECT){
					// Формируем ответ брокеру
					const auto & response = options->srv.process(http_t::process_t::RESPONSE, awh::web_t::res_t(static_cast <u_int> (200)));
					// Если ответ получен
					if(!response.empty()){
						// Тело полезной нагрузки
						vector <char> payload;
						// Отправляем ответ брокеру
						this->_core.server.write(response.data(), response.size(), it->second);
						// Получаем данные тела запроса
						while(!(payload = options->srv.payload()).empty())
							// Отправляем тело на сервер
							this->_core.server.write(payload.data(), payload.size(), it->second);
					// Выполняем отключение брокера
					} else this->close(it->second);
				// Отправляем сообщение на сервер, так-как оно пришло от брокера
				} else this->prepare(it->second, this->_scheme.sid);
			}
		}
	}
}
/**
 * connectServerCallback Функция обратного вызова при подключении к серверу
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::connectServerCallback(const size_t bid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём брокера
		this->_scheme.set(bid);
		// Получаем параметры активного клиента
		proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Устанавливаем размер чанка
			options->cli.chunk(this->_chunkSize);
			options->srv.chunk(this->_chunkSize);
			// Устанавливаем данные сервиса
			options->cli.ident(this->_sid, this->_name, this->_ver);
			options->srv.ident(this->_sid, this->_name, this->_ver);
			// Если функция обратного вызова для обработки чанков установлена
			if(this->_callback.chunking != nullptr)
				// Устанавливаем функцию обработки вызова для получения чанков
				options->cli.on(this->_callback.chunking);
			// Устанавливаем функцию обработки вызова для получения чанков
			else options->cli.on(std::bind(&proxy_t::chunking, this, _1, _2, _3));
			// Устанавливаем функцию обработки вызова для получения чанков
			options->srv.on(std::bind(&proxy_t::chunking, this, _1, _2, _3));
			// Устанавливаем метод компрессии поддерживаемый клиентом
			options->cli.compress(this->_scheme.compress);
			// Устанавливаем метод компрессии поддерживаемый сервером
			options->srv.compress(this->_scheme.compress);
			// Если данные будем передавать в зашифрованном виде
			if(this->_crypt){
				// Устанавливаем параметры шифрования
				options->cli.crypto(this->_pass, this->_salt, this->_cipher);
				options->srv.crypto(this->_pass, this->_salt, this->_cipher);
			}
			// Определяем тип авторизации
			switch(static_cast <uint8_t> (this->_authType)){
				// Если тип авторизации Basic
				case static_cast <uint8_t> (auth_t::type_t::BASIC): {
					// Устанавливаем параметры авторизации
					options->srv.authType(this->_authType);
					// Устанавливаем функцию проверки авторизации
					options->srv.authCallback(this->_callback.checkAuth);
				} break;
				// Если тип авторизации Digest
				case static_cast <uint8_t> (auth_t::type_t::DIGEST): {
					// Устанавливаем название сервера
					options->srv.realm(this->_realm);
					// Устанавливаем временный ключ сессии сервера
					options->srv.opaque(this->_opaque);
					// Устанавливаем параметры авторизации
					options->srv.authType(this->_authType, this->_authHash);
					// Устанавливаем функцию извлечения пароля
					options->srv.extractPassCallback(this->_callback.extractPass);
				} break;
			}
			// Устанавливаем количество секунд на чтение
			options->scheme.timeouts.read = 30;
			// Устанавливаем количество секунд на запись
			options->scheme.timeouts.write = 10;
			// Устанавливаем флаг ожидания входящих сообщений
			options->scheme.wait = this->_scheme.wait;
			// Выполняем установку максимального количества попыток
			options->scheme.keepAlive.cnt = this->_scheme.keepAlive.cnt;
			// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
			options->scheme.keepAlive.idle = this->_scheme.keepAlive.idle;
			// Выполняем установку интервала времени в секундах между попытками
			options->scheme.keepAlive.intvl = this->_scheme.keepAlive.intvl;
			// Устанавливаем событие подключения
			options->scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&proxy_t::connectClientCallback, this, _1, _2, _3));
			// Устанавливаем событие отключения
			options->scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&proxy_t::disconnectClientCallback, this, _1, _2, _3));
			// Устанавливаем функцию чтения данных
			options->scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&proxy_t::readClientCallback, this, _1, _2, _3, _4, _5));
			// Добавляем схему сети в сетевое ядро
			this->_core.client.add(&options->scheme);
			// Создаём пару клиента и сервера
			this->_scheme.pairs.emplace(options->scheme.sid, bid);
		}
		// Если функция обратного вызова установлена
		if(this->_callback.active != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.active(bid, mode_t::CONNECT, this);
	}
}
/**
 * disconnectClientCallback Функция обратного вызова при отключении от сервера
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::disconnectClientCallback(const size_t bid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((sid > 0) && (core != nullptr)){
		// Ищем идентификатор брокера пары
		auto it = this->_scheme.pairs.find(sid);
		// Если брокер получен
		if(it != this->_scheme.pairs.end()){
			// Получаем идентификатор брокера
			const size_t bid = it->second;
			// Удаляем пару брокера и сервера
			this->_scheme.pairs.erase(it);
			// Получаем параметры активного клиента
			proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(bid));
			// Если подключение не выполнено, отправляем ответ брокеру
			if((options != nullptr) && !options->connect)
				// Выполняем реджект
				this->reject(bid, 404);
			// Выполняем отключение клиента
			else this->close(bid);
			// Выходим из функции
			return;
		}
		// Выполняем отключение клиента
		this->_core.client.close(bid);
	}
}
/**
 * disconnectServerCallback Функция обратного вызова при отключении от сервера
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::disconnectServerCallback(const size_t bid, const size_t sid, awh::core_t * core) noexcept {
	// Принудительно выполняем отключение лкиента
	if(bid > 0) this->close(bid);
}
/**
 * acceptServerCallback Функция обратного вызова при проверке подключения брокера
 * @param ip   адрес интернет подключения брокера
 * @param mac  мак-адрес подключившегося брокера
 * @param port порт подключившегося брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат разрешения к подключению брокера
 */
bool awh::server::Proxy::acceptServerCallback(const string & ip, const string & mac, const u_int port, const size_t sid, awh::core_t * core) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (sid > 0) && (core != nullptr)){
		// Если функция обратного вызова установлена
		if(this->_callback.accept != nullptr)
			// Выполняем проверку клиента на разрешение подключения
			result = this->_callback.accept(ip, mac, port, this);
	}
	// Разрешаем подключение клиенту
	return result;
}
/**
 * readClientCallback Функция обратного вызова при чтении сообщения с сервера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Proxy::readClientCallback(const char * buffer, const size_t size, const size_t bid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((size > 0) && (bid > 0) && (sid > 0) && (buffer != nullptr) && (core != nullptr)){
		// Ищем идентификатор брокера пары
		auto it = this->_scheme.pairs.find(sid);
		// Если брокер получен
		if(it != this->_scheme.pairs.end()){
			// Получаем параметры активного клиента
			proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(it->second));
			// Если подключение выполнено, отправляем ответ брокеру
			if((options != nullptr) && options->connect){
				// Если указан метод не CONNECT и функция обработки сообщения установлена
				if((this->_callback.message != nullptr) && (options->method != web_t::method_t::CONNECT)){
					// Добавляем полученные данные в буфер
					options->client.insert(options->client.end(), buffer, buffer + size);
					// Выполняем обработку полученных данных
					while(!options->close && !options->client.empty()){
						// Выполняем парсинг полученных данных
						size_t bytes = options->cli.parse(options->client.data(), options->client.size());
						// Если все данные получены
						if(options->cli.is(http_t::state_t::END)){
							// Если включён режим отладки
							#if defined(DEBUG_MODE)
								/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
								// Получаем данные ответа
								const auto & response = options->cli.process(http_t::process_t::RESPONSE, true);
								// Если параметры ответа получены
								if(!response.empty()){
									// Выводим заголовок ответа
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
									// Выводим параметры ответа
									cout << string(response.begin(), response.end()) << endl;
									// Если тело ответа существует
									if(!options->cli.body().empty())
										// Выводим сообщение о выводе чанка тела
										cout << this->_fmk->format("<body %u>", options->cli.body().size()) << endl << endl;
									// Иначе устанавливаем перенос строки
									else cout << endl;
								}
								*/
							#endif
							// Выводим сообщение
							if(this->_callback.message(it->second, event_t::RESPONSE, &options->cli, this)){
								
								/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
								// Получаем данные ответа
								const auto & response = options->cli.process(http_t::process_t::RESPONSE);
								// Если данные ответа получены
								if(!response.empty()){
									// Тело REST сообщения
									vector <char> entity;
									// Отправляем ответ брокеру
									this->_core.server.write(response.data(), response.size(), it->second);
									// Получаем данные тела ответа
									while(!(entity = options->cli.payload()).empty())
										// Отправляем тело брокеру
										this->_core.server.write(entity.data(), entity.size(), it->second);
								}
								*/
							}
						}
						// Если парсер обработал какое-то количество байт
						if((bytes > 0) && !options->client.empty()){
							// Если размер буфера больше количества удаляемых байт
							if(options->client.size() >= bytes)
								// Удаляем количество обработанных байт
								options->client.assign(options->client.begin() + bytes, options->client.end());
							// Если байт в буфере меньше, просто очищаем буфер
							else options->client.clear();
							// Если данных для обработки не осталось, выходим
							if(options->client.empty()) break;
						// Если данных для обработки недостаточно, выходим
						} else break;
					}
				// Передаём данные так-как они есть
				} else {
					// Если функция обратного вызова установлена, выполняем
					if(this->_callback.binary != nullptr){
						// Выводим сообщение
						if(this->_callback.binary(it->second, event_t::RESPONSE, buffer, size, this))
							// Отправляем запрос на внешний сервер
							this->_core.server.write(buffer, size, it->second);
					// Отправляем запрос на внешний сервер
					} else this->_core.server.write(buffer, size, it->second);
				}
			}
		}
	}
}
/**
 * readServerCallback Функция обратного вызова при чтении сообщения с брокера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Proxy::readServerCallback(const char * buffer, const size_t size, const size_t bid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((size > 0) && (bid > 0) && (sid > 0) && (buffer != nullptr) && (core != nullptr)){
		// Получаем параметры активного клиента
		proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если указан метод CONNECT
			if(options->connect && (options->method == web_t::method_t::CONNECT)){
				// Получаем идентификатор брокера
				const size_t bid = options->scheme.bid();
				// Отправляем запрос на внешний сервер
				if(bid > 0){
					// Если функция обратного вызова установлена, выполняем
					if(this->_callback.binary != nullptr){
						// Выводим сообщение
						if(this->_callback.binary(bid, event_t::REQUEST, buffer, size, this))
							// Отправляем запрос на внешний сервер
							this->_core.client.write(buffer, size, bid);
					// Отправляем запрос на внешний сервер
					} else this->_core.client.write(buffer, size, bid);
				}
			// Если подключение ещё не выполнено
			} else {
				// Добавляем полученные данные в буфер
				options->server.insert(options->server.end(), buffer, buffer + size);
				// Выполняем обработку полученных данных
				this->prepare(bid, sid);
			}
		}
	}
}
/**
 * writeServerCallback Функция обратного вызова при записи сообщения на брокере
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Proxy::writeServerCallback(const char * buffer, const size_t size, const size_t bid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((size > 0) && (bid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры активного клиента
		proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if((options != nullptr) && options->stopped)
			// Выполняем закрытие подключения
			this->close(bid);
	}
}
/**
 * prepare Метод обработки входящих данных
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::server::Proxy::prepare(const size_t bid, const size_t sid) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0)){
		// Получаем параметры активного клиента
		proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if((options != nullptr) && !options->locked){
			// Выполняем обработку полученных данных
			while(!options->close && !options->server.empty()){
				// Выполняем парсинг полученных данных
				size_t bytes = options->srv.parse(options->server.data(), options->server.size());
				// Если все данные получены
				if(options->srv.is(http_t::state_t::END)){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Получаем данные запроса
						const auto & request = options->srv.process(http_t::process_t::REQUEST, true);
						// Если параметры запроса получены
						if(!request.empty()){
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << string(request.begin(), request.end()) << endl;
							// Если тело запроса существует
							if(!options->srv.body().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->_fmk->format("<body %u>", options->srv.body().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					#endif
					// Если подключение не установлено как постоянное
					if(!this->_alive && !options->alive){
						// Увеличиваем количество выполненных запросов
						options->requests++;
						// Если количество выполненных запросов превышает максимальный
						if(options->requests >= this->_maxRequests)
							// Устанавливаем флаг закрытия подключения
							options->close = true;
						// Получаем текущий штамп времени
						else options->checkPoint = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					// Выполняем сброс количества выполненных запросов
					} else options->requests = 0;
					// Выполняем проверку авторизации
					switch(static_cast <uint8_t> (options->srv.auth())){
						// Если запрос выполнен удачно
						case static_cast <uint8_t> (http_t::status_t::GOOD): {
							// Получаем флаг шифрованных данных
							options->crypt = options->srv.crypto();
							// Получаем поддерживаемый метод компрессии
							options->compress = options->srv.compress();
							// Если подключение не выполнено
							if(!options->connect){
								// Получаем данные запроса
								const auto & request = options->srv.request();
								// Выполняем проверку разрешено ли нам выполнять подключение
								const bool allow = (!this->_noConnect || (request.method != web_t::method_t::CONNECT));
								// Получаем URI запрос для сервера
								const auto & uri = (request.method == web_t::method_t::CONNECT ? this->_fmk->format("%s:%u", request.url.host.c_str(), request.url.port) : options->srv.header("host"));
								// Сообщение запрета подключения
								const string message = (allow ? "" : "Connect method prohibited");
								// Если URI запрос для сервера получен
								if(allow && (options->locked = !uri.empty())){
									// Запоминаем метод подключения
									options->method = request.method;
									// Формируем адрес подключения
									options->scheme.url = this->_uri.parse(this->_fmk->format("http://%s", uri.c_str()));
									// Выполняем запрос на сервер
									this->_core.client.open(options->scheme.sid);
									// Выходим из функции
									return;
								// Если хост не получен
								} else {
									// Выполняем сброс состояния HTTP парсера
									options->srv.clear();
									// Выполняем сброс состояния HTTP парсера
									options->srv.reset();
									// Выполняем очистку буфера полученных данных
									options->server.clear();
									// Формируем запрос реджекта
									const auto & response = options->srv.reject(awh::web_t::res_t(static_cast <u_int> (403), message));
									// Если ответ получен
									if(!response.empty()){
										// Тело полезной нагрузки
										vector <char> payload;
										// Отправляем ответ брокеру
										this->_core.server.write(response.data(), response.size(), bid);
										// Получаем данные тела запроса
										while(!(payload = options->srv.payload()).empty())
											// Отправляем тело на сервер
											this->_core.server.write(payload.data(), payload.size(), bid);
									// Выполняем отключение брокера
									} else this->close(bid);
									// Выходим из функции
									return;
								}
							// Если подключение выполнено
							} else {
								// Выполняем удаление заголовка авторизации на прокси-сервере
								options->srv.rm(http_t::suite_t::HEADER, "proxy-authorization");
								{
									// Получаем данные запроса
									const auto & request = options->srv.request();
									// Получаем данные заголовка Via
									string via = options->srv.header("via");
									// Если unix-сокет активирован
									if(!this->_usock.empty()){
										// Если заголовок получен
										if(!via.empty())
											// Устанавливаем Via заголовок
											via = this->_fmk->format("%s, %.1f %s", via.c_str(), request.version, this->_usock.c_str());
										// Иначе просто формируем заголовок Via
										else via = this->_fmk->format("%.1f %s", request.version, this->_usock.c_str());
									// Если активирован хост и порт
									} else {
										// Если заголовок получен
										if(!via.empty())
											// Устанавливаем Via заголовок
											via = this->_fmk->format("%s, %.1f %s:%u", via.c_str(), request.version, this->_host.c_str(), this->_port);
										// Иначе просто формируем заголовок Via
										else via = this->_fmk->format("%.1f %s:%u", request.version, this->_host.c_str(), this->_port);
									}
									// Устанавливаем заголовок Via
									options->srv.header("Via", via);
								}{
									// Название операционной системы
									const char * os = nullptr;
									// Определяем название операционной системы
									switch(static_cast <uint8_t> (this->_fmk->os())){
										// Если операционной системой является Unix
										case static_cast <uint8_t> (fmk_t::os_t::UNIX): os = "Unix"; break;
										// Если операционной системой является Linux
										case static_cast <uint8_t> (fmk_t::os_t::LINUX): os = "Linux"; break;
										// Если операционной системой является неизвестной
										case static_cast <uint8_t> (fmk_t::os_t::NONE): os = "Unknown"; break;
										// Если операционной системой является Windows
										case static_cast <uint8_t> (fmk_t::os_t::WIND32):
										case static_cast <uint8_t> (fmk_t::os_t::WIND64): os = "Windows"; break;
										// Если операционной системой является MacOS X
										case static_cast <uint8_t> (fmk_t::os_t::MACOSX): os = "MacOS X"; break;
										// Если операционной системой является FreeBSD
										case static_cast <uint8_t> (fmk_t::os_t::FREEBSD): os = "FreeBSD"; break;
									}
									// Устанавливаем наименование агента
									options->srv.header("X-Proxy-Agent", this->_fmk->format("(%s; %s) %s/%s", os, this->_name.c_str(), this->_sid.c_str(), this->_ver.c_str()));
								}
								// Если функция обратного вызова установлена, выполняем
								if(this->_callback.message != nullptr){
									// Выводим сообщение
									if(this->_callback.message(bid, event_t::REQUEST, &options->srv, this)){
										
										/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
										// Получаем данные запроса
										const auto & request = options->srv.process(http_t::process_t::REQUEST);
										// Если данные запроса получены
										if(!request.empty()){
											// Получаем идентификатор брокера
											const size_t bid = options->scheme.bid();
											// Отправляем запрос на внешний сервер
											if(bid > 0){
												// Тело REST сообщения
												vector <char> entity;
												// Отправляем серверу сообщение
												this->_core.client.write(request.data(), request.size(), bid);
												// Получаем данные тела запроса
												while(!(entity = options->srv.payload()).empty())
													// Отправляем тело на сервер
													this->_core.client.write(entity.data(), entity.size(), bid);
											}
										}
										*/

									}
								// Если функция обратного вызова не установлена
								} else {
									
									/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
									// Получаем идентификатор брокера
									const size_t bid = options->scheme.bid();
									// Отправляем запрос на внешний сервер
									if(bid > 0){
										// Получаем данные запроса
										const auto & request = options->srv.process(http_t::process_t::REQUEST);
										// Если данные запроса получены
										if(!request.empty()){
											// Тело REST сообщения
											vector <char> entity;
											// Если функция обратного вызова установлена, выполняем
											if(this->_callback.binary != nullptr){
												// Выводим сообщение
												if(this->_callback.binary(bid, event_t::REQUEST, request.data(), request.size(), this))
													// Отправляем запрос на внешний сервер
													this->_core.client.write(request.data(), request.size(), bid);
											// Отправляем запрос на внешний сервер
											} else this->_core.client.write(request.data(), request.size(), bid);
											// Получаем данные тела запроса
											while(!(entity = options->srv.payload()).empty())
												// Отправляем тело на сервер
												this->_core.client.write(entity.data(), entity.size(), bid);
										}
									}
									*/

								}
							}
							// Выполняем сброс состояния HTTP парсера
							options->srv.clear();
							// Выполняем сброс состояния HTTP парсера
							options->srv.reset();
							// Завершаем обработку
							goto Next;
						} break;
						// Если запрос неудачный
						case static_cast <uint8_t> (http_t::status_t::FAULT): {
							// Выполняем сброс состояния HTTP парсера
							options->srv.clear();
							// Выполняем сброс состояния HTTP парсера
							options->srv.reset();
							// Выполняем очистку буфера полученных данных
							options->server.clear();
							// Формируем запрос авторизации
							const auto & response = options->srv.reject(awh::web_t::res_t(static_cast <u_int> (407)));
							// Если ответ получен
							if(!response.empty()){
								// Тело полезной нагрузки
								vector <char> payload;
								// Устанавливаем флаг завершения работы
								options->stopped = !options->srv.is(http_t::state_t::ALIVE);
								// Отправляем ответ брокеру
								this->_core.server.write(response.data(), response.size(), bid);
								// Получаем данные тела запроса
								while(!(payload = options->srv.payload()).empty())
									// Отправляем тело на сервер
									this->_core.server.write(payload.data(), payload.size(), bid);
								// Выходим из функции
								return;
							}
							// Выполняем отключение брокера
							this->close(bid);
							// Выходим из функции
							return;
						}
					}
				}
				// Устанавливаем метку продолжения обработки пайплайна
				Next:
				// Если парсер обработал какое-то количество байт
				if((bytes > 0) && !options->server.empty()){
					// Если размер буфера больше количества удаляемых байт
					if(options->server.size() >= bytes)
						// Удаляем количество обработанных байт
						options->server.assign(options->server.begin() + bytes, options->server.end());
					// Если байт в буфере меньше, просто очищаем буфер
					else options->server.clear();
					// Если данных для обработки не осталось, выходим
					if(options->server.empty()) break;
				// Если данных для обработки недостаточно, выходим
				} else break;
			}
		}
	}
}
/**
 * garbage Метод удаления мусорных брокеров
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::garbage(const u_short tid, awh::core_t * core) noexcept {
	// Если список мусорных брокеров не пустой
	if(!this->_garbage.empty()){
		// Получаем текущее значение времени
		const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
		// Выполняем переход по всему списку мусорных брокеров
		for(auto it = this->_garbage.begin(); it != this->_garbage.end();){
			// Если брокер уже давно удалился
			if((date - it->second) >= 10000){
				// Выполняем удаление параметров брокера
				this->_scheme.rm(it->first);
				// Выполняем удаление объекта брокеров из списка мусора
				it = this->_garbage.erase(it);
			// Выполняем пропуск брокера
			} else ++it;
		}
	}
	// Устанавливаем таймаут времени на удаление мусорных брокеров раз в 10 секунд
	this->_core.timer.setTimeout(10000, (function <void (const u_short, awh::core_t *)>) std::bind(&proxy_t::garbage, this, _1, _2));
}
/**
 * init Метод инициализации WebSocket брокера
 * @param socket   unix-сокет для биндинга
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Proxy::init(const string & socket, const http_t::compress_t compress) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Устанавливаем unix-сокет сервера
		this->_usock = socket;
		// Выполняем установку unix-сокет
		this->_core.server.unixSocket(socket);
		// Устанавливаем тип сокета unix-сокет
		this->_core.server.family(scheme_t::family_t::NIX);
	#endif
}
/**
 * init Метод инициализации WebSocket брокера
 * @param port     порт сервера
 * @param host     хост сервера
 * @param compress метод сжатия передаваемых сообщений
 */
void awh::server::Proxy::init(const u_int port, const string & host, const http_t::compress_t compress) noexcept {
	// Устанавливаем порт сервера
	this->_port = port;
	// Устанавливаем хост сервера
	this->_host = host;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Удаляем unix-сокет сервера
		this->_usock.clear();
		// Удаляем unix-сокет ранее установленный
		this->_core.server.removeUnixSocket();
	#endif
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const size_t, const mode_t, Proxy *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.active = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const size_t, const event_t, http_t *, Proxy *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.message = callback;
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const awh::core_t::status_t status, awh::core_t * core)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.events = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const size_t, const event_t, const char *, const size_t, Proxy *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.binary = callback;
}
/**
 * on Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Proxy::on(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.extractPass = callback;
}
/**
 * on Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Proxy::on(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.checkAuth = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации брокера на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const string &, const string &, const u_int, Proxy *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.accept = callback;
}
/**
 * on Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const vector <char> &, const http_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.chunking = callback;
}
/**
 * reject Метод отправки сообщения об ошибке
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Proxy::reject(const size_t bid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если подключение выполнено
	if(this->_core.server.working()){
		// Получаем параметры активного клиента
		proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если отправка сообщений разблокированна
		if(options != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			options->srv.body(entity);
			// Устанавливаем заголовки ответа
			options->srv.headers(headers);
			// Если подключение не установлено как постоянное, но подключение долгоживущее
			if(!this->_alive && !options->alive && options->srv.is(http_t::state_t::ALIVE))
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				options->srv.header("Keep-Alive", this->_fmk->format("timeout=%d, max=%d", this->_timeAlive / 1000, this->_maxRequests));
			// Формируем запрос авторизации
			const auto & response = options->srv.reject(awh::web_t::res_t(static_cast <u_int> (code), mess));
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(response.begin(), response.end()) << endl;
			#endif
			// Устанавливаем флаг завершения работы
			options->stopped = true;
			// Отправляем серверу сообщение
			this->_core.server.write(response.data(), response.size(), bid);
			// Получаем данные полезной нагрузки ответа
			while(!(payload = options->srv.payload()).empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка полезной нагрузки
					cout << this->_fmk->format("<chunk %u>", payload.size()) << endl;
				#endif
				// Отправляем тело на сервер
				this->_core.server.write(payload.data(), payload.size(), bid);
			}
		}
	}
}
/**
 * response Метод отправки сообщения брокеру
 * @param bid     идентификатор брокера
 * @param code    код сообщения для брокера
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Proxy::response(const size_t bid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если подключение выполнено
	if(this->_core.server.working()){
		// Получаем параметры активного клиента
		proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если отправка сообщений разблокированна
		if(options != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			options->srv.body(entity);
			// Устанавливаем заголовки ответа
			options->srv.headers(headers);
			// Если подключение не установлено как постоянное, но подключение долгоживущее
			if(!this->_alive && !options->alive && options->srv.is(http_t::state_t::ALIVE))
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				options->srv.header("Keep-Alive", this->_fmk->format("timeout=%d, max=%d", this->_timeAlive / 1000, this->_maxRequests));
			// Формируем запрос авторизации
			const auto & response = options->srv.process(http_t::process_t::RESPONSE, awh::web_t::res_t(static_cast <u_int> (code), mess));
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(response.begin(), response.end()) << endl;
			#endif
			// Устанавливаем флаг завершения работы
			options->stopped = true;
			// Отправляем серверу сообщение
			this->_core.server.write(response.data(), response.size(), bid);
			// Получаем данные полезной нагрузки ответа
			while(!(payload = options->srv.payload()).empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка полезной нагрузки
					cout << this->_fmk->format("<chunk %u>", payload.size()) << endl;
				#endif
				// Отправляем тело на сервер
				this->_core.server.write(payload.data(), payload.size(), bid);
			}
		}
	}
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
u_int awh::server::Proxy::port(const size_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.port(bid);
}
/**
 * ip Метод получения IP адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::Proxy::ip(const size_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.ip(bid);
}
/**
 * mac Метод получения MAC адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::Proxy::mac(const size_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.mac(bid);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Proxy::alive(const bool mode) noexcept {
	// Устанавливаем флаг долгоживущего подключения
	this->_alive = mode;
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Proxy::alive(const size_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->_timeAlive = time;
}
/**
 * alive Метод установки долгоживущего подключения
 * @param bid  идентификатор брокера
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Proxy::alive(const size_t bid, const bool mode) noexcept {
	// Получаем параметры активного клиента
	proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены, устанавливаем флаг пдолгоживущего подключения
	if(options != nullptr) options->alive = mode;
}
/**
 * stop Метод остановки брокера
 */
void awh::server::Proxy::stop() noexcept {
	// Если подключение выполнено
	if(this->_core.server.working())
		// Завершаем работу, если разрешено остановить
		this->_core.server.stop();
}
/**
 * start Метод запуска брокера
 */
void awh::server::Proxy::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->_core.server.working())
		// Выполняем запуск биндинга
		this->_core.server.start();
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Proxy::close(const size_t bid) noexcept {
	// Если идентификатор брокера существует
	if(bid > 0){
		// Получаем параметры активного клиента
		proxy_scheme_t::options_t * options = const_cast <proxy_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены, устанавливаем флаг закрытия подключения
		if(options != nullptr){
			// Выполняем отключение всех дочерних брокеров
			this->_core.client.close(options->scheme.bid());
			// Удаляем схему сети из сетевого ядра
			this->_core.client.remove(options->scheme.sid);
		}
		// Отключаем брокера от сервера
		this->_core.server.close(bid);
		// Добавляем в очередь список мусорных брокеров
		this->_garbage.emplace(bid, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
		// Если функция обратного вызова установлена
		if(this->_callback.active != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.active(bid, mode_t::DISCONNECT, this);
	}
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Proxy::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::Proxy::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
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
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Proxy::realm(const string & realm) noexcept {
	// Устанавливаем название сервера
	this->_realm = realm;
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Proxy::opaque(const string & opaque) noexcept {
	// Устанавливаем временный ключ сессии сервера
	this->_opaque = opaque;
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::Proxy::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Устанавливаем алгоритм шифрования для Digest авторизации
	this->_authHash = hash;
	// Устанавливаем тип авторизации
	this->_authType = type;
}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::Proxy::mode(const u_short flag) noexcept {
	// Устанавливаем флаг запрещающий метод CONNECT
	this->_noConnect = (flag & static_cast <uint8_t> (flag_t::NOT_CONNECT));
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flag & static_cast <uint8_t> (flag_t::WAIT_MESS));
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_core.server.noInfo(flag & static_cast <uint8_t> (flag_t::NOT_INFO));
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Proxy::total(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	this->_core.server.total(this->_scheme.sid, total);
}
/**
 * clusterSize Метод установки количества процессов кластера
 * @param size количество рабочих процессов
 */
void awh::server::Proxy::clusterSize(const size_t size) noexcept {
	// Устанавливаем количество процессов кластера
	this->_core.server.clusterSize(size);
}
/**
 * ipV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::server::Proxy::ipV6only(const bool mode) noexcept {
	// Устанавливаем количество процессов кластера
	this->_core.server.ipV6only(mode);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Proxy::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * sonet Метод установки типа сокета подключения
 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
 */
void awh::server::Proxy::sonet(const scheme_t::sonet_t sonet) noexcept {
	// Устанавливаем тип сокета подключения
	this->_core.server.sonet(sonet);
}
/**
 * family Метод установки типа протокола интернета
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::server::Proxy::family(const scheme_t::family_t family) noexcept {
	// Устанавливаем тип протокола интернета
	this->_core.server.family(family);
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param bid   идентификатор брокера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Proxy::bandWidth(const size_t bid, const string & read, const string & write) noexcept {
	// Устанавливаем пропускную способность сети
	this->_core.server.bandWidth(bid, read, write);
}
/**
 * network Метод установки параметров сети
 * @param ips    список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
void awh::server::Proxy::network(const vector <string> & ips, const vector <string> & ns, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept {
	// Устанавливаем DNS адреса клиента
	this->_core.client.serversDNS(ns);
	// Устанавливаем DNS адреса сервера
	this->_core.server.serversDNS(ns);
	// Устанавливаем параметры сети клиента
	this->_core.client.network(ips);
	// Устанавливаем параметры сети сервера
	this->_core.server.network(ips, family, sonet);
}
/**
 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::server::Proxy::verifySSL(const bool mode) noexcept {
	// Разрешаем проверку сертификата для брокера
	this->_core.client.verifySSL(mode);
	// Разрешаем проверку сертификата для сервера
	this->_core.server.verifySSL(mode);
}
/**
 * ciphers Метод установки алгоритмов шифрования
 * @param ciphers список алгоритмов шифрования для установки
 */
void awh::server::Proxy::ciphers(const vector <string> & ciphers) noexcept {
	// Устанавливаем установки алгоритмов шифрования
	this->_core.server.ciphers(ciphers);
}
/**
 * ca Метод установки доверенного сертификата (CA-файла)
 * @param trusted адрес доверенного сертификата (CA-файла)
 * @param path    адрес каталога где находится сертификат (CA-файл)
 */
void awh::server::Proxy::ca(const string & trusted, const string & path) noexcept {
	// Устанавливаем доверенный сертификат
	this->_core.client.ca(trusted, path);
}
/**
 * certificate Метод установки файлов сертификата
 * @param chain файл цепочки сертификатов
 * @param key   приватный ключ сертификата
 */
void awh::server::Proxy::certificate(const string & chain, const string & key) noexcept {
	// Устанавливаем установки файлов сертификата
	this->_core.server.certificate(chain, key);
}
/**
 * chunkSize Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::server::Proxy::chunkSize(const size_t size) noexcept {
	// Устанавливаем размер чанка
	this->_chunkSize = (size > 0 ? size : BUFFER_CHUNK);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Proxy::maxRequests(const size_t max) noexcept {
	// Устанавливаем максимальное количество запросов
	this->_maxRequests = max;
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::Proxy::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага автоматического перезапуска
	this->_core.server.clusterAutoRestart(this->_scheme.sid, mode);
}
/**
 * compress Метод установки метода сжатия
 * @param метод сжатия сообщений
 */
void awh::server::Proxy::compress(const http_t::compress_t compress) noexcept {
	// Устанавливаем метод компрессии
	this->_scheme.compress = compress;
}
/**
 * signalInterception Метод активации перехвата сигналов
 * @param mode флаг активации
 */
void awh::server::Proxy::signalInterception(const awh::core_t::signals_t mode) noexcept {
	// Выполняем активацию перехвата сигналов
	this->_core.server.signalInterception(mode);
}
/**
 * serv Метод установки данных сервиса
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Proxy::serv(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификатор сервера
	this->_sid = id;
	// Устанавливаем версию сервера
	this->_ver = ver;
	// Устанавливаем название сервера
	this->_name = name;
}
/**
 * crypto Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Proxy::crypto(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Устанавливаем флаг шифрования
	if((this->_crypt = !pass.empty())){
		// Пароль шифрования передаваемых данных
		this->_pass = pass;
		// Соль шифрования передаваемых данных
		this->_salt = salt;
		// Размер шифрования передаваемых данных
		this->_cipher = cipher;
	}
}
/**
 * Proxy Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Proxy::Proxy(const fmk_t * fmk, const log_t * log) noexcept :
 _port(SERVER_PORT), _host(""), _usock(""), _uri(fmk), _core(fmk, log), _scheme(fmk, log),
 _sid(AWH_SHORT_NAME), _ver(AWH_VERSION), _name(AWH_NAME), _realm(""), _opaque(""),
 _pass(""), _salt(""), _cipher(hash_t::cipher_t::AES128), _authHash(auth_t::hash_t::MD5),
 _authType(auth_t::type_t::NONE), _crypt(false), _alive(false), _noConnect(false),
 _chunkSize(BUFFER_CHUNK), _timeAlive(KEEPALIVE_TIMEOUT), _maxRequests(SERVER_MAX_REQUESTS), _fmk(fmk), _log(log) {
	// Выполняем отключение информационных сообщений сетевого ядра таймера
	this->_core.timer.noInfo(true);
	// Выполняем отключение информационных сообщений сетевого ядра брокера
	this->_core.client.noInfo(true);
	// Устанавливаем протокол интернет-подключения
	this->_core.server.sonet(scheme_t::sonet_t::TCP);
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&proxy_t::openServerCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("persist", std::bind(&proxy_t::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&proxy_t::connectServerCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&proxy_t::disconnectServerCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&proxy_t::readServerCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("write", std::bind(&proxy_t::writeServerCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта брокера
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const size_t, awh::Core *)> ("accept", std::bind(&proxy_t::acceptServerCallback, this, _1, _2, _3, _4, _5));
	// Добавляем схему сети в сетевое ядро
	this->_core.server.add(&this->_scheme);
	// Разрешаем автоматический перезапуск упавших процессов
	this->_core.server.clusterAutoRestart(this->_scheme.sid, true);
	// Устанавливаем функцию активации ядра сервера
	this->_core.server.on(std::bind(&proxy_t::eventsCallback, this, _1, _2));
}
