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
#include <server/proxy2.hpp>

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
 * endClient Метод завершения запроса клиента
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param direct направление передачи данных
 */
void awh::server::Proxy::endClient(const int32_t sid, const uint64_t bid, const client::web_t::direct_t direct) noexcept {

}
/**
 * activeClient Метод идентификации активности на Web сервере (для клиента)
 * @param bid  идентификатор брокера (клиента)
 * @param mode режим события подключения
 */
void awh::server::Proxy::activeClient(const uint64_t bid, const client::web_t::mode_t mode) noexcept {

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
		case static_cast <uint8_t> (web_t::mode_t::CONNECT): {
			// Выполняем создание клиента
			auto ret = this->_clients.emplace(bid, unique_ptr <client_t> (new client_t(this->_fmk, this->_log)));
			// Выполняем подключение клиента к сетевому ядру
			this->_core.bind(&ret.first->second->core);
			// Создаём список флагов клиента
			set <client::web_t::flag_t> flags = {
				client::web_t::flag_t::ALIVE,
				client::web_t::flag_t::NOT_STOP,
				client::web_t::flag_t::REDIRECTS
			};
			// Если флаг ожидания входящих сообщений установлен
			if(this->_flags.count(flag_t::WAIT_MESS) > 0)
				// Устанавливаем флаг ожидания входящих сообщений
				flags.emplace(client::web_t::flag_t::WAIT_MESS);
			// Если флаг запрещающий вывод информационных сообщений установлен
			if(this->_flags.count(flag_t::NOT_INFO) > 0)
				// Устанавливаем флаг запрещающий вывод информационных сообщений
				flags.emplace(client::web_t::flag_t::NOT_INFO);
			// Если флаг проверки домена установлен
			if(this->_flags.count(flag_t::VERIFY_SSL) > 0)
				// Выполняем установку флага проверки домена
				flags.emplace(client::web_t::flag_t::VERIFY_SSL);
			// Если флаг отключающий метод CONNECT для прокси-клиента установлен
			if(this->_flags.count(flag_t::PROXY_NOCONNECT) > 0)
				// Выполняем установку флага отключающего метод CONNECT для прокси-клиента
				flags.emplace(client::web_t::flag_t::PROXY_NOCONNECT);
			// Устанавливаем флаги настроек модуля
			ret.first->second->awh.mode(std::move(flags));
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
			// Устанавливаем функцию обратного вызова удачного рукопожатия на удалённом сервере
			ret.first->second->awh.on((function <void (const int32_t, const client::web_t::agent_t)>) std::bind(&server::proxy_t::handshakeClient, this, _1, bid, _2));
			// Если функция обратного вызова установлена
			if(this->_callback.is("response"))
				// Выполняем установку функции обратного вызова получения ответа с сервера
				ret.first->second->awh.on(this->_callback.get <void (const int32_t, const u_int, const string &)> ("response"));
			// Если функция обратного вызова установлена
			if(this->_callback.is("entity"))
				// Выполняем установку функции обратного вызова получения тела ответа
				ret.first->second->awh.on(this->_callback.get <void (const int32_t, const u_int, const string &, const vector <char> &)> ("entity"));
			// Если функция обратного вызова установлена
			if(this->_callback.is("headers"))
				// Выполняем установку функции обратного вызова получения заголовков
				ret.first->second->awh.on(this->_callback.get <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> ("headers"));
			// Если функция обратного вызова установлена
			if(this->_callback.is("chunks"))
				// Выполняем установку функции обратного вызова получения бинарных чанков
				ret.first->second->awh.on((function <void (const int32_t, const vector <char> &)>) std::bind(this->_callback.get <void (const broker_t, const int32_t, const uint64_t, const vector <char> &)> ("chunks"), broker_t::CLIENT, _1, bid, _2));
			// Если функция обратного вызова установлена
			if(this->_callback.is("header"))
				// Выполняем установку функции обратного вызова получения заголовка
				ret.first->second->awh.on((function <void (const int32_t, const string &, const string &)>) std::bind(this->_callback.get <void (const broker_t, const int32_t, const uint64_t, const string &, const string &)> ("header"), broker_t::CLIENT, _1, bid, _2, _3));
			// Если функция обратного вызова установлена
			if(this->_callback.is("error"))
				// Выполняем установку функции обратного вызова получения ошибок клиента
				ret.first->second->awh.on((function <void (const log_t::flag_t, const http::error_t, const string &)>) std::bind(this->_callback.get <void (const broker_t, const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error"), broker_t::CLIENT, bid, _1, _2, _3));
			// Если функция обратного вызова установлена
			if(this->_callback.is("origin"))
				// Выполняем установку функции обратного вызова при получении источников подключения
				ret.first->second->awh.on((function <void (const vector <string> &)>) std::bind(this->_callback.get <void (const uint64_t, const vector <string> &)> ("origin"), bid, _1));
			// Если функция обратного вызова установлена
			if(this->_callback.is("altsvc"))
				// Устанавливаем функцию обратного вызова при получении альтернативного источника
				ret.first->second->awh.on((function <void (const string &, const string &)> ) std::bind(this->_callback.get <void (const uint64_t, const string &, const string &)> ("altsvc"), bid, _1, _2));
			// Если функция обратного вызова установлена
			if(this->_callback.is("request"))
				// Выполняем установку функции обратного вызова отправки запроса на сервер
				ret.first->second->awh.on((function <void (const int32_t, const awh::web_t::method_t, const uri_t::url_t &)>) std::bind(this->_callback.get <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request"), _1, bid, _2, _3));
			// Если функция обратного вызова установлена
			if(this->_callback.is("push"))
				// Выполняем установку функции обратного вызова при получении PUSH уведомлений
				ret.first->second->awh.on((function <void (const int32_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)>) std::bind(this->_callback.get <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("push"), _1, bid, _2, _3, _4));
		} break;
		// Если производится отключение клиента от сервера
		case static_cast <uint8_t> (web_t::mode_t::DISCONNECT):
			// Выполняем поиск клиента в списке
			auto it = this->_clients.find(bid);
			// Если клиент в списке найден
			if(it != this->_clients.end()){
				// Выполняем отключение клиента от сетевого ядра
				this->_core.unbind(&it->second->core);
				// Выполняем удаление клиента из списка клиентов
				this->_clients.erase(it);
			}
		break;
	}
	// Если функция обратного вызова установлена
	if(this->_callback.is("active"))
		// Выполняем функцию обратного вызова
		this->_callback.call <const uint64_t, const web_t::mode_t> ("active", bid, mode);
}
/**
 * handshakeClient Метод получения удачного запроса (для клиента)
 * @param sid   идентификатор потока
 * @param bid   идентификатор брокера
 * @param agent идентификатор агента клиента
 */
void awh::server::Proxy::handshakeClient(const int32_t sid, const uint64_t bid, const client::web_t::agent_t agent) noexcept {

}
/**
 * handshakeServer Метод получения удачного запроса
 * @param sid   идентификатор потока
 * @param bid   идентификатор брокера
 * @param agent идентификатор агента клиента
 */
void awh::server::Proxy::handshakeServer(const int32_t sid, const uint64_t bid, const server::web_t::agent_t agent) noexcept {

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
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const web_t::mode_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const web_t::mode_t)> ("active", callback);
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
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(callback);
}
/**
 * on Метод установки функции обратного вызова для перехвата полученных чанков
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(callback);
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
 * on Метод установки функции обратного вызова на событие получения ошибки
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const broker_t, const uint64_t, const log_t::flag_t, const http::error_t, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(std::bind(callback, broker_t::SERVER, _1, _2, _3, _4));
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const broker_t, const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", callback);
}
/**
 * on Метод установки функции вывода полученного чанка бинарных данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const broker_t, const int32_t, const uint64_t, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(std::bind(callback, broker_t::SERVER, _1, _2, _3));
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const broker_t, const int32_t, const uint64_t, const vector <char> &)> ("chunks", callback);
}
/**
 * on Метод установки функции вывода полученного заголовка с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const broker_t, const int32_t, const uint64_t, const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(std::bind(callback, broker_t::SERVER, _1, _2, _3, _4));
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const broker_t, const int32_t, const uint64_t, const string &, const string &)> ("header", callback);
}
/**
 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const int32_t, const u_int, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const int32_t, const u_int, const string &)> ("response", callback);
}
/**
 * on Метод установки функции вывода запроса клиента к серверу
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(callback);
	// Устанавливаем функцию обратного вызова для HTTP/2
	this->_callback.set <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &)> ("request", callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с сервера
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const int32_t, const u_int, const string &, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const int32_t, const u_int, const string &, const vector <char> &)> ("entity", callback);
}
/**
 * on Метод установки функции вывода полученного тела данных с клиента
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_server.on(callback);
}
/**
 * on Метод установки функции вывода полученных заголовков с сервера
 * @param callback функция обратного вызова
 */
void awh::server::Proxy::on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> ("headers", callback);
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
void awh::server::Proxy::settings(const map <web2_t::settings_t, uint32_t> & settings) noexcept {
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
 _core(fmk, log), _server(&_core, fmk, log), _callback(log),
 _compressor(http_t::compress_t::NONE), _fmk(fmk), _log(log) {
	// Устанавливаем тип сокета TCP
	this->_core.sonet(awh::scheme_t::sonet_t::TCP);
	// Устанавливаем активный протокол подключения
	this->_core.proto(awh::engine_t::proto_t::HTTP1_1);
	// Выполняем установку идентичности протокола модуля
	this->_server.identity(awh::http_t::identity_t::PROXY);
	// Устанавливаем функцию извлечения пароля
	this->_server.on((function <string (const uint64_t, const string &)>) std::bind(&server::proxy_t::passwordCallback, this, _1, _2));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	this->_server.on((function <void (const uint64_t, const server::web_t::mode_t)>) std::bind(&server::proxy_t::activeServer, this, _1, _2));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	this->_server.on((function <bool (const string &, const string &, const u_int)>) std::bind(&server::proxy_t::acceptServer, this, _1, _2, _3));
	// Устанавливаем функцию проверки ввода логина и пароля пользователя
	this->_server.on((function <bool (const uint64_t, const string &, const string &)>) std::bind(&server::proxy_t::authCallback, this, _1, _2, _3));
	// Устанавливаем функцию обратного вызова при выполнении удачного рукопожатия
	this->_server.on((function <void (const int32_t, const uint64_t, const server::web_t::agent_t)>) std::bind(&server::proxy_t::handshakeServer, this, _1, _2, _3));
}