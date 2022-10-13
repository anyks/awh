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
 * runCallback Функция обратного вызова при активации ядра сервера
 * @param mode флаг запуска/остановки
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::runCallback(const bool mode, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Выполняем биндинг базы событий для клиента
		if(mode) this->_core.server.bind(reinterpret_cast <awh::core_t *> (&this->_core.client));
		// Выполняем анбиндинг базы событий клиента
		else this->_core.server.unbind(reinterpret_cast <awh::core_t *> (&this->_core.client));
	}
}
/**
 * chunking Метод обработки получения чанков
 * @param chunk бинарный буфер чанка
 * @param http  объект модуля HTTP
 */
void awh::server::Proxy::chunking(const vector <char> & chunk, const awh::http_t * http) noexcept {
	// Если данные получены, формируем тело сообщения
	if(!chunk.empty()) const_cast <awh::http_t *> (http)->body(chunk);
}
/**
 * persistServerCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && ((adj->method != web_t::method_t::CONNECT) || adj->close) && ((!adj->alive && !this->_alive) || adj->close)){
			// Если адъютант давно должен был быть отключён, отключаем его
			if(adj->close || !adj->srv.isAlive()) reinterpret_cast <server::core_t *> (core)->close(aid);
			// Иначе проверяем прошедшее время
			else {
				// Получаем текущий штамп времени
				const time_t stamp = this->_fmk->unixTimestamp();
				// Если адъютант не ответил на пинг больше двух интервалов, отключаем его
				if((stamp - adj->checkPoint) >= this->_timeAlive)
					// Завершаем работу
					reinterpret_cast <server::core_t *> (core)->close(aid);
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
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::connectClientCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Ищем идентификатор адъютанта пары
		auto it = this->_scheme.pairs.find(sid);
		// Если адъютант получен
		if(it != this->_scheme.pairs.end()){
			// Получаем параметры подключения адъютанта
			proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(it->second));
			// Если подключение не выполнено
			if((adj != nullptr) && !adj->connect){
				// Разрешаем обработки данных
				adj->locked = false;
				// Запоминаем, что подключение выполнено
				adj->connect = true;
				// Выполняем сброс состояния HTTP парсера
				adj->srv.clear();
				// Выполняем сброс состояния HTTP парсера
				adj->srv.reset();
				// Если метод подключения CONNECT
				if(adj->method == web_t::method_t::CONNECT){
					// Формируем ответ адъютанту
					const auto & response = adj->srv.response((u_int) 200);
					// Если ответ получен
					if(!response.empty()){
						// Тело полезной нагрузки
						vector <char> payload;
						// Отправляем ответ адъютанту
						this->_core.server.write(response.data(), response.size(), it->second);
						// Получаем данные тела запроса
						while(!(payload = adj->srv.payload()).empty())
							// Отправляем тело на сервер
							this->_core.server.write(payload.data(), payload.size(), it->second);
					// Выполняем отключение адъютанта
					} else this->close(it->second);
				// Отправляем сообщение на сервер, так-как оно пришло от адъютанта
				} else this->prepare(it->second, this->_scheme.sid);
			}
		}
	}
}
/**
 * connectServerCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::connectServerCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём адъютанта
		this->_scheme.set(aid);
		// Получаем параметры подключения адъютанта
		proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Устанавливаем размер чанка
			adj->cli.chunk(this->_chunkSize);
			adj->srv.chunk(this->_chunkSize);
			// Устанавливаем данные сервиса
			adj->cli.serv(this->_sid, this->_name, this->_ver);
			adj->srv.serv(this->_sid, this->_name, this->_ver);
			// Если функция обратного вызова для обработки чанков установлена
			if(this->_callback.chunking != nullptr)
				// Устанавливаем функцию обработки вызова для получения чанков
				adj->cli.chunking(this->_callback.chunking);
			// Устанавливаем функцию обработки вызова для получения чанков
			else adj->cli.chunking(std::bind(&Proxy::chunking, this, _1, _2));
			// Устанавливаем функцию обработки вызова для получения чанков
			adj->srv.chunking(std::bind(&Proxy::chunking, this, _1, _2));
			// Устанавливаем метод компрессии поддерживаемый клиентом
			adj->cli.compress(this->_scheme.compress);
			// Устанавливаем метод компрессии поддерживаемый сервером
			adj->srv.compress(this->_scheme.compress);
			// Если данные будем передавать в зашифрованном виде
			if(this->_crypt){
				// Устанавливаем параметры шифрования
				adj->cli.crypto(this->_pass, this->_salt, this->_cipher);
				adj->srv.crypto(this->_pass, this->_salt, this->_cipher);
			}
			// Определяем тип авторизации
			switch((uint8_t) this->_authType){
				// Если тип авторизации Basic
				case (uint8_t) auth_t::type_t::BASIC: {
					// Устанавливаем параметры авторизации
					adj->srv.authType(this->_authType);
					// Устанавливаем функцию проверки авторизации
					adj->srv.authCallback(this->_callback.checkAuth);
				} break;
				// Если тип авторизации Digest
				case (uint8_t) auth_t::type_t::DIGEST: {
					// Устанавливаем название сервера
					adj->srv.realm(this->_realm);
					// Устанавливаем временный ключ сессии сервера
					adj->srv.opaque(this->_opaque);
					// Устанавливаем параметры авторизации
					adj->srv.authType(this->_authType, this->_authHash);
					// Устанавливаем функцию извлечения пароля
					adj->srv.extractPassCallback(this->_callback.extractPass);
				} break;
			}
			// Устанавливаем флаг ожидания входящих сообщений
			adj->scheme.wait = this->_scheme.wait;
			// Устанавливаем количество секунд на чтение
			adj->scheme.timeouts.read = this->_scheme.timeouts.read;
			// Устанавливаем количество секунд на запись
			adj->scheme.timeouts.write = this->_scheme.timeouts.write;
			// Выполняем установку максимального количества попыток
			adj->scheme.keepAlive.cnt = this->_scheme.keepAlive.cnt;
			// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
			adj->scheme.keepAlive.idle = this->_scheme.keepAlive.idle;
			// Выполняем установку интервала времени в секундах между попытками
			adj->scheme.keepAlive.intvl = this->_scheme.keepAlive.intvl;
			// Устанавливаем событие подключения
			adj->scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&Proxy::connectClientCallback, this, _1, _2, _3));
			// Устанавливаем событие отключения
			adj->scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&Proxy::disconnectClientCallback, this, _1, _2, _3));
			// Устанавливаем функцию чтения данных
			adj->scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&Proxy::readClientCallback, this, _1, _2, _3, _4, _5));
			// Добавляем схему сети в сетевое ядро
			this->_core.client.add(&adj->scheme);
			// Создаём пару клиента и сервера
			this->_scheme.pairs.emplace(adj->scheme.sid, aid);
		}
		// Если функция обратного вызова установлена
		if(this->_callback.active != nullptr)
			// Выполняем функцию обратного вызова
			this->_callback.active(aid, mode_t::CONNECT, this);
	}
}
/**
 * disconnectClientCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::disconnectClientCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((sid > 0) && (core != nullptr)){
		// Ищем идентификатор адъютанта пары
		auto it = this->_scheme.pairs.find(sid);
		// Если адъютант получен
		if(it != this->_scheme.pairs.end()){
			// Получаем идентификатор адъютанта
			const size_t aid = it->second;
			// Удаляем пару адъютанта и сервера
			this->_scheme.pairs.erase(it);
			// Получаем параметры подключения адъютанта
			proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(aid));
			// Если подключение не выполнено, отправляем ответ адъютанту
			if((adj != nullptr) && !adj->connect)
				// Выполняем реджект
				this->reject(aid, 404);
			// Выполняем отключение клиента
			else this->close(aid);
			// Выходим из функции
			return;
		}
		// Выполняем отключение клиента
		this->_core.client.close(aid);
	}
}
/**
 * disconnectServerCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Proxy::disconnectServerCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Принудительно выполняем отключение лкиента
	if(aid > 0) this->close(aid);
}
/**
 * acceptServerCallback Функция обратного вызова при проверке подключения адъютанта
 * @param ip   адрес интернет подключения адъютанта
 * @param mac  мак-адрес подключившегося адъютанта
 * @param port порт подключившегося адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат разрешения к подключению адъютанта
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
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Proxy::readClientCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (sid > 0) && (buffer != nullptr) && (core != nullptr)){
		// Ищем идентификатор адъютанта пары
		auto it = this->_scheme.pairs.find(sid);
		// Если адъютант получен
		if(it != this->_scheme.pairs.end()){
			// Получаем параметры подключения адъютанта
			proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(it->second));
			// Если подключение выполнено, отправляем ответ адъютанту
			if((adj != nullptr) && adj->connect){
				// Если указан метод не CONNECT и функция обработки сообщения установлена
				if((this->_callback.message != nullptr) && (adj->method != web_t::method_t::CONNECT)){
					// Добавляем полученные данные в буфер
					adj->client.insert(adj->client.end(), buffer, buffer + size);
					// Выполняем обработку полученных данных
					while(!adj->close && !adj->client.empty()){
						// Выполняем парсинг полученных данных
						size_t bytes = adj->cli.parse(adj->client.data(), adj->client.size());
						// Если все данные получены
						if(adj->cli.isEnd()){
							// Получаем параметры запроса
							const auto & query = adj->cli.query();
							// Если включён режим отладки
							#if defined(DEBUG_MODE)
								// Получаем данные ответа
								const auto & response = adj->cli.response(true);
								// Если параметры ответа получены
								if(!response.empty()){
									// Выводим заголовок ответа
									cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
									// Выводим параметры ответа
									cout << string(response.begin(), response.end()) << endl;
									// Если тело ответа существует
									if(!adj->cli.body().empty())
										// Выводим сообщение о выводе чанка тела
										cout << this->_fmk->format("<body %u>", adj->cli.body().size()) << endl << endl;
									// Иначе устанавливаем перенос строки
									else cout << endl;
								}
							#endif
							// Выводим сообщение
							if(this->_callback.message(it->second, event_t::RESPONSE, &adj->cli, this)){
								// Получаем данные ответа
								const auto & response = adj->cli.response();
								// Если данные ответа получены
								if(!response.empty()){
									// Тело REST сообщения
									vector <char> entity;
									// Отправляем ответ адъютанту
									this->_core.server.write(response.data(), response.size(), it->second);
									// Получаем данные тела ответа
									while(!(entity = adj->cli.payload()).empty())
										// Отправляем тело адъютанту
										this->_core.server.write(entity.data(), entity.size(), it->second);
								}
							}
						}
						// Если парсер обработал какое-то количество байт
						if((bytes > 0) && !adj->client.empty()){
							// Если размер буфера больше количества удаляемых байт
							if(adj->client.size() >= bytes)
								// Удаляем количество обработанных байт
								adj->client.assign(adj->client.begin() + bytes, adj->client.end());
							// Если байт в буфере меньше, просто очищаем буфер
							else adj->client.clear();
							// Если данных для обработки не осталось, выходим
							if(adj->client.empty()) break;
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
 * readServerCallback Функция обратного вызова при чтении сообщения с адъютанта
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Proxy::readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (sid > 0) && (buffer != nullptr) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если указан метод CONNECT
			if(adj->connect && (adj->method == web_t::method_t::CONNECT)){
				// Получаем идентификатор адъютанта
				const size_t aid = adj->scheme.getAid();
				// Отправляем запрос на внешний сервер
				if(aid > 0){
					// Если функция обратного вызова установлена, выполняем
					if(this->_callback.binary != nullptr){
						// Выводим сообщение
						if(this->_callback.binary(aid, event_t::REQUEST, buffer, size, this))
							// Отправляем запрос на внешний сервер
							this->_core.client.write(buffer, size, aid);
					// Отправляем запрос на внешний сервер
					} else this->_core.client.write(buffer, size, aid);
				}
			// Если подключение ещё не выполнено
			} else {
				// Добавляем полученные данные в буфер
				adj->server.insert(adj->server.end(), buffer, buffer + size);
				// Выполняем обработку полученных данных
				this->prepare(aid, sid);
			}
		}
	}
}
/**
 * writeServerCallback Функция обратного вызова при записи сообщения на адъютанте
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Proxy::writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((size > 0) && (aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && adj->stopped)
			// Выполняем закрытие подключения
			this->close(aid);
	}
}
/**
 * prepare Метод обработки входящих данных
 * @param aid идентификатор адъютанта
 * @param sid идентификатор схемы сети
 */
