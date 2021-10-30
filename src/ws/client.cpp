/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <ws/client.hpp>

/**
 * update Метод обновления входящих данных
 */
void awh::WSClient::update() noexcept {
	// Сбрасываем флаг шифрования
	this->crypt = false;
	// Отключаем сжатие ответа с сервера
	this->compress = compress_t::NONE;
	// Список доступных расширений
	vector <wstring> extensions;
	// Получаем значение заголовка Sec-Websocket-Extensions
	const string & ext = this->web.getHeader("sec-websocket-extensions");
	// Если заголовок найден
	if(!ext.empty()){
		// Выполняем разделение параметров расширений
		this->fmk->split(ext, ";", extensions);
		// Если список параметров получен
		if(!extensions.empty()){
			// Ищем поддерживаемые заголовки
			for(auto & val : extensions){
				// Если нужно производить шифрование данных
				if(val.find(L"permessage-encrypt=") != wstring::npos){
					// Устанавливаем флаг шифрования данных
					this->crypt = true;
					// Определяем размер шифрования
					switch(stoi(val.substr(19))){
						// Если шифрование произведено 128 битным ключём
						case 128: this->hash.setAES(hash_t::aes_t::AES128); break;
						// Если шифрование произведено 192 битным ключём
						case 192: this->hash.setAES(hash_t::aes_t::AES192); break;
						// Если шифрование произведено 256 битным ключём
						case 256: this->hash.setAES(hash_t::aes_t::AES256); break;
					}
				// Если получены заголовки требующие сжимать передаваемые фреймы методом Deflate
				} else if((val.compare(L"permessage-deflate") == 0) || (val.compare(L"perframe-deflate") == 0))
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					this->compress = compress_t::DEFLATE;
				// Если получены заголовки требующие сжимать передаваемые фреймы методом GZip
				else if((val.compare(L"permessage-gzip") == 0) || (val.compare(L"perframe-gzip") == 0))
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					this->compress = compress_t::GZIP;
				// Если получены заголовки требующие сжимать передаваемые фреймы методом Brotli
				else if((val.compare(L"permessage-br") == 0) || (val.compare(L"perframe-br") == 0))
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					this->compress = compress_t::BROTLI;
				// Если размер скользящего окна для клиента получен
				else if(val.find(L"client_max_window_bits=") != wstring::npos)
					// Устанавливаем размер скользящего окна
					this->wbitClient = stoi(val.substr(23));
				// Если разрешено использовать максимальный размер скользящего окна для клиента
				else if(val.compare(L"client_max_window_bits") == 0)
					// Устанавливаем максимальный размер скользящего окна
					this->wbitClient = GZIP_MAX_WBITS;
				// Если размер скользящего окна для сервера получен
				else if(val.find(L"server_max_window_bits=") != wstring::npos)
					// Устанавливаем размер скользящего окна
					this->wbitServer = stoi(val.substr(23));
				// Если разрешено использовать максимальный размер скользящего окна для сервера
				else if(val.compare(L"server_max_window_bits") == 0)
					// Устанавливаем максимальный размер скользящего окна
					this->wbitServer = GZIP_MAX_WBITS;
			}
		}
	}
	// Ищем подпротокол сервера
	const string & sub = this->web.getHeader("sec-websocket-protocol");
	// Если подпротокол найден, устанавливаем его
	if(!sub.empty()) this->sub = sub;
}
/**
 * checkKey Метод проверки ключа сервера
 * @return результат проверки
 */
bool awh::WSClient::checkKey() noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем параметры ключа сервера
	const string & auth = this->web.getHeader("sec-websocket-accept");
	// Если параметры авторизации найдены
	if(!auth.empty()){
		// Получаем ключ для проверки
		const string & key = this->getHash();
		// Если ключи не соответствуют, запрещаем работу
		result = (key.compare(auth) == 0);
	}
	// Выводим результат
	return result;
}
/**
 * checkVer Метод проверки на версию протокола
 * @return результат проверки соответствия
 */
bool awh::WSClient::checkVer() noexcept {
	// Сообщаем, что версия соответствует
	return true;
}
/**
 * checkAuth Метод проверки авторизации
 * @return результат проверки авторизации
 */
awh::Http::stath_t awh::WSClient::checkAuth() noexcept {
	// Результат работы функции
	http_t::stath_t result = http_t::stath_t::FAULT;
	// Получаем объект параметров запроса
	web_t::query_t query = this->web.getQuery();
	// Проверяем код ответа
	switch(query.code){
		// Если требуется авторизация
		case 401: {
			// Если попытки провести аутентификацию ещё небыло, пробуем ещё раз
			if(!this->failAuth && (this->authCli.getType() == auth_t::type_t::DIGEST)){
				// Получаем параметры авторизации
				const string & auth = this->web.getHeader("www-authenticate");
				// Если параметры авторизации найдены
				if((this->failAuth = !auth.empty())){
					// Устанавливаем заголовок HTTP в параметры авторизации
					this->authCli.setHeader(auth);
					// Просим повторить авторизацию ещё раз
					result = http_t::stath_t::RETRY;
				}
			}
		} break;
		// Если нужно произвести редирект
		case 301:
		case 308: {
			// Получаем параметры переадресации
			const string & location = this->web.getHeader("location");
			// Если адрес перенаправления найден
			if(!location.empty()){
				// Выполняем парсинг URL
				uri_t::url_t tmp = this->uri->parseUrl(location);
				// Если параметры URL существуют
				if(!this->url.params.empty())
					// Переходим по всему списку параметров
					for(auto & param : this->url.params) tmp.params.emplace(param);
				// Меняем IP адрес сервера
				const_cast <uri_t::url_t *> (&this->url)->ip = move(tmp.ip);
				// Меняем порт сервера
				const_cast <uri_t::url_t *> (&this->url)->port = move(tmp.port);
				// Меняем на путь сервере
				const_cast <uri_t::url_t *> (&this->url)->path = move(tmp.path);
				// Меняем доменное имя сервера
				const_cast <uri_t::url_t *> (&this->url)->domain = move(tmp.domain);
				// Меняем протокол запроса сервера
				const_cast <uri_t::url_t *> (&this->url)->schema = move(tmp.schema);
				// Устанавливаем новый список параметров
				const_cast <uri_t::url_t *> (&this->url)->params = move(tmp.params);
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
/**
 * setUser Метод установки параметров авторизации
 * @param user логин пользователя для авторизации на сервере
 * @param pass пароль пользователя для авторизации на сервере
 */
void awh::WSClient::setUser(const string & user, const string & pass) noexcept {
	// Если пользователь и пароль переданы
	if(!user.empty() && !pass.empty()){
		// Устанавливаем логин пользователя
		this->authCli.setUser(user);
		// Устанавливаем пароль пользователя
		this->authCli.setPass(pass);
	}
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param aes  алгоритм шифрования для Digest авторизации
 */
void awh::WSClient::setAuthType(const auth_t::type_t type, const auth_t::aes_t aes) noexcept {
	// Устанавливаем тип авторизации
	this->authCli.setType(type, aes);
}
