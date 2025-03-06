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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <server/proxy.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён заполнителя
 */
using namespace placeholders;

/**
 * passwordEvents Метод извлечения пароля (для авторизации методом Digest)
 * @param bid   идентификатор брокера (клиента)
 * @param login логин пользователя
 * @return      пароль пользователя хранящийся в базе данных
 */
string awh::server::Proxy::passwordEvents(const uint64_t bid, const string & login) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("extractPassword"))
		// Выполняем функцию обратного вызова
		return this->_callbacks.call <string (const uint64_t, const string &)> ("extractPassword", bid, login);
	// Сообщаем, что пароль для пользователя не найден
	return "";
}
/**
 * authEvents Метод проверки авторизации пользователя (для авторизации методом Basic)
 * @param bid      идентификатор брокера (клиента)
 * @param login    логин пользователя (от клиента)
 * @param password пароль пользователя (от клиента)
 * @return         результат авторизации
 */
bool awh::server::Proxy::authEvents(const uint64_t bid, const string & login, const string & password) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("checkPassword"))
		// Выполняем функцию обратного вызова
		return this->_callbacks.call <bool (const uint64_t, const string &, const string &)> ("checkPassword", bid, login, password);
	// Сообщаем, что пользователь не прошёл валидацию
	return false;
}
/**
 * acceptEvents Метод активации клиента на сервере
 * @param ip   адрес интернет подключения
 * @param mac  аппаратный адрес подключения
 * @param port порт подключения
 * @return     результат проверки
 */
bool awh::server::Proxy::acceptEvents(const string & ip, const string & mac, const uint32_t port) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("accept"))
		// Выполняем функцию обратного вызова
		return this->_callbacks.call <bool (const string &, const string &, const uint32_t)> ("accept", ip, mac, port);
	// Запрещаем выполнение подключения
	return false;
}
/**
 * available Метод получения событий освобождения памяти буфера полезной нагрузки
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param bid    идентификатор брокера
 * @param size   размер буфера полезной нагрузки
 * @param core   объект сетевого ядра
 */
void awh::server::Proxy::available(const broker_t broker, const uint64_t bid, const size_t size, awh::core_t * core) noexcept {
	// Ещем для указанного потока очередь полезной нагрузки
	auto i = this->_payloads.find(bid);
	// Если для потока очередь полезной нагрузки получена
	if((i != this->_payloads.end()) && !i->second->empty()){
		// Если места достаточно в буфере данных для отправки
		if(i->second->size() <= size){
			// Если сетевое ядро уже инициализированно
			if(core != nullptr){
				// Флаг разрешения отправки ранее неотправленных данных из временного буфера полезной нагрузки
				bool allow = true;
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("available"))
					// Выполняем функцию обратного вызова
					allow = this->_callbacks.call <bool (const broker_t, const uint64_t, const size_t)> ("available", broker, bid, size);
				// Если разрешено добавить неотправленную запись во временный буфер полезной нагрузки
				if(allow){
					// Определяем переданного брокера
					switch(static_cast <uint8_t> (broker)){
						// Если брокером является клиент
						case static_cast <uint8_t> (broker_t::CLIENT): {
							// Выполняем отправку заголовков запроса на сервер
							if(dynamic_cast <client::core_t *> (core)->send(reinterpret_cast <const char *> (i->second->get()), i->second->size(), bid))
								// Выполняем удаление буфера полезной нагрузки
								i->second->pop();
						} break;
						// Если брокером является сервер
						case static_cast <uint8_t> (broker_t::SERVER): {
							// Выполняем отправку заголовков запроса на сервер
							if(dynamic_cast <server::core_t *> (core)->send(reinterpret_cast <const char *> (i->second->get()), i->second->size(), bid))
								// Выполняем удаление буфера полезной нагрузки
								i->second->pop();
						} break;
					}
				}
			// Выполняем удаление буфера полезной нагрузки
			} else i->second->pop();
		}
	}
}
/**
 * unavailable Метод получения событий недоступности памяти буфера полезной нагрузки
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param bid    идентификатор брокера
 * @param buffer буфер полезной нагрузки которую не получилось отправить
 * @param size   размер буфера полезной нагрузки
 */
void awh::server::Proxy::unavailable(const broker_t broker, const uint64_t bid, const char * buffer, const size_t size) noexcept {
	// Флаг разрешения добавления неотправленных данных во временный буфер полезной нагрузки
	bool allow = true;
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("unavailable"))
		// Выполняем функцию обратного вызова
		allow = this->_callbacks.call <bool (const broker_t, const uint64_t, const char *, const size_t)> ("unavailable", broker, bid, buffer, size);
	// Если разрешено добавить неотправленную запись во временный буфер полезной нагрузки
	if(allow){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Ещем для указанного потока очередь полезной нагрузки
			auto i = this->_payloads.find(bid);
			// Если для потока очередь полезной нагрузки получена
			if(i != this->_payloads.end())
				// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
				i->second->push(buffer, size);
			// Если для потока почередь полезной нагрузки ещё не сформированна
			else {
				// Создаём новую очередь полезной нагрузки
				auto ret = this->_payloads.emplace(bid, unique_ptr <queue_t> (new queue_t(this->_log)));
				// Добавляем в очередь полезной нагрузки наш буфер полезной нагрузки
				ret.first->second->push(buffer, size);
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const bad_alloc &) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (broker), bid, buffer, size), log_t::flag_t::CRITICAL, "Memory allocation error");
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Memory allocation error");
			#endif
			// Выходим из приложения
			::exit(EXIT_FAILURE);
		}
	}
}
/**
 * eventCallback Метод отлавливания событий контейнера функций обратного вызова
 * @param event событие контейнера функций обратного вызова
 * @param idw   идентификатор функции обратного вызова
 * @param name  название функции обратного вызова
 * @param dump  дамп данных функции обратного вызова
 */
void awh::server::Proxy::eventCallback(const fn_t::event_t event, [[maybe_unused]] const uint64_t idw, const string & name, [[maybe_unused]] const fn_t::dump_t * dump) noexcept {
	// Определяем входящее событие контейнера функций обратного вызова
	switch(static_cast <uint8_t> (event)){
		// Если событием является установка функции обратного вызова
		case static_cast <uint8_t> (fn_t::event_t::SET): {
			// Если функция обратного вызова для получения событий запуска и остановки сетевого ядра передана
			if(this->_fmk->compare(name, "status"))
				// Выполняем установку функции обратного вызова для получения событий запуска и остановки сетевого ядра
				this->_server.callback <void (const awh::core_t::status_t)> ("status", this->_callbacks.get <void (const awh::core_t::status_t)> ("status"));
			// Если функция установки обратного вызова на событие получении ошибки передана
			else if(this->_fmk->compare(name, "error"))
				// Выполняем установку функции обратного вызова на событие получения ошибки
				this->_server.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", std::bind(this->_callbacks.get <void (const uint64_t, const broker_t, const log_t::flag_t, const http::error_t, const string &)> ("error"), _1, broker_t::SERVER, _2, _3, _4));
		} break;
	}
}
/** 
 * eraseClient Метод удаления подключённого клиента
 * @param bid идентификатор брокера
 */
void awh::server::Proxy::eraseClient(const uint64_t bid) noexcept {
	// Выполняем поиск объекта клиента
	auto i = this->_clients.find(bid);
	// Если активный клиент найден
	if(i != this->_clients.end())
		// Выполняем удаление клиента из списка клиентов
		this->_clients.erase(i);
	// Выполняем поиск неотправленных буферов полезной нагрузки
	auto j = this->_payloads.find(bid);
	// Если неотправленные буферы полезной нагрузки найдены
	if(j != this->_payloads.end())
		// Выполняем удаление неотправленные буферы полезной нагрузки
		this->_payloads.erase(j);
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("erase"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const uint64_t)> ("erase", bid);
}
/**
 * endClient Метод завершения запроса клиента
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param rid    идентификатор запроса
 * @param direct направление передачи данных
 */
