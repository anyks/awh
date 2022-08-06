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
#include <auth/client.hpp>

/**
 * uri Метод установки параметров HTTP запроса
 * @param uri строка параметров HTTP запроса
 */
void awh::client::Auth::uri(const string & uri) noexcept {
	// Если параметры HTTP запроса переданы
	if(!uri.empty()) this->_digest.uri = uri;
}
/**
 * user Метод установки логина пользователя
 * @param user логин пользователя для установки
 */
void awh::client::Auth::user(const string & user) noexcept {
	// Устанавливаем пользователя
	this->_user = user;
}
/**
 * pass Метод установки пароля пользователя
 * @param pass пароль пользователя для установки
 */
void awh::client::Auth::pass(const string & pass) noexcept {
	// Устанавливаем пароль пользвоателя
	this->_pass = pass;
}
/**
 * header Метод установки параметров авторизации из заголовков
 * @param header заголовок HTTP с параметрами авторизации
 */
void awh::client::Auth::header(const string & header) noexcept {
	// Если заголовок передан
	if(!header.empty() && (this->_fmk != nullptr)){
		// Выполняем поиск авторизации
		size_t pos = string::npos;
		// Если тип авторизации Basic получен
		if((pos = header.find("Basic")) != string::npos)
			// Устанавливаем тип авторизации
			this->_type = type_t::BASIC;
		// Если тип авторизации Digest получен
		else if((pos = header.find("Digest")) != string::npos) {
			// Устанавливаем тип авторизации
			this->_type = type_t::DIGEST;
			// Получаем параметры авторизации
			const string & digest = header.substr(pos + 7);
			// Если параметры дайджест авторизации получены
			if(!digest.empty()){
				// Список параметров
				vector <wstring> params;
				// Выполняем разделение параметров расширений
				this->_fmk->split(digest, ",", params);
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
								this->_digest.realm = this->_fmk->convert(value);
							// Если параметр является ключём сгенерированным сервером
							} else if(key.compare(L"nonce") == 0) {
								// Удаляем кавычки
								value.assign(value.begin() + 1, value.end() - 1);
								// Устанавливаем nonce
								this->_digest.nonce = this->_fmk->convert(value);
							// Если параметр является ключём сервера
							} else if(key.compare(L"opaque") == 0) {
								// Удаляем кавычки
								value.assign(value.begin() + 1, value.end() - 1);
								// Устанавливаем opaque
								this->_digest.opaque = this->_fmk->convert(value);
							// Если параметр является алгоритмом
							} else if(key.compare(L"algorithm") == 0) {
								// Удаляем кавычки
								value.assign(value.begin() + 1, value.end() - 1);
								// Переводим в нижний регистр
								value = this->_fmk->toLower(value);
								// Если алгоритм является MD5
								if(value.compare(L"md5") == 0) this->_digest.hash = hash_t::MD5;
								// Если алгоритм является SHA1
								else if(value.compare(L"sha1") == 0) this->_digest.hash = hash_t::SHA1;
								// Если алгоритм является SHA256
								else if(value.compare(L"sha256") == 0) this->_digest.hash = hash_t::SHA256;
								// Если алгоритм является SHA512
								else if(value.compare(L"sha512") == 0) this->_digest.hash = hash_t::SHA512;
							// Если параметр является типом авторизации
							} else if(key.compare(L"qop") == 0)
								// Если тип авторизации передан верно
								if(value.find(L"auth") != wstring::npos) this->_digest.qop = "auth";
						}
					}
				}
			}
		}
	}
}
/**
 * header Метод получения строки авторизации HTTP заголовка
 * @param method метод HTTP запроса
 * @param mode   режим вывода только значения заголовка
 * @return       строка авторизации
 */
const string awh::client::Auth::header(const string & method, const bool mode) noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если логин и пароль установлены
		if(!this->_user.empty() && !this->_pass.empty()){
			// Определяем тип авторизации
			switch((uint8_t) this->_type){
				// Если тип авторизации Digest
				case (uint8_t) type_t::DIGEST: {
					// Если данные необходимые для продолжения работы переданы сервером
					if(!method.empty() && !this->_digest.nonce.empty() && !this->_digest.opaque.empty()){
						// Параметры проверки дайджест авторизации
						digest_t digest;
						// Если ключ клиента не создан, создаём его
						if(this->_digest.cnonce.empty()){
							// Устанавливаем ключ клиента
							this->_digest.cnonce = this->_fmk->md5(to_string(time(nullptr)));
							// Обрезаем лишние символы
							this->_digest.cnonce.assign(this->_digest.cnonce.begin() + 12, this->_digest.cnonce.end() - 12);
						}
						// Выполняем инкрементацию счётчика
						this->_digest.nc = this->_fmk->decToHex((this->_fmk->hexToDec(this->_digest.nc) + 1));
						// Добавляем нули в начало счётчика
						for(u_short i = 0; i < (8 - this->_digest.nc.size()); i++)
							// Добавляем ноль в начало счётчика
							this->_digest.nc.insert(0, "0");
						// Устанавливаем параметры для проверки
						digest.nc     = this->_digest.nc;
						digest.uri    = this->_digest.uri;
						digest.qop    = this->_digest.qop;
						digest.hash   = this->_digest.hash;
						digest.realm  = this->_digest.realm;
						digest.nonce  = this->_digest.nonce;
						digest.opaque = this->_digest.opaque;
						digest.cnonce = this->_digest.cnonce;
						// Формируем ответ серверу
						const string & response = this->response(this->_fmk->toUpper(method), this->_user, this->_pass, digest);
						// Если ответ получен
						if(!response.empty()){
							// Если нужно вывести только значение заголовка
							if(mode)
								// Создаём строку запроса авторизации
								result = this->_fmk->format(
									"Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", qop=%s, nc=%s, cnonce=\"%s\", opaque=\"%s\", response=\"%s\"",
									this->_user.c_str(),
									this->_digest.realm.c_str(),
									this->_digest.nonce.c_str(),
									this->_digest.uri.c_str(),
									this->_digest.qop.c_str(),
									this->_digest.nc.c_str(),
									this->_digest.cnonce.c_str(),
									this->_digest.opaque.c_str(),
									response.c_str()
								);
							// Если нужно вывести полную строку запроса
							else {
								result = this->_fmk->format(
									"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", qop=%s, nc=%s, cnonce=\"%s\", opaque=\"%s\", response=\"%s\"\r\n",
									this->_user.c_str(),
									this->_digest.realm.c_str(),
									this->_digest.nonce.c_str(),
									this->_digest.uri.c_str(),
									this->_digest.qop.c_str(),
									this->_digest.nc.c_str(),
									this->_digest.cnonce.c_str(),
									this->_digest.opaque.c_str(),
									response.c_str()
								);
							}
						}
					}
				} break;
				// Если тип авторизации Basic
				case (uint8_t) type_t::BASIC: {					
					// Выводим результат
					result = base64_t().encode(this->_fmk->format("%s:%s", this->_user.c_str(), this->_pass.c_str()));
					// Если нужно вывести только значение заголовка
					if(mode)
						// Формируем заголовок авторизации
						result = this->_fmk->format("Basic %s", result.c_str());
					// Если нужно вывести полную строку запроса
					else result = this->_fmk->format("Authorization: Basic %s\r\n", result.c_str());
				} break;
			}
		}
	// Выполняем прехват ошибки
	} catch(const exception & error) {
		// Выводим в лог сообщение
		this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
	}
	// Выводим результат
	return result;
}
