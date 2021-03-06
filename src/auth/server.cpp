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
	switch((uint8_t) this->type){
		// Если тип авторизации - Базовая
		case (uint8_t) type_t::BASIC:
			// Выполняем проверку авторизации
			result = (this->authFn != nullptr ? this->authFn(this->user, this->pass, this->ctx.at(1)) : result);
		break;
		// Если тип авторизации - Дайджест
		case (uint8_t) type_t::DIGEST: {
			// Если данные пользователя переданы
			if(!method.empty() && !this->user.empty() && !this->userDigest.nc.empty() && !this->userDigest.uri.empty() && !this->userDigest.cnonce.empty() && !this->userDigest.resp.empty()){
				// Если на сервере счётчик меньше
				if((this->fmk->hexToDec(this->digest.nc) <= this->fmk->hexToDec(this->userDigest.nc)) && (this->extractPassFn != nullptr)){
					// Получаем пароль пользователя
					const string & pass = this->extractPassFn(this->user, this->ctx.at(0));
					// Если пароль пользователя получен
					if(!pass.empty()){
						// Параметры проверки дайджест авторизации
						digest_t digest;
						// Устанавливаем счётчик клиента
						this->digest.nc = this->userDigest.nc;
						// Устанавливаем параметры для проверки
						digest.nc     = this->digest.nc;
						digest.hash   = this->digest.hash;
						digest.uri    = this->userDigest.uri;
						digest.qop    = this->userDigest.qop;
						digest.realm  = this->userDigest.realm;
						digest.nonce  = this->userDigest.nonce;
						digest.opaque = this->userDigest.opaque;
						digest.cnonce = this->userDigest.cnonce;
						// Выполняем проверку авторизации
						result = (this->response(this->fmk->toUpper(method), this->user, pass, digest).compare(this->userDigest.resp) == 0);
					}
				}
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
void awh::server::Auth::setRealm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty()) this->digest.realm = realm;
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Auth::setOpaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty()) this->digest.opaque = opaque;
}
/**
 * setExtractPassCallback Метод добавления функции извлечения пароля
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Auth::setExtractPassCallback(void * ctx, function <string (const string &, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(0) = ctx;
	// Устанавливаем функцию извлечения пароля
	this->extractPassFn = callback;
}
/**
 * setAuthCallback Метод добавления функции обработки авторизации
 * @param ctx      контекст для вывода в сообщении
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Auth::setAuthCallback(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept {
	// Устанавливаем контекст передаваемого объекта
	this->ctx.at(1) = ctx;
	// Устанавливаем функцию проверки авторизации
	this->authFn = callback;
}
/**
 * setHeader Метод установки параметров авторизации из заголовков
 * @param header заголовок HTTP с параметрами авторизации
 */
void awh::server::Auth::setHeader(const string & header) noexcept {
	// Если заголовок передан
	if(!header.empty() && (this->fmk != nullptr)){
		// Если тип авторизации Basic
		if(this->type == type_t::BASIC){
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
						this->user = base64.substr(0, pos);
						// Записываем полученный пароль клиента
						this->pass = base64.substr(pos + 1);
					}
				}
			}
		// Если тип авторизации Digest
		} else if(this->type == type_t::DIGEST) {
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
					this->fmk->split(digest, ",", params);
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
									this->user = this->fmk->convert(value);
								// Если параметр является идентификатором сайта
								} else if(key.compare(L"realm") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем relam
									this->userDigest.realm = this->fmk->convert(value);
								// Если параметр является ключём сгенерированным сервером
								} else if(key.compare(L"nonce") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем nonce
									this->userDigest.nonce = this->fmk->convert(value);
								// Если параметр являеются параметры запроса
								} else if(key.compare(L"uri") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем uri
									this->userDigest.uri = this->fmk->convert(value);
								// Если параметр является ключём сгенерированным клиентом
								} else if(key.compare(L"cnonce") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем cnonce
									this->userDigest.cnonce = this->fmk->convert(value);
								// Если параметр является ключём ответа клиента
								} else if(key.compare(L"response") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем response
									this->userDigest.resp = this->fmk->convert(value);
								// Если параметр является ключём сервера
								} else if(key.compare(L"opaque") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем opaque
									this->userDigest.opaque = this->fmk->convert(value);
								// Если параметр является типом авторизации
								} else if(key.compare(L"qop") == 0)
									// Устанавливаем qop
									this->userDigest.qop = this->fmk->convert(value);
								// Если параметр является счётчиком запросов
								else if(key.compare(L"nc") == 0)
									// Устанавливаем nc
									this->userDigest.nc = this->fmk->convert(value);
							}
						}
					}
				}
			}
		}
	}
}
/**
 * getHeader Метод получения строки авторизации HTTP заголовка
 * @param mode режим вывода только значения заголовка
 * @return     строка авторизации
 */
