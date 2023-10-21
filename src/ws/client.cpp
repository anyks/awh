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
	// Если данные ещё не зафиксированы
	if(this->_status == status_t::NONE){
		// Сбрасываем флаг шифрования
		this->_crypto = false;
		// Выполняем проверку авторизации
		this->_status = this->status();
		// Если ключ соответствует
		if(this->_status == status_t::GOOD)
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
			// Если заголовок получен с описанием методов компрессии
			if(this->_fmk->compare(header.first, "content-encoding")){
				// Если найден запрашиваемый метод компрессии BROTLI
				if(this->_fmk->compare(header.second, "br"))
					// Переключаем метод компрессии на BROTLI
					http_t::_compress = compress_t::BROTLI;
				// Если найден запрашиваемый метод компрессии GZip
				else if(this->_fmk->compare(header.second, "gzip"))
					// Переключаем метод компрессии на GZIP
					http_t::_compress = compress_t::GZIP;
				// Если найден запрашиваемый метод компрессии Deflate
				else if(this->_fmk->compare(header.second, "deflate"))
					// Переключаем метод компрессии на DEFLATE
					http_t::_compress = compress_t::DEFLATE;
				// Отключаем поддержку сжатия на сервере
				else http_t::_compress = compress_t::NONE;
				// Устанавливаем флаг в каком виде у нас хранится полезная нагрузка
				http_t::_inflated = http_t::_compress;
			// Если заголовок расширения найден
			} else if(this->_fmk->compare(header.first, "sec-websocket-extensions")) {
				// Запись названия расширения
				string extension = "";
				// Выполняем перебор записи расширения
				for(auto & letter : header.second){
					// Определяем чему соответствует буква
					switch(letter){
						// Если буква соответствует разделителю расширения
						case ';': {
							// Если слово собранно
							if(!extension.empty() && !this->extractExtension(extension))
								// Выполняем добавление слова в список записей
								extensions.push_back(std::move(extension));
							// Выполняем очистку слова записи
							extension.clear();
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
							if(!extension.empty() && !this->extractExtension(extension))
								// Выполняем добавление слова в список записей
								extensions.push_back(std::move(extension));
							// Выполняем очистку слова записи
							extension.clear();
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
				// Выполняем очистку слова записи
				extension.clear();
			// Если заголовок сабпротокола найден
			} else if(this->_fmk->compare(header.first, "sec-websocket-protocol")){
				// Проверяем, соответствует ли желаемый подпротокол нашему установленному
				if(this->_supportedProtocols.find(header.second) != this->_supportedProtocols.end())
					// Устанавливаем выбранный подпротокол
					this->_selectedProtocols.emplace(header.second);
			}
		}
		// Если список записей собран
		if(!extensions.empty())
			// Выполняем добавление списка записей в список расширений
			this->_extensions.push_back(std::move(extensions));
	}
}
/**
 * status Метод проверки текущего статуса
 * @return результат проверки текущего статуса
 */
awh::Http::status_t awh::client::WS::status() noexcept {
	// Результат работы функции
	http_t::status_t result = http_t::status_t::FAULT;
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
						result = http_t::status_t::RETRY;
					}
				} break;
				// Если производится авторизация BASIC
				case static_cast <uint8_t> (awh::auth_t::type_t::BASIC):
					// Просим повторить авторизацию ещё раз
					result = http_t::status_t::RETRY;
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
				result = http_t::status_t::RETRY;
			}
		} break;
		// Сообщаем, что авторизация прошла успешно если протокол соответствует HTTP/1.1
		case 101: result = (response.version < 2.0f ? http_t::status_t::GOOD : result); break;
		// Сообщаем, что авторизация прошла успешно если протокол соответствует HTTP/2
		case 200: result = (response.version >= 2.0f ? http_t::status_t::GOOD : result); break;
	}
	// Выводим результат
	return result;
}
/**
 * check Метод проверки шагов рукопожатия
 * @param flag флаг выполнения проверки
 * @return     результат проверки соответствия
 */
bool awh::client::WS::check(const flag_t flag) noexcept {
	// Определяем флаг выполнения проверки
	switch(static_cast <uint8_t> (flag)){
		// Если требуется выполнить проверку соответствие ключа
		case static_cast <uint8_t> (flag_t::KEY): {
			// Получаем параметры ключа сервера
			const string & auth = this->_web.header("sec-websocket-accept");
			// Если параметры авторизации найдены
			if(!auth.empty()){
				// Получаем ключ для проверки
				const string & key = this->sha1();
				// Если ключи не соответствуют, запрещаем работу
				return this->_fmk->compare(key, auth);
			}
		} break;
		// Если требуется выполнить проверку версию протокола
		case static_cast <uint8_t> (flag_t::VERSION):
			// Сообщаем, что версия соответствует
			return true;
		// Если требуется выполнить проверки на переключение протокола
		case static_cast <uint8_t> (flag_t::UPGRADE):
			// Выполняем проверку переключения протокола
			return ws_core_t::check(flag);
	}
	// Выводим результат
	return false;
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
/**
 * WS Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::client::WS::WS(const fmk_t * fmk, const log_t * log) noexcept : ws_core_t(fmk, log) {
	// Выполняем установку идентичность клиента к протоколу WebSocket
	this->_identity = identity_t::WS;
	// Устанавливаем тип HTTP-модуля (Клиент)
	this->_web.hid(awh::web_t::hid_t::CLIENT);
}
