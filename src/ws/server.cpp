/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <ws/server.hpp>

/**
 * update Метод обновления входящих данных
 */
void awh::WSServer::update() noexcept {
	// Сбрасываем флаг шифрования
	this->crypt = false;
	// Отключаем сжатие ответа с сервера
	this->compress = compress_t::NONE;
	// Список доступных расширений
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
				// Если размер скользящего окна для клиента получен
				else if(val.find(L"client_max_window_bits=") != wstring::npos)
					// Устанавливаем размер скользящего окна
					this->wbitClient = stoi(val.substr(23));
				// Если разрешено использовать максимальный размер скользящего окна для клиента
				else if(val.compare(L"client_max_window_bits") == 0)
					// Устанавливаем максимальный размер скользящего окна
					this->wbitClient = GZIP_MAX_WBITS;
			}
		}
	}
	// Если протоколы установлены и система является сервером
	if(!this->subs.empty()){
		// Получаем список подпротоколов
		auto ret = this->headers.equal_range("sec-websocket-protocol");
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
bool awh::WSServer::checkKey() noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем параметры ключа клиента
	auto it = this->headers.find("sec-websocket-key");
	// Если параметры авторизации найдены
	if((result = (it != this->headers.end())))
		// Устанавливаем ключ клиента
		this->key = it->second;
	// Выводим результат
	return result;
}
/**
 * checkVer Метод проверки на версию протокола
 * @return результат проверки соответствия
 */
bool awh::WSServer::checkVer() noexcept {
	// Результат работы функции
	bool result = false;
	// Получаем список версий протоколов
	auto ret = this->headers.equal_range("sec-websocket-version");
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
awh::Http::stath_t awh::WSServer::checkAuth() noexcept {
	// Результат работы функции
	http_t::stath_t result = http_t::stath_t::FAULT;
	// Если авторизация требуется
	if(this->auth->getType() != auth_t::type_t::NONE){
		// Получаем параметры авторизации
		auto it = this->headers.find("authorization");
		// Если параметры авторизации найдены
		if(it != this->headers.end()){
			// Создаём объект авторизации для клиента
			auth_t auth(this->fmk, this->log, true);
			// Получаем тип алгоритма Дайджест
			const auto & digest = this->auth->getDigest();
			// Устанавливаем тип авторизации
			auth.setType(this->auth->getType(), digest.algorithm);
			// Устанавливаем заголовок HTTP в параметры авторизации
			auth.setHeader(it->second);
			// Выполняем проверку авторизации
			if(this->auth->check(auth))
				// Запоминаем, что авторизация пройдена
				result = http_t::stath_t::GOOD;
		}
	// Сообщаем, что авторизация прошла успешно
	} else result = http_t::stath_t::GOOD;
	// Выводим результат
	return result;
}
