/**
 * @file: server.cpp
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
#include <auth/server.hpp>

/**
 * data Метод извлечения данных авторизации
 * @return данные модуля авторизации
 */
awh::server::Auth::data_t awh::server::Auth::data() const noexcept {
	// Результат работы функции
	data_t result;
	// Выполняем установку типа авторизации
	result.type = &this->_type;
	// Выполняем установку параметров Digest авторизации
	result.digest = &this->_digest;
	// Выполняем установку пользовательских параметров Digest авторизации
	result.locale = &this->_locale;
	// Выполняем установку логина пользователя
	result.user = &this->_user;
	// Выполняем установку пароля пользователя
	result.pass = &this->_pass;
	// Выводим результат
	return result;
}
/**
 * data Метод установки данных авторизации
 * @param data данные авторизации для установки
 */
void awh::server::Auth::data(const data_t & data) noexcept {
	// Если данные переданы
	if((data.type != nullptr) && (data.digest != nullptr) && (data.locale != nullptr) && (data.user != nullptr) && (data.pass != nullptr)){
		// Выполняем установку типа авторизации
		this->_type = (* data.type);
		// Выполняем установку параметров Digest авторизации
		this->_digest = (* data.digest);
		// Выполняем установку пользовательских параметров Digest авторизации
		this->_locale = (* data.locale);
		// Выполняем установку логина пользователя
		this->_user.assign(data.user->begin(), data.user->end());
		// Выполняем установку пароля пользователя
		this->_pass.assign(data.pass->begin(), data.pass->end());
	}
}
/**
 * check Метод проверки авторизации
 * @param method метод HTTP запроса
 * @return       результат проверки авторизации
 */