void awh::server::Proxy::prepare(const size_t aid, const size_t sid) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0)){
		// Получаем параметры подключения адъютанта
		proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && !adj->locked){
			// Выполняем обработку полученных данных
			while(!adj->close && !adj->server.empty()){
				// Выполняем парсинг полученных данных
				size_t bytes = adj->srv.parse(adj->server.data(), adj->server.size());
				// Если все данные получены
				if(adj->srv.isEnd()){
					// Если включён режим отладки
					#if defined(DEBUG_MODE)
						// Получаем данные запроса
						const auto & request = adj->srv.request(true);
						// Если параметры запроса получены
						if(!request.empty()){
							// Выводим заголовок запроса
							cout << "\x1B[33m\x1B[1m^^^^^^^^^ REQUEST ^^^^^^^^^\x1B[0m" << endl;
							// Выводим параметры запроса
							cout << string(request.begin(), request.end()) << endl;
							// Если тело запроса существует
							if(!adj->srv.body().empty())
								// Выводим сообщение о выводе чанка тела
								cout << this->_fmk->format("<body %u>", adj->srv.body().size()) << endl << endl;
							// Иначе устанавливаем перенос строки
							else cout << endl;
						}
					#endif
					// Если подключение не установлено как постоянное
					if(!this->_alive && !adj->alive){
						// Увеличиваем количество выполненных запросов
						adj->requests++;
						// Если количество выполненных запросов превышает максимальный
						if(adj->requests >= this->_maxRequests)
							// Устанавливаем флаг закрытия подключения
							adj->close = true;
						// Получаем текущий штамп времени
						else adj->checkPoint = this->_fmk->unixTimestamp();
					// Выполняем сброс количества выполненных запросов
					} else adj->requests = 0;
					// Выполняем проверку авторизации
					switch((uint8_t) adj->srv.getAuth()){
						// Если запрос выполнен удачно
						case (uint8_t) http_t::stath_t::GOOD: {
							// Получаем флаг шифрованных данных
							adj->crypt = adj->srv.isCrypt();
							// Получаем поддерживаемый метод компрессии
							adj->compress = adj->srv.compress();
							// Если подключение не выполнено
							if(!adj->connect){
								// Получаем данные запроса
								const auto & query = adj->srv.query();
								// Выполняем проверку разрешено ли нам выполнять подключение
								const bool allow = (!this->_noConnect || (query.method != web_t::method_t::CONNECT));
								// Получаем URI запрос для сервера
								const auto & uri = (query.method == web_t::method_t::CONNECT ? query.uri : adj->srv.header("host"));
								// Сообщение запрета подключения
								const string message = (allow ? "" : "Connect method prohibited");
								// Если URI запрос для сервера получен
								if(allow && (adj->locked = !uri.empty())){
									// Запоминаем метод подключения
									adj->method = query.method;
									// Формируем адрес подключения
									adj->scheme.url = this->_scheme.uri.parse(this->_fmk->format("http://%s", uri.c_str()));
									// Выполняем запрос на сервер
									this->_core.client.open(adj->scheme.sid);
									// Выходим из функции
									return;
								// Если хост не получен
								} else {
									// Выполняем сброс состояния HTTP парсера
									adj->srv.clear();
									// Выполняем сброс состояния HTTP парсера
									adj->srv.reset();
									// Выполняем очистку буфера полученных данных
									adj->server.clear();
									// Формируем запрос реджекта
									const auto & response = adj->srv.reject(403, message);
									// Если ответ получен
									if(!response.empty()){
										// Тело полезной нагрузки
										vector <char> payload;
										// Отправляем ответ адъютанту
										this->_core.server.write(response.data(), response.size(), aid);
										// Получаем данные тела запроса
										while(!(payload = adj->srv.payload()).empty())
											// Отправляем тело на сервер
											this->_core.server.write(payload.data(), payload.size(), aid);
									// Выполняем отключение адъютанта
									} else this->close(aid);
									// Выходим из функции
									return;
								}
							// Если подключение выполнено
							} else {
								// Выполняем удаление заголовка авторизации на прокси-сервере
								adj->srv.rmHeader("proxy-authorization");
								{
									// Получаем данные запроса
									const auto & query = adj->srv.query();
									// Получаем данные заголовка Via
									string via = adj->srv.header("via");
									// Если unix-сокет активирован
									if(!this->_usock.empty()){
										// Если заголовок получен
										if(!via.empty())
											// Устанавливаем Via заголовок
											via = this->_fmk->format("%s, %.1f %s", via.c_str(), query.ver, this->_usock.c_str());
										// Иначе просто формируем заголовок Via
										else via = this->_fmk->format("%.1f %s", query.ver, this->_usock.c_str());
									// Если активирован хост и порт
									} else {
										// Если заголовок получен
										if(!via.empty())
											// Устанавливаем Via заголовок
											via = this->_fmk->format("%s, %.1f %s:%u", via.c_str(), query.ver, this->_host.c_str(), this->_port);
										// Иначе просто формируем заголовок Via
										else via = this->_fmk->format("%.1f %s:%u", query.ver, this->_host.c_str(), this->_port);
									}
									// Устанавливаем заголовок Via
									adj->srv.header("Via", via);
								}{
									// Название операционной системы
									const char * os = nullptr;
									// Определяем название операционной системы
									switch((uint8_t) this->_fmk->os()){
										// Если операционной системой является Unix
										case (uint8_t) fmk_t::os_t::UNIX: os = "Unix"; break;
										// Если операционной системой является Linux
										case (uint8_t) fmk_t::os_t::LINUX: os = "Linux"; break;
										// Если операционной системой является неизвестной
										case (uint8_t) fmk_t::os_t::NONE: os = "Unknown"; break;
										// Если операционной системой является Windows
										case (uint8_t) fmk_t::os_t::WIND32:
										case (uint8_t) fmk_t::os_t::WIND64: os = "Windows"; break;
										// Если операционной системой является MacOS X
										case (uint8_t) fmk_t::os_t::MACOSX: os = "MacOS X"; break;
										// Если операционной системой является FreeBSD
										case (uint8_t) fmk_t::os_t::FREEBSD: os = "FreeBSD"; break;
									}
									// Устанавливаем наименование агента
									adj->srv.header("X-Proxy-Agent", this->_fmk->format("(%s; %s) %s/%s", os, this->_name.c_str(), this->_sid.c_str(), this->_ver.c_str()));
								}
								// Если функция обратного вызова установлена, выполняем
								if(this->_callback.message != nullptr){
									// Выводим сообщение
									if(this->_callback.message(aid, event_t::REQUEST, &adj->srv, this)){
										// Получаем данные запроса
										const auto & request = adj->srv.request();
										// Если данные запроса получены
										if(!request.empty()){
											// Получаем идентификатор адъютанта
											const size_t aid = adj->scheme.getAid();
											// Отправляем запрос на внешний сервер
											if(aid > 0){
												// Тело REST сообщения
												vector <char> entity;
												// Отправляем серверу сообщение
												this->_core.client.write(request.data(), request.size(), aid);
												// Получаем данные тела запроса
												while(!(entity = adj->srv.payload()).empty())
													// Отправляем тело на сервер
													this->_core.client.write(entity.data(), entity.size(), aid);
											}
										}
									}
								// Если функция обратного вызова не установлена
								} else {
									// Получаем идентификатор адъютанта
									const size_t aid = adj->scheme.getAid();
									// Отправляем запрос на внешний сервер
									if(aid > 0){
										// Получаем данные запроса
										const auto & request = adj->srv.request();
										// Если данные запроса получены
										if(!request.empty()){
											// Тело REST сообщения
											vector <char> entity;
											// Если функция обратного вызова установлена, выполняем
											if(this->_callback.binary != nullptr){
												// Выводим сообщение
												if(this->_callback.binary(aid, event_t::REQUEST, request.data(), request.size(), this))
													// Отправляем запрос на внешний сервер
													this->_core.client.write(request.data(), request.size(), aid);
											// Отправляем запрос на внешний сервер
											} else this->_core.client.write(request.data(), request.size(), aid);
											// Получаем данные тела запроса
											while(!(entity = adj->srv.payload()).empty())
												// Отправляем тело на сервер
												this->_core.client.write(entity.data(), entity.size(), aid);
										}
									}
								}
							}
							// Выполняем сброс состояния HTTP парсера
							adj->srv.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->srv.reset();
							// Завершаем обработку
							goto Next;
						} break;
						// Если запрос неудачный
						case (uint8_t) http_t::stath_t::FAULT: {
							// Выполняем сброс состояния HTTP парсера
							adj->srv.clear();
							// Выполняем сброс состояния HTTP парсера
							adj->srv.reset();
							// Выполняем очистку буфера полученных данных
							adj->server.clear();
							// Формируем запрос авторизации
							const auto & response = adj->srv.reject(407);
							// Если ответ получен
							if(!response.empty()){
								// Тело полезной нагрузки
								vector <char> payload;
								// Устанавливаем флаг завершения работы
								adj->stopped = !adj->srv.isAlive();
								// Отправляем ответ адъютанту
								this->_core.server.write(response.data(), response.size(), aid);
								// Получаем данные тела запроса
								while(!(payload = adj->srv.payload()).empty())
									// Отправляем тело на сервер
									this->_core.server.write(payload.data(), payload.size(), aid);
								// Выходим из функции
								return;
							}
							// Выполняем отключение адъютанта
							this->close(aid);
							// Выходим из функции
							return;
						}
					}
				}
				// Устанавливаем метку продолжения обработки пайплайна
				Next:
				// Если парсер обработал какое-то количество байт
				if((bytes > 0) && !adj->server.empty()){
					// Если размер буфера больше количества удаляемых байт
					if(adj->server.size() >= bytes)
						// Удаляем количество обработанных байт
						adj->server.assign(adj->server.begin() + bytes, adj->server.end());
					// Если байт в буфере меньше, просто очищаем буфер
					else adj->server.clear();
					// Если данных для обработки не осталось, выходим
					if(adj->server.empty()) break;
				// Если данных для обработки недостаточно, выходим
				} else break;
			}
		}
	}
}
/**
 * init Метод инициализации WebSocket адъютанта
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
 * init Метод инициализации WebSocket адъютанта
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
	// Устанавливаем функцию запуска и остановки
	this->_callback.active = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const size_t, const event_t, http_t *, Proxy *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.message = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const size_t, const event_t, const char *, const size_t, Proxy *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.binary = callback;
}
/**
 * on Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Proxy::on(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для извлечения пароля
	this->_callback.extractPass = callback;
}
/**
 * on Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Proxy::on(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для обработки авторизации
	this->_callback.checkAuth = callback;
}
/**
 * on Метод установки функции обратного вызова для получения чанков
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const vector <char> &, const http_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения чанков
	this->_callback.chunking = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const string &, const string &, const u_int, Proxy *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.accept = callback;
}
/**
 * reject Метод отправки сообщения об ошибке
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для адъютанта
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Proxy::reject(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если подключение выполнено
	if(this->_core.server.working()){
		// Получаем параметры подключения адъютанта
		proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если отправка сообщений разблокированна
		if(adj != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			adj->srv.body(entity);
			// Устанавливаем заголовки ответа
			adj->srv.headers(headers);
			// Если подключение не установлено как постоянное, но подключение долгоживущее
			if(!this->_alive && !adj->alive && adj->srv.isAlive())
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				adj->srv.header("Keep-Alive", this->_fmk->format("timeout=%d, max=%d", this->_timeAlive / 1000, this->_maxRequests));
			// Формируем запрос авторизации
			const auto & response = adj->srv.reject(code, mess);
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(response.begin(), response.end()) << endl;
			#endif
			// Устанавливаем флаг завершения работы
			adj->stopped = true;
			// Отправляем серверу сообщение
			this->_core.server.write(response.data(), response.size(), aid);
			// Получаем данные полезной нагрузки ответа
			while(!(payload = adj->srv.payload()).empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка полезной нагрузки
					cout << this->_fmk->format("<chunk %u>", payload.size()) << endl;
				#endif
				// Отправляем тело на сервер
				this->_core.server.write(payload.data(), payload.size(), aid);
			}
		}
	}
}
/**
 * response Метод отправки сообщения адъютанту
 * @param aid     идентификатор адъютанта
 * @param code    код сообщения для адъютанта
 * @param mess    отправляемое сообщение об ошибке
 * @param entity  данные полезной нагрузки (тело сообщения)
 * @param headers HTTP заголовки сообщения
 */
