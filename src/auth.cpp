/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <auth.hpp>

/**
 * check Метод проверки авторизации
 * @param auth параметры авторизации
 * @return     результат проверки авторизации
 */
const bool awh::Authorization::check(Authorization & auth) noexcept {
	// Результат работы функции
	bool result = false;
	// Если тип авторизации Basic
	if(this->type == type_t::BASIC)
		// Выполняем проверку авторизации
		return this->check(auth.getLogin(), auth.getPassword());
	// Если тип авторизации Digest
	else if(this->type == type_t::DIGEST) {
		// Получаем параметры Digest авторизации у клиента
		const auto & digest = auth.getDigest();
		// Выполняем проверку авторизации
		return this->check(auth.getLogin(), digest.nc, digest.uri, digest.cnonce, digest.response);
	}
	// Выводим результат
	return result;
}
/**
 * check Метод проверки авторизации
 * @param username логин пользователя
 * @param password пароль пользователя
 * @return         результат проверки авторизации
 */
const bool awh::Authorization::check(const string & username, const string & password) noexcept {
	// Результат работы функции
	bool result = false;
	// Если пользователь и пароль переданы
	if(!username.empty() && !password.empty() && (this->type == type_t::BASIC) && (this->users != nullptr)){
		// Ищем пользователя в списке пользователей
		auto it = this->users->find(username);
		// Если пользователь найден, выполняем проверку авторизации
		if(it != this->users->end()) result = (it->second.compare(password) == 0);
	}
	// Выводим результат
	return result;
}
/**
 * check Метод проверки авторизации
 * @param username логин пользователя для проверки
 * @param nc       счётчик HTTP запроса
 * @param uri      параметры HTTP запроса
 * @param cnonce   ключ сгенерированный клиентом
 * @param response хэш ответа клиента
 * @return         результат проверки авторизации
 */
