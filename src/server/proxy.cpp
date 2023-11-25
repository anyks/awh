/**
 * @file: proxy.cpp
 * @date: 2023-11-12
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
#include <server/proxy.hpp>

/**
 * passwordCallback Метод извлечения пароля (для авторизации методом Digest)
 * @param bid   идентификатор брокера (клиента)
 * @param login логин пользователя
 * @return      пароль пользователя хранящийся в базе данных
 */
string awh::server::Proxy::passwordCallback(const uint64_t bid, const string & login) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("extractPassword"))
		// Выполняем функцию обратного вызова
		return this->_callback.apply <string, const uint64_t, const string &> ("extractPassword", bid, login);
	// Сообщаем, что пароль для пользователя не найден
	return "";
}
/**
 * authCallback Метод проверки авторизации пользователя (для авторизации методом Basic)
 * @param bid      идентификатор брокера (клиента)
 * @param login    логин пользователя (от клиента)
 * @param password пароль пользователя (от клиента)
 * @return         результат авторизации
 */
bool awh::server::Proxy::authCallback(const uint64_t bid, const string & login, const string & password) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("checkPassword"))
		// Выполняем функцию обратного вызова
		return this->_callback.apply <bool, const uint64_t, const string &, const string &> ("checkPassword", bid, login, password);
	// Сообщаем, что пользователь не прошёл валидацию
	return false;
}
/**
 * acceptServer Метод активации клиента на сервере
 * @param ip   адрес интернет подключения
 * @param mac  аппаратный адрес подключения
 * @param port порт подключения
 * @return     результат проверки
 */
bool awh::server::Proxy::acceptServer(const string & ip, const string & mac, const u_int port) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callback.is("accept"))
		// Выполняем функцию обратного вызова
		return this->_callback.apply <bool, const string &, const string &, const u_int> ("accept", ip, mac, port);
	// Запрещаем выполнение подключения
	return false;
}
/** 
 * eraseClient Метод удаления подключённого клиента
 * @param bid идентификатор брокера
 */