void awh::server::Proxy::response(const size_t aid, const u_int code, const string & mess, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Если подключение выполнено
	if(this->_core.server.working()){
		// Получаем параметры подключения адъютанта
		proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если отправка сообщений разблокированна
		if(adj != nullptr){
			// Тело полезной нагрузки
			vector <char> payload;
			// Устанавливаем полезную нагрузку
			adj->srv.body(entity);
			// Устанавливаем заголовки ответа
			adj->srv.headers(headers);
			// Если подключение не установлено как постоянное, но подключение долгоживущее
			if(!this->_alive && !adj->alive && adj->srv.isAlive())
				// Указываем сколько запросов разрешено выполнить за указанный интервал времени
				adj->srv.header("Keep-Alive", this->_fmk->format("timeout=%d, max=%d", this->_timeAlive / 1000, this->_maxRequests));
			// Формируем запрос авторизации
			const auto & response = adj->srv.response(code, mess);
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(response.begin(), response.end()) << endl;
			#endif
			// Устанавливаем флаг завершения работы
			adj->stopped = true;
			// Отправляем серверу сообщение
			this->_core.server.write(response.data(), response.size(), aid);
			// Получаем данные полезной нагрузки ответа
			while(!(payload = adj->srv.payload()).empty()){
				// Если включён режим отладки
				#if defined(DEBUG_MODE)
					// Выводим сообщение о выводе чанка полезной нагрузки
					cout << this->_fmk->format("<chunk %u>", payload.size()) << endl;
				#endif
				// Отправляем тело на сервер
				this->_core.server.write(payload.data(), payload.size(), aid);
			}
		}
	}
}
/**
 * port Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    порт подключения адъютанта
 */