bool awh::server::Auth::check(const string & method) noexcept {
	// Результат работы функции
	bool result = false;
	// Определяем тип авторизации
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип авторизации - Базовая
		case static_cast <uint8_t> (type_t::BASIC): {
			// Если функция обратного вызова установлена
			if(this->_callbacks.is("auth"))
				// Выполняем проверку авторизации
				return this->_callbacks.call <bool (const string &, const string &)> ("auth", this->_user, this->_pass);
		} break;
		// Если тип авторизации - Дайджест
		case static_cast <uint8_t> (type_t::DIGEST): {
			// Если данные пользователя переданы
			if(!method.empty() && !this->_user.empty() && !this->_locale.nc.empty() && !this->_locale.uri.empty() && !this->_locale.cnonce.empty() && !this->_locale.resp.empty()){
				// Если на сервере счётчик меньше
				if((this->_fmk->atoi(this->_digest.nc, 16) <= this->_fmk->atoi(this->_locale.nc, 16)) && this->_callbacks.is("extract")){
					// Получаем пароль пользователя
					const string & pass = this->_callbacks.call <string (const string &)> ("extract", this->_user);
					// Если пароль пользователя получен
					if(!pass.empty()){
						// Параметры проверки дайджест авторизации
						digest_t digest;
						// Устанавливаем счётчик клиента
						this->_digest.nc = this->_locale.nc;
						// Устанавливаем параметры для проверки
						digest.nc     = this->_digest.nc;
						digest.hash   = this->_digest.hash;
						digest.uri    = this->_locale.uri;
						digest.qop    = this->_locale.qop;
						digest.realm  = this->_locale.realm;
						digest.nonce  = this->_locale.nonce;
						digest.opaque = this->_locale.opaque;
						digest.cnonce = this->_locale.cnonce;
						// Выполняем проверку авторизации
						result = (this->_fmk->compare(this->response(this->_fmk->transform(method, fmk_t::transform_t::UPPER), this->_user, pass, digest), this->_locale.resp));
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
	if(!realm.empty())
		// Выполняем установку название сервера
		this->_digest.realm = realm;
}
/**
 * opaque Метод установки временного ключа сессии сервера
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Auth::opaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty())
		// Выполняем установку временного ключа сессии сервера
		this->_digest.opaque = opaque;
}
/**
 * extractPassCallback Метод добавления функции извлечения пароля
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Auth::extractPassCallback(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.set <string (const string &)> ("extract", callback);
}
/**
 * authCallback Метод добавления функции обработки авторизации
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Auth::authCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callbacks.set <bool (const string &, const string &)> ("auth", callback);
}
/**
 * header Метод установки параметров авторизации из заголовков
 * @param header заголовок HTTP с параметрами авторизации
 */
void awh::server::Auth::header(const string & header) noexcept {
	// Если заголовок передан
	if(!header.empty() && (this->_fmk != nullptr)){
		// Определяем тип авторизации
		switch(static_cast <uint8_t> (this->_type)){
			// Если тип авторизации Digest
			case static_cast <uint8_t> (type_t::DIGEST): {
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
						vector <string> params;
						// Выполняем разделение параметров расширений
						if(!this->_fmk->split(digest, ",", params).empty()){
							// Позиция поиска разделителя
							size_t pos = wstring::npos;
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
									// Если параметр является именем пользователя
									if(this->_fmk->compare(key, "username")){
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Получаем логин пользователя
										this->_user = value;
									// Если параметр является идентификатором сайта
									} else if(this->_fmk->compare(key, "realm")) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем relam
										this->_locale.realm = value;
									// Если параметр является ключём сгенерированным сервером
									} else if(this->_fmk->compare(key, "nonce")) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем nonce
										this->_locale.nonce = value;
									// Если параметр являеются параметры запроса
									} else if(this->_fmk->compare(key, "uri")) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем uri
										this->_locale.uri = value;
									// Если параметр является ключём сгенерированным клиентом
									} else if(this->_fmk->compare(key, "cnonce")) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем cnonce
										this->_locale.cnonce = value;
									// Если параметр является ключём ответа клиента
									} else if(this->_fmk->compare(key, "response")) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем response
										this->_locale.resp = value;
									// Если параметр является ключём сервера
									} else if(this->_fmk->compare(key, "opaque")) {
										// Удаляем кавычки
										value.assign(value.begin() + 1, value.end() - 1);
										// Устанавливаем opaque
										this->_locale.opaque = value;
									// Если параметр является типом авторизации
									} else if(this->_fmk->compare(key, "qop"))
										// Устанавливаем qop
										this->_locale.qop = value;
									// Если параметр является счётчиком запросов
									else if(this->_fmk->compare(key, "nc"))
										// Устанавливаем nc
										this->_locale.nc = value;
								}
							}
						}
					}
				}
			} break;
			// Если тип авторизации Basic
			case static_cast <uint8_t> (type_t::BASIC): {
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
			} break;
		}
	}
}
/**
 * Оператор вывода строки авторизации
 * @return строка авторизации
 */
