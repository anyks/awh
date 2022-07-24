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
 * update Метод обновления входящих данных
 */
void awh::server::WS::update() noexcept {
	// Сбрасываем флаг шифрования
	this->crypt = false;
	// Список доступных расширений
	vector <wstring> extensions;
	// Получаем значение заголовка Sec-Websocket-Extensions
	const string & ext = this->web.getHeader("sec-websocket-extensions");
	// Если заголовки расширений найдены
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
				// Если клиент просит отключить перехват контекста сжатия для сервера
				} else if(val.compare(L"server_no_context_takeover") == 0) {
					// Выполняем отключение перехвата контекста
					this->noServerTakeover = true;
					// Выполняем отключение перехвата контекста
					this->noClientTakeover = true;
				// Если клиент просит отключить перехват контекста сжатия для клиента
				} else if(val.compare(L"client_no_context_takeover") == 0)
					// Выполняем отключение перехвата контекста
					this->noClientTakeover = true;
				// Если получены заголовки требующие сжимать передаваемые фреймы методом Deflate
				else if((val.compare(L"permessage-deflate") == 0) || (val.compare(L"perframe-deflate") == 0)) {
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					if((this->compress != compress_t::DEFLATE) && (this->compress != compress_t::ALL_COMPRESS)) this->compress = compress_t::NONE;
				// Если получены заголовки требующие сжимать передаваемые фреймы методом GZip
				} else if((val.compare(L"permessage-gzip") == 0) || (val.compare(L"perframe-gzip") == 0)) {
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					if((this->compress != compress_t::GZIP) && (this->compress != compress_t::ALL_COMPRESS)) this->compress = compress_t::NONE;
				// Если получены заголовки требующие сжимать передаваемые фреймы методом Brotli
				} else if((val.compare(L"permessage-br") == 0) || (val.compare(L"perframe-br") == 0)) {
					// Устанавливаем требование выполнять компрессию полезной нагрузки
					if((this->compress != compress_t::BROTLI) && (this->compress != compress_t::ALL_COMPRESS)) this->compress = compress_t::NONE;
				// Если размер скользящего окна для клиента получен
				} else if(val.find(L"client_max_window_bits=") != wstring::npos) {
					// Устанавливаем размер скользящего окна
					if(this->compress != compress_t::NONE) this->wbitClient = stoi(val.substr(23));
				// Если разрешено использовать максимальный размер скользящего окна для клиента
				} else if(val.compare(L"client_max_window_bits") == 0) {
					// Устанавливаем максимальный размер скользящего окна
					if(this->compress != compress_t::NONE) this->wbitClient = GZIP_MAX_WBITS;
				}
			}
		}
	}
	// Если протоколы установлены и система является сервером
	if(!this->subs.empty()){
		// Получаем список доступных заголовков
		const auto & headers = this->web.getHeaders();
		// Получаем список подпротоколов
		auto ret = headers.equal_range("sec-websocket-protocol");
		// Перебираем все варианты желаемой версии
		for(auto it = ret.first; it != ret.second; ++it){
			// Проверяем, соответствует ли желаемый подпротокол нашему
			if((this->subs.count(it->second) > 0)){
				// Устанавливаем выбранный подпротокол
				this->sub = it->second;
				// Выходим из цикла
				break;
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
	const string & key = this->web.getHeader("sec-websocket-key");
	// Если параметры авторизации найдены
	if((result = !key.empty()))
		// Устанавливаем ключ клиента
		this->key = key;
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
	// Получаем список доступных заголовков
	const auto & headers = this->web.getHeaders();
	// Получаем список версий протоколов
	auto ret = headers.equal_range("sec-websocket-version");
	// Перебираем все варианты желаемой версии
	for(auto it = ret.first; it != ret.second; ++it){
		// Проверяем, совпадает ли желаемая версия протокола
		result = (stoi(it->second) == int(WS_VERSION));
		// Если версия протокола совпадает, выходим
		if(result) break;
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
	if(this->auth.server.getType() != awh::auth_t::type_t::NONE){
		// Получаем параметры авторизации
		const string & auth = this->web.getHeader("authorization");
		// Если параметры авторизации найдены
		if(!auth.empty()){
			// Устанавливаем заголовок HTTP в параметры авторизации
			this->auth.server.setHeader(auth);
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
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::WS::setRealm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty()) this->auth.server.setRealm(realm);
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::WS::setOpaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty()) this->auth.server.setOpaque(opaque);
}
/**
 * setExtractPassCallback Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::WS::setExtractPassCallback(function <string (const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->auth.server.setExtractPassCallback(callback);
}
/**
 * setAuthCallback Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::WS::setAuthCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем внешнюю функцию
	this->auth.server.setAuthCallback(callback);
}
/**
 * setAuthType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::server::WS::setAuthType(const awh::auth_t::type_t type, const awh::auth_t::hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->auth.server.setType(type, hash);
}