u_int awh::server::Proxy::port(const size_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getPort(aid);
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::Proxy::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getIp(aid);
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::Proxy::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getMac(aid);
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
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Proxy::alive(const size_t aid, const bool mode) noexcept {
	// Получаем параметры подключения адъютанта
	proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг пдолгоживущего подключения
	if(adj != nullptr) adj->alive = mode;
}
/**
 * stop Метод остановки адъютанта
 */
void awh::server::Proxy::stop() noexcept {
	// Если подключение выполнено
	if(this->_core.server.working())
		// Завершаем работу, если разрешено остановить
		this->_core.server.stop();
}
/**
 * start Метод запуска адъютанта
 */
void awh::server::Proxy::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->_core.server.working())
		// Выполняем запуск биндинга
		this->_core.server.start();
}
/**
 * close Метод закрытия подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::Proxy::close(const size_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	proxy_scheme_t::coffer_t * adj = const_cast <proxy_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr){
		// Выполняем отключение всех дочерних адъютантов
		this->_core.client.close(adj->scheme.getAid());
		// Удаляем схему сети из сетевого ядра
		this->_core.client.remove(adj->scheme.sid);
	}
	// Отключаем адъютанта от сервера
	this->_core.server.close(aid);
	// Выполняем удаление параметров адъютанта
	if(adj != nullptr) this->_scheme.rm(aid);
	// Если функция обратного вызова установлена
	if(this->_callback.active != nullptr)
		// Выполняем функцию обратного вызова
		this->_callback.active(aid, mode_t::DISCONNECT, this);
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
	this->_noConnect = (flag & (uint8_t) flag_t::NOCONNECT);
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flag & (uint8_t) flag_t::WAITMESS);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	this->_core.server.noInfo(flag & (uint8_t) flag_t::NOINFO);
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
 * @param aid   идентификатор адъютанта
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Proxy::bandWidth(const size_t aid, const string & read, const string & write) noexcept {
	// Устанавливаем пропускную способность сети
	this->_core.server.bandWidth(aid, read, write);
}
/**
 * network Метод установки параметров сети
 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
void awh::server::Proxy::network(const vector <string> & ip, const vector <string> & ns, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept {
	// Устанавливаем параметры сети адъютанта
	this->_core.client.network(ip, ns);
	// Устанавливаем параметры сети сервера
	this->_core.server.network(ip, ns, family, sonet);
}
/**
 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::server::Proxy::verifySSL(const bool mode) noexcept {
	// Разрешаем проверку сертификата для адъютанта
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
 * serverName Метод добавления названия сервера
 * @param name название сервера для добавления
 */
