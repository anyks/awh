/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Подключаем заголовочный файл
#include <auth/core.hpp>

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
 * response Метод создания ответа на дайджест авторизацию
 * @param digest параметры дайджест авторизации
 * @param user   логин пользователя для проверки
 * @param pass   пароль пользователя для проверки
 * @return       ответ в 16-м виде
 */
const string awh::Authorization::response(const string & user, const string & pass, const digest_t & digest) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные пользователя переданы
	if(!user.empty() && !pass.empty() && !digest.nonce.empty() && !digest.cnonce.empty() && (this->fmk != nullptr)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем алгоритм шифрования
			switch((u_short) this->digest.alg){
				// Если алгоритм шифрования MD5
				case (u_short) alg_t::MD5: {
					// Создаем первый этап
					const string & ha1 = this->fmk->md5(this->fmk->format("%s:%s:%s", user.c_str(), digest.realm.c_str(), pass.c_str()));
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->fmk->md5(this->fmk->format("GET:%s", digest.uri.c_str()));
						// Если второй этап создан, создаём результат ответа
						if(!ha2.empty()) result = this->fmk->md5(this->fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()));
					}
				} break;
				// Если алгоритм шифрования SHA1
				case (u_short) alg_t::SHA1: {
					// Создаем первый этап
					const string & ha1 = this->fmk->sha1(this->fmk->format("%s:%s:%s", user.c_str(), digest.realm.c_str(), pass.c_str()));
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->fmk->sha1(this->fmk->format("GET:%s", digest.uri.c_str()));
						// Если второй этап создан, создаём результат ответа
						if(!ha2.empty()) result = this->fmk->sha1(this->fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()));
					}
				} break;
				// Если алгоритм шифрования SHA256
				case (u_short) alg_t::SHA256: {
					// Создаем первый этап
					const string & ha1 = this->fmk->sha256(this->fmk->format("%s:%s:%s", user.c_str(), digest.realm.c_str(), pass.c_str()));
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->fmk->sha256(this->fmk->format("GET:%s", digest.uri.c_str()));
						// Если второй этап создан, создаём результат ответа
						if(!ha2.empty()) result = this->fmk->sha256(this->fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()));
					}
				} break;
				// Если алгоритм шифрования SHA512
				case (u_short) alg_t::SHA512: {
					// Создаем первый этап
					const string & ha1 = this->fmk->sha512(this->fmk->format("%s:%s:%s", user.c_str(), digest.realm.c_str(), pass.c_str()));
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
/**
 * setType Метод установки типа авторизации
 * @param type тип авторизации
 * @param alg  алгоритм шифрования для Digest авторизации
 */
void awh::Authorization::setType(const type_t type, const alg_t alg) noexcept {
	// Устанавливаем тип авторизации
	this->type = type;
	// Устанавливаем алгоритм шифрования для авторизации Digest
	this->digest.alg = alg;
}