const string awh::server::Auth::getHeader(const bool mode) noexcept {
	// Результат работы функции
	string result = "";
	// Если фреймворк установлен
	if(this->fmk != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если тип авторизации Digest
			if(this->type == type_t::DIGEST){
				// Флаг нужно ли клиенту повторить запрос
				string stale = "FALSE";
				// Алгоритм шифрования
				string algorithm = "MD5";
				// Флаг создания нового ключа nonce
				bool createNonce = false;
				// Получаем текущее значение штампа времени
				const time_t stamp = this->fmk->unixTimestamp();
				// Если ключ клиента не создан или прошло времени больше 30-ти минут
				if((createNonce = (this->digest.nonce.empty() || ((stamp - this->digest.stamp) >= DIGEST_ALIVE_NONCE)))){
					// Устанавливаем штамп времени
					this->digest.stamp = stamp;
					// Если ключ клиента, ещё небыл сгенерирован
					if(!this->digest.nonce.empty()) stale = "TRUE";
				}
				// Определяем алгоритм шифрования
				switch((u_short) this->digest.hash){
					// Если алгоритм шифрования MD5
					case (u_short) hash_t::MD5: {
						// Устанавливаем тип шифрования
						algorithm = "MD5";
						// Выполняем создание ключа клиента
						if(createNonce) this->digest.nonce = this->fmk->md5(to_string(time(nullptr)));
						// Создаём ключ сервера
						if(this->digest.opaque.empty()) this->digest.opaque = this->fmk->md5(AWH_SITE);
					} break;
					// Если алгоритм шифрования SHA1
					case (u_short) hash_t::SHA1: {
						// Устанавливаем тип шифрования
						algorithm = "SHA1";
						// Выполняем создание ключа клиента
						if(createNonce) this->digest.nonce = this->fmk->sha1(to_string(time(nullptr)));
						// Создаём ключ сервера
						if(this->digest.opaque.empty()) this->digest.opaque = this->fmk->sha1(AWH_SITE);
					} break;
					// Если алгоритм шифрования SHA256
					case (u_short) hash_t::SHA256: {
						// Устанавливаем тип шифрования
						algorithm = "SHA256";
						// Выполняем создание ключа клиента
						if(createNonce) this->digest.nonce = this->fmk->sha256(to_string(time(nullptr)));
						// Создаём ключ сервера
						if(this->digest.opaque.empty()) this->digest.opaque = this->fmk->sha256(AWH_SITE);
					} break;
					// Если алгоритм шифрования SHA512
					case (u_short) hash_t::SHA512: {
						// Устанавливаем тип шифрования
						algorithm = "SHA512";
						// Выполняем создание ключа клиента
						if(createNonce) this->digest.nonce = this->fmk->sha512(to_string(time(nullptr)));
						// Создаём ключ сервера
						if(this->digest.opaque.empty()) this->digest.opaque = this->fmk->sha512(AWH_SITE);
					} break;
				}
				// Если нужно вывести только значение заголовка
				if(mode)
					// Создаём строку запроса авторизации
					result = this->fmk->format(
						"Digest realm=\"%s\", qop=\"%s\", stale=%s, algorithm=\"%s\", nonce=\"%s\", opaque=\"%s\"",
						this->digest.realm.c_str(),
						this->digest.qop.c_str(),
						stale.c_str(), algorithm.c_str(),
						this->digest.nonce.c_str(),
						this->digest.opaque.c_str()
					);
				// Если нужно вывести полную строку запроса
				else result = this->fmk->format(
						"WWW-Authenticate: Digest realm=\"%s\", qop=\"%s\", stale=%s, algorithm=\"%s\", nonce=\"%s\", opaque=\"%s\"\r\n",
						this->digest.realm.c_str(),
						this->digest.qop.c_str(),
						stale.c_str(), algorithm.c_str(),
						this->digest.nonce.c_str(),
						this->digest.opaque.c_str()
					);
			// Если тип авторизации Basic
			} else if(this->type == type_t::BASIC) {
				// Если нужно вывести только значение заголовка
				if(mode)
					// Создаём строку запроса авторизации
					result = this->fmk->format("Basic realm=\"%s\", charset=\"UTF-8\"", "Please login for access");
				// Если нужно вывести полную строку запроса
				else result = this->fmk->format("WWW-Authenticate: Basic realm=\"%s\", charset=\"UTF-8\"\r\n", "Please login for access");
			}
		// Выполняем прехват ошибки
		} catch(const exception & error) {
			// Выводим в лог сообщение
			this->log->print("%s", log_t::flag_t::CRITICAL, error.what());
		}
	}
	// Выводим результат
	return result;
}