const bool awh::Authorization::check(const string & username, const string & nc, const string & uri, const string & cnonce, const string & response) noexcept {
	// Результат работы функции
	bool result = false;
	// Если пользователь и пароль переданы
	if(!username.empty() && !nc.empty() && !uri.empty() && !cnonce.empty() && !response.empty() && (this->type == type_t::DIGEST)){
		// Если на сервере счётчик меньше
		if((stoi(this->digest.nc) < stoi(nc)) && (this->users != nullptr)){
			// Ищем пользователя в списке пользователей
			auto it = this->users->find(username);
			// Если пользователь найден
			if(it != this->users->end()){
				// Параметры проверки дайджест авторизации
				digest_t digest;
				// Устанавливаем счётчик клиента
				this->digest.nc = nc;
				// Устанавливаем параметры для проверки
				digest.uri       = uri;
				digest.cnonce    = cnonce;
				digest.nc        = this->digest.nc;
				digest.qop       = this->digest.qop;
				digest.realm     = this->digest.realm;
				digest.nonce     = this->digest.nonce;
				digest.opaque    = this->digest.opaque;
				digest.algorithm = this->digest.algorithm;
				// Выполняем проверку авторизации
				result = (this->response(username, it->second, digest).compare(response) == 0);
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * setUsers Метод добавления списка пользователей
 * @param users список пользователей для добавления
 */
void awh::Authorization::setUsers(const unordered_map <string, string> * users) noexcept {
	// Если данные переданы и модуль является сервером
	if(this->server && (users != nullptr) && !users->empty())
		// Устанавливаем список пользователей
		this->users = users;
}
/**
 * setUri Метод установки параметров HTTP запроса
 * @param uri строка параметров HTTP запроса
 */
void awh::Authorization::setUri(const string & uri) noexcept {
	// Если параметры HTTP запроса переданы
	if(!uri.empty()) this->digest.uri = uri;
}
/**
 * setRealm Метод установки название сервера
 * @param realm название сервера
 */
void awh::Authorization::setRealm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty()) this->digest.realm = realm;
}
/**
 * setNonce Метод установки уникального ключа клиента выданного сервером
 * @param nonce уникальный ключ клиента
 */
void awh::Authorization::setNonce(const string & nonce) noexcept {
	// Если уникальный ключ клиента передан
	if(!nonce.empty()) this->digest.nonce = nonce;
}
/**
 * setHeader Метод установки параметров авторизации из заголовков
 * @param header заголовок HTTP с параметрами авторизации
 */
void awh::Authorization::setHeader(const string & header) noexcept {
	// Если заголовок передан
	if(!header.empty() && (this->fmk != nullptr)){
		// Если система является сервером
		if(this->server){
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
							this->username = base64.substr(0, pos);
							// Записываем полученный пароль клиента
							this->password = base64.substr(pos + 1);
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
										this->username = this->fmk->convert(value);
									// Если параметр является идентификатором сайта
									} else if(key.compare(L"realm") == 0) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем relam
										this->digest.realm = this->fmk->convert(value);
									// Если параметр является ключём сгенерированным сервером
									} else if(key.compare(L"nonce") == 0) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем nonce
										this->digest.nonce = this->fmk->convert(value);
									// Если параметр являеются параметры запроса
									} else if(key.compare(L"uri") == 0) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем uri
										this->digest.uri = this->fmk->convert(value);
									// Если параметр является ключём сгенерированным клиентом
									} else if(key.compare(L"cnonce") == 0) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем cnonce
										this->digest.cnonce = this->fmk->convert(value);
									// Если параметр является ключём ответа клиента
									} else if(key.compare(L"response") == 0) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем response
										this->digest.response = this->fmk->convert(value);
									// Если параметр является ключём сервера
									} else if(key.compare(L"opaque") == 0) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем opaque
										this->digest.opaque = this->fmk->convert(value);
									// Если параметр является типом авторизации
									} else if(key.compare(L"qop") == 0)
										// Устанавливаем qop
										this->digest.qop = this->fmk->convert(value);
									// Если параметр является счётчиком запросов
									else if(key.compare(L"nc") == 0)
										// Устанавливаем nc
										this->digest.nc = this->fmk->convert(value);
								}
							}
						}
					}
				}
			}
		// Если система является клиентом
		} else {
			// Выполняем поиск авторизации
			size_t pos = string::npos;
			// Если тип авторизации Basic получен
			if((pos = header.find("Basic")) != string::npos)
				// Устанавливаем тип авторизации
				this->type = type_t::BASIC;
			// Если тип авторизации Digest получен
			else if((pos = header.find("Digest")) != string::npos) {
				// Устанавливаем тип авторизации
				this->type = type_t::DIGEST;
				// Получаем параметры авторизации
				const string & digest = header.substr(pos + 7);
				// Если параметры дайджест авторизации получены
				if(!digest.empty()){
					// Список параметров
					vector <wstring> params;
					// Выполняем разделение параметров расширений
					this->fmk->split(digest, ",", params);
					// Если список параметров получен
					if(!params.empty()){
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
								// Если параметр является идентификатором сайта
								if(key.compare(L"realm") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем relam
									this->digest.realm = this->fmk->convert(value);
								// Если параметр является ключём сгенерированным сервером
								} else if(key.compare(L"nonce") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем nonce
									this->digest.nonce = this->fmk->convert(value);
								// Если параметр является ключём сервера
								} else if(key.compare(L"opaque") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Устанавливаем opaque
									this->digest.opaque = this->fmk->convert(value);
								// Если параметр является алгоритмом
								} else if(key.compare(L"algorithm") == 0) {
									// Удаляем кавычки
									value.assign(value.begin() + 1, value.end() - 1);
									// Переводим в нижний регистр
									value = this->fmk->toLower(value);
									// Если алгоритм является MD5
									if(value.compare(L"md5") == 0) this->digest.algorithm = algorithm_t::MD5;
									// Если алгоритм является SHA1
									else if(value.compare(L"sha1") == 0) this->digest.algorithm = algorithm_t::SHA1;
									// Если алгоритм является SHA256
									else if(value.compare(L"sha256") == 0) this->digest.algorithm = algorithm_t::SHA256;
									// Если алгоритм является SHA512
									else if(value.compare(L"sha512") == 0) this->digest.algorithm = algorithm_t::SHA512;
								// Если параметр является типом авторизации
								} else if(key.compare(L"qop") == 0){
									// Если тип авторизации передан верно
									if(value.find(L"auth") != wstring::npos) this->digest.qop = "auth";
								}
							}
						}
					}
				}
			}
		}
	}
}
/**
 * setOpaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::Authorization::setOpaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty()) this->digest.opaque = opaque;
}
/**
 * setLogin Метод установки логина пользователя
 * @param username логин пользователя для установки
 */
