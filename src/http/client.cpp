/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <http/client.hpp>

/**
 * updateExtensions Метод проверки полученных расширений
 */
void awh::HClient::updateExtensions() noexcept {
	// Отключаем сжатие ответа с сервера
	this->gzip = false;
	// Список расширений
	vector <wstring> extensions;
	// Выполняем поиск расширений
	auto it = this->headers.find("sec-websocket-extensions");
	// Если заголовки расширений найдены
	if(it != this->headers.end()){
		// Выполняем разделение параметров расширений
		this->fmk->split(it->second, ";", extensions);
		// Если список параметров получен
		if(!extensions.empty()){
			// Ищем поддерживаемые заголовки
			for(auto & val : extensions){
				// Если получены заголовки требующие сжимать передаваемые фреймы
				if((val.compare(L"permessage-deflate") == 0) || (val.compare(L"perframe-deflate") == 0))
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					this->gzip = true;
				// Если размер окна в битах получено
				else if(val.find(L"client_max_window_bits") != wstring::npos)
					// Устанавливаем максимальный размер окна для сжатия в GZIP
					this->wbit = stoi(val.substr(23));
			}
		}
	}
}
/**
 * updateSubProtocol Метод извлечения доступного сабпротокола
 */
void awh::HClient::updateSubProtocol() noexcept {
	// Ищем подпротокол сервера
	auto it = this->headers.find("sec-websocket-protocol");
	// Если подпротокол найден, устанавливаем его
	if(it != this->headers.end()) this->sub = it->second;
}
/**
 * checkKey Метод проверки ключа сервера
 * @return результат проверки
 */
bool awh::HClient::checkKey() noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем параметры ключа сервера
	auto it = this->headers.find("sec-websocket-accept");
	// Если параметры авторизации найдены
	if(it != this->headers.end()){
		// Получаем ключ для проверки
		const string & key = this->generateHash();
		// Если ключи не соответствуют, запрещаем работу
		result = (key.compare(it->second) == 0);
	}
	// Выводим результат
	return result;
}
/**
 * checkVersion Метод проверки на версию протокола WebSocket
 * @return результат проверки соответствия
 */
bool awh::HClient::checkVersion() noexcept {
	// Сообщаем, что версия соответствует
	return true;
}
/**
 * checkAuthenticate Метод проверки авторизации
 * @return результат проверки авторизации
 */
awh::http_t::stath_t awh::HClient::checkAuthenticate() noexcept {
	// Результат работы функции
	http_t::stath_t result = http_t::stath_t::FAULT;
	// Проверяем код ответа
	switch(this->code){
		// Если требуется авторизация
		case 401: {
			// Если попытки провести аутентификацию ещё небыло, пробуем ещё раз
			if(!this->checkAuth && (this->auth->getType() == auth_t::type_t::DIGEST)){
				// Получаем параметры авторизации
				auto it = this->headers.find("www-authenticate");
				// Если параметры авторизации найдены
				if((this->checkAuth = (it != this->headers.end()))){
					// Устанавливаем заголовок HTTP в параметры авторизации
					this->auth->setHeader(it->second);
					// Просим повторить авторизацию ещё раз
					result = http_t::stath_t::RETRY;
				}
			}
		} break;
		// Если нужно произвести редирект
		case 301:
		case 308: {
			// Получаем параметры авторизации
			auto it = this->headers.find("location");
			// Если адрес перенаправления найден
			if(it != this->headers.end()){
				// Выполняем парсинг URL
				uri_t::url_t tmp = this->uri->parseUrl(it->second);
				// Если параметры URL существуют
				if(!this->url->params.empty())
					// Переходим по всему списку параметров
					for(auto & param : this->url->params) tmp.params.emplace(param);
				// Меняем IP адрес сервера
				const_cast <uri_t::url_t *> (this->url)->ip = move(tmp.ip);
				// Меняем порт сервера
				const_cast <uri_t::url_t *> (this->url)->port = move(tmp.port);
				// Меняем на путь сервере
				const_cast <uri_t::url_t *> (this->url)->path = move(tmp.path);
				// Меняем доменное имя сервера
				const_cast <uri_t::url_t *> (this->url)->domain = move(tmp.domain);
				// Меняем протокол запроса сервера
				const_cast <uri_t::url_t *> (this->url)->schema = move(tmp.schema);
				// Устанавливаем новый список параметров
				const_cast <uri_t::url_t *> (this->url)->params = move(tmp.params);
				// Просим повторить авторизацию ещё раз
				result = http_t::stath_t::RETRY;
			}
		} break;
		// Сообщаем, что авторизация прошла успешно
		case 101: result = http_t::stath_t::GOOD; break;
	}
	// Выводим результат
	return result;
}
