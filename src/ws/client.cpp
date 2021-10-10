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
	// Выполняем поиск расширений
	auto it = this->headers.find("sec-websocket-extensions");
	// Если заголовок найден
	if(it != this->headers.end()){
		// Выполняем разделение параметров расширений
		this->fmk->split(it->second, ";", extensions);
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
						case 128: this->hash->setAES(hash_t::aes_t::AES128); break;
						// Если шифрование произведено 192 битным ключём
						case 192: this->hash->setAES(hash_t::aes_t::AES192); break;
						// Если шифрование произведено 256 битным ключём
						case 256: this->hash->setAES(hash_t::aes_t::AES256); break;
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
	it = this->headers.find("sec-websocket-protocol");
	// Если подпротокол найден, устанавливаем его
	if(it != this->headers.end()) this->sub = it->second;
}
/**
 * checkKey Метод проверки ключа сервера
 * @return результат проверки
 */
bool awh::WSClient::checkKey() noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем параметры ключа сервера
	auto it = this->headers.find("sec-websocket-accept");
	// Если параметры авторизации найдены
	if(it != this->headers.end()){
		// Получаем ключ для проверки
		const string & key = this->getHash();
		// Если ключи не соответствуют, запрещаем работу
		result = (key.compare(it->second) == 0);
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
	// Проверяем код ответа
	switch(this->query.code){
		// Если требуется авторизация
		case 401: {
			// Если попытки провести аутентификацию ещё небыло, пробуем ещё раз
			if(!this->failAuth && (this->auth->getType() == auth_t::type_t::DIGEST)){
				// Получаем параметры авторизации
				auto it = this->headers.find("www-authenticate");
				// Если параметры авторизации найдены
				if((this->failAuth = (it != this->headers.end()))){
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
		reinterpret_cast <authCli_t *> (this->auth)->setUser(user);
		// Устанавливаем пароль пользователя
		reinterpret_cast <authCli_t *> (this->auth)->setPass(pass);
	}
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param alg  алгоритм шифрования для Digest авторизации
 */
void awh::WSClient::setAuthType(const auth_t::type_t type, const auth_t::alg_t alg) noexcept {
	// Если объект авторизации создан
	if(this->auth != nullptr)
		// Устанавливаем тип авторизации
		reinterpret_cast <authCli_t *> (this->auth)->setType(type, alg);
}
/**
 * WSClient Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::WSClient::WSClient(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : ws_t(fmk, log, uri) {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаём объект для работы с авторизацией
		this->auth = new authCli_t(fmk, log);
	// Если происходит ошибка то игнорируем её
	} catch(const bad_alloc&) {
		// Выводим сообщение об ошибке
		log->print("%s", log_t::flag_t::CRITICAL, "memory could not be allocated");
		// Выходим из приложения
		exit(EXIT_FAILURE);
	}
}
/**
 * ~WSClient Деструктор
 */
awh::WSClient::~WSClient() noexcept {
	// Удаляем объект авторизации
	if(this->auth != nullptr) delete this->auth;
}