void awh::server::Proxy::serverName(const string & name) noexcept {
	// Устанавливаем названия сервера
	this->_core.server.serverName(name);
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
 _port(SERVER_PORT), _host(""), _usock(""), _core(fmk, log), _scheme(fmk, log),
 _sid(AWH_SHORT_NAME), _ver(AWH_VERSION), _name(AWH_NAME), _realm(""), _opaque(""),
 _pass(""), _salt(""), _cipher(hash_t::cipher_t::AES128), _authHash(auth_t::hash_t::MD5),
 _authType(auth_t::type_t::NONE), _crypt(false), _alive(false), _noConnect(false),
 _chunkSize(BUFFER_CHUNK), _timeAlive(KEEPALIVE_TIMEOUT), _maxRequests(SERVER_MAX_REQUESTS), _fmk(fmk), _log(log) {
	// Устанавливаем флаг запрещающий вывод информационных сообщений для адъютанта
	this->_core.client.noInfo(true);
	// Устанавливаем протокол интернет-подключения
	this->_core.server.sonet(scheme_t::sonet_t::TCP);
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&Proxy::openServerCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("persist", std::bind(&Proxy::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&Proxy::connectServerCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&Proxy::disconnectServerCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&Proxy::readServerCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("write", std::bind(&Proxy::writeServerCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта адъютанта
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const size_t, awh::Core *)> ("accept", std::bind(&Proxy::acceptServerCallback, this, _1, _2, _3, _4, _5));
	// Активируем персистентный запуск для работы пингов
	this->_core.server.persistEnable(true);
	// Добавляем схему сети в сетевое ядро
	this->_core.server.add(&this->_scheme);
	// Разрешаем автоматический перезапуск упавших процессов
	this->_core.server.clusterAutoRestart(this->_scheme.sid, true);
	// Устанавливаем функцию активации ядра сервера
	this->_core.server.callback(std::bind(&Proxy::runCallback, this, _1, _2));
}
