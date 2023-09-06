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
	this->crypt = false;
	// Список доступных расширений
	vector <string> extensions;
	// Выполняем проверку авторизации
	this->stath = this->checkAuth();
	// Если ключ соответствует
	if(this->stath == stath_t::GOOD)
		// Устанавливаем стейт рукопожатия
		this->state = state_t::GOOD;
	// Поменяем данные как бракованные
	else this->state = state_t::BROKEN;
	// Получаем значение заголовка Sec-Websocket-Extensions
	const string & ext = this->web.header("sec-websocket-extensions");
	// Если заголовки расширений найдены
	if(!ext.empty()){
		// Выполняем разделение параметров расширений
		if(!this->fmk->split(ext, ";", extensions).empty()){
			// Ищем поддерживаемые заголовки
			for(auto & val : extensions){
				// Если нужно производить шифрование данных
				if((this->crypt = this->fmk->exists("permessage-encrypt=", val))){
					// Определяем размер шифрования
					switch(stoi(val.substr(19))){
						// Если шифрование произведено 128 битным ключём
						case 128: this->hash.cipher(hash_t::cipher_t::AES128); break;
						// Если шифрование произведено 192 битным ключём
						case 192: this->hash.cipher(hash_t::cipher_t::AES192); break;
						// Если шифрование произведено 256 битным ключём
						case 256: this->hash.cipher(hash_t::cipher_t::AES256); break;
					}
				// Если клиент просит отключить перехват контекста сжатия для сервера
				} else if(this->fmk->compare(val, "server_no_context_takeover")) {
					// Выполняем отключение перехвата контекста
					this->_noServerTakeover = true;
					// Выполняем отключение перехвата контекста
					this->_noClientTakeover = true;
				// Если клиент просит отключить перехват контекста сжатия для клиента
				} else if(this->fmk->compare(val, "client_no_context_takeover"))
					// Выполняем отключение перехвата контекста
					this->_noClientTakeover = true;
				// Если получены заголовки требующие сжимать передаваемые фреймы методом Deflate
				else if(this->fmk->compare(val, "permessage-deflate") || this->fmk->compare(val, "perframe-deflate")) {
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					if((this->_compress != compress_t::DEFLATE) && (this->_compress != compress_t::ALL_COMPRESS))
						// Выполняем сброс типа компрессии
						this->_compress = compress_t::NONE;
				// Если получены заголовки требующие сжимать передаваемые фреймы методом GZip
				} else if(this->fmk->compare(val, "permessage-gzip") || this->fmk->compare(val, "perframe-gzip")) {
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					if((this->_compress != compress_t::GZIP) && (this->_compress != compress_t::ALL_COMPRESS))
						// Выполняем сброс типа компрессии
						this->_compress = compress_t::NONE;
				// Если получены заголовки требующие сжимать передаваемые фреймы методом Brotli
				} else if(this->fmk->compare(val, "permessage-br") || this->fmk->compare(val, "perframe-br")) {
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					if((this->_compress != compress_t::BROTLI) && (this->_compress != compress_t::ALL_COMPRESS))
						// Выполняем сброс типа компрессии
						this->_compress = compress_t::NONE;
				// Если размер скользящего окна для клиента получен
				} else if(this->fmk->exists("client_max_window_bits=", val)) {
					// Устанавливаем размер скользящего окна
					if(this->_compress != compress_t::NONE)
						// Устанавливаем размер скользящего окна
						this->_wbitClient = ::stoi(val.substr(23));
				// Если разрешено использовать максимальный размер скользящего окна для клиента
				} else if(this->fmk->compare(val, "client_max_window_bits")) {
					// Устанавливаем максимальный размер скользящего окна
					if(this->_compress != compress_t::NONE)
						// Устанавливаем максимальный размер скользящего окна
						this->_wbitClient = GZIP_MAX_WBITS;
				}
			}
		}
	}
	// Если протоколы установлены и система является сервером
	if(!this->_subs.empty()){
		// Переходим по всему списку заголовков
		for(auto & header : this->web.headers()){
			// Если заголовок найден
			if(this->fmk->compare(header.first, "sec-websocket-protocol")){
				// Проверяем, соответствует ли желаемый подпротокол нашему
				if((this->_subs.count(header.second) > 0)){
					// Устанавливаем выбранный подпротокол
					this->_sub = header.second;
					// Выходим из цикла
					break;
				}
			}
		}
	}
}
/**
 * checkKey Метод проверки ключа сервера
 * @return результат проверки
 */
bool awh::server::WS::checkKey() noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем параметры ключа клиента
	const string & key = this->web.header("sec-websocket-key");
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
	for(auto & header : this->web.headers()){
		// Если заголовок найден
		if(this->fmk->compare(header.first, "sec-websocket-version")){
			// Проверяем, совпадает ли желаемая версия протокола
			result = (::stoi(header.second) == static_cast <int> (WS_VERSION));
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
	if(this->auth.server.type() != awh::auth_t::type_t::NONE){
		// Получаем параметры авторизации
		const string & auth = this->web.header("authorization");
		// Если параметры авторизации найдены
		if(!auth.empty()){
			// Устанавливаем заголовок HTTP в параметры авторизации
			this->auth.server.header(auth);
			// Выполняем проверку авторизации
			if(this->auth.server.check("get"))
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
	if(!realm.empty()) this->auth.server.realm(realm);
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::WS::opaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty()) this->auth.server.opaque(opaque);
}
/**
 * extractPassCallback Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::WS::extractPassCallback(function <string (const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->auth.server.extractPassCallback(callback);
}
/**
 * authCallback Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::WS::authCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->auth.server.authCallback(callback);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::WS::authType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->auth.server.type(type, hash);
}