void awh::Authorization::setLogin(const string & username) noexcept {
	// Если пользователь передан
	if(!this->server && !username.empty()) this->username = username;
}
/**
 * setPassword Метод установки пароля пользователя
 * @param password пароль пользователя для установки
 */
void awh::Authorization::setPassword(const string & password) noexcept {
	// Если пароль передан
	if(!this->server && !password.empty()) this->password = password;
}
/**
 * setFramework Метод установки объекта фреймворка
 * @param fmk объект фреймворка для установки
 * @param log объект для работы с логами
 */
void awh::Authorization::setFramework(const fmk_t * fmk, const log_t * log) noexcept {
	// Устанавливаем объект фреймворка
	this->fmk = fmk;
	// Устанавливаем объект для работы с логами
	this->log = log;
}
/**
 * setType Метод установки типа авторизации
 * @param type      тип авторизации
 * @param algorithm алгоритм шифрования для Digest авторизации
 */
void awh::Authorization::setType(const type_t type, const algorithm_t algorithm) noexcept {
	// Устанавливаем тип авторизации
	this->type = type;
	// Устанавливаем алгоритм шифрования для авторизации Digest
	this->digest.algorithm = algorithm;
}
/**
 * getType Метод получени типа авторизации
 * @return тип авторизации
 */
const awh::Authorization::type_t awh::Authorization::getType() const noexcept {
	// Выводим тип авторизации
	return this->type;
}
/**
 * getDigest Метод получения параметров Digest авторизации
 * @return параметры Digest авторизации
 */
const awh::Authorization::digest_t & awh::Authorization::getDigest() const noexcept {
	// Выводим параметры Digest авторизации
	return this->digest;
}
/**
 * getLogin Метод получения имени пользователя
 * @return установленное имя пользователя
 */
const string & awh::Authorization::getLogin() const noexcept {
	// Выводим установленное имя пользователя
	return this->username;
}
/**
 * getPassword Метод получения пароля пользователя
 * @return установленный пароль пользователя
 */
const string & awh::Authorization::getPassword() const noexcept {
	// Выводим установленный пароль пользователя
	return this->password;
}
/**
 * header Метод получения строки авторизации HTTP заголовка
 * @param mode режим вывода только значения заголовка
 * @return     строка авторизации
 */
