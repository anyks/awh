/**
 * @file: client.cpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

// Подключаем заголовочный файл
#include <ws/client.hpp>

/**
 * commit Метод применения полученных результатов
 */
void awh::client::WS::commit() noexcept {
	// Сбрасываем флаг шифрования
	this->_crypt = false;
	// Выполняем включение перехвата контекста
	this->_server.takeover = true;
	this->_client.takeover = true;
	// Выполняем проверку авторизации
	this->_stath = this->checkAuth();
	// Если ключ соответствует
	if(this->_stath == stath_t::GOOD)
		// Устанавливаем стейт рукопожатия
		this->_state = state_t::GOOD;
	// Поменяем данные как бракованные
	else this->_state = state_t::BROKEN;
	// Отключаем сжатие ответа с сервера
	this->_compress = compress_t::NONE;
	// Список доступных расширений
	vector <string> extensions;
	// Переходим по всему списку заголовков
	for(auto & header : this->_web.headers()){
		// Если заголовок расширения найден
		if(this->_fmk->compare(header.first, "sec-websocket-extensions")){
			// Запись названия расширения
			string extension = "";
			// Выполняем перебор записи расширения
			for(auto & letter : header.second){
				// Определяем чему соответствует буква
				switch(letter){
					// Если буква соответствует разделителю расширения
					case ';': {
						// Если слово собранно
						if(!extension.empty() && !this->extractExtension(extension)){
							// Выполняем добавление слова в список записей
							extensions.push_back(std::move(extension));
							// Выполняем очистку слова записи
							extension.clear();
						}
						// Если список записей собран
						if(!extensions.empty()){
							// Выполняем добавление списка записей в список расширений
							this->_extensions.push_back(std::move(extensions));
							// Выполняем очистку списка расширений
							extensions.clear();
						}
					} break;
					// Если буква соответствует разделителю группы расширений
					case ',': {
						// Если слово собранно
						if(!extension.empty() && !this->extractExtension(extension)){
							// Выполняем добавление слова в список записей
							extensions.push_back(std::move(extension));
							// Выполняем очистку слова записи
							extension.clear();
						}
					} break;
					// Если буква соответствует пробелу
					case ' ': break;
					// Если буква соответствует знаку табуляции
					case '\t': break;
					// Если буква соответствует букве
					default: extension.append(1, letter);
				}
			}
			// Если слово собранно
			if(!extension.empty() && !this->extractExtension(extension))
				// Выполняем добавление слова в список записей
				extensions.push_back(std::move(extension));
		}
	}
	// Если список записей собран
	if(!extensions.empty())
		// Выполняем добавление списка записей в список расширений
		this->_extensions.push_back(std::move(extensions));
	// Ищем подпротокол сервера
	this->_sub = this->_web.header("sec-websocket-protocol");
}
/**
 * checkKey Метод проверки ключа сервера
 * @return результат проверки
 */
bool awh::client::WS::checkKey() noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем параметры ключа сервера
	const string & auth = this->_web.header("sec-websocket-accept");
	// Если параметры авторизации найдены
	if(!auth.empty()){
		// Получаем ключ для проверки
		const string & key = this->sha1();
		// Если ключи не соответствуют, запрещаем работу
		result = this->_fmk->compare(key, auth);
	}
	// Выводим результат
	return result;
}
/**
 * checkVer Метод проверки на версию протокола
 * @return результат проверки соответствия
 */
bool awh::client::WS::checkVer() noexcept {
	// Сообщаем, что версия соответствует
	return true;
}
/**
 * checkAuth Метод проверки авторизации
 * @return результат проверки авторизации
 */
awh::Http::stath_t awh::client::WS::checkAuth() noexcept {
	// Результат работы функции
	http_t::stath_t result = http_t::stath_t::FAULT;
	// Получаем объект параметров ответа
	const web_t::res_t & response = this->_web.response();
	// Проверяем код ответа
	switch(response.code){
		// Если требуется авторизация
		case 401: {
			// Определяем тип авторизации
			switch(static_cast <uint8_t> (this->_auth.client.type())){
				// Если производится авторизация DIGEST
				case static_cast <uint8_t> (awh::auth_t::type_t::DIGEST): {
					// Получаем параметры авторизации
					const string & auth = this->_web.header("www-authenticate");
					// Если параметры авторизации найдены
					if(!auth.empty()){
						// Устанавливаем заголовок HTTP в параметры авторизации
						this->_auth.client.header(auth);
						// Просим повторить авторизацию ещё раз
						result = http_t::stath_t::RETRY;
					}
				} break;
				// Если производится авторизация BASIC
				case static_cast <uint8_t> (awh::auth_t::type_t::BASIC):
					// Просим повторить авторизацию ещё раз
					result = http_t::stath_t::RETRY;
				break;
			}
		} break;
		// Если нужно произвести редирект
		case 301:
		case 302:
		case 303:
		case 307:
		case 308: {
			// Получаем параметры переадресации
			const string & location = this->_web.header("location");
			// Если адрес перенаправления найден
			if(!location.empty()){
				// Получаем объект параметров запроса
				web_t::req_t request = this->_web.request();
				// Выполняем парсинг полученного URL-адреса
				request.url = this->_uri.parse(location);
				// Выполняем установку параметров запроса
				this->_web.request(std::move(request));
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
 * user Метод установки параметров авторизации
 * @param user логин пользователя для авторизации на сервере
 * @param pass пароль пользователя для авторизации на сервере
 */
void awh::client::WS::user(const string & user, const string & pass) noexcept {
	// Если пользователь и пароль переданы
	if(!user.empty() && !pass.empty()){
		// Устанавливаем логин пользователя
		this->_auth.client.user(user);
		// Устанавливаем пароль пользователя
		this->_auth.client.pass(pass);
	}
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::client::WS::authType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->_auth.client.type(type, hash);
}