void awh::server::Proxy::endClient([[maybe_unused]] const int32_t sid, const uint64_t bid, [[maybe_unused]] const uint64_t rid, const client::web_t::direct_t direct) noexcept {
	// Если мы получили данные
	if(direct == client::web_t::direct_t::RECV){
		// Выполняем поиск объекта клиента
		auto i = this->_clients.find(bid);
		// Если активный клиент найден
		if(i != this->_clients.end()){
			// Выполняем поиск идентификатора потока
			auto j = i->second->streams.find(rid);
			// Если идентификатор потока найден, удаляем его
			if(j != i->second->streams.end()){
				// Выводим полученный результат
				this->completed(j->second, bid);
				// Выполняем удаление потока из списка
				i->second->streams.erase(j);
			}
		}
	}
}
/**
 * responseClient Метод получения сообщения с удалённого сервера
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера (клиента)
 * @param rid     идентификатор запроса
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 */
void awh::server::Proxy::responseClient([[maybe_unused]] const int32_t sid, const uint64_t bid, [[maybe_unused]] const uint64_t rid, const uint32_t code, const string & message) noexcept {
	// Если возникла ошибка, выводим сообщение
	if(code >= 300)
		// Выводим сообщение о неудачном запросе
		this->_log->print("Response from [%zu] failed: %u %s", log_t::flag_t::WARNING, bid, code, message.c_str());
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("response"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const uint64_t, const uint32_t, const string &)> ("response", bid, code, message);
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
			// Выполняем установку размера памяти для хранения полезной нагрузки всех брокеров
			ret.first->second->core.memoryAvailableSize(this->_memoryAvailableSize);
			// Выполняем установку размера хранимой полезной нагрузки для одного брокера
			ret.first->second->core.brokerAvailableSize(this->_brokerAvailableSize);
			// Устанавливаем функцию обратного вызова на получение событий очистки буферов полезной нагрузки
			ret.first->second->core.callback <void (const uint64_t, const size_t)> ("available", std::bind(&server::proxy_t::available, this, broker_t::CLIENT, _1, _2, &ret.first->second->core));
			// Устанавливаем функцию обратного вызова на получение событий очистки буферов полезной нагрузки
			ret.first->second->core.callback <void (const uint64_t, const char *, const size_t)> ("unavailable", std::bind(&server::proxy_t::unavailable, this, broker_t::CLIENT, _1, _2, _3));
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
			// Если таймаут DNS-запроса установлен
			if(this->_settings.dns.timeout > 0)
				// Устанавливаем таймаут DNS-запроса
				ret.first->second->awh.timeoutDNS(this->_settings.dns.timeout);
			// Если список поддерживаемых компрессоров установлен
			if(!this->_settings.compressors.empty())
				// Устанавливаем список поддерживаемых компрессоров
				ret.first->second->awh.compressors(this->_settings.compressors);
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
			ret.first->second->core.verbose(false);
			// Устанавливаем тип сокета подключения (TCP / UDP)
			ret.first->second->core.sonet(this->_settings.sonet);
			// Если флаг синхронизации протоколов клиента и сервера установлен
			if(this->_flags.find(flag_t::SYNCPROTO) != this->_flags.end())
				// Устанавливаем тип протокола с которого подключён клиент
				ret.first->second->core.proto(this->_core.proto(bid));
			// Устанавливаем тип протокола интернета HTTP/2
			else ret.first->second->core.proto(awh::engine_t::proto_t::HTTP2);
			// Устанавливаем параметры SSL-шифрования
			ret.first->second->core.ssl(this->_settings.ssl);
			// Устанавливаем параметры идентификатора Web-клиента
			ret.first->second->awh.ident(this->_ident.id, this->_ident.name, this->_ident.ver);
			// Устанавливаем параметры авторизации на удалённом сервере
			ret.first->second->awh.authType(this->_settings.auth.type, this->_settings.auth.hash);
			// Выполняем установку сетевых параметров подключения
			ret.first->second->awh.network(this->_settings.ips, this->_settings.ns, this->_settings.family);
			// Устанавливаем параметры авторизации на удалённом прокси-сервере
			ret.first->second->awh.authTypeProxy(this->_settings.proxy.auth.type, this->_settings.proxy.auth.hash);
			// Выполняем установку времени ожидания получения данных
			ret.first->second->awh.waitMessage(this->_settings.wtd.wait);
			// Выполняем установку таймаутов на обмен данными в миллисекундах
			ret.first->second->awh.waitTimeDetect(this->_settings.wtd.read, this->_settings.wtd.write, this->_settings.wtd.connect);
			// Устанавливаем функцию обратного вызова активности клиента на Web-сервере
			ret.first->second->awh.callback <void (const client::web_t::mode_t)> ("active", std::bind(&server::proxy_t::activeClient, this, bid, _1));
			// Устанавливаем функцию обратного вызова при завершении работы потока передачи данных клиента
			ret.first->second->awh.callback <void (const int32_t, const uint64_t, const client::web_t::direct_t)> ("end", std::bind(&server::proxy_t::endClient, this, _1, bid, _2, _3));
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("origin"))
				// Выполняем установку функции обратного вызова при получении источников подключения
				ret.first->second->awh.callback <void (const vector <string> &)> ("origin", std::bind(this->_callbacks.get <void (const uint64_t, const vector <string> &)> ("origin"), bid, _1));
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("altsvc"))
				// Устанавливаем функцию обратного вызова при получении альтернативного источника
				ret.first->second->awh.callback <void (const string &, const string &)> ("altsvc", std::bind(this->_callbacks.get <void (const uint64_t, const string &, const string &)> ("altsvc"), bid, _1, _2));
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("push"))
				// Выполняем установку функции обратного вызова при получении PUSH уведомлений
				ret.first->second->awh.callback <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("push", std::bind(&server::proxy_t::pushClient, this, _1, bid, _2, _3, _4, _5));
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("error"))
				// Выполняем установку функции обратного вызова получения ошибок клиента
				ret.first->second->awh.callback <void (const log_t::flag_t, const http::error_t, const string &)> ("error", std::bind(this->_callbacks.get <void (const uint64_t, const broker_t, const log_t::flag_t, const http::error_t, const string &)> ("error"), bid, broker_t::CLIENT, _1, _2, _3));
		} break;
		// Если производится отключение клиента от сервера
		case static_cast <uint8_t> (server::web_t::mode_t::DISCONNECT): {
			// Выполняем поиск клиента в списке
			auto i = this->_clients.find(bid);
			// Если клиент в списке найден
			if(i != this->_clients.end()){
				// Выполняем остановку подключения
				i->second->awh.stop();
				// Выполняем сброс метода подклюения
				i->second->method = awh::web_t::method_t::NONE;
				// Выполняем отключение клиента от сетевого ядра
				this->_core.unbind(&i->second->core);
			}
		} break;
	}
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("active"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const uint64_t, const broker_t, const web_t::mode_t)> ("active", bid, broker_t::SERVER, mode);
}
/**
 * activeClient Метод идентификации активности на Web сервере (для клиента)
 * @param bid  идентификатор брокера (клиента)
 * @param mode режим события подключения
 */
