/**
 * @file: client.cpp
 * @date: 2022-09-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
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
				vector <string> params;
				// Выполняем разделение параметров расширений
				if(!this->_fmk->split(digest, ",", params).empty()){
					// Ключ и значение параметра
					string key = "", value = "";
					// Переходим по всему списку параметров
					for(auto & param : params){
						// Ищем разделитель параметров
						if((pos = param.find("=")) != wstring::npos){
							// Получаем ключ параметра
							key = param.substr(0, pos);
							// Получаем значение параметра
							value = param.substr(pos + 1);
							// Если параметр является идентификатором сайта
							if(this->_fmk->compare(key, "realm")){
								// Удаляем кавычки
								value.assign(value.begin() + 1, value.end() - 1);
								// Устанавливаем relam
								this->_digest.realm = value;
							// Если параметр является ключём сгенерированным сервером
							} else if(this->_fmk->compare(key, "nonce")) {
								// Удаляем кавычки
								value.assign(value.begin() + 1, value.end() - 1);
								// Устанавливаем nonce
								this->_digest.nonce = value;
							// Если параметр является ключём сервера
							} else if(this->_fmk->compare(key, "opaque")) {
								// Удаляем кавычки
								value.assign(value.begin() + 1, value.end() - 1);
								// Устанавливаем opaque
								this->_digest.opaque = value;
							// Если параметр является алгоритмом
							} else if(this->_fmk->compare(key, "algorithm")) {
								// Удаляем кавычки
								value.assign(value.begin() + 1, value.end() - 1);
								// Переводим в нижний регистр
								this->_fmk->transform(value, fmk_t::transform_t::LOWER);
								// Если алгоритм является MD5
								if(this->_fmk->compare(value, "MD5"))
									// Выполняем установку типа хэша MD5
									this->_digest.hash = hash_t::MD5;
								// Если алгоритм является SHA1
								else if(this->_fmk->compare(value, "SHA1"))
									// Выполняем установку типа хэша SHA1
									this->_digest.hash = hash_t::SHA1;
								// Если алгоритм является SHA256
								else if(this->_fmk->compare(value, "SHA256"))
									// Выполняем установку типа хэша SHA256
									this->_digest.hash = hash_t::SHA256;
								// Если алгоритм является SHA512
								else if(this->_fmk->compare(value, "SHA512"))
									// Выполняем установку типа хэша SHA512
									this->_digest.hash = hash_t::SHA512;
							// Если параметр является типом авторизации
							} else if(this->_fmk->compare(key, "qop")) {
								// Если тип авторизации передан верно
								if(value.find("auth") != wstring::npos)
									// Выполняем установку типа авторизации
									this->_digest.qop = "auth";
							}
						}
					}
				}
			}
		}
	}
}
/**
 * auth Метод получения строки авторизации HTTP-заголовка
 * @param method метод HTTP-запроса
 * @return       строка авторизации
 */
string awh::client::Auth::auth(const string & method) noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если логин и пароль установлены
		if(!this->_user.empty() && !this->_pass.empty()){
			// Определяем тип авторизации
			switch(static_cast <uint8_t> (this->_type)){
				// Если тип авторизации Digest
				case static_cast <uint8_t> (type_t::DIGEST): {
					// Если данные необходимые для продолжения работы переданы сервером
					if(!method.empty() && !this->_digest.nonce.empty() && !this->_digest.opaque.empty()){
						// Параметры проверки дайджест авторизации
						digest_t digest;
						// Если ключ клиента не создан, создаём его
						if(this->_digest.cnonce.empty()){
							// Устанавливаем ключ клиента
							this->_digest.cnonce = this->_fmk->hash(to_string(time(nullptr)), fmk_t::hash_t::MD5);
							// Обрезаем лишние символы
							this->_digest.cnonce.assign(this->_digest.cnonce.begin() + 12, this->_digest.cnonce.end() - 12);
						}
						// Выполняем инкрементацию счётчика
						this->_digest.nc = this->_fmk->itoa((this->_fmk->atoi(this->_digest.nc, 16) + 1), 16);
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
						const string & response = this->response(
							this->_fmk->transform(method, fmk_t::transform_t::UPPER),
							this->_user, this->_pass, digest
						);
						// Если ответ получен
						if(!response.empty())
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
					}
				} break;
				// Если тип авторизации Basic
				case static_cast <uint8_t> (type_t::BASIC):
					// Формируем заголовок авторизации
					result = this->_fmk->format("Basic %s", base64_t().encode(this->_fmk->format("%s:%s", this->_user.c_str(), this->_pass.c_str())).c_str());
				break;
			}
		}
	// Выполняем прехват ошибки
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
