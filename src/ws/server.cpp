/**
 * @file: server.cpp
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
#include <ws/server.hpp>

/**
 * commit Метод применения полученных результатов
 */
void awh::server::WS::commit() noexcept {
	// Сбрасываем флаг шифрования
	this->_crypted = false;
	// Выполняем проверку авторизации
	this->_stath = this->checkAuth();
	// Если ключ соответствует
	if(this->_stath == stath_t::GOOD)
		// Устанавливаем стейт рукопожатия
		this->_state = state_t::GOOD;
	// Поменяем данные как бракованные
	else this->_state = state_t::BROKEN;
	// Список доступных расширений
	vector <string> extensions;
	// Переходим по всему списку заголовков
	for(auto & header : this->_web.headers()){
		// Если заголовок получен с описанием методов компрессии
		if(this->_fmk->compare(header.first, "accept-encoding")){
			// Если конкретный метод сжатия не запрашивается
			if(this->_fmk->compare(header.second, "*"))
				// Переключаем метод компрессии на BROTLI
				http_t::_compress = compress_t::BROTLI;
			// Если запрашиваются конкретные методы сжатия
			else {
				// Если найден запрашиваемый метод компрессии BROTLI
				if(this->_fmk->exists("br", header.second))
					// Переключаем метод компрессии на BROTLI
					http_t::_compress = compress_t::BROTLI;
				// Если найден запрашиваемый метод компрессии GZip
				else if(this->_fmk->exists("gzip", header.second))
					// Переключаем метод компрессии на GZIP
					http_t::_compress = compress_t::GZIP;
				// Если найден запрашиваемый метод компрессии Deflate
				else if(this->_fmk->exists("deflate", header.second))
					// Переключаем метод компрессии на DEFLATE
					http_t::_compress = compress_t::DEFLATE;
				// Отключаем поддержку сжатия на сервере
				else http_t::_compress = compress_t::NONE;
			}
			// Устанавливаем флаг в каком виде у нас хранится полезная нагрузка
			http_t::_inflated = http_t::_compress;
		// Если заголовок сабпротокола найден
		} else if(this->_fmk->compare(header.first, "sec-websocket-protocol")) {
			// Проверяем, соответствует ли желаемый подпротокол нашему установленному
			if(this->_supportedProtocols.find(header.second) != this->_supportedProtocols.end())
				// Устанавливаем выбранный подпротокол
				this->_selectedProtocols.emplace(header.second);
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
		}
	}
	// Если список записей собран
	if(!extensions.empty())
		// Выполняем добавление списка записей в список расширений
		this->_extensions.push_back(std::move(extensions));
}
/**
 * checkKey Метод проверки ключа сервера
 * @return результат проверки
 */
bool awh::server::WS::checkKey() noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем параметры ключа клиента
	const string & key = this->_web.header("sec-websocket-key");
	// Если параметры авторизации найдены
	if((result = !key.empty()))
		// Устанавливаем ключ клиента
		this->_key = key;
	// Выводим результат
	return result;
}
/**
 * checkVer Метод проверки на версию протокола
 * @return результат проверки соответствия
 */
bool awh::server::WS::checkVer() noexcept {
	// Результат работы функции
	bool result = false;
	// Переходим по всему списку заголовков
	for(auto & header : this->_web.headers()){
		// Если заголовок найден
		if(this->_fmk->compare(header.first, "sec-websocket-version")){
			// Проверяем, совпадает ли желаемая версия протокола
			result = (static_cast <uint8_t> (::stoi(header.second)) == static_cast <uint8_t> (WS_VERSION));
			// Если версия протокола совпадает, выходим
			if(result) break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * checkAuth Метод проверки авторизации
 * @return результат проверки авторизации
 */
awh::Http::stath_t awh::server::WS::checkAuth() noexcept {
	// Результат работы функции
	http_t::stath_t result = http_t::stath_t::FAULT;
	// Если авторизация требуется
	if(this->_auth.server.type() != awh::auth_t::type_t::NONE){
		// Получаем параметры авторизации
		const string & auth = this->_web.header("authorization");
		// Если параметры авторизации найдены
		if(!auth.empty()){
			// Метод HTTP запроса
			string method = "";
			// Устанавливаем заголовок HTTP в параметры авторизации
			this->_auth.server.header(auth);
			// Определяем метод запроса
			switch(static_cast <uint8_t> (this->_web.request().method)){
				// Если метод запроса указан как GET
				case static_cast <uint8_t> (web_t::method_t::GET):
					// Устанавливаем метод запроса
					method = "get";
				break;
				// Если метод запроса указан как CONNECT
				case static_cast <uint8_t> (web_t::method_t::CONNECT):
					// Устанавливаем метод запроса
					method = "connect";
				break;
			}
			// Выполняем проверку авторизации
			if(this->_auth.server.check(method))
				// Устанавливаем успешный результат авторизации
				result = http_t::stath_t::GOOD;
		}
	// Сообщаем, что авторизация прошла успешно
	} else result = http_t::stath_t::GOOD;
	// Выводим результат
	return result;
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::WS::realm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty())
		// Устанавливаем название сервера
		this->_auth.server.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::WS::opaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty())
		// Устанавливаем временный ключ сессии
		this->_auth.server.opaque(opaque);
}
/**
 * extractPassCallback Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::WS::extractPassCallback(function <string (const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->_auth.server.extractPassCallback(callback);
}
/**
 * authCallback Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::WS::authCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->_auth.server.authCallback(callback);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::WS::authType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->_auth.server.type(type, hash);
}