void awh::server::Proxy::activeClient(const uint64_t bid, const client::web_t::mode_t mode) noexcept {
	// Выполняем поиск объекта клиента
	auto i = this->_clients.find(bid);
	// Если активный клиент найден
	if(i != this->_clients.end()){
		// Определяем активность клиента
		switch(static_cast <uint8_t> (mode)){
			// Если производится подключение клиента к серверу
			case static_cast <uint8_t> (client::web_t::mode_t::CONNECT): {
				// Снимаем флаг занятости сервера
				i->second->busy = !i->second->busy;
				// Определяем активный метод запроса клиента
				switch(static_cast <uint8_t> (i->second->request.params.method)){
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
						// Выполняем установку активного агента клиента
						request.agent = i->second->agent;
						// Устанавливаем тепло запроса
						request.entity = i->second->request.entity;
						// Запоминаем переданные заголовки
						request.headers = i->second->request.headers;
						// Устанавливаем адрес запроса
						request.url = i->second->request.params.url;
						// Устанавливаем метод запроса
						request.method = i->second->request.params.method;
						// Выполняем генерацию идентификатора запроса
						request.id = this->_fmk->timestamp(fmk_t::chrono_t::NANOSECONDS);
						// Выполняем установку метода подключения
						i->second->method = i->second->request.params.method;
						// Выполняем установку потока в список потоков
						i->second->streams.emplace(request.id, i->second->sid);
						// Выполняем запрос на сервер
						i->second->awh.send(request);
					} break;
					// Если запрашивается клиентом метод CONNECT
					case static_cast <uint8_t> (awh::web_t::method_t::CONNECT): {
						// Если подключение ещё не выполнено
						if(i->second->method == awh::web_t::method_t::NONE){
							// Выполняем установку метода подключения
							i->second->method = i->second->request.params.method;
							// Если активирован Websocket клиент
							if(i->second->agent == client::web_t::agent_t::WEBSOCKET){
								// Создаём объект запроса
								client::web_t::request_t request;
								// Выполняем установку активного агента клиента
								request.agent = i->second->agent;
								// Устанавливаем тепло запроса
								request.entity = i->second->request.entity;
								// Запоминаем переданные заголовки
								request.headers = i->second->request.headers;
								// Устанавливаем адрес запроса
								request.url = i->second->request.params.url;
								// Устанавливаем метод запроса
								request.method = i->second->request.params.method;
								// Выполняем генерацию идентификатора запроса
								request.id = this->_fmk->timestamp(fmk_t::chrono_t::NANOSECONDS);
								// Выполняем установку потока в список потоков
								i->second->streams.emplace(request.id, i->second->sid);
								// Выполняем запрос на сервер
								i->second->awh.send(request);
							// Если клиент активирован как HTTP
							} else {
								// Если тип сокета установлен как TCP/IP
								if(this->_core.sonet() == awh::scheme_t::sonet_t::TCP)
									// Подписываемся на получение сырых данных полученных клиентом с удалённого сервера
									i->second->awh.callback <bool (const char *, const size_t)> ("raw", std::bind(&server::proxy_t::raw, this, bid, broker_t::CLIENT, _1, _2));
								// Выполняем отправку ответа клиенту
								this->_server.send(i->second->sid, bid);
							}
						}
					} break;
				}
			} break;
			// Если производится отключение клиента от сервера
			case static_cast <uint8_t> (client::web_t::mode_t::DISCONNECT): {
				// Выполняем сброс метода подклюения
				i->second->method = awh::web_t::method_t::NONE;
				// Если результат не получен, просто отключаемся
				if(i->second->response.headers.empty())
					// Выполняем закрытие подключения
					this->close(bid);
			} break;
		}
	}
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("active")){
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
		this->_callbacks.call <void (const uint64_t, const broker_t, const web_t::mode_t)> ("active", bid, broker_t::CLIENT, result);
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
	auto i = this->_clients.find(bid);
	// Если активный клиент найден
	if((i != this->_clients.end()) && !i->second->busy){
		// Если тело запроса с сервера получено
		if(!entity.empty()){
			// Устанавливаем полученные данные тела запроса
			i->second->request.entity.assign(entity.begin(), entity.end());
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("entityServer"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, vector <char> *)> ("entityServer", bid, method, url, &i->second->request.entity);
		// Если тело запроса с сервера не получено
		} else {
			// Выполняем очистку тела запроса
			i->second->request.entity.clear();
			// Если размер выделенной памяти выше максимального размера буфера
			if(i->second->request.entity.capacity() > AWH_BUFFER_SIZE)
				// Выполняем очистку временного буфера данных
				vector <char> ().swap(i->second->request.entity);
		}
	}
}
/**
 * entityClient Метод получения тела ответа с сервера клиенту
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера (клиента)
 * @param rid     идентификатор запроса
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 * @param entity  тело ответа клиенту с сервера
 */
