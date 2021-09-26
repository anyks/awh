/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <core/cli.hpp>

/**
 * request Метод выполнения HTTP запроса
 */
void awh::Client::request() noexcept {
	// Очищаем объект запроса
	this->http->clear();
	// Если список заголовков получен
	if((this->req.headers != nullptr) && !this->req.headers->empty()){
		// Переходим по всему списку заголовков
		for(auto & header : * this->req.headers){
			// Устанавливаем заголовок
			this->http->addHeader(header.first, header.second);
		}
	}
	// Если тело запроса существует
	if((this->req.entity != nullptr) && !this->req.entity->empty())
		// Устанавливаем тело запроса
		this->http->addBody(this->req.entity->data(), this->req.entity->size());
	// Получаем бинарные данные REST запроса
	const auto & rest = this->http->request(* this->req.uri, this->req.method);
	// Если бинарные данные запроса получены
	if(!rest.empty()){
		// Тело REST сообщения
		vector <char> entity;
		// Активируем разрешение на запись и чтение
		bufferevent_enable(this->bev, EV_WRITE | EV_READ);
		// Отправляем серверу сообщение
		bufferevent_write(this->bev, rest.data(), rest.size());
		// Получаем данные тела запроса
		while(!(entity = this->http->chunkBody()).empty()){
			// Отправляем тело на сервер
			bufferevent_write(this->bev, entity.data(), entity.size());
		}
	}
	// Выполняем сброс всех отправленных данных
	this->http->clear();
}
/**
 * processing Метод обработки поступающих данных
 * @param size размер полученных данных
 */
void awh::Client::processing(const size_t size) noexcept {
	// Если данные существуют
	if(size > 0){
		// Выполняем парсинг полученных данных
		this->http->parse(this->wdt, size);
		// Если все данные получены
		if(this->http->isEnd()){
			// Получаем параметры запроса
			const auto & query = this->http->getQuery();
			// Устанавливаем код ответа
			this->res.code = query.code;
			// Устанавливаем сообщение ответа
			this->res.mess = query.message;
			// Получаем статус авторизации на сервере
			const auto stath = this->http->getAuth();
			// Выполняем проверку авторизации
			switch((u_short) stath){
				// Если нужно попытаться ещё раз
				case (u_short) http_t::stath_t::RETRY: {
					// Если попытка повторить авторизацию ещё не проводилась
					if(!this->failAuth){
						// Запоминаем, что попытка выполнена
						this->failAuth = true;
						// Выполняем запрос заново
						this->REST(this->http->getUrl(), this->req.method, * this->req.entity, * this->req.headers);
						// Завершаем работу
						return;
					}
				} break;
				// Если запрос выполнен удачно
				case (u_short) http_t::stath_t::GOOD: {
					// Запоминаем, что запрос выполнен удачно
					this->res.ok = true;
					// Получаем тело запроса
					const auto & entity = this->http->getBody();
					// Устанавливаем тело ответа
					this->res.entity.assign(entity.begin(), entity.end());
					// Устанавливаем заголовки ответа
					this->res.headers = this->http->getHeaders();
				} break;
			}
			// Выполняем сброс количество попыток
			this->failAuth = false;
			// Завершаем работу
			this->stop();
		}
	}
}
/**
 * GET Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const string awh::Client::GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";
	// Если URL адрес передан
	if(!url.empty()){
		/**
		 * Выполняем REST запрос в отдельном потоке, чтобы не мешать другим методам
		 */
		thread thr([&]{
			// Выполняем REST запрос
			this->REST(url, http_t::method_t::GET, {}, headers);
			// Проверяем на наличие ошибок
			if(!this->res.ok) this->log->print("request failed: %u %s", log_t::flag_t::WARNING, this->res.code, this->res.mess.c_str());
			// Если тело ответа получено
			if(!this->res.entity.empty()) result = this->res.entity;
		});
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
const string awh::Client::DEL(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";

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
const string awh::Client::PUT(const uri_t::url_t & url, const json & body, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";

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
const string awh::Client::PUT(const uri_t::url_t & url, const string & body, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";

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
const string awh::Client::PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";

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
const string awh::Client::POST(const uri_t::url_t & url, const json & body, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";

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
const string awh::Client::POST(const uri_t::url_t & url, const string & body, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";

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
const string awh::Client::POST(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";

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
const string awh::Client::PATCH(const uri_t::url_t & url, const json & body, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";

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
const string awh::Client::PATCH(const uri_t::url_t & url, const string & body, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";

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
const string awh::Client::PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	string result = "";

	// Выводим результат
	return result;
}
/**
 * HEAD Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const unordered_multimap <string, string> awh::Client::HEAD(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	unordered_multimap <string, string> result;

	// Выводим результат
	return result;
}
/**
 * TRACE Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const unordered_multimap <string, string> awh::Client::TRACE(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	unordered_multimap <string, string> result;

	// Выводим результат
	return result;
}
/**
 * OPTIONS Метод REST запроса
 * @param url     адрес запроса
 * @param headers список http заголовков
 * @return        результат запроса
 */
const unordered_multimap <string, string> awh::Client::OPTIONS(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	unordered_multimap <string, string> result;

	// Выводим результат
	return result;
}
/**
 * REST Метод выполнения REST запроса на сервер
 * @param url     адрес запроса
 * @param method  метод запроса
 * @param body    тело запроса
 * @param headers список заголовков
 */
void awh::Client::REST(const uri_t::url_t & url, const http_t::method_t method, const string & body, const unordered_multimap <string, string> & headers) noexcept {
	// Если URL адрес передан
	if(!url.empty() && (method != http_t::method_t::NONE)){
		// Устанавливаем параметры запроса
		this->req.uri = &url;
		// Устанавливаем тело запроса
		this->req.entity = &body;
		// Устанавливаем метод запроса
		this->req.method = method;
		// Устанавливаем заголовки запроса
		this->req.headers = &headers;
		// Если прокси-сервер не установлен
		if(this->proxy.type == proxy_t::type_t::NONE)
			// Устанавливаем адрес запроса
			this->url = url;
		// Запускаем работу
		this->start();
	}
}