void awh::server::Proxy::eraseClient(const uint64_t bid) noexcept {
	// Выполняем поиск объекта клиента
	auto it = this->_clients.find(bid);
	// Если активный клиент найден
	if(it != this->_clients.end())
		// Выполняем удаление клиента из списка клиентов
		this->_clients.erase(it);
	// Если функция обратного вызова установлена
	if(this->_callback.is("erase"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const uint64_t> ("erase", bid);
}
/**
 * endClient Метод завершения запроса клиента
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param direct направление передачи данных
 */
void awh::server::Proxy::endClient(const int32_t sid, const uint64_t bid, const client::web_t::direct_t direct) noexcept {
	// Блокируем пустую переменную
	(void) sid;
	// Если мы получили данные
	if(direct == client::web_t::direct_t::RECV)
		// Выводим полученный результат
		this->completed(bid);
}
/**
 * responseClient Метод получения сообщения с удалённого сервера
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера (клиента)
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 */
void awh::server::Proxy::responseClient(const int32_t sid, const uint64_t bid, const u_int code, const string & message) noexcept {
	// Блокируем пустую переменную
	(void) sid;
	// Если возникла ошибка, выводим сообщение
	if(code >= 300)
		// Выводим сообщение о неудачном запросе
		this->_log->print("Response from [%zu] failed: %u %s", log_t::flag_t::WARNING, bid, code, message.c_str());
	// Если функция обратного вызова установлена
	if(this->_callback.is("response"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const uint64_t, const u_int, const string &> ("response", bid, code, message);
}
/**
 * activeClient Метод идентификации активности на Web сервере (для клиента)
 * @param bid  идентификатор брокера (клиента)
 * @param mode режим события подключения
 */
void awh::server::Proxy::activeClient(const uint64_t bid, const client::web_t::mode_t mode) noexcept {
	// Выполняем поиск объекта клиента
	auto it = this->_clients.find(bid);
	// Если активный клиент найден
	if(it != this->_clients.end()){
		// Определяем активность клиента
		switch(static_cast <uint8_t> (mode)){
			// Если производится подключение клиента к серверу
			case static_cast <uint8_t> (client::web_t::mode_t::CONNECT): {
				// Определяем активный метод запроса клиента
				switch(static_cast <uint8_t> (it->second->request.params.method)){
					// Если запрашивается клиентом метод GET
					case static_cast <uint8_t> (awh::web_t::method_t::GET):
					// Если запрашивается клиентом метод PUT
					case static_cast <uint8_t> (awh::web_t::method_t::PUT):
					// Если запрашивается клиентом метод DEL
					case static_cast <uint8_t> (awh::web_t::method_t::DEL):
					// Если запрашивается клиентом метод POST
					case static_cast <uint8_t> (awh::web_t::method_t::POST):
					// Если запрашивается клиентом метод HEAD
					case static_cast <uint8_t> (awh::web_t::method_t::HEAD):
					// Если запрашивается клиентом метод PATCH
					case static_cast <uint8_t> (awh::web_t::method_t::PATCH):
					// Если запрашивается клиентом метод TRACE
					case static_cast <uint8_t> (awh::web_t::method_t::TRACE):
					// Если запрашивается клиентом метод OPTIONS
					case static_cast <uint8_t> (awh::web_t::method_t::OPTIONS): {
						// Создаём объект запроса
						client::web_t::request_t request;
						// Устанавливаем адрес запроса
						request.url = it->second->request.params.url;
						// Устанавливаем тепло запроса
						request.entity = it->second->response.entity;
						// Запоминаем переданные заголовки
						request.headers = it->second->request.headers;
						// Устанавливаем метод запроса
						request.method = it->second->request.params.method;
						// Выполняем запрос на сервер
						it->second->awh.send(std::move(request));
					} break;
					// Если запрашивается клиентом метод CONNECT
					case static_cast <uint8_t> (awh::web_t::method_t::CONNECT): {
						// Если подключение ещё не выполнено
						if(!it->second->connected){
							// Запоминаем что подключение установлено
							it->second->connected = !it->second->connected;
							// Если тип сокета установлен как TCP/IP
							if(this->_core.sonet() == awh::scheme_t::sonet_t::TCP)
								// Подписываемся на получение сырых данных полученных клиентом с удалённого сервера
								it->second->awh.on((function <bool (const char *, const size_t)>) std::bind(&server::proxy_t::raw, this, bid, broker_t::CLIENT, _1, _2));
							// Выполняем отправку ответа клиенту
							this->_server.send(bid);
						}
					} break;
				}
			} break;
			// Если производится отключение клиента от сервера
			case static_cast <uint8_t> (client::web_t::mode_t::DISCONNECT): {
				// Снимаем флаг выполненного подключения
				it->second->connected = false;
				// Выполняем закрытие подключения
				this->close(bid);
			} break;
		}
	}
	// Если функция обратного вызова установлена
	if(this->_callback.is("active")){
		// Результат получения события
		server::web_t::mode_t result;
		// Определяем активность клиента
		switch(static_cast <uint8_t> (mode)){
			// Если производится подключение клиента к серверу
			case static_cast <uint8_t> (client::web_t::mode_t::CONNECT):
				// Устанавливаем полученное событие подклчюения к серверу
				result = server::web_t::mode_t::CONNECT;
			break;
			// Если производится отключение клиента от сервера
			case static_cast <uint8_t> (client::web_t::mode_t::DISCONNECT):
				// Устанавливаем полученное событие отключения от сервера
				result = server::web_t::mode_t::DISCONNECT;
			break;
		}
		// Выполняем функцию обратного вызова
		this->_callback.call <const uint64_t, const broker_t, const web_t::mode_t> ("active", bid, broker_t::CLIENT, result);
	}
}
/**
 * activeServer Метод идентификации активности на Web сервере (для сервера)
 * @param bid  идентификатор брокера (клиента)
 * @param mode режим события подключения
 */
void awh::server::Proxy::activeServer(const uint64_t bid, const server::web_t::mode_t mode) noexcept {
	// Определяем активность клиента
	switch(static_cast <uint8_t> (mode)){
		// Если производится подключение клиента к серверу
		case static_cast <uint8_t> (server::web_t::mode_t::CONNECT): {
			// Устанавливаем постоянное подключение для клиента
			this->_server.alive(bid, true);
			// Выполняем создание клиента
			auto ret = this->_clients.emplace(bid, unique_ptr <client_t> (new client_t(this->_fmk, this->_log)));
			// Если чёрный список DNS-адресов установлен
			if(!this->_settings.dns.blacklist.empty()){
				// Выполняем перебор всего чёрного списка DNS-адресов
				for(auto & item : this->_settings.dns.blacklist)
					// Выполняем добавление домена в чёрный список
					ret.first->second->awh.setToDNSBlackList(item.first, item.second);
			}
			// Если размер передаваемых чанков установлен
			if(this->_settings.chunk > 0)
				// Устанавливаем размер передаваемых чанков
				ret.first->second->awh.chunk(this->_settings.chunk);
			// Если адрес файла с локальными хостами установлен
			if(!this->_settings.dns.hosts.empty())
				// Устанавливаем адрес файла с локальными хостами
				ret.first->second->awh.hosts(this->_settings.dns.hosts);
			// Если количество попыток выполнения редиректов установлено
			if(this->_settings.attempts > 0)
				// Устанавливаем количество попыток выполнения редиректов
				ret.first->second->awh.attempts(this->_settings.attempts);
			// Если заголовок User-Agent установлен
			if(!this->_settings.userAgent.empty())
				// Устанавливаем заголовок User-Agent
				ret.first->second->awh.userAgent(this->_settings.userAgent);
			// Если префикс переменной окружения установлен
			if(!this->_settings.dns.prefix.empty())
				// Выполняем установку префикса переменной окружения
				ret.first->second->awh.prefixDNS(this->_settings.dns.prefix);
			// Если время жизни DNS-кэша установлено
			if(this->_settings.dns.ttl > 0)
				// Выполняем установку времени жизни DNS-кэша
				ret.first->second->awh.timeToLiveDNS(this->_settings.dns.ttl);
			// Если таймаут DNS-запроса установлен
			if(this->_settings.dns.timeout > 0)
				// Устанавливаем таймаут DNS-запроса
				ret.first->second->awh.timeoutDNS(this->_settings.dns.timeout);
			// Если список поддерживаемых компрессоров установлен
			if(!this->_settings.compressors.empty())
				// Устанавливаем список поддерживаемых компрессоров
				ret.first->second->awh.compressors(this->_settings.compressors);
			// Если список алгоритмов шифрования установлен
			if(!this->_settings.encryption.ciphers.empty())
				// Устанавливаем список алгоритмов шифрования
				ret.first->second->core.ciphers(this->_settings.encryption.ciphers);
			// Если флаг активации механизма шифрования установлен
			if(this->_settings.encryption.mode)
				// Выполняем установку флага активации механизма шифрования
				ret.first->second->awh.encryption(this->_settings.encryption.mode);
			// Если параметры авторизации пользователя на удалённом сервере установлены
			if(!this->_settings.login.empty() && !this->_settings.password.empty())
				// Выполняем установку данных пользователя для авторизации на удалённом сервере
				ret.first->second->awh.user(this->_settings.login, this->_settings.password);
			// Если параметры авторизации на прокси-сервере установлены
			if(!this->_settings.proxy.uri.empty())
				// Выполняем установку параметров авторизации на прокси-серверов
				ret.first->second->awh.proxy(this->_settings.proxy.uri, this->_settings.proxy.family);
			// Если параметры времени жизни подключения установлены
			if((this->_settings.ka.cnt > 0) && (this->_settings.ka.idle > 0) && (this->_settings.ka.intvl > 0))
				// Устанавливаем параметры времени жизни подключения
				ret.first->second->awh.keepAlive(this->_settings.ka.cnt, this->_settings.ka.idle, this->_settings.ka.intvl);
			// Если параметры шифрования установлены
			if(!this->_settings.encryption.pass.empty())
				// Устанавливаем параметры шифрования
				ret.first->second->awh.encryption(this->_settings.encryption.pass, this->_settings.encryption.salt, this->_settings.encryption.cipher);
			// Устанавливаем флаг запрещающий вывод информационных сообщений
			ret.first->second->core.noInfo(true);
			// Устанавливаем тип сокета подключения (TCP / UDP)
			ret.first->second->core.sonet(this->_settings.sonet);
			// Устанавливаем тип протокола интернета HTTP/2
			ret.first->second->core.proto(awh::engine_t::proto_t::HTTP2);
			// Устанавливаем флаг верификации доменного имени
			ret.first->second->core.verifySSL(this->_settings.encryption.verify);
			// Выполняем установку CA-файлов сертификата
			ret.first->second->core.ca(this->_settings.ca.trusted, this->_settings.ca.path);
			// Устанавливаем параметры идентификатора Web-клиента
			ret.first->second->awh.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
			// Устанавливаем параметры авторизации на удалённом сервере
			ret.first->second->awh.authType(this->_settings.auth.type, this->_settings.auth.hash);
			// Устанавливаем параметры маркеров детектирования обмена данных
			ret.first->second->awh.bytesDetect(this->_settings.marker.read, this->_settings.marker.write);
			// Выполняем установку сетевых параметров подключения
			ret.first->second->awh.network(this->_settings.ips, this->_settings.ns, this->_settings.family);
			// Устанавливаем параметры авторизации на удалённом прокси-сервере
			ret.first->second->awh.authTypeProxy(this->_settings.proxy.auth.type, this->_settings.proxy.auth.hash);
			// Выполняем установку таймаутов на обмен данными в миллисекундах
			ret.first->second->awh.waitTimeDetect(this->_settings.wtd.read, this->_settings.wtd.write, this->_settings.wtd.connect);
			// Устанавливаем функцию обратного вызова активности на Web-сервере
			ret.first->second->awh.on((function <void (const client::web_t::mode_t)>) std::bind(&server::proxy_t::activeClient, this, bid, _1));
			// Устанавливаем функцию обратного вызова при завершении работы потока передачи данных клиента
			ret.first->second->awh.on((function <void (const int32_t, const client::web_t::direct_t)>) std::bind(&server::proxy_t::endClient, this, _1, bid, _2));
			// Если функция обратного вызова установлена
			if(this->_callback.is("chunks"))
				// Выполняем установку функции обратного вызова получения бинарных чанков
				ret.first->second->awh.on((function <void (const int32_t, const vector <char> &)>) std::bind(this->_callback.get <void (const int32_t, const uint64_t, const broker_t, const vector <char> &)> ("chunks"), _1, bid, broker_t::CLIENT, _2));
			// Если функция обратного вызова установлена
			if(this->_callback.is("header"))
				// Выполняем установку функции обратного вызова получения заголовка
				ret.first->second->awh.on((function <void (const int32_t, const string &, const string &)>) std::bind(this->_callback.get <void (const int32_t, const uint64_t, const broker_t, const string &, const string &)> ("header"), _1, bid, broker_t::CLIENT, _2, _3));
			// Если функция обратного вызова установлена
			if(this->_callback.is("error"))
				// Выполняем установку функции обратного вызова получения ошибок клиента
				ret.first->second->awh.on((function <void (const log_t::flag_t, const http::error_t, const string &)>) std::bind(this->_callback.get <void (const uint64_t, const broker_t, const log_t::flag_t, const http::error_t, const string &)> ("error"), bid, broker_t::CLIENT, _1, _2, _3));
			// Если функция обратного вызова установлена
			if(this->_callback.is("origin"))
				// Выполняем установку функции обратного вызова при получении источников подключения
				ret.first->second->awh.on((function <void (const vector <string> &)>) std::bind(this->_callback.get <void (const uint64_t, const vector <string> &)> ("origin"), bid, _1));
			// Если функция обратного вызова установлена
			if(this->_callback.is("altsvc"))
				// Устанавливаем функцию обратного вызова при получении альтернативного источника
				ret.first->second->awh.on((function <void (const string &, const string &)> ) std::bind(this->_callback.get <void (const uint64_t, const string &, const string &)> ("altsvc"), bid, _1, _2));
			// Если функция обратного вызова установлена
			if(this->_callback.is("push"))
				// Выполняем установку функции обратного вызова при получении PUSH уведомлений
				ret.first->second->awh.on((function <void (const int32_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)>) std::bind(this->_callback.get <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("push"), _1, bid, _2, _3, _4));
		} break;
		// Если производится отключение клиента от сервера
		case static_cast <uint8_t> (server::web_t::mode_t::DISCONNECT): {
			// Выполняем поиск клиента в списке
			auto it = this->_clients.find(bid);
			// Если клиент в списке найден
			if(it != this->_clients.end()){
				// Снимаем флаг выполненного подключения
				it->second->connected = false;
				// Выполняем отключение клиента от сетевого ядра
				this->_core.unbind(&it->second->core);
			}
		} break;
	}
	// Если функция обратного вызова установлена
	if(this->_callback.is("active"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const uint64_t, const broker_t, const web_t::mode_t> ("active", bid, broker_t::SERVER, mode);
}
/**
 * entityClient Метод получения тела ответа с сервера клиенту
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера (клиента)
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 * @param entity  тело ответа клиенту с сервера
 */
void awh::server::Proxy::entityClient(const int32_t sid, const uint64_t bid, const u_int code, const string & message, const vector <char> & entity) noexcept {
	// Выполняем поиск объекта клиента
	auto it = this->_clients.find(bid);
	// Если активный клиент найден
	if(it != this->_clients.end()){
		// Если тело ответа с сервера получено
		if(!entity.empty())
			// Устанавливаем полученные данные тела ответа
			it->second->response.entity.assign(entity.begin(), entity.end());
		// Выполняем очистку тела ответа
		else it->second->response.entity.clear();
		// Если функция обратного вызова установлена
		if(this->_callback.is("entityClient"))
			// Выполняем функцию обратного вызова
			this->_callback.call <const uint64_t, const u_int, const string &, vector <char> *> ("entityClient", bid, code, message, &it->second->response.entity);
		// Выводим полученный результат
		this->completed(bid);
	}
}
/**
 * entityServer Метод получения тела запроса с клиента на сервере
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера (клиента)
 * @param method метод запроса на уделённый сервер
 * @param url    URL-адрес параметров запроса
 * @param entity тело запроса с клиента на сервере
 */
void awh::server::Proxy::entityServer(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const vector <char> & entity) noexcept {
	// Выполняем поиск объекта клиента
	auto it = this->_clients.find(bid);
	// Если активный клиент найден
	if(it != this->_clients.end()){
		// Если тело запроса с сервера получено
		if(!entity.empty())
			// Устанавливаем полученные данные тела запроса
			it->second->request.entity.assign(entity.begin(), entity.end());
		// Выполняем очистку тела запроса
		else it->second->request.entity.clear();
	}
	// Если функция обратного вызова установлена
	if(this->_callback.is("entityServer"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, vector <char> *> ("entityServer", bid, method, url, &it->second->request.entity);
}
/**
 * headersClient Метод получения заголовков ответа с сервера клиенту
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера (клиента)
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 * @param headers заголовки HTTP-ответа
 */
void awh::server::Proxy::headersClient(const int32_t sid, const uint64_t bid, const u_int code, const string & message, const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем поиск объекта клиента
	auto it = this->_clients.find(bid);
	// Если активный клиент найден
	if(it != this->_clients.end()){
		// Если заголовки ответа получены
		if(!headers.empty()){
			// Список заголовков Via
			vector <string> via;
			// Устанавливаем полученные заголовки
			it->second->response.headers = headers;
			// Устанавливаем код ответа сервера
			it->second->response.params.code = code;
			// Устанавливаем сообщение ответа сервера
			it->second->response.params.message = message;
			// Компрессор которым необходимо выполнить сжатие контента
			http_t::compress_t compress = http_t::compress_t::NONE;
			// Выполняем перебор всех полученных заголовков
			for(auto jt = it->second->response.headers.begin(); jt != it->second->response.headers.end();){
				// Если получен заголовок Via
				if(this->_fmk->exists("via", jt->first)){
					// Добавляем заголовок в список
					via.push_back(jt->second);
					// Выполняем удаление заголовка
					jt = it->second->response.headers.erase(jt);
					// Продолжаем перебор дальше
					continue;
				// Если мы получили заголовок сообщающий о том, в каком формате закодированны данные
				} else if(this->_fmk->exists("content-encoding", jt->first)) {
					// Если флаг рекомпрессии данных прокси-сервером не установлен
					if(this->_flags.count(flag_t::RECOMPRESS) == 0){
						// Если данные пришли сжатые методом Brotli
						if(this->_fmk->exists("br", jt->second))
							// Устанавливаем тип компрессии полезной нагрузки
							compress = http_t::compress_t::BROTLI;
						// Если данные пришли сжатые методом GZip
						else if(this->_fmk->exists("gzip", jt->second))
							// Устанавливаем тип компрессии полезной нагрузки
							compress = http_t::compress_t::GZIP;
						// Если данные пришли сжатые методом Deflate
						else if(this->_fmk->exists("deflate", jt->second))
							// Устанавливаем тип компрессии полезной нагрузки
							compress = http_t::compress_t::DEFLATE;
					}
				}
				// Продолжаем перебор дальше
				++jt;
			}
			// Если флаг рекомпрессии данных прокси-сервером установлен
			if(this->_flags.count(flag_t::RECOMPRESS) > 0)
				// Устанавливаем компрессор рекомпрессии
				compress = this->_compressor;
			// Получаем объект HTTP-парсера
			const awh::http_t * http = this->_server.parser(bid);
			// Если объект HTTP-парсера получен
			if(http != nullptr)
				// Устанавливаем параметры компрессии
				const_cast <awh::http_t *> (http)->compression(compress);
			// Выполняем получение заголовка Via
			const string & header = this->via(bid, via);
			// Если заголовок получен
			if(!header.empty())
				// Устанавливаем загловок Via
				it->second->response.headers.emplace("Via", header);
			// Если функция обратного вызова установлена
			if(this->_callback.is("headersClient"))
				// Выполняем функцию обратного вызова
				this->_callback.call <const uint64_t, const u_int, const string &, unordered_multimap <string, string> *> ("headersClient", bid, code, message, &it->second->response.headers);
		}
	}
}
/**
 * headersServer Метод получения заголовков запроса с клиента на сервере
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера (клиента)
 * @param method  метод запроса на уделённый сервер
 * @param url     URL-адрес параметров запроса
 * @param headers заголовки HTTP-запроса
 */
void awh::server::Proxy::headersServer(const int32_t sid, const uint64_t bid, const awh::web_t::method_t method, const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем поиск объекта клиента
	auto it = this->_clients.find(bid);
	// Если активный клиент найден
	if(it != this->_clients.end()){
		// Если заголовки ответа получены
		if(!headers.empty()){
			// Список заголовков Via
			vector <string> via;
			// Устанавливаем полученные заголовки
			it->second->request.headers = headers;
			// Устанавливаем URL-адрес запроса
			it->second->request.params.url = url;
			// Устанавливаем метод запроса
			it->second->request.params.method = method;
			// Выполняем перебор всех полученных заголовков
			for(auto jt = it->second->request.headers.begin(); jt != it->second->request.headers.end();){
				// Если получен заголовок Via
				if(this->_fmk->exists("via", jt->first)){
					// Добавляем заголовок в список
					via.push_back(jt->second);
					// Выполняем удаление заголовка
					jt = it->second->request.headers.erase(jt);
					// Продолжаем перебор дальше
					continue;
				// Если заголовок соответствует прокси-серверу
				} else if(this->_fmk->exists("te", jt->first) || this->_fmk->exists("proxy-", jt->first)) {
					// Выполняем удаление заголовка
					jt = it->second->request.headers.erase(jt);
					// Продолжаем перебор дальше
					continue;
				// Если найден заголовок подключения
				} else if(this->_fmk->exists("connection", jt->first)) {
					// Переводим значение в нижний регистр
					this->_fmk->transform(jt->second, fmk_t::transform_t::LOWER);
					// Выполняем поиск заголовка Transfer-Encoding
					const size_t pos = jt->second.find("te");
					// Если заголовок найден
					if(pos != string::npos){
						// Выполняем удаление значение TE из заголовка
						jt->second.erase(pos, 2);
						// Если первый символ является запятой, удаляем
						if(jt->second.front() == ',')
							// Удаляем запятую
							jt->second.erase(0, 1);
						// Выполняем удаление лишних пробелов
						this->_fmk->transform(jt->second, fmk_t::transform_t::TRIM);
					}
				}
				// Продолжаем перебор дальше
				++jt;
			}
			// Выполняем получение заголовка Via
			const string & header = this->via(bid, via);
			// Если заголовок получен
			if(!header.empty())
				// Устанавливаем загловок Via
				it->second->request.headers.emplace("Via", header);
			// Если функция обратного вызова установлена
			if(this->_callback.is("headersServer"))
				// Выполняем функцию обратного вызова
				this->_callback.call <const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, unordered_multimap <string, string> *> ("headersServer", bid, method, url, &it->second->request.headers);
		}
	}
}
/**
 * handshake Метод получения удачного запроса
 * @param sid   идентификатор потока
 * @param bid   идентификатор брокера
 * @param agent идентификатор агента клиента
 */
void awh::server::Proxy::handshake(const int32_t sid, const uint64_t bid, const server::web_t::agent_t agent) noexcept {
	// Если агент клиента соответствует HTTP протоколу
	if(agent == server::web_t::agent_t::HTTP){
		// Выполняем поиск объекта клиента
		auto it = this->_clients.find(bid);
		// Если активный клиент найден
		if(it != this->_clients.end()){
			// Если функция обратного вызова установлена
			if(this->_callback.is("handshake"))
				// Выполняем функцию обратного вызова
				this->_callback.call <const uint64_t, const engine_t::proto_t> ("handshake", bid, this->_core.proto(bid));
			// Определяем тип активного сокета сервера
			switch(static_cast <uint8_t> (this->_core.sonet())){
				// Если тип сокета установлен как TCP/IP
				case static_cast <uint8_t> (awh::scheme_t::sonet_t::TCP): {
					// Запоминаем идентификатор потока
					it->second->sid = sid;
					// Создаём список флагов клиента
					set <client::web_t::flag_t> flags = {
						client::web_t::flag_t::ALIVE,
						client::web_t::flag_t::NOT_STOP,
						client::web_t::flag_t::NOT_INFO,
						client::web_t::flag_t::REDIRECTS
					};
					// Если флаг ожидания входящих сообщений установлен
					if(this->_flags.count(flag_t::WAIT_MESS) > 0)
						// Устанавливаем флаг ожидания входящих сообщений
						flags.emplace(client::web_t::flag_t::WAIT_MESS);
					// Если флаг проверки домена установлен
					if(this->_flags.count(flag_t::VERIFY_SSL) > 0)
						// Выполняем установку флага проверки домена
						flags.emplace(client::web_t::flag_t::VERIFY_SSL);
					// Если флаг резрешающий метод CONNECT для прокси-клиента установлен
					if(this->_flags.count(flag_t::CONNECT_METHOD_CLIENT_ENABLE) > 0)
						// Выполняем установку флага разрешающего метода CONNECT для прокси-клиента
						flags.emplace(client::web_t::flag_t::CONNECT_METHOD_ENABLE);
					// Если метод запроса не является методом CONNECT
					if(it->second->request.params.method != awh::web_t::method_t::CONNECT){
						// Подписываемся на получение сообщения сервера
						it->second->awh.on((function <void (const int32_t, const u_int, const string &)>) std::bind(&server::proxy_t::responseClient, this, _1, bid, _2, _3));
						// Устанавливаем функцию обратного вызова при получении HTTP-тела ответа с сервера клиенту
						it->second->awh.on((function <void (const int32_t, const u_int, const string &, const vector <char> &)>) std::bind(&server::proxy_t::entityClient, this, _1, bid, _2, _3, _4));
						// Устанавливаем функцию обратного вызова при получении HTTP-заголовков ответа с сервера клиенту
						it->second->awh.on((function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)>) std::bind(&server::proxy_t::headersClient, this, _1, bid, _2, _3, _4));
					// Если метод CONNECT не разрешён для запроса
					} else if(this->_flags.count(flag_t::CONNECT_METHOD_SERVER_ENABLE) < 1) {
						// Формируем сообщение ответа
						const string message = "CONNECT method is forbidden on the proxy-server";
						// Формируем тело ответа
						const string & body = this->_fmk->format("<html>\n<head>\n<title>%u %s</title>\n</head>\n<body>\n<h2>%u %s</h2>\n</body>\n</html>\n", 403, message.c_str(), 403, message.c_str());
						// Если метод CONNECT запрещено использовать
						this->_server.send(bid, 403, message, vector <char> (body.begin(), body.end()), {
							{"Connection", "close"},
							{"Proxy-Connection", "close"},
							{"Content-type", "text/html; charset=utf-8"}
						});
						// Выходим из функции
						return;
					// Если метод CONNECT разрешён, выполняем запрет инициализации SSL-контекста
					} else flags.emplace(client::web_t::flag_t::NO_INIT_SSL);
					// Устанавливаем флаги настроек модуля
					it->second->awh.mode(std::move(flags));
					// Выполняем инициализацию подключения
					it->second->awh.init(this->_uri.origin(it->second->request.params.url), {
						awh::http_t::compress_t::BROTLI,
						awh::http_t::compress_t::GZIP,
						awh::http_t::compress_t::DEFLATE
					});
					// Выполняем подключение клиента к сетевому ядру
					this->_core.bind(&it->second->core);
				} break;
				// Если тип сокета установлен как TCP/IP TLS
				case static_cast <uint8_t> (awh::scheme_t::sonet_t::TLS): {
					// Определяем активный метод запроса клиента
					switch(static_cast <uint8_t> (it->second->request.params.method)){
						// Если запрашивается клиентом метод GET
						case static_cast <uint8_t> (awh::web_t::method_t::GET):
						// Если запрашивается клиентом метод PUT
						case static_cast <uint8_t> (awh::web_t::method_t::PUT):
						// Если запрашивается клиентом метод DEL
						case static_cast <uint8_t> (awh::web_t::method_t::DEL):
						// Если запрашивается клиентом метод POST
						case static_cast <uint8_t> (awh::web_t::method_t::POST):
						// Если запрашивается клиентом метод HEAD
						case static_cast <uint8_t> (awh::web_t::method_t::HEAD):
						// Если запрашивается клиентом метод PATCH
						case static_cast <uint8_t> (awh::web_t::method_t::PATCH):
						// Если запрашивается клиентом метод TRACE
						case static_cast <uint8_t> (awh::web_t::method_t::TRACE):
						// Если запрашивается клиентом метод OPTIONS
						case static_cast <uint8_t> (awh::web_t::method_t::OPTIONS): {
							// Если подключение ещё не выполнено
							if(!it->second->connected){
								// Запоминаем идентификатор потока
								it->second->sid = sid;
								// Создаём список флагов клиента
								set <client::web_t::flag_t> flags = {
									client::web_t::flag_t::ALIVE,
									client::web_t::flag_t::NOT_STOP,
									// client::web_t::flag_t::NOT_INFO,
									client::web_t::flag_t::REDIRECTS
								};
								// Если флаг ожидания входящих сообщений установлен
								if(this->_flags.count(flag_t::WAIT_MESS) > 0)
									// Устанавливаем флаг ожидания входящих сообщений
									flags.emplace(client::web_t::flag_t::WAIT_MESS);
								// Если флаг проверки домена установлен
								if(this->_flags.count(flag_t::VERIFY_SSL) > 0)
									// Выполняем установку флага проверки домена
									flags.emplace(client::web_t::flag_t::VERIFY_SSL);
								// Если флаг резрешающий метод CONNECT для прокси-клиента установлен
								if(this->_flags.count(flag_t::CONNECT_METHOD_CLIENT_ENABLE) > 0)
									// Выполняем установку флага разрешающего метода CONNECT для прокси-клиента
									flags.emplace(client::web_t::flag_t::CONNECT_METHOD_ENABLE);
								// Если порт сервера не стандартный, устанавливаем схему протокола
								if((it->second->request.params.url.port != 80) && (it->second->request.params.url.port != 443))
									// Выполняем установку защищённого протокола
									it->second->request.params.url.schema = "https";
								// Подписываемся на получение сообщения сервера
								it->second->awh.on((function <void (const int32_t, const u_int, const string &)>) std::bind(&server::proxy_t::responseClient, this, _1, bid, _2, _3));
								// Устанавливаем функцию обратного вызова при получении HTTP-тела ответа с сервера клиенту
								it->second->awh.on((function <void (const int32_t, const u_int, const string &, const vector <char> &)>) std::bind(&server::proxy_t::entityClient, this, _1, bid, _2, _3, _4));
								// Устанавливаем функцию обратного вызова при получении HTTP-заголовков ответа с сервера клиенту
								it->second->awh.on((function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)>) std::bind(&server::proxy_t::headersClient, this, _1, bid, _2, _3, _4));
								// Устанавливаем флаги настроек модуля
								it->second->awh.mode(std::move(flags));
								// Выполняем инициализацию подключения
								it->second->awh.init(this->_uri.origin(it->second->request.params.url), {
									awh::http_t::compress_t::BROTLI,
									awh::http_t::compress_t::GZIP,
									awh::http_t::compress_t::DEFLATE
								});
								// Выполняем подключение клиента к сетевому ядру
								this->_core.bind(&it->second->core);
							// Если подключение уже выполнено
							} else {
								// Создаём объект запроса
								client::web_t::request_t request;
								// Устанавливаем адрес запроса
								request.url = it->second->request.params.url;
								// Устанавливаем тепло запроса
								request.entity = it->second->response.entity;
								// Запоминаем переданные заголовки
								request.headers = it->second->request.headers;
								// Устанавливаем метод запроса
								request.method = it->second->request.params.method;
								// Выполняем запрос на сервер
								it->second->awh.send(std::move(request));
							}
						} break;
						// Если запрашивается клиентом метод CONNECT
						case static_cast <uint8_t> (awh::web_t::method_t::CONNECT): {
							// Если метод CONNECT не разрешён для запроса
							if(this->_flags.count(flag_t::CONNECT_METHOD_SERVER_ENABLE) < 1){
								// Формируем сообщение ответа
								const string message = "CONNECT method is forbidden on the proxy-server";
								// Формируем тело ответа
								const string & body = this->_fmk->format("<html>\n<head>\n<title>%u %s</title>\n</head>\n<body>\n<h2>%u %s</h2>\n</body>\n</html>\n", 403, message.c_str(), 403, message.c_str());
								// Если метод CONNECT запрещено использовать
								this->_server.send(bid, 403, message, vector <char> (body.begin(), body.end()), {
									{"Connection", "close"},
									{"Proxy-Connection", "close"},
									{"Content-type", "text/html; charset=utf-8"}
								});
								// Выходим из функции
								return;
							}
							// Запоминаем идентификатор потока
							it->second->sid = sid;
							// Создаём список флагов клиента
							set <client::web_t::flag_t> flags = {
								client::web_t::flag_t::ALIVE,
								client::web_t::flag_t::NOT_STOP,
								// client::web_t::flag_t::NOT_INFO,
								client::web_t::flag_t::REDIRECTS
							};
							// Если флаг ожидания входящих сообщений установлен
							if(this->_flags.count(flag_t::WAIT_MESS) > 0)
								// Устанавливаем флаг ожидания входящих сообщений
								flags.emplace(client::web_t::flag_t::WAIT_MESS);
							// Если флаг проверки домена установлен
							if(this->_flags.count(flag_t::VERIFY_SSL) > 0)
								// Выполняем установку флага проверки домена
								flags.emplace(client::web_t::flag_t::VERIFY_SSL);
							// Если флаг резрешающий метод CONNECT для прокси-клиента установлен
							if(this->_flags.count(flag_t::CONNECT_METHOD_CLIENT_ENABLE) > 0)
								// Выполняем установку флага разрешающего метода CONNECT для прокси-клиента
								flags.emplace(client::web_t::flag_t::CONNECT_METHOD_ENABLE);
							// Устанавливаем флаги настроек модуля
							it->second->awh.mode(std::move(flags));
							// Если порт сервера не стандартный, устанавливаем схему протокола
							if((it->second->request.params.url.port != 80) && (it->second->request.params.url.port != 443))
								// Выполняем установку защищённого протокола
								it->second->request.params.url.schema = "https";
							// Выполняем инициализацию подключения
							it->second->awh.init(this->_uri.origin(it->second->request.params.url), {
								awh::http_t::compress_t::BROTLI,
								awh::http_t::compress_t::GZIP,
								awh::http_t::compress_t::DEFLATE
							});
							// Подписываемся на получение сообщения сервера
							it->second->awh.on((function <void (const int32_t, const u_int, const string &)>) std::bind(&server::proxy_t::responseClient, this, _1, bid, _2, _3));
							// Устанавливаем функцию обратного вызова при получении HTTP-тела ответа с сервера клиенту
							it->second->awh.on((function <void (const int32_t, const u_int, const string &, const vector <char> &)>) std::bind(&server::proxy_t::entityClient, this, _1, bid, _2, _3, _4));
							// Устанавливаем функцию обратного вызова при получении HTTP-заголовков ответа с сервера клиенту
							it->second->awh.on((function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)>) std::bind(&server::proxy_t::headersClient, this, _1, bid, _2, _3, _4));
							// Выполняем подключение клиента к сетевому ядру
							this->_core.bind(&it->second->core);
						} break;
					}
				} break;
			}
		}
	}
}
/**
 * via Метод генерации заголовка Via
 * @param bid       идентификатор брокера (клиента)
 * @param mediators список предыдущих посредников
 * @return          сгенерированный заголовок
 */
string awh::server::Proxy::via(const uint64_t bid, const vector <string> & mediators) const noexcept {
	// Результат работы функции
	string result = "";
	// Получаем объект HTTP-парсера
	const awh::http_t * http = this->_server.parser(bid);
	// Если объект HTTP-парсера получен
	if(http != nullptr){
		// Получаем параметры хоста сервера
		const auto & host = this->_core.host();
		// Если посредники найдены
		if(!mediators.empty()){
			// Если список посредников содержит больше 1-го элемента
			if(mediators.size() > 1){
				// Выполняем переход по всему списку посредников
				for(auto & item : mediators){
					// Если заголовки в списке уже есть
					if(!result.empty())
						// Добавляем сначала разделитель
						result.append(", ");
					// Выполняем заполнение заголовка
					result.append(item);
				}
			// Если заголовок всего один
			} else result.append(mediators.front());
			// Добавляем сначала разделитель
			result.append(", ");
		} 
		// Если unix-сокет активирован
		if(this->_core.family() == scheme_t::family_t::NIX)
			// Выполняем формирование заголовка
			result.append(this->_fmk->format("HTTP/%.1f %s (%s)", 1.1f, host.sock.c_str(), http->ident(awh::http_t::process_t::RESPONSE).c_str()));
		// Если активирован хост и порт
		else {
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (this->_core.proto(bid))){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Если порт установлен не стандартный
					if(host.port != (this->_core.sonet() == awh::scheme_t::sonet_t::TLS ? 3129 : 3128))
						// Формируем заголовок Via
						result.append(this->_fmk->format("HTTP/%.1f %s:%u (%s)", 1.1f, host.addr.c_str(), host.port, http->ident(awh::http_t::process_t::RESPONSE).c_str()));
					// Будем считать, что порт установлен стандартный
					else result.append(this->_fmk->format("HTTP/%.1f %s (%s)", 1.1f, host.addr.c_str(), http->ident(awh::http_t::process_t::RESPONSE).c_str()));
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Если порт установлен не стандартный
					if(host.port != (this->_core.sonet() == awh::scheme_t::sonet_t::TLS ? 3129 : 3128))
						// Формируем заголовок Via
						result.append(this->_fmk->format("HTTP/%u %s:%u (%s)", 2, host.addr.c_str(), host.port, http->ident(awh::http_t::process_t::RESPONSE).c_str()));
					// Будем считать, что порт установлен стандартный
					else result.append(this->_fmk->format("HTTP/%u %s (%s)", 2, host.addr.c_str(), http->ident(awh::http_t::process_t::RESPONSE).c_str()));
				} break;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * raw Метод получения сырых данных с сервера и клиента
 * @param bid    идентификатор брокера (клиента)
 * @param broker брокер получивший данные
 * @param buffer буфер бинарных данных
 * @param size   разбмер буфера бинарных данных
 * @return       флаг обязательной следующей обработки данных
 */
bool awh::server::Proxy::raw(const uint64_t bid, const broker_t broker, const char * buffer, const size_t size) noexcept {
	// Результат работы функции
	bool result = true;
	// Если тип сокета установлен как TCP/IP
	if(this->_core.sonet() == awh::scheme_t::sonet_t::TCP){
		// Если бинарные данные получены
		if((buffer != nullptr) && (size > 0)){
			// Выполняем поиск объекта клиента
			auto it = this->_clients.find(bid);
			// Если активный клиент найден и подключение установлено
			if((it != this->_clients.end()) && (it->second->connected)){
				// Если установлен метод CONNECT
				if(!(result = (it->second->request.params.method != awh::web_t::method_t::CONNECT))){
					// Определяем переданного брокера
					switch(static_cast <uint8_t> (broker)){
						// Если брокером является клиент
						case static_cast <uint8_t> (broker_t::CLIENT):
							// Выполняем отправку клиенту полученных сырых данных с удалённого сервера
							this->_server.send(bid, buffer, size);
						break;
						// Если брокером является сервер
						case static_cast <uint8_t> (broker_t::SERVER):
							// Выполняем отправку сообщения клиенту в бинарном виде
							it->second->awh.send(buffer, size);
						break;
					}
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * completed Метод завершения получения данных
 * @param bid идентификатор брокера (клиента)
 */
void awh::server::Proxy::completed(const uint64_t bid) noexcept {
	// Выполняем поиск объекта клиента
	auto it = this->_clients.find(bid);
	// Если активный клиент найден
	if(!it->second->sending && (it->second->sending = (it != this->_clients.end()))){
		// Если заголовки ответа получены
		if(!it->second->response.headers.empty()){
			// Отправляем сообщение клиенту
			this->_server.send(bid, it->second->response.params.code, it->second->response.params.message, it->second->response.entity, it->second->response.headers);
			// Если функция обратного вызова установлена
			if(this->_callback.is("completed"))
				// Выполняем функцию обратного вызова
				this->_callback.call <const uint64_t, const u_int, const string &, const vector <char> &, const unordered_multimap <string, string> &> ("completed", bid, it->second->response.params.code, it->second->response.params.message, it->second->response.entity, it->second->response.headers);
		}
	}
}
/**
 * proto Метод извлечения поддерживаемого протокола подключения
 * @param bid идентификатор брокера
 * @return    поддерживаемый протокол подключения (HTTP1_1, HTTP2)
 */
awh::engine_t::proto_t awh::server::Proxy::proto(const uint64_t bid) const noexcept {
	// Выполняем извлечение поддерживаемого протокола подключения
	return this->_server.proto(bid);
}
/**
 * parser Метод извлечения объекта HTTP-парсера
 * @param bid идентификатор брокера
 * @return    объект HTTP-парсера
 */
const awh::http_t * awh::server::Proxy::parser(const uint64_t bid) const noexcept {
	// Выполняем извлечение объекта HTTP-парсера
	return this->_server.parser(bid);
}
/**
 * init Метод инициализации PROXY-сервера
 * @param socket     unix-сокет для биндинга
 * @param compressor поддерживаемый компрессор для рекомпрессии пересылаемых данных
 */
void awh::server::Proxy::init(const string & socket, const http_t::compress_t compressor) noexcept {
	// Выполняем инициализацию PROXY-сервера
	this->_server.init(socket);
	// Устанавливаем компрессор для рекомпрессии пересылаемых данных
	this->_compressor = compressor;
}
/**
 * init Метод инициализации PROXY-сервера
 * @param port       порт сервера
 * @param host       хост сервера
 * @param compressor поддерживаемый компрессор для рекомпрессии пересылаемых данных
 */
void awh::server::Proxy::init(const u_int port, const string & host, const http_t::compress_t compressor) noexcept {
	// Выполняем инициализацию PROXY-сервера
	this->_server.init(port, host);
	// Устанавливаем компрессор для рекомпрессии пересылаемых данных
	this->_compressor = compressor;
}
/**
 * on Метод установки функция обратного вызова при удаление клиента из стека сервера
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t)> ("erase", callback);
}
/**
 * on Метод установки функции обратного вызова при выполнении рукопожатия на прокси-сервере
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const engine_t::proto_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const engine_t::proto_t)> ("handshake", callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(callback);
}
/**
 * on Метод установки функции обратного вызова для извлечения пароля
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <string (const uint64_t, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <string (const uint64_t, const string &)> ("extractPassword", callback);
}
/**
 * on Метод установки функции обратного вызова для обработки авторизации
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const uint64_t, const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <bool (const uint64_t, const string &, const string &)> ("checkPassword", callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации брокера на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <bool (const string &, const string &, const u_int)> ("accept", callback);
}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const uint64_t, const u_int, const string &)> ("response", callback);
}
/**
 * on Метод установки функции обратного вызова при получении источников подключения
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const vector <string> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const vector <string> &)> ("origin", callback);
}
/**
 * on Метод установки функции обратного вызова при получении альтернативных сервисов
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const string &, const string &)> ("altsvc", callback);
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const broker_t, const web_t::mode_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const broker_t, const web_t::mode_t)> ("active", callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const int32_t, const uint64_t, const broker_t, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(std::bind(callback, _1, _2, broker_t::SERVER, _3));
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const int32_t, const uint64_t, const broker_t, const vector <char> &)> ("chunks", callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const int32_t, const uint64_t, const broker_t, const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(std::bind(callback, _1, _2, broker_t::SERVER, _3, _4));
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const int32_t, const uint64_t, const broker_t, const string &, const string &)> ("header", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const broker_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(std::bind(callback, _1, broker_t::SERVER, _2, _3, _4));
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const broker_t, const log_t::flag_t, const http::error_t, const string &)> ("error", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения тела ответа с удалённого сервера
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const u_int, const string &, vector <char> *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const u_int, const string &, vector <char> *)> ("entityClient", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения тела запроса на прокси-сервере
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, vector <char> *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, vector <char> *)> ("entityServer", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения заголовков ответа с удалённого сервера
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const u_int, const string &, unordered_multimap <string, string> *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const u_int, const string &, unordered_multimap <string, string> *)> ("headersClient", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения заголовков запроса на прокси-сервере
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, unordered_multimap <string, string> *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, unordered_multimap <string, string> *)> ("headersServer", callback);
}
/**
 * on Метод установки функции обратного вызова на событие формирования готового ответа клиенту подключённого к прокси-серверу
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const u_int, const string &, const vector <char> &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const u_int, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("completed", callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("push", callback);
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
u_int awh::server::Proxy::port(const uint64_t bid) const noexcept {
	// Выполняем получение порта подключения брокера
	return this->_server.port(bid);
}
/**
 * ip Метод получения IP-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::Proxy::ip(const uint64_t bid) const noexcept {
	// Выполняем получение IP-адреса брокера
	return this->_server.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::Proxy::mac(const uint64_t bid) const noexcept {
	// Выполняем получение MAC-адреса брокера
	return this->_server.mac(bid);
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Proxy::stop() noexcept {
	// Выполняем остановку сервера
	this->_server.stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::Proxy::start() noexcept {
	// Выполняем запуск сервера
	this->_server.start();
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Proxy::close(const uint64_t bid) noexcept {
	// Выполняем закрытие подключения брокера
	this->_server.close(bid);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::Proxy::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага перезапуска процессов
	this->_server.clusterAutoRestart(mode);
}
/**
 * clusterSize Метод установки количества процессов кластера
 * @param size количество рабочих процессов
 */
void awh::server::Proxy::clusterSize(const uint16_t size) noexcept {
	// Выполняем установку размера кластера
	this->_core.clusterSize(size);
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Proxy::total(const u_short total) noexcept {
	// Выполняем установку максимального количества одновременных подключений
	this->_server.total(total);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::server::Proxy::mode(const set <flag_t> & flags) noexcept {
	// Выполняем установку флагов приложения
	this->_flags = flags;
	// Создаём список флагов сервера
	set <server::web_t::flag_t> server;
	// Если флаг анбиндинга ядра сетевого модуля установлен
	if(flags.count(flag_t::NOT_STOP) == 0)
		// Устанавливаем флаг анбиндинга ядра сетевого модуля
		server.emplace(server::web_t::flag_t::NOT_STOP);
	// Если флаг поддержания автоматического подключения установлен
	if(flags.count(flag_t::ALIVE) > 0)
		// Устанавливаем флаг поддержания автоматического подключения
		server.emplace(server::web_t::flag_t::ALIVE);
	// Если флаг ожидания входящих сообщений установлен
	if(flags.count(flag_t::WAIT_MESS) > 0)
		// Устанавливаем флаг ожидания входящих сообщений
		server.emplace(server::web_t::flag_t::WAIT_MESS);
	// Если флаг запрещающий вывод информационных сообщений установлен
	if(flags.count(flag_t::NOT_INFO) > 0)
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		server.emplace(server::web_t::flag_t::NOT_INFO);
	// Если флаг проверки домена установлен
	if(flags.count(flag_t::VERIFY_SSL) > 0)
		// Выполняем установку флага проверки домена
		server.emplace(server::web_t::flag_t::VERIFY_SSL);
	// Если флаг разрешающий метод CONNECT установлен
	if(flags.count(flag_t::CONNECT_METHOD_SERVER_ENABLE) > 0)
		// Выполняем установку флага сервера
		server.emplace(server::web_t::flag_t::CONNECT_METHOD_ENABLE);
	// Устанавливаем флаги настроек модуля
	this->_server.mode(std::move(server));
}
/**
 * addOrigin Метод добавления разрешённого источника
 * @param origin разрешённый источнико
 */
void awh::server::Proxy::addOrigin(const string & origin) noexcept {
	// Выполняем добавление разрешённого источника
	this->_server.addOrigin(origin);
}
/**
 * setOrigin Метод установки списка разрешённых источников
 * @param origins список разрешённых источников
 */
void awh::server::Proxy::setOrigin(const vector <string> & origins) noexcept {
	// Выполняем установку списка разрешённых источников
	this->_server.setOrigin(origins);
}
/**
 * addAltSvc Метод добавления альтернативного сервиса
 * @param origin название альтернативного сервиса
 * @param field  поле альтернативного сервиса
 */
void awh::server::Proxy::addAltSvc(const string & origin, const string & field) noexcept {
	// Выполняем добавление альтернативного сервиса
	this->_server.addAltSvc(origin, field);
}
/**
 * setAltSvc Метод установки списка разрешённых источников
 * @param origins список альтернативных сервисов
 */
void awh::server::Proxy::setAltSvc(const unordered_multimap <string, string> & origins) noexcept {
	// Выполняем установку списка разрешённых источников
	this->_server.setAltSvc(origins);
}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::server::Proxy::settings(const map <awh::http2_t::settings_t, uint32_t> & settings) noexcept {
	// Выполняем установку настроек протокола HTTP/2
	this->_server.settings(settings);
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Proxy::realm(const string & realm) noexcept {
	// Выполняем установку названия сервера
	this->_server.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Proxy::opaque(const string & opaque) noexcept {
	// Выполняем установку временного ключа сессии сервера
	this->_server.opaque(opaque);
}
/**
 * maxRequests Метод установки максимального количества запросов
 * @param max максимальное количество запросов
 */
void awh::server::Proxy::maxRequests(const size_t max) noexcept {
	// Выполняем установку максимального количества запросов
	this->_server.maxRequests(max);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Proxy::alive(const bool mode) noexcept {
	// Устанавливаем долгоживущее подключения
	this->_server.alive(mode);
}
/**
 * alive Метод установки времени жизни подключения
 * @param time время жизни подключения
 */
void awh::server::Proxy::alive(const time_t time) noexcept {
	// Устанавливаем время жизни подключения
	this->_server.alive(time);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param bid  идентификатор брокера
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Proxy::alive(const uint64_t bid, const bool mode) noexcept {
	// Устанавливаем долгоживущее подключение
	this->_server.alive(bid, mode);
}
/**
 * ipV6only Метод установки флага использования только сети IPv6
 * @param mode флаг для установки
 */
void awh::server::Proxy::ipV6only(const bool mode) noexcept {
	// Выполняем установку флага использования только сети IPv6
	this->_core.ipV6only(mode);
}
/**
 * bandWidth Метод установки пропускной способности сети
 * @param bid   идентификатор брокера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Proxy::bandWidth(const size_t bid, const string & read, const string & write) noexcept {
	// Выполняем установку пропускной способности сети
	this->_core.bandWidth(bid, read, write);
}
/**
 * chunk Метод установки размера чанка
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param size   размер чанка для установки
 */
void awh::server::Proxy::chunk(const broker_t broker, const size_t size) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT):
			// Устанавливаем флаг верификации доменного имени
			this->_settings.chunk = size;
		break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку размера чанка
			this->_server.chunk(size);
		break;
	}
}
/**
 * hosts Метод загрузки файла со списком хостов
 * @param broker   брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param filename адрес файла для загрузки
 */
void awh::server::Proxy::hosts(const broker_t broker, const string & filename) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT):
			// Устанавливаем адрес файла для загрузки
			this->_settings.dns.hosts = filename;
		break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем загрузку файла со списком хостов
			this->_server.hosts(filename);
		break;
	}
}
/**
 * certificate Метод установки файлов сертификата
 * @param chain файл цепочки сертификатов
 * @param key   приватный ключ сертификата
 */
void awh::server::Proxy::certificate(const string & chain, const string & key) noexcept {
	// Если адрес файла сертификата и ключа передан
	if(!chain.empty() && !key.empty()){
		// Устанавливаем тип сокета TLS
		this->_core.sonet(awh::scheme_t::sonet_t::TLS);
		// Устанавливаем активный протокол подключения
		this->_core.proto(awh::engine_t::proto_t::HTTP2);
		// Устанавливаем SSL сертификаты сервера
		this->_core.certificate(chain, key);
	}
}
/**
 * verifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param mode   флаг состояния разрешения проверки
 */
void awh::server::Proxy::verifySSL(const broker_t broker, const bool mode) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT):
			// Устанавливаем флаг верификации доменного имени
			this->_settings.encryption.verify = mode;
		break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку  разрешения выполнения проверки соответствия, сертификата домену
			this->_core.verifySSL(mode);
		break;
	}
}
/**
 * ciphers Метод установки алгоритмов шифрования
 * @param broker  брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param ciphers список алгоритмов шифрования для установки
 */
void awh::server::Proxy::ciphers(const broker_t broker, const vector <string> & ciphers) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT):
			// Устанавливаем алгоритмы шифрования
			this->_settings.encryption.ciphers = ciphers;
		break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку алгоритмов шифрования
			this->_core.ciphers(ciphers);
		break;
	}
}
/**
 * ca Метод установки доверенного сертификата (CA-файла)
 * @param broker  брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param trusted адрес доверенного сертификата (CA-файла)
 * @param path    адрес каталога где находится сертификат (CA-файл)
 */
void awh::server::Proxy::ca(const broker_t broker, const string & trusted, const string & path) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT): {
			// Устанавливаем адрес каталога где находится сертификат (CA-файл)
			this->_settings.ca.path = path;
			// Устанавливаем адрес доверенного сертификата (CA-файла)
			this->_settings.ca.trusted = trusted;
		} break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку доверенного сертификата (CA-файла)
			this->_core.ca(trusted, path);
		break;
	}
}
/**
 * keepAlive Метод установки жизни подключения
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param cnt    максимальное количество попыток
 * @param idle   интервал времени в секундах через которое происходит проверка подключения
 * @param intvl  интервал времени в секундах между попытками
 */
void awh::server::Proxy::keepAlive(const broker_t broker, const int cnt, const int idle, const int intvl) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT): {
			// Устанавливаем максимальное количество попыток
			this->_settings.ka.cnt = cnt;
			// Устанавливаем интервал времени в секундах через которое происходит проверка подключения
			this->_settings.ka.idle = idle;
			// Устанавливаем интервал времени в секундах между попытками
			this->_settings.ka.intvl = intvl;
		} break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку жизни подключения
			this->_server.keepAlive(cnt, idle, intvl);
		break;
	}
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param broker      брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Proxy::compressors(const broker_t broker, const vector <http_t::compress_t> & compressors) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT):
			// Устанавливаем список поддерживаемых компрессоров
			this->_settings.compressors = compressors;
		break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку списка поддерживаемых компрессоров
			this->_server.compressors(compressors);
		break;
	}
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param read   количество байт для детекции по чтению
 * @param write  количество байт для детекции по записи
 */
