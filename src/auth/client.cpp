/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <auth/client.hpp>

/**
 * setUri Метод установки параметров HTTP запроса
 * @param uri строка параметров HTTP запроса
 */
void awh::client::Auth::setUri(const string & uri) noexcept {
	// Если параметры HTTP запроса переданы
	if(!uri.empty()) this->digest.uri = uri;
}
/**
 * setUser Метод установки логина пользователя
 * @param user логин пользователя для установки
 */
void awh::client::Auth::setUser(const string & user) noexcept {
	// Устанавливаем пользователя
	this->user = user;
}
/**
 * setPass Метод установки пароля пользователя
 * @param pass пароль пользователя для установки
 */
void awh::client::Auth::setPass(const string & pass) noexcept {
	// Устанавливаем пароль пользвоателя
	this->pass = pass;
}
/**
 * setHeader Метод установки параметров авторизации из заголовков
 * @param header заголовок HTTP с параметрами авторизации
 */
void awh::client::Auth::setHeader(const string & header) noexcept {
	// Если заголовок передан
	if(!header.empty() && (this->fmk != nullptr)){
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
								if(value.compare(L"md5") == 0) this->digest.hash = hash_t::MD5;
								// Если алгоритм является SHA1
								else if(value.compare(L"sha1") == 0) this->digest.hash = hash_t::SHA1;
								// Если алгоритм является SHA256
								else if(value.compare(L"sha256") == 0) this->digest.hash = hash_t::SHA256;
								// Если алгоритм является SHA512
								else if(value.compare(L"sha512") == 0) this->digest.hash = hash_t::SHA512;
							// Если параметр является типом авторизации
							} else if(key.compare(L"qop") == 0)
								// Если тип авторизации передан верно
								if(value.find(L"auth") != wstring::npos) this->digest.qop = "auth";
						}
					}
				}
			}
		}
	}
}
/**
 * getHeader Метод получения строки авторизации HTTP заголовка
 * @param method метод HTTP запроса
 * @param mode   режим вывода только значения заголовка
 * @return       строка авторизации
 */
const string awh::client::Auth::getHeader(const string & method, const bool mode) noexcept {
	// Результат работы функции
	string result = "";
	// Если фреймворк установлен
	if(!method.empty() && (this->fmk != nullptr)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если логин и пароль установлены
			if(!this->user.empty() && !this->pass.empty()){
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
						this->digest.nc = this->fmk->decToHex((this->fmk->hexToDec(this->digest.nc) + 1));
						// Добавляем нули в начало счётчика
						for(u_short i = 0; i < (8 - this->digest.nc.size()); i++)
							// Добавляем ноль в начало счётчика
							this->digest.nc.insert(0, "0");
						// Устанавливаем параметры для проверки
						digest.nc     = this->digest.nc;
						digest.uri    = this->digest.uri;
						digest.qop    = this->digest.qop;
						digest.hash   = this->digest.hash;
						digest.realm  = this->digest.realm;
						digest.nonce  = this->digest.nonce;
						digest.opaque = this->digest.opaque;
						digest.cnonce = this->digest.cnonce;
						// Формируем ответ серверу
						const string & response = this->response(this->fmk->toUpper(method), this->user, this->pass, digest);
						// Если ответ получен
						if(!response.empty()){
							// Если нужно вывести только значение заголовка
							if(mode)
								// Создаём строку запроса авторизации
								result = this->fmk->format(
									"Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", qop=%s, nc=%s, cnonce=\"%s\", opaque=\"%s\", response=\"%s\"",
									this->user.c_str(),
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
									this->user.c_str(),
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
					result = base64_t().encode(this->fmk->format("%s:%s", this->user.c_str(), this->pass.c_str()));
					// Если нужно вывести только значение заголовка
					if(mode)
						// Формируем заголовок авторизации
						result = this->fmk->format("Basic %s", result.c_str());
					// Если нужно вывести полную строку запроса
					else result = this->fmk->format("Authorization: Basic %s\r\n", result.c_str());
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
