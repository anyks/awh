/**
 * @file: core.cpp
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
#include <auth/core.hpp>

/**
 * digest Метод получения параметров Digest авторизации
 * @return параметры Digest авторизации
 */
const awh::Authorization::digest_t & awh::Authorization::digest() const noexcept {
	// Выводим параметры Digest авторизации
	return this->_digest;
}
/**
 * response Метод создания ответа на дайджест авторизацию
 * @param method метод HTTP запроса
 * @param digest параметры дайджест авторизации
 * @param user   логин пользователя для проверки
 * @param pass   пароль пользователя для проверки
 * @return       ответ в 16-м виде
 */
string awh::Authorization::response(const string & method, const string & user, const string & pass, const digest_t & digest) const noexcept {
	// Результат работы функции
	string result = "";
	// Если данные пользователя переданы
	if(!method.empty() && !user.empty() && !pass.empty() && !digest.nonce.empty() && !digest.cnonce.empty() && (this->_fmk != nullptr)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем алгоритм шифрования
			switch(static_cast <uint16_t> (this->_digest.hash)){
				// Если алгоритм шифрования MD5
				case static_cast <uint16_t> (hash_t::MD5): {
					// Создаем первый этап
					const string & ha1 = this->_hash.hashing <string> (this->_fmk->format("%s:%s:%s", user.c_str(), digest.realm.c_str(), pass.c_str()), awh::hash_t::type_t::MD5);
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->_hash.hashing <string> (this->_fmk->format("%s:%s", method.c_str(), digest.uri.c_str()), awh::hash_t::type_t::MD5);
						// Если второй этап создан
						if(!ha2.empty())
							// Создаём результат ответа
							this->_hash.hashing(this->_fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()), awh::hash_t::type_t::MD5, result);
					}
				} break;
				// Если алгоритм шифрования SHA1
				case static_cast <uint16_t> (hash_t::SHA1): {
					// Создаем первый этап
					const string & ha1 = this->_hash.hashing <string> (this->_fmk->format("%s:%s:%s", user.c_str(), digest.realm.c_str(), pass.c_str()), awh::hash_t::type_t::SHA1);
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->_hash.hashing <string> (this->_fmk->format("%s:%s", method.c_str(), digest.uri.c_str()), awh::hash_t::type_t::SHA1);
						// Если второй этап создан
						if(!ha2.empty())
							// Создаём результат ответа
							this->_hash.hashing(this->_fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()), awh::hash_t::type_t::SHA1, result);
					}
				} break;
				// Если алгоритм шифрования SHA224
				case static_cast <uint16_t> (hash_t::SHA224): {
					// Создаем первый этап
					const string & ha1 = this->_hash.hashing <string> (this->_fmk->format("%s:%s:%s", user.c_str(), digest.realm.c_str(), pass.c_str()), awh::hash_t::type_t::SHA224);
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->_hash.hashing <string> (this->_fmk->format("%s:%s", method.c_str(), digest.uri.c_str()), awh::hash_t::type_t::SHA224);
						// Если второй этап создан
						if(!ha2.empty())
							// Создаём результат ответа
							this->_hash.hashing(this->_fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()), awh::hash_t::type_t::SHA224, result);
					}
				} break;
				// Если алгоритм шифрования SHA256
				case static_cast <uint16_t> (hash_t::SHA256): {
					// Создаем первый этап
					const string & ha1 = this->_hash.hashing <string> (this->_fmk->format("%s:%s:%s", user.c_str(), digest.realm.c_str(), pass.c_str()), awh::hash_t::type_t::SHA256);
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->_hash.hashing <string> (this->_fmk->format("%s:%s", method.c_str(), digest.uri.c_str()), awh::hash_t::type_t::SHA256);
						// Если второй этап создан
						if(!ha2.empty())
							// Создаём результат ответа
							this->_hash.hashing(this->_fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()), awh::hash_t::type_t::SHA256, result);
					}
				} break;
				// Если алгоритм шифрования SHA384
				case static_cast <uint16_t> (hash_t::SHA384): {
					// Создаем первый этап
					const string & ha1 = this->_hash.hashing <string> (this->_fmk->format("%s:%s:%s", user.c_str(), digest.realm.c_str(), pass.c_str()), awh::hash_t::type_t::SHA384);
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->_hash.hashing <string> (this->_fmk->format("%s:%s", method.c_str(), digest.uri.c_str()), awh::hash_t::type_t::SHA384);
						// Если второй этап создан
						if(!ha2.empty())
							// Создаём результат ответа
							this->_hash.hashing(this->_fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()), awh::hash_t::type_t::SHA384, result);
					}
				} break;
				// Если алгоритм шифрования SHA512
				case static_cast <uint16_t> (hash_t::SHA512): {
					// Создаем первый этап
					const string & ha1 = this->_hash.hashing <string> (this->_fmk->format("%s:%s:%s", user.c_str(), digest.realm.c_str(), pass.c_str()), awh::hash_t::type_t::SHA512);
					// Если первый этап получен
					if(!ha1.empty()){
						// Создаём второй этап
						const string & ha2 = this->_hash.hashing <string> (this->_fmk->format("%s:%s", method.c_str(), digest.uri.c_str()), awh::hash_t::type_t::SHA512);
						// Если второй этап создан
						if(!ha2.empty())
							// Создаём результат ответа
							this->_hash.hashing(this->_fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()), awh::hash_t::type_t::SHA512, result);
					}
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const std::exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(method, user, pass, static_cast <uint16_t> (digest.hash)), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * type Метод получени типа авторизации
 * @return тип авторизации
 */
awh::Authorization::type_t awh::Authorization::type() const noexcept {
	// Выводим тип авторизации
	return this->_type;
}
/**
 * type Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::Authorization::type(const type_t type, const hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->_type = type;
	// Устанавливаем алгоритм шифрования для авторизации Digest
	this->_digest.hash = hash;
}