void awh::server::Proxy::bytesDetect(const broker_t broker, const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT): {
			// Устанавливаем маркер детектирования чтения байт
			this->_settings.marker.read = read;
			// Устанавливаем маркер детектирования записи байт
			this->_settings.marker.write = write;
		} break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку детекцию сообщений по количеству байт
			this->_server.bytesDetect(read, write);
		break;
	}
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param broker  брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для подключения к серверу
 */
void awh::server::Proxy::waitTimeDetect(const broker_t broker, const time_t read, const time_t write, const time_t connect) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT): {
			// Если количество секунд для детекции по чтению передано
			if(read > 0)
				// Выполняем установку количества секунд для детекции по чтению
				this->_settings.wtd.read = read;
			// Если количество секунд для детекции по записи передано
			if(write > 0)
				// Выполняем установку количества секунд для детекции по записи
				this->_settings.wtd.write = write;
			// Если количество секунд для подключения к серверу передано
			if(connect > 0)
				// Выполняем установку количества секунд для подключения к серверу
				this->_settings.wtd.connect = connect;	
		} break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку детекцию сообщений по количеству секунд
			this->_server.waitTimeDetect(read, write);
		break;
	}
}
/**
 * sonet Метод установки типа сокета подключения
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param sonet  тип сокета подключения (TCP / UDP / SCTP)
 */
