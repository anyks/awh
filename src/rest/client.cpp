/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <rest/client.hpp>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * winSocketInit Метод инициализации WinSock
	 */
	void awh::Rest::winSocketInit() const noexcept {
		// Если winSock ещё не инициализирован
		if(!this->winSock){
			// Идентификатор ошибки
			int error = 0;
			// Объект данных запроса
			WSADATA wsaData;
			// Выполняем инициализацию сетевого контекста
			if((error = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0){ // 0x202
				// Сообщаем, что сетевой контекст не поднят
				this->log->print("WSAStartup failed with error: %d", log_t::flag_t::CRITICAL, error);
				// Выходим из приложения
				exit(EXIT_FAILURE);
			}
			// Выполняем проверку версии WinSocket
			if((2 != LOBYTE(wsaData.wVersion)) || (2 != HIBYTE(wsaData.wVersion))){
				// Сообщаем, что версия WinSocket не подходит
				this->log->print("%s", log_t::flag_t::CRITICAL, "WSADATA version is not correct");
				// Очищаем сетевой контекст
				this->winSocketClean();
				// Выходим из приложения
				exit(EXIT_FAILURE);
			}
			// Запоминаем, что winSock уже инициализирован
			this->winSock = true;
		}
	}
	/**
	 * winSocketClean Метод очистки WinSock
	 */
	void awh::Rest::winSocketClean() const noexcept {
		// Очищаем сетевой контекст
		WSACleanup();
		// Запоминаем, что winSock не инициализирован
		this->winSock = false;
	}
#endif
/**
 * GET Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::GET(const uri_t::url_t & url, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers){
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_GET, headers);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
		}, ref(url), ref(headers));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * DEL Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::DEL(const uri_t::url_t & url, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers){
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_DELETE, headers);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
		}, ref(url), ref(headers));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PUT Метод REST запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PUT(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty() && !body.empty()){
		// Добавляем заголовок типа контента
		const_cast <unordered_map <string, string> *> (&headers)->emplace("Content-Type", "application/json");
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const json & body){
			// Результирующая строка
			const string bodyData = body.dump();
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PUT, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PUT Метод REST запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PUT(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const string & body){
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PUT, headers, body);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PUT Метод REST запроса
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty() && !body.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const unordered_multimap <string, string> & body){
			// Результирующая строка
			string bodyData = "";
			// Переходим по всему списку тела запроса
			for(auto & param: body){
				// Есди данные уже набраны
				if(!bodyData.empty()) bodyData.append("&");
				// Добавляем в список набор параметров
				bodyData.append(this->uri->urlEncode(param.first));
				// Добавляем разделитель
				bodyData.append("=");
				// Добавляем значение
				bodyData.append(this->uri->urlEncode(param.second));
			}
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PUT, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * POST Метод HTTP запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::POST(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty() && !body.empty()){
		// Добавляем заголовок типа контента
		const_cast <unordered_map <string, string> *> (&headers)->emplace("Content-Type", "application/json");
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const json & body){
			// Результирующая строка
			const string bodyData = body.dump();
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_POST, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * POST Метод REST запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::POST(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const string & body){
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_POST, headers, body);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * POST Метод REST запроса
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::POST(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty() && !body.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const unordered_multimap <string, string> & body){
			// Результирующая строка
			string bodyData = "";
			// Переходим по всему списку тела запроса
			for(auto & param: body){
				// Есди данные уже набраны
				if(!bodyData.empty()) bodyData.append("&");
				// Добавляем в список набор параметров
				bodyData.append(this->uri->urlEncode(param.first));
				// Добавляем разделитель
				bodyData.append("=");
				// Добавляем значение
				bodyData.append(this->uri->urlEncode(param.second));
			}
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_POST, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PATCH Метод REST запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PATCH(const uri_t::url_t & url, const json & body, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		// Добавляем заголовок типа контента
		const_cast <unordered_map <string, string> *> (&headers)->emplace("Content-Type", "application/json");
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const json & body){
			// Результирующая строка
			const string bodyData = body.dump();
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PATCH, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PATCH Метод REST запроса в формате JSON
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PATCH(const uri_t::url_t & url, const string & body, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const string & body){
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PATCH, headers, body);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * PATCH Метод REST запроса
 * @param url     адрес запроса
 * @param body    тело запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Rest::PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty() && !body.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers, const unordered_multimap <string, string> & body){
			// Результирующая строка
			string bodyData = "";
			// Переходим по всему списку тела запроса
			for(auto & param: body){
				// Есди данные уже набраны
				if(!bodyData.empty()) bodyData.append("&");
				// Добавляем в список набор параметров
				bodyData.append(this->uri->urlEncode(param.first));
				// Добавляем разделитель
				bodyData.append("=");
				// Добавляем значение
				bodyData.append(this->uri->urlEncode(param.second));
			}
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_PATCH, headers, bodyData);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.body.empty()) result = response.body;
		}, ref(url), ref(headers), ref(body));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * HEAD Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const unordered_map <string, string> awh::Rest::HEAD(const uri_t::url_t & url, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	unordered_map <string, string> result;
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers){
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_HEAD, headers);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.headers.empty()) result = response.headers;
		}, ref(url), ref(headers));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * TRACE Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const unordered_map <string, string> awh::Rest::TRACE(const uri_t::url_t & url, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	unordered_map <string, string> result;
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers){
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_TRACE, headers);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.headers.empty()) result = response.headers;
		}, ref(url), ref(headers));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * OPTIONS Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const unordered_map <string, string> awh::Rest::OPTIONS(const uri_t::url_t & url, const unordered_map <string, string> & headers) const noexcept {
	// Результат работы функции
	unordered_map <string, string> result;
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&result, this](const uri_t::url_t & url, const unordered_map <string, string> & headers){
			// Выполняем REST запрос
			const auto & response = this->REST(url, EVHTTP_REQ_OPTIONS, headers);
			// Проверяем на наличие ошибок
			if(!response.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, response.code, response.mess.c_str());
			// Если тело ответа получено
			if(!response.headers.empty()) result = response.headers;
		}, ref(url), ref(headers));
		// Ожидаем завершение запроса
		thr.join();
	}
	// Выводим результат
	return result;
}
/**
 * REST Метод выполнения REST запроса на сервер
 * @param url параметры адреса запроса
 * @param type тип REST запроса
 * @param headers список заголовков для REST запроса
 * @param body    телоо REST запроса
 * @return        результат REST запроса
 */
const awh::Rest::res_t awh::Rest::REST(const uri_t::url_t & url, evhttp_cmd_type type, const unordered_map <string, string> & headers, const string & body) const noexcept {
	// Результат работы функции
	res_t result;
	// Если URL адрес получен
	if(!url.empty()){
		try {
			// Запоминаем родительский объект
			result.ctx = this;
			// Буфер событий SSL
			ssl_t::ctx_t sslctx;
			// Создаём базу событий
			struct event_base * base = event_base_new();
			// Если база событий создана
			if(base != nullptr){
				// HTTP запрос на сервере
				string uri = "";
				// Сокет подключения
				evutil_socket_t fd = -1;
				// База событий DNS
				struct evdns_base * dnsbase = nullptr;
				// Создаём DNS резолвер
				dns_t resolver(this->fmk, this->log, this->nwk);
				// Определяем тип сети
				switch(url.family){
					// Если - это IPv4
					case AF_INET: {
						// Добавляем список серверов в резолвер
						resolver.setNameServers(this->net.v4.second);
						// Создаём базу событий DNS
						dnsbase = resolver.init(url.domain, AF_INET, base);
						// Создаем сокет подключения
						fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					} break;
					// Если - это IPv6
					case AF_INET6: {
						// Добавляем список серверов в резолвер
						resolver.setNameServers(this->net.v6.second);
						// Создаём базу событий DNS
						dnsbase = resolver.init(url.domain, AF_INET6, base);
						// Создаем сокет подключения
						fd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
					} break;
				}
				// Если файловый дескриптор создан
				if(fd > -1){
					// Если - это Unix
					#if !defined(_WIN32) && !defined(_WIN64)
						// Выполняем игнорирование сигнала неверной инструкции процессора
						sockets_t::noSigill(this->log);
						// Устанавливаем разрешение на повторное использование сокета
						sockets_t::reuseable(fd, this->log);
						// Отключаем сигнал записи в оборванное подключение
						sockets_t::noSigpipe(fd, this->log);
						// Отключаем алгоритм Нейгла для сервера и клиента
						sockets_t::tcpNodelay(fd, this->log);
						// Разблокируем сокет
						sockets_t::nonBlocking(fd, this->log);
						// Активируем keepalive
						sockets_t::keepAlive(fd, this->alive.keepcnt, this->alive.keepidle, this->alive.keepintvl, this->log);
					// Если - это Windows
					#else
						// Выполняем инициализацию WinSock
						this->winSocketInit();
						// Переводим сокет в блокирующий режим
						// sockets_t::blocking(fd);
						evutil_make_socket_nonblocking(fd);
						// evutil_make_socket_closeonexec(fd);
						evutil_make_listen_socket_reuseable(fd);
					#endif
				}
				// Выполняем получение контекста сертификата
				sslctx = this->ssl->init(url);
				// Если защищённое соединение не нужно, создаём буфер событий
				if(!sslctx.mode) result.bev = bufferevent_socket_new(base, fd, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
				// Если требуется защищённое соединение
				else {
					// Создаём буфер событий в защищённом подключении
					result.bev = bufferevent_openssl_socket_new(base, fd, sslctx.ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS);
					// Разрешаем грязное отключение
					bufferevent_openssl_set_allow_dirty_shutdown(result.bev, 1);
				}
				// Если буфер событий не создан
				if(result.bev == nullptr){
					// Сообщаем, что буфер событий не может быть создан
					this->log->print("%s", log_t::flag_t::CRITICAL, "the event buffer could not be created");
					// Завершаем работу функции
					return result;
				}
				// Создаём событие подключения
				struct evhttp_connection * evcon = evhttp_connection_base_bufferevent_new(base, dnsbase, result.bev, (!url.ip.empty() ? url.ip.c_str() : url.domain.c_str()), url.port);
				// Если событие подключения не создан
				if(evcon == nullptr){
					// Сообщаем, что событие подключения не создано
					this->log->print("%s", log_t::flag_t::CRITICAL, "connection event not created");
					// Завершаем работу функции
					return result;
				}
				// Заставляем выполнять подключение по указанному протоколу сети
				evhttp_connection_set_family(evcon, url.family);
				// Выполняем 5 попыток запросить данные
				evhttp_connection_set_retries(evcon, 5);
				// Таймаут на выполнение в 5 секунд
				evhttp_connection_set_timeout(evcon, 5);
				/**
				 * callbackFn Функция вывода результата получения данных
				 * @param req объект REST запроса
				 * @param ctx контекст родительского объекта
				 */
				auto callbackFn = [](struct evhttp_request * req, void * ctx){
					// Если контекст объекта ответа сервера получен
					if(ctx != nullptr){
						// Создаём объект ответа сервера
						res_t * res = reinterpret_cast <res_t *> (ctx);
						/**
						 * Выполняем обработку ошибок
						 */
						try {
							// Буфер для получения данных
							char buffer[256];
							// Зануляем буфер данных
							memset(buffer, 0, sizeof(buffer));
							// Если параметры получены
							if((req != nullptr) && evhttp_request_get_response_code(req)){
								// Количество полученных данных
								size_t size = 0;
								// Получаем объект заголовков
								struct evkeyvalq * headers = evhttp_request_get_input_headers(req);
								// Получаем первый заголовок
								struct evkeyval * header = headers->tqh_first;
								// Перебираем все полученные заголовки
								while(header){
									// Собираем все доступные заголовки
									res->headers.emplace(res->ctx->fmk->toLower(header->key), header->value);
									// Переходим к следующему заголовку
									header = header->next.tqe_next;
								}
								// Получаем код ответа сервера
								res->code = evhttp_request_get_response_code(req);
								// Получаем текст ответа сервера
								res->mess = evhttp_request_get_response_code_line(req);
								// Считываем в буфер тело ответа сервера
								while((size = evbuffer_remove(evhttp_request_get_input_buffer(req), buffer, sizeof(buffer))) > 0){
									/**
									 * Получаем произвольные фрагменты по 256 байт.
									 * Это не строки, поэтому мы не можем получать их построчно.
									 */
									res->body.append(buffer, size);
								}
							// Если объект REST запроса не получен
							} else {
								// Ошибка на сервере
								size_t error = 0;
								// Флаг получения информации об ошибке
								bool mode = false;
								/**
								 * Если ответ не получен, это означает, что произошла ошибка,
								 * но, к сожалению, нам остается только догадываться, в чем могла быть ошибка.
								 */
								const int code = EVUTIL_SOCKET_ERROR();
								// Сообщаем, что на сервере произошла ошибкаа
								res->ctx->log->print("%s", log_t::flag_t::CRITICAL, "some request failed - no idea which one though!");
								/**
								 * Выводим очередь ошибок OpenSSL,
								 * которую libevent получил для нас, если такие имеются.
								 */
								while((error = bufferevent_get_openssl_error(res->bev))){
									// Запоминаем, что описание ошибки получено
									mode = true;
									// Зануляем буфер данных
									memset(buffer, 0, sizeof(buffer));
									// Получаем описание ошибки
									ERR_error_string_n(error, buffer, sizeof(buffer));
									// Выводим ошибку
									res->ctx->log->print("%s", log_t::flag_t::CRITICAL, buffer);
								}
								/**
								 * Если очередь ошибок OpenSSL пустая, возможно,
								 * это была ошибка сокета, попробуем получить описание.
								 */
								if(!mode) res->ctx->log->print("socket error = %s (%d)", log_t::flag_t::CRITICAL, evutil_socket_error_to_string(code), code);
							}
							// Разблокируем базу событий
							event_base_loopbreak(bufferevent_get_base(res->bev));
						// Если происходит ошибка то игнорируем её
						} catch(exception & error) {
							// Выводим сообщение об ошибке
							res->ctx->log->print("%s", log_t::flag_t::CRITICAL, error.what());
						}
					}
				};
				// Создаём объект выполнения REST запроса
				struct evhttp_request * req = evhttp_request_new(callbackFn, &result);
				// Если объект REST запроса не создан
				if(req == nullptr){
					// Сообщаем, что событие REST запроса не создано
					this->log->print("%s", log_t::flag_t::CRITICAL, "REST request event not created");
					// Завершаем работу функции
					return result;
				}
				// Список существующих заголовков
				set <header_t> availableHeaders;
				// Получаем объект заголовков
				struct evkeyvalq * store = evhttp_request_get_output_headers(req);
				// Устанавливаем параметры REST запроса
				const_cast <auth_t *> (this->auth)->setUri(this->uri->createUrl(url));
				// Переходим по всему списку заголовков
				for(auto & header : headers){
					// Получаем анализируемый заголовок
					const string & head = this->fmk->toLower(header.first);
					// Если заголовок Host передан, запоминаем , что мы его нашли
					if(head.compare("host") == 0) availableHeaders.emplace(header_t::HOST);
					// Если заголовок Accept передан, запоминаем , что мы его нашли
					if(head.compare("accept") == 0) availableHeaders.emplace(header_t::ACCEPT);
					// Если заголовок Origin перадан, запоминаем, что мы его нашли
					else if(head.compare("origin") == 0) availableHeaders.emplace(header_t::ORIGIN);
					// Если заголовок User-Agent передан, запоминаем, что мы его нашли
					else if(head.compare("user-agent") == 0) availableHeaders.emplace(header_t::USERAGENT);
					// Если заголовок Connection перадан, запоминаем, что мы его нашли
					else if(head.compare("connection") == 0) availableHeaders.emplace(header_t::CONNECTION);
					// Если заголовок Accept-Language передан, запоминаем, что мы его нашли
					else if(head.compare("accept-language") == 0) availableHeaders.emplace(header_t::ACCEPTLANGUAGE);
					// Добавляем заголовок в запрос
					evhttp_add_header(store, header.first.c_str(), header.second.c_str());
				}
				// Устанавливаем Host если не передан
				if(availableHeaders.count(header_t::HOST) < 1)
					// Устанавливаем заголовок запроса
					evhttp_add_header(store, "Host", url.domain.c_str());
				// Устанавливаем Origin если не передан
				if(availableHeaders.count(header_t::ORIGIN) < 1)
					// Устанавливаем заголовок запроса
					evhttp_add_header(store, "Origin", this->uri->createOrigin(url).c_str());
				// Устанавливаем User-Agent если не передан
				if(availableHeaders.count(header_t::USERAGENT) < 1)
					// Устанавливаем заголовок запроса
					evhttp_add_header(store, "User-Agent", this->userAgent.c_str());
				// Устанавливаем Connection если не передан
				if(availableHeaders.count(header_t::CONNECTION) < 1)
					// Устанавливаем заголовок запроса
					evhttp_add_header(store, "Connection", "keep-alive");
				// Устанавливаем Accept-Language если не передан
				if(availableHeaders.count(header_t::ACCEPTLANGUAGE) < 1)
					// Устанавливаем заголовок запроса
					evhttp_add_header(store, "Accept-Language", "*");
				// Устанавливаем Accept если не передан
				if(availableHeaders.count(header_t::ACCEPT) < 1)
					// Устанавливаем заголовок запроса
					evhttp_add_header(store, "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
				// Если нужно произвести сжатие контента
				if(this->zip != zip_t::NONE)
					// Устанавливаем заголовок запроса
					evhttp_add_header(store, "Accept-Encoding", "gzip, deflate");
				// Получаем заголовок авторизации
				const string & authHeader = this->auth->header();
				// Если данные авторизации получены
				if(!authHeader.empty()){
					// Получаем значение заголовка
					const_cast <string *> (&authHeader)->assign(authHeader.begin() + 15, authHeader.end() - 2);
					// Устанавливаем авторизационные параметры
					evhttp_add_header(store, "Authorization", authHeader.c_str());
				}
				// Если тело запроса существует
				if(!body.empty()){
					// Получаем буфер тела запроса
					struct evbuffer * buffer = evhttp_request_get_output_buffer(req);
					// Если нужно производить шифрование
					if(this->crypt){
						// Выполняем шифрование переданных данных
						const auto & res = this->hash->encrypt(body.data(), body.size());
						// Если данные зашифрованы, заменяем тело данных
						if(!res.empty()){
							// Заменяем тело запроса на зашифрованное
							const_cast <string *> (&body)->assign(res.begin(), res.end());
							// Устанавливаем заголовок шифрования
							evhttp_add_header(store, "X-AWH-Encryption", to_string((u_int) this->hash->getAES()).c_str());
						}
					}
					// Определяем метод сжатия тела сообщения
					switch((u_short) this->zip){
						// Если сжимать тело не нужно
						case (u_short) zip_t::NONE:
							// Добавляем в буфер, тело запроса
							evbuffer_add(buffer, body.data(), body.size());
							// Если передавать тело запроса чанками не нужно, устанавливаем размер тела
							if(!this->chunked) evhttp_add_header(store, "Content-Length", to_string(body.size()).c_str());
						break;
						// Если нужно сжать тело методом GZIP
						case (u_short) zip_t::GZIP: {
							// Выполняем сжатие тела сообщения
							const auto & gzip = this->hash->compressGzip(body.data(), body.size());
							// Добавляем в буфер, тело запроса
							evbuffer_add(buffer, gzip.data(), gzip.size());
							// Указываем метод, которым было выполненно сжатие тела
							evhttp_add_header(store, "Content-Encoding", "gzip");
							// Если передавать тело запроса чанками не нужно, устанавливаем размер тела
							if(!this->chunked) evhttp_add_header(store, "Content-Length", to_string(gzip.size()).c_str());
						} break;
						// Если нужно сжать тело методом DEFLATE
						case (u_short) zip_t::DEFLATE: {
							// Выполняем сжатие тела сообщения
							auto deflate = this->hash->compress(body.data(), body.size());
							// Удаляем хвост в полученных данных
							this->hash->rmTail(deflate);
							// Добавляем в буфер, тело запроса
							evbuffer_add(buffer, deflate.data(), deflate.size());
							// Указываем метод, которым было выполненно сжатие тела
							evhttp_add_header(store, "Content-Encoding", "deflate");
							// Если передавать тело запроса чанками не нужно, устанавливаем размер тела
							if(!this->chunked) evhttp_add_header(store, "Content-Length", to_string(deflate.size()).c_str());
						} break;
					}
					// Если нужно передавать тело в виде чанков
					if(this->chunked) evhttp_add_header(store, "Transfer-Encoding", "chunked");
				}
				// Выполняем REST запрос на сервер
				const int request = evhttp_make_request(evcon, req, type, this->uri->createUrl(url).c_str());
				// Если запрос не выполнен
				if(request != 0){
					// Сообщаем, что запрос не выполнен
					this->log->print("%s", log_t::flag_t::CRITICAL, "REST request failed");
					// Завершаем работу функции
					return result;
				}
				// Блокируем базу событий
				event_base_dispatch(base);
				// Удаляем событие подключения
				evhttp_connection_free(evcon);
				// Если событие сервера существует
				if(result.bev != nullptr){
					// Если - это Windows
					#if defined(_WIN32) || defined(_WIN64)
						// Отключаем подключение для сокета
						if(fd > 0) shutdown(fd, SD_BOTH);
					// Если - это Unix
					#else
						// Отключаем подключение для сокета
						if(fd > 0) shutdown(fd, SHUT_RDWR);
					#endif
					// Запрещаем чтение запись данных серверу
					bufferevent_disable(result.bev, EV_WRITE | EV_READ);
					// Закрываем подключение
					if(fd > 0) evutil_closesocket(fd);
					// Удаляем сокет буфера событий
					// bufferevent_setfd(result.bev, -1);
					// Удаляем буфер события
					bufferevent_free(result.bev);
				}
				// Удаляем базу dns
				evdns_base_free(dnsbase, 0);
			// Выводим сообщение в лог
			} else this->log->print("%s", log_t::flag_t::CRITICAL, "the event base could not be created");
			// Удаляем базу событий
			event_base_free(base);
			// Выполняем удаление контекста SSL
			this->ssl->clear(sslctx);
			// Если код ответа не требует авторизации, разрешаем дальнейшие попытки
			if(result.code != 401) this->checkAuth = false;
			// Определяем, был ли ответ сервера удачным
			result.ok = ((result.code >= 200) && (result.code <= 206) || (result.code == 100));
			// Если запрос выполнен успешно
			if(result.ok){
				// Проверяем пришли ли сжатые данные
				auto it = result.headers.find("content-encoding");
				// Если данные пришли зашифрованные
				if(it != result.headers.end()){
					// Если данные пришли сжатые методом GZIP
					if(it->second.compare("gzip") == 0){
						// Выполняем декомпрессию данных
						const auto & body = this->hash->decompressGzip(result.body.data(), result.body.size());
						// Заменяем полученное тело
						result.body.assign(body.begin(), body.end());
					// Если данные пришли сжатые методом Deflate
					} else if(it->second.compare("deflate") == 0) {
						// Получаем данные тела в бинарном виде
						vector <char> buffer(result.body.begin(), result.body.end());
						// Добавляем хвост в полученные данные
						this->hash->setTail(buffer);
						// Выполняем декомпрессию данных
						const auto & body = this->hash->decompress(buffer.data(), buffer.size());
						// Заменяем полученное тело
						result.body.assign(body.begin(), body.end());
					}
				}
				// Выполняем поиск заголовка шифрования
				it = result.headers.find("x-awh-encryption");
				// Если данные пришли зашифрованные
				if(it != result.headers.end()){
					// Определяем размер шифрования
					switch(stoi(it->second)){
						// Если шифрование произведено 128 битным ключём
						case 128: this->hash->setAES(hash_t::aes_t::AES128); break;
						// Если шифрование произведено 192 битным ключём
						case 192: this->hash->setAES(hash_t::aes_t::AES192); break;
						// Если шифрование произведено 256 битным ключём
						case 256: this->hash->setAES(hash_t::aes_t::AES256); break;
					}
					// Выполняем дешифрование полученных данных
					const auto & res = this->hash->decrypt(result.body.data(), result.body.size());
					// Если данные расшифрованны, заменяем тело данных
					if(!res.empty()) result.body.assign(res.begin(), res.end());
				}
			// Если запрос не был выполнен успешно
			} else {
				// Определяем код ответа
				switch(result.code){
					// Если требуется авторизация
					case 401: {
						// Если попытки провести аутентификацию ещё небыло, пробуем ещё раз
						if(!this->checkAuth && (this->auth->getType() == auth_t::type_t::DIGEST)){
							// Получаем параметры авторизации
							auto it = result.headers.find("www-authenticate");
							// Если параметры авторизации найдены
							if((this->checkAuth = (it != result.headers.end()))){
								// Устанавливаем заголовок HTTP в параметры авторизации
								this->auth->setHeader(it->second);
								// Просим повторить авторизацию ещё раз
								return this->REST(url, type, headers, body);
							}
						}
					} break;
					// Если требуется провести перенаправление
					case 301:
					case 308: {
						// Получаем адрес перенаправления
						auto it = result.headers.find("location");
						// Если данные пришли зашифрованные
						if(it != result.headers.end()){
							// Выполняем парсинг URL
							uri_t::url_t tmp = this->uri->parseUrl(it->second);
							// Если параметры URL существуют
							if(!url.params.empty())
								// Переходим по всему списку параметров
								for(auto & param : url.params) tmp.params.emplace(param);
							// Устанавливаем пользователя запроса
							tmp.user = url.user;
							// Устанавливаем пароль запроса
							tmp.pass = url.pass;
							// Устанавливаем функцию генерации цифровой подписи запроса
							tmp.sign = url.sign;
							// Устанавливаем якорь запроса
							tmp.anchor = url.anchor;
							// Выполняем новый запрос
							return this->REST(tmp, type, headers, body);
						}
					} break;
				}
			}
		// Если происходит ошибка то игнорируем её
		} catch(exception & error) {
			// Выводим сообщение об ошибке
			this->log->print("%s", log_t::flag_t::CRITICAL, error.what());
		}
	}
	// Выводим результат
	return result;
}
/**
 * setZip Метод активации работы с сжатым контентом
 * @param method метод установки формата сжатия
 */
void awh::Rest::setZip(const zip_t method) noexcept {
	// Устанавливаем флаг сжатого контента
	this->zip = method;
}
/**
 * setChunked Метод активации режима передачи тела запроса чанками
 * @param mode флаг активации режима передачи тела запроса чанками
 */
void awh::Rest::setChunked(const bool mode) noexcept {
	// Устанавливаем флаг активации режима передачи тела запроса чанками
	this->chunked = mode;
}
/**
 * setVerifySSL Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 * @param mode флаг состояния разрешения проверки
 */
void awh::Rest::setVerifySSL(const bool mode) noexcept {
	// Выполняем установку флага проверки домена
	this->ssl->setVerify(mode);
}
/**
 * setFamily Метод установки тип протокола интернета
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Rest::setFamily(const int family) noexcept {
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
}
/**
 * setUserAgent Метод установки User-Agent для HTTP запроса
 * @param userAgent агент пользователя для HTTP запроса
 */
void awh::Rest::setUserAgent(const string & userAgent) noexcept {
	// Устанавливаем UserAgent
	if(!userAgent.empty()) this->userAgent = userAgent;
}
/**
 * setUser Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::Rest::setUser(const string & login, const string & password) noexcept {
	// Если пользователь и пароль переданы
	if(!login.empty() && !password.empty()){
		// Устанавливаем логин пользователя
		this->auth->setLogin(login);
		// Устанавливаем пароль пользователя
		this->auth->setPassword(password);
	}
}
/**
 * setCA Метод установки CA-файла корневого SSL сертификата
 * @param cafile адрес CA-файла
 * @param capath адрес каталога где находится CA-файл
 */
void awh::Rest::setCA(const string & cafile, const string & capath) noexcept {
	// Устанавливаем адрес CA-файла
	this->ssl->setCA(cafile, capath);
}
/**
 * setNet Метод установки параметров сети
 * @param ip     список IP адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета AF_INET или AF_INET6
 */
void awh::Rest::setNet(const vector <string> & ip, const vector <string> & ns, const int family) noexcept {
	// Устанавливаем тип активного интернет-подключения
	this->net.family = family;
	// Определяем тип интернет-протокола
	switch(this->net.family){
		// Если - это интернет-протокол IPv4
		case AF_INET: {
			// Если IP адреса переданы, устанавливаем их
			if(!ip.empty()) this->net.v4.first.assign(ip.cbegin(), ip.cend());
			// Если сервера имён переданы, устанавливаем их
			if(!ns.empty()) this->net.v4.second.assign(ns.cbegin(), ns.cend());
		} break;
		// Если - это интернет-протокол IPv6
		case AF_INET6: {
			// Если IP адреса переданы, устанавливаем их
			if(!ip.empty()) this->net.v6.first.assign(ip.cbegin(), ip.cend());
			// Если сервера имён переданы, устанавливаем их
			if(!ns.empty()) this->net.v6.second.assign(ns.cbegin(), ns.cend());
		} break;
	}
}
/**
 * setCrypt Метод установки параметров шифрования
 * @param pass пароль шифрования передаваемых данных
 * @param salt соль шифрования передаваемых данных
 * @param aes  размер шифрования передаваемых данных
 */
void awh::Rest::setCrypt(const string & pass, const string & salt, const hash_t::aes_t aes) noexcept {
	// Устанавливаем флаг шифрования
	this->crypt = !pass.empty();
	// Устанавливаем размер шифрования
	this->hash->setAES(aes);
	// Устанавливаем соль шифрования
	this->hash->setSalt(salt);
	// Устанавливаем пароль шифрования
	this->hash->setPassword(pass);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::Rest::setAuthType(const auth_t::type_t type, const auth_t::algorithm_t algorithm) noexcept {
	// Если объект авторизации создан
	if(this->auth != nullptr) this->auth->setType(type, algorithm);
}
/**
 * Rest Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 * @param nwk объект методов для работы с сетью
 */
awh::Rest::Rest(const fmk_t * fmk, const log_t * log, const uri_t * uri, const network_t * nwk) noexcept {
	try {
		// Устанавливаем зависимые модули
		this->fmk = fmk;
		this->log = log;
		this->uri = uri;
		this->nwk = nwk;
		// Создаём объект для работы с компрессией/декомпрессией
		this->hash = new hash_t(this->fmk, this->log);
		// Создаём объект для работы с авторизацией
		this->auth = new auth_t(this->fmk, this->log);
		// Создаём объект для работы с SSL
		this->ssl = new ssl_t(this->fmk, this->log, this->uri);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~Rest Деструктор
 */
awh::Rest::~Rest() noexcept {
	// Если объект для работы с SSL создан
	if(this->ssl != nullptr) delete this->ssl;
	// Удаляем объект работы с авторизацией
	if(this->auth != nullptr) delete this->auth;
	// Если объект для компрессии/декомпрессии создан
	if(this->hash != nullptr) delete this->hash;
	// Если - это Windows
	#if defined(_WIN32) || defined(_WIN64)
		// Очищаем сетевой контекст
		this->winSocketClean();
	#endif
}