void awh::server::Proxy::entityClient(const int32_t sid, const uint64_t bid, const uint64_t rid, const uint32_t code, const string & message, const vector <char> & entity) noexcept {
	// Выполняем поиск объекта клиента
	auto i = this->_clients.find(bid);
	// Если активный клиент найден
	if(i != this->_clients.end()){
		// Если тело ответа с сервера получено
		if(!entity.empty()){
			// Устанавливаем полученные данные тела ответа
			i->second->response.entity.assign(entity.begin(), entity.end());
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("entityClient"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const uint32_t, const string &, vector <char> *)> ("entityClient", bid, code, message, &i->second->response.entity);
		// Если тело ответа с сервера не получено
		} else {
			// Выполняем очистку тела ответа
			i->second->response.entity.clear();
			// Если размер выделенной памяти выше максимального размера буфера
			if(i->second->response.entity.capacity() > AWH_BUFFER_SIZE)
				// Выполняем очистку временного буфера данных
				vector <char> ().swap(i->second->response.entity);
		}
		// Снимаем флаг отправки результата
		i->second->sending = false;
		// Выполняем поиск идентификатора потока
		auto j = i->second->streams.find(rid);
		// Если идентификатор потока найден, удаляем его
		if(j != i->second->streams.end()){
			// Выводим полученный результат
			this->completed(j->second, bid);
			// Выполняем удаление потока из списка
			i->second->streams.erase(j);
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
	auto i = this->_clients.find(bid);
	// Если активный клиент найден
	if(i != this->_clients.end()){
		// Если заголовки ответа получены и сервер ещё не занят
		if(!headers.empty() && !i->second->busy){
			// Список заголовков Via
			vector <string> via;
			// Запоминаем идентификатор потока
			i->second->sid = sid;
			// Снимаем флаг отправки результата
			i->second->sending = false;
			// Устанавливаем полученные заголовки
			i->second->request.headers = headers;
			// Устанавливаем URL-адрес запроса
			i->second->request.params.url = url;
			// Устанавливаем метод запроса
			i->second->request.params.method = method;
			// Выполняем перебор всех полученных заголовков
			for(auto j = i->second->request.headers.begin(); j != i->second->request.headers.end();){
				// Если получен заголовок Via
				if(this->_fmk->compare("via", j->first)){
					// Добавляем заголовок в список
					via.push_back(j->second);
					// Выполняем удаление заголовка
					j = i->second->request.headers.erase(j);
					// Продолжаем перебор дальше
					continue;
				// Если заголовок соответствует прокси-серверу
				} else if(this->_fmk->compare("te", j->first) || this->_fmk->exists("proxy-", j->first)) {
					// Выполняем удаление заголовка
					j = i->second->request.headers.erase(j);
					// Продолжаем перебор дальше
					continue;
				// Если найден заголовок подключения
				} else if(this->_fmk->compare("connection", j->first)) {
					// Переводим значение в нижний регистр
					this->_fmk->transform(j->second, fmk_t::transform_t::LOWER);
					// Выполняем поиск заголовка Transfer-Encoding
					const size_t pos = j->second.find("te");
					// Если заголовок найден
					if(pos != string::npos){
						// Выполняем удаление значение TE из заголовка
						j->second.erase(pos, 2);
						// Если первый символ является запятой, удаляем
						if(j->second.front() == ',')
							// Удаляем запятую
							j->second.erase(0, 1);
						// Выполняем удаление лишних пробелов
						this->_fmk->transform(j->second, fmk_t::transform_t::TRIM);
					}
				}
				// Продолжаем перебор дальше
				++j;
			}
			// Получаем объект HTTP-парсера
			const awh::http_t * http = this->_server.parser(sid, bid);
			// Если объект HTTP-парсера получен
			if(http != nullptr){
				// Если нужно переключиться на протокол Websocket
				if(this->_fmk->exists("websocket", http->upgrade()))
					// Устанавливаем агента Websocket
					i->second->agent = client::web_t::agent_t::WEBSOCKET;
			}
			// Выполняем получение заголовка Via
			const string & header = this->via(sid, bid, via);
			// Если заголовок получен
			if(!header.empty())
				// Устанавливаем загловок Via
				i->second->request.headers.emplace("Via", header);
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("headersServer"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, unordered_multimap <string, string> *)> ("headersServer", bid, method, url, &i->second->request.headers);
		}
	}
}
/**
 * headersClient Метод получения заголовков ответа с сервера клиенту
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера (клиента)
 * @param rid     идентификатор запроса
 * @param code    код ответа сервера
 * @param message сообщение ответа сервера
 * @param headers заголовки HTTP-ответа
 */
void awh::server::Proxy::headersClient(const int32_t sid, const uint64_t bid, const uint64_t rid, const uint32_t code, const string & message, const unordered_multimap <string, string> & headers) noexcept {
	// Выполняем поиск объекта клиента
	auto i = this->_clients.find(bid);
	// Если активный клиент найден
	if(i != this->_clients.end()){
		// Если заголовки ответа получены
		if(!headers.empty()){
			// Выполняем поиск идентификатора потока
			auto j = i->second->streams.find(rid);
			// Если идентификатор потока найден, удаляем его
			if(j != i->second->streams.end()){
				// Список заголовков Via
				vector <string> via;
				// Устанавливаем полученные заголовки
				i->second->response.headers = headers;
				// Устанавливаем код ответа сервера
				i->second->response.params.code = code;
				// Устанавливаем сообщение ответа сервера
				i->second->response.params.message = message;
				// Компрессор которым необходимо выполнить сжатие контента
				http_t::compressor_t compressor = http_t::compressor_t::NONE;
				// Выполняем перебор всех полученных заголовков
				for(auto j = i->second->response.headers.begin(); j != i->second->response.headers.end();){
					// Если получен заголовок Via
					if(this->_fmk->exists("via", j->first)){
						// Добавляем заголовок в список
						via.push_back(j->second);
						// Выполняем удаление заголовка
						j = i->second->response.headers.erase(j);
						// Продолжаем перебор дальше
						continue;
					// Если мы получили заголовок сообщающий о том, в каком формате закодированны данные
					} else if(this->_fmk->exists("content-encoding", j->first)) {
						// Если флаг рекомпрессии данных прокси-сервером не установлен
						if(this->_flags.find(flag_t::RECOMPRESS) == this->_flags.end()){
							// Если данные пришли сжатые методом LZ4
							if(this->_fmk->exists("lz4", j->second))
								// Устанавливаем тип компрессии полезной нагрузки
								compressor = http_t::compressor_t::LZ4;
							// Если данные пришли сжатые методом Zstandard
							else if(this->_fmk->exists("zstd", j->second))
								// Устанавливаем тип компрессии полезной нагрузки
								compressor = http_t::compressor_t::ZSTD;
							// Если данные пришли сжатые методом LZma
							else if(this->_fmk->exists("xz", j->second))
								// Устанавливаем тип компрессии полезной нагрузки
								compressor = http_t::compressor_t::LZMA;
							// Если данные пришли сжатые методом Brotli
							else if(this->_fmk->exists("br", j->second))
								// Устанавливаем тип компрессии полезной нагрузки
								compressor = http_t::compressor_t::BROTLI;
							// Если данные пришли сжатые методом BZip2
							else if(this->_fmk->exists("bzip2", j->second))
								// Устанавливаем тип компрессии полезной нагрузки
								compressor = http_t::compressor_t::BZIP2;
							// Если данные пришли сжатые методом GZip
							else if(this->_fmk->exists("gzip", j->second))
								// Устанавливаем тип компрессии полезной нагрузки
								compressor = http_t::compressor_t::GZIP;
							// Если данные пришли сжатые методом Deflate
							else if(this->_fmk->exists("deflate", j->second))
								// Устанавливаем тип компрессии полезной нагрузки
								compressor = http_t::compressor_t::DEFLATE;
						}
					}
					// Продолжаем перебор дальше
					++j;
				}
				// Если флаг рекомпрессии данных прокси-сервером установлен
				if(this->_flags.find(flag_t::RECOMPRESS) != this->_flags.end())
					// Устанавливаем компрессор рекомпрессии
					compressor = this->_compressor;
				// Получаем объект HTTP-парсера
				const awh::http_t * http = this->_server.parser(j->second, bid);
				// Если объект HTTP-парсера получен
				if(http != nullptr)
					// Устанавливаем параметры компрессии
					const_cast <awh::http_t *> (http)->compression(compressor);
				// Выполняем получение заголовка Via
				const string & header = this->via(j->second, bid, via);
				// Если заголовок получен
				if(!header.empty())
					// Устанавливаем загловок Via
					i->second->response.headers.emplace("Via", header);
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("headersClient"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint64_t, const uint32_t, const string &, unordered_multimap <string, string> *)> ("headersClient", bid, code, message, &i->second->response.headers);
				// Если производится активация Websocket
				if(i->second->agent == client::web_t::agent_t::WEBSOCKET){
					// Флаг удачно-выполненного подключения
					bool connected = false;
					// Определяем протокола подключения
					switch(static_cast <uint8_t> (this->_core.proto(bid))){
						// Если протокол подключения соответствует HTTP/1.1
						case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1):
							// Если переключение на протокол Websocket произведено
							connected = (i->second->response.params.code == 101);
						break;
						// Если протокол подключения соответствует HTTP/2
						case static_cast <uint8_t> (engine_t::proto_t::HTTP2):
							// Если переключение на протокол Websocket произведено
							connected = (i->second->response.params.code == 200);
						break;
					}
					// Если подключение установлено
					if(connected){
						// Выполняем снятие переключение протокола
						i->second->upgrade = false;
						// Выполняем установку метода подключения
						i->second->method = awh::web_t::method_t::CONNECT;
						// Меняем метод подключения на CONNECT
						i->second->request.params.method = awh::web_t::method_t::CONNECT;
						// Подписываемся на получение сырых данных полученных клиентом с удалённого сервера
						i->second->awh.callback <bool (const char *, const size_t)> ("raw", std::bind(&server::proxy_t::raw, this, bid, broker_t::CLIENT, _1, _2));
						// Выводим полученный результат
						this->completed(j->second, bid);
						// Выполняем удаление потока из списка
						i->second->streams.erase(j);
					}
				}
			}
		}
	}
}
/**
 * pushClient Метод получения заголовков выполненного запроса (PUSH HTTP/2)
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера (клиента)
 * @param rid     идентификатор запроса
 * @param method  метод запроса на уделённый сервер
 * @param url     URL-адрес параметров запроса
 * @param headers заголовки HTTP-запроса
 */
void awh::server::Proxy::pushClient([[maybe_unused]] const int32_t sid, const uint64_t bid, [[maybe_unused]] const uint64_t rid, const awh::web_t::method_t method, const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Если функция обратного вызова установлена
	if(this->_callbacks.is("push"))
		// Выполняем функцию обратного вызова
		this->_callbacks.call <void (const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("push", bid, method, url, headers);
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
		auto i = this->_clients.find(bid);
		// Если активный клиент найден
		if(i != this->_clients.end()){
			// Если сервер ещё не занят
			if(!i->second->busy){
				// Если функция обратного вызова установлена
				if(this->_callbacks.is("handshake"))
					// Выполняем функцию обратного вызова
					this->_callbacks.call <void (const uint64_t, const engine_t::proto_t)> ("handshake", bid, this->_core.proto(bid));
				// Определяем тип активного сокета сервера
				switch(static_cast <uint8_t> (this->_core.sonet())){
					// Если тип сокета установлен как TCP/IP
					case static_cast <uint8_t> (awh::scheme_t::sonet_t::TCP): {
						// Если подключение ещё не выполнено
						if(i->second->method == awh::web_t::method_t::NONE){
							// Помечаем, что сервер занят
							i->second->busy = !i->second->busy;
							// Создаём список флагов клиента
							set <client::web_t::flag_t> flags = {
								client::web_t::flag_t::NOT_STOP,
								client::web_t::flag_t::NOT_INFO,
								client::web_t::flag_t::WEBSOCKET_ENABLE
							};
							// Если флаг запрета выполнения пингов установлен
							if(this->_flags.find(flag_t::NOT_PING) != this->_flags.end())
								// Устанавливаем флаг запрета выполнения пингов
								flags.emplace(client::web_t::flag_t::NOT_PING);
							// Если флаг разрешения выполнения редиректов установлен
							if(this->_flags.find(flag_t::REDIRECTS) != this->_flags.end())
								// Устанавливаем флаг разрешения выполнения редиректов
								flags.emplace(client::web_t::flag_t::REDIRECTS);
							// Если флаг резрешающий метод CONNECT для прокси-клиента установлен
							if(this->_flags.find(flag_t::CONNECT_METHOD_CLIENT_ENABLE) != this->_flags.end())
								// Выполняем установку флага разрешающего метода CONNECT для прокси-клиента
								flags.emplace(client::web_t::flag_t::CONNECT_METHOD_ENABLE);
							// Если метод запроса не является методом CONNECT
							if(i->second->request.params.method != awh::web_t::method_t::CONNECT){
								// Подписываемся на получение сообщения сервера
								i->second->awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", std::bind(&server::proxy_t::responseClient, this, _1, bid, _2, _3, _4));
								// Устанавливаем функцию обратного вызова при получении HTTP-тела ответа с сервера клиенту
								i->second->awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &)> ("entity", std::bind(&server::proxy_t::entityClient, this, _1, bid, _2, _3, _4, _5));
								// Устанавливаем функцию обратного вызова при получении HTTP-заголовков ответа с сервера клиенту
								i->second->awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headers", std::bind(&server::proxy_t::headersClient, this, _1, bid, _2, _3, _4, _5));
							// Если метод CONNECT не разрешён для запроса
							} else if(this->_flags.find(flag_t::CONNECT_METHOD_SERVER_ENABLE) == this->_flags.end()) {
								// Формируем сообщение ответа
								const string message = "CONNECT method is forbidden on the proxy-server";
								// Формируем тело ответа
								const string & body = this->_fmk->format("<html>\n<head>\n<title>%u %s</title>\n</head>\n<body>\n<h2>%u %s</h2>\n</body>\n</html>\n", 403, message.c_str(), 403, message.c_str());
								// Если метод CONNECT запрещено использовать
								this->_server.send(sid, bid, 403, message, vector <char> (body.begin(), body.end()), {
									{"Connection", "close"},
									{"Proxy-Connection", "close"},
									{"Content-type", "text/html; charset=utf-8"}
								});
								// Выходим из функции
								return;
							// Если метод CONNECT разрешён, выполняем запрет инициализации SSL-контекста
							} else flags.emplace(client::web_t::flag_t::NO_INIT_SSL);
							// Устанавливаем флаги настроек модуля
							i->second->awh.mode(flags);
							// Выполняем инициализацию подключения
							i->second->awh.init(this->_uri.origin(i->second->request.params.url), {
								awh::http_t::compressor_t::ZSTD,
								awh::http_t::compressor_t::BROTLI,
								awh::http_t::compressor_t::GZIP,
								awh::http_t::compressor_t::DEFLATE
							});
							// Выполняем подключение клиента к сетевому ядру
							this->_core.bind(&i->second->core);
							// Выполняем установку подключения
							i->second->awh.start();
						// Если подключение уже выполнено
						} else {
							// Создаём объект запроса
							client::web_t::request_t request;
							// Выполняем установку активного агента клиента
							request.agent = i->second->agent;
							// Устанавливаем тепло запроса
							request.entity = i->second->request.entity;
							// Запоминаем переданные заголовки
							request.headers = i->second->request.headers;
							// Устанавливаем адрес запроса
							request.url = i->second->request.params.url;
							// Устанавливаем метод запроса
							request.method = i->second->request.params.method;
							// Выполняем генерацию идентификатора запроса
							request.id = this->_fmk->timestamp(fmk_t::chrono_t::NANOSECONDS);
							// Выполняем установку потока в список потоков
							i->second->streams.emplace(request.id, i->second->sid);
							// Выполняем запрос на сервер
							i->second->awh.send(request);
						}
					} break;
					// Если тип сокета установлен как TCP/IP TLS
					case static_cast <uint8_t> (awh::scheme_t::sonet_t::TLS): {
						// Определяем активный метод запроса клиента
						switch(static_cast <uint8_t> (i->second->request.params.method)){
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
								if(i->second->method == awh::web_t::method_t::NONE){
									// Помечаем, что сервер занят
									i->second->busy = !i->second->busy;
									// Создаём список флагов клиента
									set <client::web_t::flag_t> flags = {
										client::web_t::flag_t::NOT_STOP,
										client::web_t::flag_t::NOT_INFO,
										client::web_t::flag_t::WEBSOCKET_ENABLE
									};
									// Если флаг запрета выполнения пингов установлен
									if(this->_flags.find(flag_t::NOT_PING) != this->_flags.end())
										// Устанавливаем флаг запрета выполнения пингов
										flags.emplace(client::web_t::flag_t::NOT_PING);
									// Если флаг разрешения выполнения редиректов установлен
									if(this->_flags.find(flag_t::REDIRECTS) != this->_flags.end())
										// Устанавливаем флаг разрешения выполнения редиректов
										flags.emplace(client::web_t::flag_t::REDIRECTS);
									// Если флаг резрешающий метод CONNECT для прокси-клиента установлен
									if(this->_flags.find(flag_t::CONNECT_METHOD_CLIENT_ENABLE) != this->_flags.end())
										// Выполняем установку флага разрешающего метода CONNECT для прокси-клиента
										flags.emplace(client::web_t::flag_t::CONNECT_METHOD_ENABLE);
									// Если порт сервера не стандартный, устанавливаем схему протокола
									if((i->second->request.params.url.port != 80) && (i->second->request.params.url.port != 443))
										// Выполняем установку защищённого протокола
										i->second->request.params.url.schema = "https";
									// Подписываемся на получение сообщения сервера
									i->second->awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", std::bind(&server::proxy_t::responseClient, this, _1, bid, _2, _3, _4));
									// Устанавливаем функцию обратного вызова при получении HTTP-тела ответа с сервера клиенту
									i->second->awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &)> ("entity", std::bind(&server::proxy_t::entityClient, this, _1, bid, _2, _3, _4, _5));
									// Устанавливаем функцию обратного вызова при получении HTTP-заголовков ответа с сервера клиенту
									i->second->awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headers", std::bind(&server::proxy_t::headersClient, this, _1, bid, _2, _3, _4, _5));
									// Устанавливаем флаги настроек модуля
									i->second->awh.mode(flags);
									// Выполняем инициализацию подключения
									i->second->awh.init(this->_uri.origin(i->second->request.params.url), {
										awh::http_t::compressor_t::ZSTD,
										awh::http_t::compressor_t::BROTLI,
										awh::http_t::compressor_t::GZIP,
										awh::http_t::compressor_t::DEFLATE
									});
									// Выполняем подключение клиента к сетевому ядру
									this->_core.bind(&i->second->core);
									// Выполняем установку подключения
									i->second->awh.start();
								// Если подключение уже выполнено
								} else {
									// Создаём объект запроса
									client::web_t::request_t request;
									// Выполняем установку активного агента клиента
									request.agent = i->second->agent;
									// Устанавливаем тепло запроса
									request.entity = i->second->request.entity;
									// Запоминаем переданные заголовки
									request.headers = i->second->request.headers;
									// Устанавливаем адрес запроса
									request.url = i->second->request.params.url;
									// Устанавливаем метод запроса
									request.method = i->second->request.params.method;
									// Выполняем генерацию идентификатора запроса
									request.id = this->_fmk->timestamp(fmk_t::chrono_t::NANOSECONDS);
									// Выполняем установку потока в список потоков
									i->second->streams.emplace(request.id, i->second->sid);
									// Выполняем запрос на сервер
									i->second->awh.send(request);
								}
							} break;
							// Если запрашивается клиентом метод CONNECT
							case static_cast <uint8_t> (awh::web_t::method_t::CONNECT): {
								// Если активирован Websocket клиент и подключение уже выполненно
								if((i->second->method == awh::web_t::method_t::CONNECT) &&
								   (i->second->agent == client::web_t::agent_t::WEBSOCKET)){
									// Создаём объект запроса
									client::web_t::request_t request;
									// Выполняем установку активного агента клиента
									request.agent = i->second->agent;
									// Устанавливаем тепло запроса
									request.entity = i->second->request.entity;
									// Запоминаем переданные заголовки
									request.headers = i->second->request.headers;
									// Устанавливаем адрес запроса
									request.url = i->second->request.params.url;
									// Устанавливаем метод запроса
									request.method = i->second->request.params.method;
									// Выполняем генерацию идентификатора запроса
									request.id = this->_fmk->timestamp(fmk_t::chrono_t::NANOSECONDS);
									// Выполняем установку потока в список потоков
									i->second->streams.emplace(request.id, i->second->sid);
									// Выполняем запрос на сервер
									i->second->awh.send(request);
								// Если активирован режим работы HTTP-клиента и подключение не выполненно
								} else if(i->second->method == awh::web_t::method_t::NONE) {
									// Помечаем, что сервер занят
									i->second->busy = !i->second->busy;
									// Если метод CONNECT не разрешён для запроса
									if(this->_flags.find(flag_t::CONNECT_METHOD_SERVER_ENABLE) == this->_flags.end()){
										// Формируем сообщение ответа
										const string message = "CONNECT method is forbidden on the proxy-server";
										// Формируем тело ответа
										const string & body = this->_fmk->format("<html>\n<head>\n<title>%u %s</title>\n</head>\n<body>\n<h2>%u %s</h2>\n</body>\n</html>\n", 403, message.c_str(), 403, message.c_str());
										// Если метод CONNECT запрещено использовать
										this->_server.send(sid, bid, 403, message, vector <char> (body.begin(), body.end()), {
											{"Connection", "close"},
											{"Proxy-Connection", "close"},
											{"Content-type", "text/html; charset=utf-8"}
										});
										// Выходим из функции
										return;
									}
									// Создаём список флагов клиента
									set <client::web_t::flag_t> flags = {
										client::web_t::flag_t::NOT_STOP,
										client::web_t::flag_t::NOT_INFO,
										client::web_t::flag_t::WEBSOCKET_ENABLE
									};
									// Если флаг запрета выполнения пингов установлен
									if(this->_flags.find(flag_t::NOT_PING) != this->_flags.end())
										// Устанавливаем флаг запрета выполнения пингов
										flags.emplace(client::web_t::flag_t::NOT_PING);
									// Если флаг разрешения выполнения редиректов установлен
									if(this->_flags.find(flag_t::REDIRECTS) != this->_flags.end())
										// Устанавливаем флаг разрешения выполнения редиректов
										flags.emplace(client::web_t::flag_t::REDIRECTS);
									// Если флаг резрешающий метод CONNECT для прокси-клиента установлен
									if(this->_flags.find(flag_t::CONNECT_METHOD_CLIENT_ENABLE) != this->_flags.end())
										// Выполняем установку флага разрешающего метода CONNECT для прокси-клиента
										flags.emplace(client::web_t::flag_t::CONNECT_METHOD_ENABLE);
									// Устанавливаем флаги настроек модуля
									i->second->awh.mode(flags);
									// Если порт сервера не стандартный, устанавливаем схему протокола
									if((i->second->request.params.url.port != 80) && (i->second->request.params.url.port != 443))
										// Выполняем установку защищённого протокола
										i->second->request.params.url.schema = "https";
									// Выполняем инициализацию подключения
									i->second->awh.init(this->_uri.origin(i->second->request.params.url), {
										awh::http_t::compressor_t::ZSTD,
										awh::http_t::compressor_t::BROTLI,
										awh::http_t::compressor_t::GZIP,
										awh::http_t::compressor_t::DEFLATE
									});
									// Подписываемся на получение сообщения сервера
									i->second->awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", std::bind(&server::proxy_t::responseClient, this, _1, bid, _2, _3, _4));
									// Устанавливаем функцию обратного вызова при получении HTTP-тела ответа с сервера клиенту
									i->second->awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &)> ("entity", std::bind(&server::proxy_t::entityClient, this, _1, bid, _2, _3, _4, _5));
									// Устанавливаем функцию обратного вызова при получении HTTP-заголовков ответа с сервера клиенту
									i->second->awh.callback <void (const int32_t, const uint64_t, const uint32_t, const string &, const unordered_multimap <string, string> &)> ("headers", std::bind(&server::proxy_t::headersClient, this, _1, bid, _2, _3, _4, _5));
									// Выполняем подключение клиента к сетевому ядру
									this->_core.bind(&i->second->core);
									// Выполняем установку подключения
									i->second->awh.start();
								}
							} break;
						}
					} break;
				}
			// Если сервер уже занят, выполняем отправку ответа клиенту, что сервер занят
			} else this->_server.send(sid, bid, 409, "Another request is currently being processed", {}, {
				{"Connection", "keep-alive"},
				{"Proxy-Connection", "keep-alive"}
			});
		}
	}
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
	// Если бинарные данные получены
	if((buffer != nullptr) && (size > 0)){
		// Выполняем поиск объекта клиента
		auto i = this->_clients.find(bid);
		// Если активный клиент найден и подключение установлено
		if((i != this->_clients.end()) && (i->second->method == awh::web_t::method_t::CONNECT)){
			// Если тип сокета установлен как TCP/IP
			if(i->second->upgrade || (this->_core.sonet() == awh::scheme_t::sonet_t::TCP)){
				// Если установлен метод CONNECT
				if(!(result = (i->second->request.params.method != awh::web_t::method_t::CONNECT))){
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
							i->second->awh.send(buffer, size);
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
 * via Метод генерации заголовка Via
 * @param sid       идентификатор потока
 * @param bid       идентификатор брокера (клиента)
 * @param mediators список предыдущих посредников
 * @return          сгенерированный заголовок
 */
string awh::server::Proxy::via(const int32_t sid, const uint64_t bid, const vector <string> & mediators) const noexcept {
	// Результат работы функции
	string result = "";
	// Получаем объект HTTP-парсера
	const awh::http_t * http = this->_server.parser(sid, bid);
	// Если объект HTTP-парсера получен
	if(http != nullptr){
		// Выполняем получение идентификатора сети
		const uint16_t sid = this->_core.sid(bid);
		// Получаем порт сервера
		const uint32_t port = this->_core.port(sid);
		// Получаем хост сервера
		const string & host = this->_core.host(sid);
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
			result.append(this->_fmk->format("HTTP/%.1f %s (%s)", 1.1f, host.c_str(), http->ident(awh::http_t::process_t::RESPONSE).c_str()));
		// Если активирован хост и порт
		else {
			// Определяем протокола подключения
			switch(static_cast <uint8_t> (this->_core.proto(bid))){
				// Если протокол подключения соответствует HTTP/1.1
				case static_cast <uint8_t> (engine_t::proto_t::HTTP1_1): {
					// Если порт установлен не стандартный
					if(port != (this->_core.sonet() == awh::scheme_t::sonet_t::TLS ? SERVER_PROXY_SEC_PORT : SERVER_PROXY_PORT))
						// Формируем заголовок Via
						result.append(this->_fmk->format("HTTP/%.1f %s:%u (%s)", 1.1f, host.c_str(), port, http->ident(awh::http_t::process_t::RESPONSE).c_str()));
					// Будем считать, что порт установлен стандартный
					else result.append(this->_fmk->format("HTTP/%.1f %s (%s)", 1.1f, host.c_str(), http->ident(awh::http_t::process_t::RESPONSE).c_str()));
				} break;
				// Если протокол подключения соответствует HTTP/2
				case static_cast <uint8_t> (engine_t::proto_t::HTTP2): {
					// Если порт установлен не стандартный
					if(port != (this->_core.sonet() == awh::scheme_t::sonet_t::TLS ? SERVER_PROXY_SEC_PORT : SERVER_PROXY_PORT))
						// Формируем заголовок Via
						result.append(this->_fmk->format("HTTP/%u %s:%u (%s)", 2, host.c_str(), port, http->ident(awh::http_t::process_t::RESPONSE).c_str()));
					// Будем считать, что порт установлен стандартный
					else result.append(this->_fmk->format("HTTP/%u %s (%s)", 2, host.c_str(), http->ident(awh::http_t::process_t::RESPONSE).c_str()));
				} break;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * completed Метод завершения получения данных
 * @param sid идентификатор потока
 * @param bid идентификатор брокера (клиента)
 */
void awh::server::Proxy::completed(const int32_t sid, const uint64_t bid) noexcept {
	// Выполняем поиск объекта клиента
	auto i = this->_clients.find(bid);
	// Если активный клиент найден
	if(!i->second->sending && (i->second->sending = (i != this->_clients.end()))){
		// Если заголовки ответа получены
		if(!i->second->response.headers.empty()){
			// Отправляем сообщение клиенту
			this->_server.send(sid, bid, i->second->response.params.code, i->second->response.params.message, i->second->response.entity, i->second->response.headers);
			// Выполняем переключение протокола
			i->second->upgrade = (i->second->agent == client::web_t::agent_t::WEBSOCKET);
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("completed"))
				// Выполняем функцию обратного вызова
				this->_callbacks.call <void (const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("completed", bid, i->second->response.params.code, i->second->response.params.message, i->second->response.entity, i->second->response.headers);
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
 * @param sid идентификатор потока
 * @param bid идентификатор брокера
 * @return    объект HTTP-парсера
 */
const awh::http_t * awh::server::Proxy::parser(const int32_t sid, const uint64_t bid) const noexcept {
	// Выполняем извлечение объекта HTTP-парсера
	return this->_server.parser(sid, bid);
}
/**
 * init Метод инициализации PROXY-сервера
 * @param socket     unix-сокет для биндинга
 * @param compressor поддерживаемый компрессор для рекомпрессии пересылаемых данных
 */
void awh::server::Proxy::init(const string & socket, const http_t::compressor_t compressor) noexcept {
	// Выполняем инициализацию PROXY-сервера
	this->_server.init(socket);
	// Устанавливаем компрессор для рекомпрессии пересылаемых данных
	this->_compressor = compressor;
	// Выполняем установку типа протокола интернета
	this->_core.family(scheme_t::family_t::NIX);
}
/**
 * init Метод инициализации PROXY-сервера
 * @param port       порт сервера
 * @param host       хост сервера
 * @param compressor поддерживаемый компрессор для рекомпрессии пересылаемых данных
 * @param family     тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::server::Proxy::init(const uint32_t port, const string & host, const http_t::compressor_t compressor, const scheme_t::family_t family) noexcept {
	// Выполняем инициализацию PROXY-сервера
	this->_server.init(port, host);
	// Устанавливаем компрессор для рекомпрессии пересылаемых данных
	this->_compressor = compressor;
	// Определяем тип интернет-протокола
	switch(static_cast <uint8_t> (family)){
		// Если активирован интернет-протокол IPv4
		case static_cast <uint8_t> (scheme_t::family_t::IPV4):
		// Если активирован интернет-протокол IPv6
		case static_cast <uint8_t> (scheme_t::family_t::IPV6):
			// Выполняем установку типа протокола интернета
			this->_core.family(family);
		break;
		// Если установлен любой другой-интернет протокол
		default: this->_core.family(scheme_t::family_t::IPV4);
	}
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::server::Proxy::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем установку функции обратного вызова для вывода полученных заголовков с клиента
	this->_callbacks.set("push", callbacks);
	// Выполняем установку функции обратного вызова на событие получения ошибки
	this->_callbacks.set("error", callbacks);
	// Выполняем установку функции обратного вызова при удаление клиента из стека сервера
	this->_callbacks.set("erase", callbacks);
	// Выполняем установку функции обратного вызова на событие активации брокера на сервере
	this->_callbacks.set("accept", callbacks);
	// Выполняем установку функции обратного вызова при получении источников подключения
	this->_callbacks.set("origin", callbacks);
	// Выполняем установку функции обратного вызова при получении альтернативных сервисов
	this->_callbacks.set("altsvc", callbacks);
	// Выполняем установку функции обратного вызова на событие запуска или остановки подключения
	this->_callbacks.set("active", callbacks);
	// Выполняем установку функции обратного вызова для вывода ответа сервера на ранее выполненный запрос
	this->_callbacks.set("response", callbacks);
	// Выполняем установку функции обратного вызова при выполнении рукопожатия на прокси-сервере
	this->_callbacks.set("handshake", callbacks);
	// Выполняем установку функции обратного вызова на событие формирования готового ответа клиенту подключённого к прокси-серверу
	this->_callbacks.set("completed", callbacks);
	// Выполняем установку функции обратного вызова при освобождении буфера хранения полезной нагрузки
	this->_callbacks.set("available", callbacks);
	// Выполняем установку функции обратного вызова при заполнении буфера хранения полезной нагрузки
	this->_callbacks.set("unavailable", callbacks);
	// Выполняем установку функции обратного вызова на событие получения тела ответа с удалённого сервера
	this->_callbacks.set("entityClient", callbacks);
	// Выполняем установку функции обратного вызова на событие получения тела запроса на прокси-сервере
	this->_callbacks.set("entityServer", callbacks);
	// Выполняем установку функции обратного вызова на событие получения заголовков ответа с удалённого сервера
	this->_callbacks.set("headersClient", callbacks);
	// Выполняем установку функции обратного вызова на событие получения заголовков запроса на прокси-сервере
	this->_callbacks.set("headersServer", callbacks);
	// Выполняем установку функции обратного вызова для обработки авторизации
	this->_callbacks.set("checkPassword", callbacks);
	// Выполняем установку функции обратного вызова для извлечения пароля
	this->_callbacks.set("extractPassword", callbacks);
	// Если функция обратного вызова для получения событий запуска и остановки сетевого ядра передана
	if(callbacks.is("status"))
		// Выполняем установку функции обратного вызова для получения событий запуска и остановки сетевого ядра
		this->_server.callback <void (const awh::core_t::status_t)> ("status", callbacks.get <void (const awh::core_t::status_t)> ("status"));
	// Если функция установки обратного вызова на событие получении ошибки передана
	if(this->_callbacks.is("error"))
		// Выполняем установку функции обратного вызова на событие получения ошибки
		this->_server.callback <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error", std::bind(this->_callbacks.get <void (const uint64_t, const broker_t, const log_t::flag_t, const http::error_t, const string &)> ("error"), _1, broker_t::SERVER, _2, _3, _4));
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
uint32_t awh::server::Proxy::port(const uint64_t bid) const noexcept {
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
	this->_core.clusterAutoRestart(mode);
}
/**
 * cluster Метод установки количества процессов кластера
 * @param mode флаг активации/деактивации кластера
 * @param size количество рабочих процессов
 */
void awh::server::Proxy::cluster(const awh::scheme_t::mode_t mode, const uint16_t size) noexcept {
	// Выполняем установку размера кластера
	this->_core.cluster(mode, size);
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Proxy::total(const uint16_t total) noexcept {
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
	// Если флаг запрета выполнения пингов установлен
	if(flags.find(flag_t::NOT_PING) != flags.end())
		// Устанавливаем флаг запрета выполнения пингов
		server.emplace(server::web_t::flag_t::NOT_PING);
	// Если флаг анбиндинга ядра сетевого модуля установлен
	if(flags.find(flag_t::NOT_STOP) == flags.end())
		// Устанавливаем флаг анбиндинга ядра сетевого модуля
		server.emplace(server::web_t::flag_t::NOT_STOP);
	// Если флаг поддержания автоматического подключения установлен
	if(flags.find(flag_t::ALIVE) != flags.end())
		// Устанавливаем флаг поддержания автоматического подключения
		server.emplace(server::web_t::flag_t::ALIVE);
	// Если флаг запрещающий вывод информационных сообщений установлен
	if(flags.find(flag_t::NOT_INFO) != flags.end())
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		server.emplace(server::web_t::flag_t::NOT_INFO);
	// Если флаг разрешающий метод CONNECT установлен
	if(flags.find(flag_t::CONNECT_METHOD_SERVER_ENABLE) != flags.end())
		// Выполняем установку флага сервера
		server.emplace(server::web_t::flag_t::CONNECT_METHOD_ENABLE);
	// Устанавливаем флаги настроек модуля
	this->_server.mode(server);
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
 * ssl Метод установки параметров SSL-шифрования
 * @param ssl объект параметров SSL-шифрования
 */
void awh::server::Proxy::ssl(const node_t::ssl_t & ssl) noexcept {
	// Выполняем установку параметров SSL-шифрования
	this->_core.ssl(ssl);
	// Запоминаем параметры SSL-шифрования
	this->_settings.ssl = ssl;
	// Если адрес файла сертификата и ключа передан
	if(!ssl.cert.empty() && !ssl.key.empty()){
		// Устанавливаем тип сокета TLS
		this->_core.sonet(awh::scheme_t::sonet_t::TLS);
		// Устанавливаем активный протокол подключения
		this->_core.proto(awh::engine_t::proto_t::HTTP2);
	}
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
 * @param sec время жизни подключения
 */
void awh::server::Proxy::alive(const time_t sec) noexcept {
	// Устанавливаем время жизни подключения
	this->_server.alive(sec);
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
 * memoryAvailableSize Метод получения максимального рамзера памяти для хранения полезной нагрузки всех брокеров
 * @return размер памяти для хранения полезной нагрузки всех брокеров
 */
size_t awh::server::Proxy::memoryAvailableSize() const noexcept {
	// Выводим размер памяти для хранения полезной нагрузки всех брокеров
	return this->_core.memoryAvailableSize();
}
/**
 * memoryAvailableSize Метод установки максимального рамзера памяти для хранения полезной нагрузки всех брокеров
 * @param size размер памяти для хранения полезной нагрузки всех брокеров
 */
void awh::server::Proxy::memoryAvailableSize(const size_t size) noexcept {
	// Запоминаем размер памяти для хранения полезной нагрузки всех брокеров
	this->_memoryAvailableSize = size;
	// Выполняем установку размера памяти для хранения полезной нагрузки всех брокеров
	this->_core.memoryAvailableSize(size);
}
/**
 * brokerAvailableSize Метод получения максимального размера хранимой полезной нагрузки для одного брокера
 * @return размер хранимой полезной нагрузки для одного брокера
 */
size_t awh::server::Proxy::brokerAvailableSize() const noexcept {
	// Выводим размер хранимой полезной нагрузки для одного брокера
	return this->_core.brokerAvailableSize();
}
/**
 * brokerAvailableSize Метод установки максимального размера хранимой полезной нагрузки для одного брокера
 * @param size размер хранимой полезной нагрузки для одного брокера
 */
void awh::server::Proxy::brokerAvailableSize(const size_t size) noexcept {
	// Запоминаем размер хранимой полезной нагрузки для одного брокера
	this->_brokerAvailableSize = size;
	// Выполняем установку размера хранимой полезной нагрузки для одного брокера
	this->_core.brokerAvailableSize(static_cast <size_t> (size));
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
 * bandwidth Метод установки пропускной способности сети
 * @param bid   идентификатор брокера
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::server::Proxy::bandwidth(const size_t bid, const string & read, const string & write) noexcept {
	// Выполняем установку пропускной способности сети
	this->_core.bandwidth(bid, read, write);
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
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param broker      брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param compressors список поддерживаемых компрессоров
 */
void awh::server::Proxy::compressors(const broker_t broker, const vector <http_t::compressor_t> & compressors) noexcept {
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
 * keepAlive Метод установки жизни подключения
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param cnt    максимальное количество попыток
 * @param idle   интервал времени в секундах через которое происходит проверка подключения
 * @param intvl  интервал времени в секундах между попытками
 */
void awh::server::Proxy::keepAlive(const broker_t broker, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
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
 * waitMessage Метод ожидания входящих сообщений
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param sec    интервал времени в секундах
 */
void awh::server::Proxy::waitMessage(const broker_t broker, const time_t sec) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT):
			// Выполняем установку времени ожидания входящих сообщений
			this->_settings.wtd.wait = sec;
		break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем установку времени ожидания входящих сообщений
			this->_server.waitMessage(sec);
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
			// Выполняем установку количества секунд для детекции по чтению
			this->_settings.wtd.read = read;
			// Выполняем установку количества секунд для детекции по записи
			this->_settings.wtd.write = write;
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
	auto i = this->_clients.find(bid);
	// Если указанный клиент найден
	if(i != this->_clients.end())
		// Выполняем сброс кэша DNS-резолвера
		return i->second->awh.flushDNS();
	// Выводим результат
	return false;
}
/**
 * timeoutDNS Метод установки времени ожидания выполнения запроса
 * @param sec интервал времени выполнения запроса в секундах
 */
void awh::server::Proxy::timeoutDNS(const uint8_t sec) noexcept {
	// Выполняем установку времени ожидания выполнения запроса
	this->_settings.dns.timeout = sec;
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
		for(auto i = this->_settings.dns.blacklist.begin(); i != this->_settings.dns.blacklist.end();){
			// Если IP-адрес соответствует указанному
			if(this->_fmk->compare(ip, i->second))
				// Выполняем удаление доменного имени
				i = this->_settings.dns.blacklist.erase(i);
			// Выполняем перебор списка доменов дальше
			else ++i;
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
 * cork Метод отключения/включения алгоритма TCP/CORK
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param bid    идентификатор брокера
 * @param mode   режим применимой операции
 * @return       результат выполенния операции
 */
bool awh::server::Proxy::cork(const broker_t broker, const uint64_t bid, const engine_t::mode_t mode) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT): {
			// Выполняем поиск объекта клиента
			auto i = this->_clients.find(bid);
			// Если активный клиент найден
			if(i != this->_clients.end())
				// Выполняем отключение/включение алгоритма TCP/CORK
				return i->second->core.cork(bid, mode);
		} break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем отключение/включение алгоритма TCP/CORK
			return this->_core.cork(bid, mode);
	}
	// Сообщаем, что ничего не установлено
	return false;
}
/**
 * nodelay Метод отключения/включения алгоритма Нейгла
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param bid    идентификатор брокера
 * @param mode   режим применимой операции
 * @return       результат выполенния операции
 */
bool awh::server::Proxy::nodelay(const broker_t broker, const uint64_t bid, const engine_t::mode_t mode) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT): {
			// Выполняем поиск объекта клиента
			auto i = this->_clients.find(bid);
			// Если активный клиент найден
			if(i != this->_clients.end())
				// Выполняем отключение/включение алгоритма Нейгла
				return i->second->core.nodelay(bid, mode);
		} break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем отключение/включение алгоритма Нейгла
			return this->_core.nodelay(bid, mode);
	}
	// Сообщаем, что ничего не установлено
	return false;
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
 * encrypt Метод активации шифрования для клиента
 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param mode   флаг активации шифрования
 */
void awh::server::Proxy::encrypt(const broker_t broker, const int32_t sid, const uint64_t bid, const bool mode) noexcept {
	// Определяем переданного брокера
	switch(static_cast <uint8_t> (broker)){
		// Если брокером является клиент
		case static_cast <uint8_t> (broker_t::CLIENT): {
			// Выполняем поиск объекта клиента
			auto i = this->_clients.find(bid);
			// Если активный клиент найден
			if(i != this->_clients.end())
				// Выполняем активацию шифрования для клиента
				i->second->awh.encryption(mode);
		} break;
		// Если брокером является сервер
		case static_cast <uint8_t> (broker_t::SERVER):
			// Выполняем активацию шифрования для сервера
			this->_server.encrypt(sid, bid, mode);
		break;
	}
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
 _uri(fmk, log), _callbacks(log), _core(fmk, log), _server(&_core, fmk, log),
 _memoryAvailableSize(AWH_WINDOW_SIZE), _brokerAvailableSize(AWH_PAYLOAD_SIZE),
 _compressor(http_t::compressor_t::NONE), _fmk(fmk), _log(log) {
	// Устанавливаем тип сокета TCP
	this->_core.sonet(awh::scheme_t::sonet_t::TCP);
	// Устанавливаем активный протокол подключения
	this->_core.proto(awh::engine_t::proto_t::HTTP1_1);
	// Выполняем установку идентичности протокола модуля
	this->_server.identity(awh::http_t::identity_t::PROXY);
	// Выполняем активацию ловушки событий контейнера функций обратного вызова
	this->_callbacks.callback(std::bind(&server::proxy_t::eventCallback, this, _1, _2, _3, _4));
	// Устанавливаем функцию обратного вызова на получение событий очистки буферов полезной нагрузки
	this->_core.callback <void (const uint64_t, const size_t)> ("available", std::bind(&server::proxy_t::available, this, broker_t::SERVER, _1, _2, &this->_core));
	// Устанавливаем функцию обратного вызова на получение событий очистки буферов полезной нагрузки
	this->_core.callback <void (const uint64_t, const char *, const size_t)> ("unavailable", std::bind(&server::proxy_t::unavailable, this, broker_t::SERVER, _1, _2, _3));
	// Устанавливаем функцию удаления клиента из стека подключений сервера
	this->_server.callback <void (const uint64_t)> ("erase", std::bind(&server::proxy_t::eraseClient, this, _1));
	// Установливаем функцию обратного вызова на событие запуска или остановки подключения
	this->_server.callback <void (const uint64_t, const server::web_t::mode_t)> ("active", std::bind(&server::proxy_t::activeServer, this, _1, _2));
	// Устанавливаем функцию извлечения пароля пользователя для прохождения авторизации на сервере
	this->_server.callback <string (const uint64_t, const string &)> ("extractPassword", std::bind(&server::proxy_t::passwordEvents, this, _1, _2));
	// Установливаем функцию обратного вызова на событие активации клиента на сервере
	this->_server.callback <bool (const string &, const string &, const uint32_t)> ("accept", std::bind(&server::proxy_t::acceptEvents, this, _1, _2, _3));
	// Устанавливаем функцию получения сырых данных полученных сервером с клиента
	this->_server.callback <bool (const uint64_t, const char *, const size_t)> ("raw", std::bind(&server::proxy_t::raw, this, _1, broker_t::SERVER, _2, _3));
	// Устанавливаем функцию проверки ввода логина и пароля пользователя
	this->_server.callback <bool (const uint64_t, const string &, const string &)> ("checkPassword", std::bind(&server::proxy_t::authEvents, this, _1, _2, _3));
	// Устанавливаем функцию обратного вызова при выполнении удачного рукопожатия
	this->_server.callback <void (const int32_t, const uint64_t, const server::web_t::agent_t)> ("handshake", std::bind(&server::proxy_t::handshake, this, _1, _2, _3));
	// Устанавливаем функцию обратного вызова при получении тела запроса с клиента
	this->_server.callback <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const vector <char> &)> ("entity", std::bind(&server::proxy_t::entityServer, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию обратного вызова при получении HTTP-заголовков запроса с клиента
	this->_server.callback <void (const int32_t, const uint64_t, const awh::web_t::method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> ("headers", std::bind(&server::proxy_t::headersServer, this, _1, _2, _3, _4, _5));
}