void awh::server::Proxy::sonet(const broker_t broker, const scheme_t::sonet_t sonet) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT):
			// Устанавливаем тип сокета подключения (TCP / UDP)
			this->_settings.sonet = sonet;
		break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку типа сокета подключения
			this->_core.sonet(sonet);
		break;
	}
}
/**
 * family Метод установки типа протокола интернета
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::server::Proxy::family(const broker_t broker, const scheme_t::family_t family) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT):
			// Устанавливаем тип протокола интернета (IPV4 / IPV6 / NIX)
			this->_settings.family = family;
		break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку типа протокола интернета
			this->_core.family(family);
		break;
	}
}
/**
 * network Метод установки параметров сети
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 * @param sonet  тип сокета подключения (TCP / UDP)
 */
void awh::server::Proxy::network(const broker_t broker, const vector <string> & ips, const vector <string> & ns, const scheme_t::family_t family, const scheme_t::sonet_t sonet) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT): {
			// Устанавливаем тип сокета подключения (TCP / UDP)
			this->_settings.sonet = sonet;
			// Устанавливаем тип протокола интернета (IPV4 / IPV6 / NIX)
			this->_settings.family = family;
			// Если список серверов имён, через которые необходимо производить резолвинг доменов передан
			if(!ns.empty())
				// Выполняем установку списока серверов имён, через которые необходимо производить резолвинг доменов
				this->_settings.ns = ns;
			// Если список IP-адресов компьютера с которых разрешено выходить в интернет передан
			if(!ips.empty())
				// Выполняем установку списока IP-адресов компьютера с которых разрешено выходить в интернет
				this->_settings.ips = ips;
		} break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку параметров сети
			this->_core.network(ips, family, sonet);
		break;
	}
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::server::Proxy::userAgent(const string & userAgent) noexcept {
	// Если User-Agent для HTTP-запроса передан
	if(!userAgent.empty())
		// Выполняем установку User-Agent для HTTP-запроса
		this->_settings.userAgent = userAgent;
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::server::Proxy::user(const string & login, const string & password) noexcept {
	// Если логин и пароль переданы
	if(!login.empty() && !password.empty()){
		// Выполняем установку логина пользователя для авторизации на сервере
		this->_settings.login = login;
		// Выполняем установку пароля пользователя для авторизации на сервере
		this->_settings.password = password;
	}
}
/**
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::server::Proxy::ident(const string & id, const string & name, const string & ver) noexcept {
	// Устанавливаем идентификатор клиента
	this->_ident.id = id;
	// Устанавливаем версию клиента
	this->_ident.ver = ver;
	// Устанавливаем название клиента
	this->_ident.name = name;
	// Выполняем установку идентификации клиента
	this->_server.ident(id, name, ver);
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::server::Proxy::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Если параметры прокси-сервера переданы
	if(!uri.empty()){
		// Выполняем установку параметров прокси-сервера
		this->_settings.proxy.uri = uri;
		// Выполняем установку семейстова интернет протоколов
		this->_settings.proxy.family = family;
	}
}
/**
 * flushDNS Метод сброса кэша DNS-резолвера
 * @param bid идентификатор брокера
 * @return    результат работы функции
 */
bool awh::server::Proxy::flushDNS(const uint64_t bid) noexcept {
	// Выполняем поиск указанного клиента
	auto it = this->_clients.find(bid);
	// Если указанный клиент найден
	if(it != this->_clients.end())
		// Выполняем сброс кэша DNS-резолвера
		return it->second->awh.flushDNS();
	// Выводим результат
	return false;
}
/**
 * timeoutDNS Метод установки времени ожидания выполнения запроса
 * @param sec интервал времени выполнения запроса в секундах
 */
void awh::server::Proxy::timeoutDNS(const uint8_t sec) noexcept {
	// Если время ожидания выполнения запроса передано
	if(sec > 0)
		// Выполняем установку времени ожидания выполнения запроса
		this->_settings.dns.timeout = sec;
}
/**
 * timeToLiveDNS Метод установки времени жизни DNS-кэша
 * @param ttl время жизни DNS-кэша в миллисекундах
 */
void awh::server::Proxy::timeToLiveDNS(const time_t ttl) noexcept {
	// Если время жизни DNS-кэша передано
	if(ttl > 0)
		// Выполняем установку время жизни DNS-кэша
		this->_settings.dns.ttl = ttl;
}
/**
 * prefixDNS Метод установки префикса переменной окружения для извлечения серверов имён
 * @param prefix префикс переменной окружения для установки
 */
void awh::server::Proxy::prefixDNS(const string & prefix) noexcept {
	// Если префикс переменной окружения для извлечения серверов имён передан
	if(!prefix.empty())
		// Выполняем установку префикса переменной окружения
		this->_settings.dns.prefix = prefix;
}
/**
 * clearDNSBlackList Метод очистки чёрного списка
 * @param domain доменное имя для которого очищается чёрный список
 */
void awh::server::Proxy::clearDNSBlackList(const string & domain) noexcept {
	// Если доменное имя передано
	if(!domain.empty() && !this->_settings.dns.blacklist.empty())
		// Выполняем очистку чёрного списка
		this->_settings.dns.blacklist.erase(domain);
}
/**
 * delInDNSBlackList Метод удаления IP-адреса из чёрного списока
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::server::Proxy::delInDNSBlackList(const string & domain, const string & ip) noexcept {
	// Если данные доменного имени переданы
	if(!domain.empty() && !ip.empty() && !this->_settings.dns.blacklist.empty()){
		// Выполняем перебор всего чёрного списка
		for(auto it = this->_settings.dns.blacklist.begin(); it != this->_settings.dns.blacklist.end();){
			// Если IP-адрес соответствует указанному
			if(this->_fmk->compare(ip, it->second))
				// Выполняем удаление доменного имени
				it = this->_settings.dns.blacklist.erase(it);
			// Выполняем перебор списка доменов дальше
			else ++it;
		}
	}
}
/**
 * setToDNSBlackList Метод добавления IP-адреса в чёрный список
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::server::Proxy::setToDNSBlackList(const string & domain, const string & ip) noexcept {
	// Если данные доменного имени переданы
	if(!domain.empty() && !ip.empty())
		// Выполняем добавление IP-адреса в чёрный список
		this->_settings.dns.blacklist.emplace(domain, ip);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::server::Proxy::authTypeProxy(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_settings.proxy.auth.type = type;
	// Выполняем установку алгоритма шифрования для Digest авторизации
	this->_settings.proxy.auth.hash = hash;
}
/**
 * authType Метод установки типа авторизации
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param type   тип авторизации
 * @param hash   алгоритм шифрования для Digest авторизации
 */
void awh::server::Proxy::authType(const broker_t broker, const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT): {
			// Выполняем установку типа авторизации
			this->_settings.auth.type = type;
			// Выполняем установку алгоритма шифрования для Digest авторизации
			this->_settings.auth.hash = hash;
		} break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку типа авторизации
			this->_server.authType(type, hash);
		break;
	}
}
/**
 * crypted Метод получения флага шифрования
 * @param bid идентификатор брокера
 * @return    результат проверки
 */