const string awh::Authorization::header(const bool mode) noexcept {
	// Результат работы функции
	string result = "";
	// Если фреймворк установлен
	if(this->fmk != nullptr){
		// Выполняем перехват ошибки
		try {
			// Если система является сервером
			if(this->server){
				// Если тип авторизации Digest
				if(this->type == type_t::DIGEST){
					// Флаг нужно ли клиенту повторить запрос
					string stale = "FALSE";
					// Алгоритм шифрования
					string algorithm = "MD5";
					// Флаг создания нового ключа nonce
					bool createNonce = false;
					// Получаем текущее значение штампа времени
					const time_t timestamp = this->fmk->unixTimestamp();
					// Если ключ клиента не создан или прошло времени больше 30-ти минут
					if((createNonce = (this->digest.nonce.empty() ||
					((timestamp - this->digest.timestamp) >= 1800000)))){
						// Устанавливаем штамп времени
						this->digest.timestamp = timestamp;
						// Если ключ клиента, ещё небыл сгенерирован
						if(!this->digest.nonce.empty()) stale = "TRUE";
					}
					// Определяем алгоритм шифрования
					switch((u_short) this->digest.algorithm){
						// Если алгоритм шифрования MD5
						case (u_short) algorithm_t::MD5: {
							// Устанавливаем тип шифрования
							algorithm = "MD5";
							// Выполняем создание ключа клиента
							if(createNonce) this->digest.nonce = this->fmk->md5(to_string(time(nullptr)));
							// Создаём ключ сервера
							if(this->digest.opaque.empty()) this->digest.opaque = this->fmk->md5(AWH_SITE);
						} break;
						// Если алгоритм шифрования SHA1
						case (u_short) algorithm_t::SHA1: {
							// Устанавливаем тип шифрования
							algorithm = "SHA1";
							// Выполняем создание ключа клиента
							if(createNonce) this->digest.nonce = this->fmk->sha1(to_string(time(nullptr)));
							// Создаём ключ сервера
							if(this->digest.opaque.empty()) this->digest.opaque = this->fmk->sha1(AWH_SITE);
						} break;
						// Если алгоритм шифрования SHA256
						case (u_short) algorithm_t::SHA256: {
							// Устанавливаем тип шифрования
							algorithm = "SHA256";
							// Выполняем создание ключа клиента
							if(createNonce) this->digest.nonce = this->fmk->sha256(to_string(time(nullptr)));
							// Создаём ключ сервера
							if(this->digest.opaque.empty()) this->digest.opaque = this->fmk->sha256(AWH_SITE);
						} break;
						// Если алгоритм шифрования SHA512
						case (u_short) algorithm_t::SHA512: {
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
						result = this->fmk->format("Basic realm=\"%s\"", AWH_HOST);
					// Если нужно вывести полную строку запроса
					else result = this->fmk->format("WWW-Authenticate: Basic realm=\"%s\"\r\n", AWH_HOST);
				}
			// Если система является клиентом
			} else {
				// Если логин и пароль установлены
				if(!this->username.empty() && !this->password.empty()){
					// Если тип авторизации Digest
					if(this->type == type_t::DIGEST){
						// Если данные необходимые для продолжения работы переданы сервером
						if(!this->digest.nonce.empty() && !this->digest.opaque.empty()){
							// Параметры проверки дайджест авторизации
							digest_t digest;
							// Если ключ клиента не создан, создаём его
							if(this->digest.cnonce.empty()){
								// Устанавливаем ключ клиента
								this->digest.cnonce = this->fmk->md5(to_string(time(nullptr)));
								// Обрезаем лишние символы
								this->digest.cnonce.assign(this->digest.cnonce.begin() + 12, this->digest.cnonce.end() - 12);
							}
							// Выполняем инкрементацию счётчика
							this->digest.nc = to_string(stoi(this->digest.nc) + 1);
							// Получаем количество цифр в счётчике
							const u_short count = this->digest.nc.size();
							// Добавляем нули в начало счётчика
							for(u_short i = 0; i < (8 - count); i++){
								// Добавляем ноль в начало счётчика
								this->digest.nc.insert(0, "0");
							}
							// Устанавливаем параметры для проверки
							digest.nc        = this->digest.nc;
							digest.uri       = this->digest.uri;
							digest.qop       = this->digest.qop;
							digest.realm     = this->digest.realm;
							digest.nonce     = this->digest.nonce;
							digest.opaque    = this->digest.opaque;
							digest.cnonce    = this->digest.cnonce;
							digest.algorithm = this->digest.algorithm;
							// Формируем ответ серверу
							const string & response = this->response(this->username, this->password, digest);
							// Если ответ получен
							if(!response.empty()){
								// Если нужно вывести только значение заголовка
								if(mode)
									// Создаём строку запроса авторизации
									result = this->fmk->format(
										"Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", qop=%s, nc=%s, cnonce=\"%s\", opaque=\"%s\", response=\"%s\"",
										this->username.c_str(),
										this->digest.realm.c_str(),
										this->digest.nonce.c_str(),
										this->digest.uri.c_str(),
										this->digest.qop.c_str(),
										this->digest.nc.c_str(),
										this->digest.cnonce.c_str(),
										this->digest.opaque.c_str(),
										response.c_str()
									);
								// Если нужно вывести полную строку запроса
								else {
									result = this->fmk->format(
										"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", qop=%s, nc=%s, cnonce=\"%s\", opaque=\"%s\", response=\"%s\"\r\n",
										this->username.c_str(),
										this->digest.realm.c_str(),
										this->digest.nonce.c_str(),
										this->digest.uri.c_str(),
										this->digest.qop.c_str(),
										this->digest.nc.c_str(),
										this->digest.cnonce.c_str(),
										this->digest.opaque.c_str(),
										response.c_str()
									);
								}
							}
						}
					// Если тип авторизации Basic
					} else if(this->type == type_t::BASIC) {
						// Выводим результат
						result = base64_t().encode(this->fmk->format("%s:%s", this->username.c_str(), this->password.c_str()));
						// Если нужно вывести только значение заголовка
						if(mode)
							// Формируем заголовок авторизации
							result = this->fmk->format("Basic %s", result.c_str());
						// Если нужно вывести полную строку запроса
						else result = this->fmk->format("Authorization: Basic %s\r\n", result.c_str());
					}
				}
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
/**
 * response Метод создания ответа на дайджест авторизацию
 * @param digest   параметры дайджест авторизации
 * @param username логин пользователя для проверки
 * @param password пароль пользователя для проверки
 * @return         ответ в 16-м виде
 */
const string awh::Authorization::response(const string & username, const string & password, const digest_t & digest) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные пользователя переданы
	if(!username.empty() && !password.empty() && !digest.nonce.empty() && !digest.cnonce.empty() && (this->fmk != nullptr)){
		// Выполняем перехват ошибки
		try {
			// Определяем алгоритм шифрования
			switch((u_short) this->digest.algorithm){
				// Если алгоритм шифрования MD5
				case (u_short) algorithm_t::MD5: {
					// Создаем первый этап
					const string & ha1 = this->fmk->md5(this->fmk->format("%s:%s:%s", username.c_str(), digest.realm.c_str(), password.c_str()));
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->fmk->md5(this->fmk->format("GET:%s", digest.uri.c_str()));
						// Если второй этап создан, создаём результат ответа
						if(!ha2.empty()) result = this->fmk->md5(this->fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()));
					}
				} break;
				// Если алгоритм шифрования SHA1
				case (u_short) algorithm_t::SHA1: {
					// Создаем первый этап
					const string & ha1 = this->fmk->sha1(this->fmk->format("%s:%s:%s", username.c_str(), digest.realm.c_str(), password.c_str()));
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->fmk->sha1(this->fmk->format("GET:%s", digest.uri.c_str()));
						// Если второй этап создан, создаём результат ответа
						if(!ha2.empty()) result = this->fmk->sha1(this->fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()));
					}
				} break;
				// Если алгоритм шифрования SHA256
				case (u_short) algorithm_t::SHA256: {
					// Создаем первый этап
					const string & ha1 = this->fmk->sha256(this->fmk->format("%s:%s:%s", username.c_str(), digest.realm.c_str(), password.c_str()));
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->fmk->sha256(this->fmk->format("GET:%s", digest.uri.c_str()));
						// Если второй этап создан, создаём результат ответа
						if(!ha2.empty()) result = this->fmk->sha256(this->fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()));
					}
				} break;
				// Если алгоритм шифрования SHA512
				case (u_short) algorithm_t::SHA512: {
					// Создаем первый этап
					const string & ha1 = this->fmk->sha512(this->fmk->format("%s:%s:%s", username.c_str(), digest.realm.c_str(), password.c_str()));
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->fmk->sha512(this->fmk->format("GET:%s", digest.uri.c_str()));
						// Если второй этап создан, создаём результат ответа
						if(!ha2.empty()) result = this->fmk->sha512(this->fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()));
					}
				} break;
			}
		// Выполняем прехват ошибки
		} catch(const exception & error) {
			// Выводим в лог сообщение
			if(this->fmk != nullptr) this->log->print("%s", log_t::flag_t::CRITICAL, error.what());
		}
	}
	// Выводим результат
	return result;
}