awh::server::Auth::operator std::string() noexcept {
	// Результат работы функции
	string result = "";
	// Если фреймворк установлен
	if(this->_fmk != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем тип авторизации
			switch(static_cast <uint8_t> (this->_type)){
				// Если тип авторизации Digest
				case static_cast <uint8_t> (type_t::DIGEST): {
					// Флаг нужно ли клиенту повторить запрос
					string stale = "FALSE";
					// Алгоритм шифрования
					string algorithm = "MD5";
					// Флаг создания нового ключа nonce
					bool createNonce = false;
					// Получаем текущее значение штампа времени
					const time_t stamp = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
					// Если ключ клиента не создан или прошло времени больше 30-ти минут
					if((createNonce = (this->_digest.nonce.empty() || ((stamp - this->_digest.stamp) >= DIGEST_ALIVE_NONCE)))){
						// Устанавливаем штамп времени
						this->_digest.stamp = stamp;
						// Если ключ клиента, ещё небыл сгенерирован
						if(!this->_digest.nonce.empty())
							// Выполняем установку полученного значения
							stale = "TRUE";
					}
					// Определяем алгоритм шифрования
					switch(static_cast <uint16_t> (this->_digest.hash)){
						// Если алгоритм шифрования MD5
						case static_cast <uint16_t> (hash_t::MD5): {
							// Устанавливаем тип шифрования
							algorithm = "MD5";
							// Выполняем создание ключа клиента
							if(createNonce)
								// Выполняем установку полученного значения
								this->_digest.nonce = this->_fmk->hash(std::to_string(time(nullptr)), fmk_t::hash_t::MD5);
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_digest.opaque = this->_fmk->hash(AWH_SITE, fmk_t::hash_t::MD5);
						} break;
						// Если алгоритм шифрования SHA1
						case static_cast <uint16_t> (hash_t::SHA1): {
							// Устанавливаем тип шифрования
							algorithm = "SHA1";
							// Выполняем создание ключа клиента
							if(createNonce)
								// Выполняем установку полученного значения
								this->_digest.nonce = this->_fmk->hash(std::to_string(time(nullptr)), fmk_t::hash_t::SHA1);
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_digest.opaque = this->_fmk->hash(AWH_SITE, fmk_t::hash_t::SHA1);
						} break;
						// Если алгоритм шифрования SHA224
						case static_cast <uint16_t> (hash_t::SHA224): {
							// Устанавливаем тип шифрования
							algorithm = "SHA224";
							// Выполняем создание ключа клиента
							if(createNonce)
								// Выполняем установку полученного значения
								this->_digest.nonce = this->_fmk->hash(std::to_string(time(nullptr)), fmk_t::hash_t::SHA224);
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_digest.opaque = this->_fmk->hash(AWH_SITE, fmk_t::hash_t::SHA224);
						} break;
						// Если алгоритм шифрования SHA256
						case static_cast <uint16_t> (hash_t::SHA256): {
							// Устанавливаем тип шифрования
							algorithm = "SHA256";
							// Выполняем создание ключа клиента
							if(createNonce)
								// Выполняем установку полученного значения
								this->_digest.nonce = this->_fmk->hash(std::to_string(time(nullptr)), fmk_t::hash_t::SHA256);
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_digest.opaque = this->_fmk->hash(AWH_SITE, fmk_t::hash_t::SHA256);
						} break;
						// Если алгоритм шифрования SHA384
						case static_cast <uint16_t> (hash_t::SHA384): {
							// Устанавливаем тип шифрования
							algorithm = "SHA384";
							// Выполняем создание ключа клиента
							if(createNonce)
								// Выполняем установку полученного значения
								this->_digest.nonce = this->_fmk->hash(std::to_string(time(nullptr)), fmk_t::hash_t::SHA384);
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_digest.opaque = this->_fmk->hash(AWH_SITE, fmk_t::hash_t::SHA384);
						} break;
						// Если алгоритм шифрования SHA512
						case static_cast <uint16_t> (hash_t::SHA512): {
							// Устанавливаем тип шифрования
							algorithm = "SHA512";
							// Выполняем создание ключа клиента
							if(createNonce)
								// Выполняем установку полученного значения
								this->_digest.nonce = this->_fmk->hash(std::to_string(time(nullptr)), fmk_t::hash_t::SHA512);
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_digest.opaque = this->_fmk->hash(AWH_SITE, fmk_t::hash_t::SHA512);
						} break;
					}
					// Создаём строку запроса авторизации
					result = this->_fmk->format(
						"Digest realm=\"%s\", qop=\"%s\", stale=%s, algorithm=\"%s\", nonce=\"%s\", opaque=\"%s\"",
						this->_digest.realm.c_str(),
						this->_digest.qop.c_str(),
						stale.c_str(), algorithm.c_str(),
						this->_digest.nonce.c_str(),
						this->_digest.opaque.c_str()
					);
				} break;
				// Если тип авторизации Basic
				case static_cast <uint8_t> (type_t::BASIC):
					// Создаём строку запроса авторизации
					result = this->_fmk->format("Basic realm=\"%s\", charset=\"UTF-8\"", "Please login for access");
				break;
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
	}
	// Выводим результат
	return result;
}