bool awh::server::Proxy::crypted(const uint64_t bid) const noexcept {
	// Выполняем получение флага шифрования
	return this->_server.crypted(bid);
}
/**
 * encrypt Метод активации шифрования для клиента
 * @param bid  идентификатор брокера
 * @param mode флаг активации шифрования
 */
void awh::server::Proxy::encrypt(const uint64_t bid, const bool mode) noexcept {
	// Выполняем активацию шифрования для клиента
	this->_server.encrypt(bid, mode);
}
/**
 * encryption Метод активации шифрования
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param mode   флаг активации шифрования
 */
void awh::server::Proxy::encryption(const broker_t broker, const bool mode) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT):
			// Выполняем установку флага активации шифрования
			this->_settings.encryption.mode = mode;
		break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполянем активацию шифрования
			this->_server.encryption(mode);
		break;
	}
}
/**
 * encryption Метод установки параметров шифрования
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::server::Proxy::encryption(const broker_t broker, const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT): {
			// Выполняем установку пароля шифрования передаваемых данных
			this->_settings.encryption.pass = pass;
			// Выполняем установку соли шифрования передаваемых данных
			this->_settings.encryption.salt = salt;
			// Выполняем установку размера шифрования передаваемых данных
			this->_settings.encryption.cipher = cipher;
		} break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку параметров шифрования
			this->_server.encryption(pass, salt, cipher);
		break;
	}
}
/**
 * Proxy Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Proxy::Proxy(const fmk_t * fmk, const log_t * log) noexcept :
 _uri(fmk), _core(fmk, log), _server(&_core, fmk, log),
 _callback(log), _compressor(http_t::compress_t::NONE), _fmk(fmk), _log(log) {
	// Устанавливаем тип сокета TCP
	this->_core.sonet(awh::scheme_t::sonet_t::TCP);
	// Устанавливаем активный протокол подключения
	this->_core.proto(awh::engine_t::proto_t::HTTP1_1);
	// Выполняем установку идентичности протокола модуля
	this->_server.identity(awh::http_t::identity_t::PROXY);
	// Устанавливаем функцию удаления клиента из стека подключений сервера
	this->_server.on((function <void (const uint64_t)>) std::bind(&server::proxy_t::eraseClient, this, _1));
	// Устанавливаем функцию извлечения пароля
	this->_server.on((function <string (const uint64_t, const string &)>) std::bind(&server::proxy_t::passwordCallback, this, _1, _2));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	this->_server.on((function <void (const uint64_t, const server::web_t::mode_t)>) std::bind(&server::proxy_t::activeServer, this, _1, _2));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	this->_server.on((function <bool (const string &, const string &, const u_int)>) std::bind(&server::proxy_t::acceptServer, this, _1, _2, _3));
	// Устанавливаем функцию проверки ввода логина и пароля пользователя
	this->_server.on((function <bool (const uint64_t, const string &, const string &)>) std::bind(&server::proxy_t::authCallback, this, _1, _2, _3));
	// Устанавливаем функцию получения сырых данных полученных сервером с клиента
	this->_server.on((function <bool (const uint64_t, const char *, const size_t)>) std::bind(&server::proxy_t::raw, this, _1, broker_t::SERVER, _2, _3));
	// Устанавливаем функцию обратного вызова при выполнении удачного рукопожатия
	this->_server.on((function <void (const int32_t, const uint64_t, const server::web_t::agent_t)>) std::bind(&server::proxy_t::handshake, this, _1, _2, _3));
	// Устанавливаем функцию обратного вызова при получении тела запроса с клиента
	this->_server.on((function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t, const vector <char> &)>) std::bind(&server::proxy_t::entityServer, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию обратного вызова при получении HTTP-заголовков запроса с клиента
	this->_server.on((function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t, const unordered_multimap <string, string> &)>) std::bind(&server::proxy_t::headersServer, this, _1, _2, _3, _4, _5));
}
