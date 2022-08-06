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
#include <auth/server.hpp>

/**
 * check Метод проверки авторизации
 * @param method метод HTTP запроса
 * @return       результат проверки авторизации
 */
const bool awh::server::Auth::check(const string & method) noexcept {
	// Результат работы функции
	bool result = false;
	// Определяем тип авторизации
	switch((uint8_t) this->_type){
		// Если тип авторизации - Базовая
		case (uint8_t) type_t::BASIC:
			// Выполняем проверку авторизации
			result = (this->_callback.auth != nullptr ? this->_callback.auth(this->_user, this->_pass) : result);
		break;
		// Если тип авторизации - Дайджест
		case (uint8_t) type_t::DIGEST: {
			// Если данные пользователя переданы
			if(!method.empty() && !this->_user.empty() && !this->_userDigest.nc.empty() && !this->_userDigest.uri.empty() && !this->_userDigest.cnonce.empty() && !this->_userDigest.resp.empty()){
				// Если на сервере счётчик меньше
				if((this->_fmk->hexToDec(this->_digest.nc) <= this->_fmk->hexToDec(this->_userDigest.nc)) && (this->_callback.extractPass != nullptr)){
					// Получаем пароль пользователя
					const string & pass = this->_callback.extractPass(this->_user);
					// Если пароль пользователя получен
					if(!pass.empty()){
						// Параметры проверки дайджест авторизации
						digest_t digest;
						// Устанавливаем счётчик клиента
						this->_digest.nc = this->_userDigest.nc;
						// Устанавливаем параметры для проверки
						digest.nc     = this->_digest.nc;
						digest.hash   = this->_digest.hash;
						digest.uri    = this->_userDigest.uri;
						digest.qop    = this->_userDigest.qop;
						digest.realm  = this->_userDigest.realm;
						digest.nonce  = this->_userDigest.nonce;
						digest.opaque = this->_userDigest.opaque;
						digest.cnonce = this->_userDigest.cnonce;
						// Выполняем проверку авторизации
						result = (this->response(this->_fmk->toUpper(method), this->_user, pass, digest).compare(this->_userDigest.resp) == 0);
					}
				}
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * realm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Auth::realm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty()) this->_digest.realm = realm;
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Auth::opaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty()) this->_digest.opaque = opaque;
}
/**
 * extractPassCallback Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Auth::extractPassCallback(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию извлечения пароля
	this->_callback.extractPass = callback;
}
/**
 * authCallback Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Auth::authCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию проверки авторизации
	this->_callback.auth = callback;
}
/**
 * header Метод установки параметров авторизации из заголовков
 * @param header заголовок HTTP с параметрами авторизации
 */
void awh::server::Auth::header(const string & header) noexcept {
	// Если заголовок передан
	if(!header.empty() && (this->_fmk != nullptr)){
		// Если тип авторизации Basic
		if(this->_type == type_t::BASIC){
			// Тип авторизации на сервере
			const string type = "Basic";
			// Выполняем поиск Basic авторизации
			size_t pos = header.find(type);
			// Если авторизация получена
			if((pos != string::npos) && ((pos + type.length()) < header.length())){
				// Получаем хэш авторизации
				const string & base64 = base64_t().decode(header.substr(pos + type.length() + 1));
				// Если хэш получен
				if(!base64.empty()){
					// Выполняем поиск разделителя
					pos = base64.find(":");
					// Если разделитель получен
					if(pos != string::npos){
						// Записываем полученный логин клиента
						this->_user = base64.substr(0, pos);
						// Записываем полученный пароль клиента
						this->_pass = base64.substr(pos + 1);
					}
				}
			}
		// Если тип авторизации Digest
		} else if(this->_type == type_t::DIGEST) {
			// Тип авторизации на сервере
			const string type = "Digest";
			// Выполняем поиск Basic авторизации
			size_t pos = header.find(type);
			// Если авторизация получена
			if((pos != string::npos) && ((pos + type.length()) < header.length())){
				// Получаем параметры авторизации
				const string & digest = header.substr(pos + type.length() + 1);
				// Если параметры дайджест авторизации получены
				if(!digest.empty()){
					// Список параметров
					vector <wstring> params;
					// Выполняем разделение параметров расширений
					this->_fmk->split(digest, ",", params);
					// Если список параметров получен
					if(!params.empty()){
						// Позиция поиска разделителя
						size_t pos = wstring::npos;
						// Ключ и значение параметра
						wstring key = L"", value = L"";
						// Переходим по всему списку параметров
						for(auto & param : params){
							// Ищем разделитель параметров
							if((pos = param.find(L"=")) != wstring::npos){
								// Получаем ключ параметра
								key = param.substr(0, pos);
								// Получаем значение параметра
								value = param.substr(pos + 1);
								// Если параметр является именем пользователя
								if(key.compare(L"username") == 0){
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Получаем логин пользователя
									this->_user = this->_fmk->convert(value);
								// Если параметр является идентификатором сайта
								} else if(key.compare(L"realm") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем relam
									this->_userDigest.realm = this->_fmk->convert(value);
								// Если параметр является ключём сгенерированным сервером
								} else if(key.compare(L"nonce") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем nonce
									this->_userDigest.nonce = this->_fmk->convert(value);
								// Если параметр являеются параметры запроса
								} else if(key.compare(L"uri") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем uri
									this->_userDigest.uri = this->_fmk->convert(value);
								// Если параметр является ключём сгенерированным клиентом
								} else if(key.compare(L"cnonce") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем cnonce
									this->_userDigest.cnonce = this->_fmk->convert(value);
								// Если параметр является ключём ответа клиента
								} else if(key.compare(L"response") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем response
									this->_userDigest.resp = this->_fmk->convert(value);
								// Если параметр является ключём сервера
								} else if(key.compare(L"opaque") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем opaque
									this->_userDigest.opaque = this->_fmk->convert(value);
								// Если параметр является типом авторизации
								} else if(key.compare(L"qop") == 0)
									// Устанавливаем qop
									this->_userDigest.qop = this->_fmk->convert(value);
								// Если параметр является счётчиком запросов
								else if(key.compare(L"nc") == 0)
									// Устанавливаем nc
									this->_userDigest.nc = this->_fmk->convert(value);
							}
						}
					}
				}
			}
		}
	}
}
/**
 * header Метод получения строки авторизации HTTP заголовка
 * @param mode режим вывода только значения заголовка
 * @return     строка авторизации
 */
const string awh::server::Auth::header(const bool mode) noexcept {
	// Результат работы функции
	string result = "";
	// Если фреймворк установлен
	if(this->_fmk != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если тип авторизации Digest
			if(this->_type == type_t::DIGEST){
				// Флаг нужно ли клиенту повторить запрос
				string stale = "FALSE";
				// Алгоритм шифрования
				string algorithm = "MD5";
				// Флаг создания нового ключа nonce
				bool createNonce = false;
				// Получаем текущее значение штампа времени
				const time_t stamp = this->_fmk->unixTimestamp();
				// Если ключ клиента не создан или прошло времени больше 30-ти минут
				if((createNonce = (this->_digest.nonce.empty() || ((stamp - this->_digest.stamp) >= DIGEST_ALIVE_NONCE)))){
					// Устанавливаем штамп времени
					this->_digest.stamp = stamp;
					// Если ключ клиента, ещё небыл сгенерирован
					if(!this->_digest.nonce.empty()) stale = "TRUE";
				}
				// Определяем алгоритм шифрования
				switch((u_short) this->_digest.hash){
					// Если алгоритм шифрования MD5
					case (u_short) hash_t::MD5: {
						// Устанавливаем тип шифрования
						algorithm = "MD5";
						// Выполняем создание ключа клиента
						if(createNonce) this->_digest.nonce = this->_fmk->md5(to_string(time(nullptr)));
						// Создаём ключ сервера
						if(this->_digest.opaque.empty()) this->_digest.opaque = this->_fmk->md5(AWH_SITE);
					} break;
					// Если алгоритм шифрования SHA1
					case (u_short) hash_t::SHA1: {
						// Устанавливаем тип шифрования
						algorithm = "SHA1";
						// Выполняем создание ключа клиента
						if(createNonce) this->_digest.nonce = this->_fmk->sha1(to_string(time(nullptr)));
						// Создаём ключ сервера
						if(this->_digest.opaque.empty()) this->_digest.opaque = this->_fmk->sha1(AWH_SITE);
					} break;
					// Если алгоритм шифрования SHA256
					case (u_short) hash_t::SHA256: {
						// Устанавливаем тип шифрования
						algorithm = "SHA256";
						// Выполняем создание ключа клиента
						if(createNonce) this->_digest.nonce = this->_fmk->sha256(to_string(time(nullptr)));
						// Создаём ключ сервера
						if(this->_digest.opaque.empty()) this->_digest.opaque = this->_fmk->sha256(AWH_SITE);
					} break;
					// Если алгоритм шифрования SHA512
					case (u_short) hash_t::SHA512: {
						// Устанавливаем тип шифрования
						algorithm = "SHA512";
						// Выполняем создание ключа клиента
						if(createNonce) this->_digest.nonce = this->_fmk->sha512(to_string(time(nullptr)));
						// Создаём ключ сервера
						if(this->_digest.opaque.empty()) this->_digest.opaque = this->_fmk->sha512(AWH_SITE);
					} break;
				}
				// Если нужно вывести только значение заголовка
				if(mode)
					// Создаём строку запроса авторизации
					result = this->_fmk->format(
						"Digest realm=\"%s\", qop=\"%s\", stale=%s, algorithm=\"%s\", nonce=\"%s\", opaque=\"%s\"",
						this->_digest.realm.c_str(),
						this->_digest.qop.c_str(),
						stale.c_str(), algorithm.c_str(),
						this->_digest.nonce.c_str(),
						this->_digest.opaque.c_str()
					);
				// Если нужно вывести полную строку запроса
				else result = this->_fmk->format(
						"WWW-Authenticate: Digest realm=\"%s\", qop=\"%s\", stale=%s, algorithm=\"%s\", nonce=\"%s\", opaque=\"%s\"\r\n",
						this->_digest.realm.c_str(),
						this->_digest.qop.c_str(),
						stale.c_str(), algorithm.c_str(),
						this->_digest.nonce.c_str(),
						this->_digest.opaque.c_str()
					);
			// Если тип авторизации Basic
			} else if(this->_type == type_t::BASIC) {
				// Если нужно вывести только значение заголовка
				if(mode)
					// Создаём строку запроса авторизации
					result = this->_fmk->format("Basic realm=\"%s\", charset=\"UTF-8\"", "Please login for access");
				// Если нужно вывести полную строку запроса
				else result = this->_fmk->format("WWW-Authenticate: Basic realm=\"%s\", charset=\"UTF-8\"\r\n", "Please login for access");
			}
		// Выполняем прехват ошибки
		} catch(const exception & error) {
			// Выводим в лог сообщение
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		}
	}
	// Выводим результат
	return result;
}
