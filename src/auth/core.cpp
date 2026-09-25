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
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <auth/core.hpp>

/**
 * Стандартные модули
 */
#include <cctype>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод получения параметров Digest авторизации
 *
 * @return параметры Digest авторизации
 */
const awh::Authorization::digest_t & awh::Authorization::digest() const noexcept {
	// Выводим параметры Digest авторизации
	return this->_digest;
}
/**
 * @brief Метод создания ответа на дайджест авторизацию
 *
 * @param method метод HTTP запроса
 * @param user   логин пользователя для проверки
 * @param pass   пароль пользователя для проверки
 * @param digest параметры дайджест авторизации
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
			/**
			 * Определяем алгоритм шифрования
			 */
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
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
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
 * @brief Метод разбора параметров заголовка авторизации (RFC 7616, раздел 3.3)
 *
 * @param text строка параметров после названия схемы авторизации
 * @return     список пар ключ-значение, значения без кавычек и экранирования
 */
vector <std::pair <string, string>> awh::Authorization::params(const string & text) const noexcept {
	// Результат работы функции
	vector <std::pair <string, string>> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Текущая позиция в строке
		size_t i = 0;
		// Длина строки параметров
		const size_t size = text.size();
		// Переходим по всей строке параметров
		while(i < size){
			// Пропускаем пробелы и запятые перед ключом
			while((i < size) && ((text[i] == ' ') || (text[i] == '\t') || (text[i] == ',')))
				// Переходим к следующему символу
				i++;
			// Начало ключа
			const size_t start = i;
			// Ищем конец ключа
			while((i < size) && (text[i] != '=') && (text[i] != ','))
				// Переходим к следующему символу
				i++;
			// Получаем ключ параметра
			string key = this->_fmk->transform(text.substr(start, i - start), fmk_t::transform_t::TRIM);
			// Значение параметра
			string value = "";
			// Если после ключа идёт значение
			if((i < size) && (text[i] == '=')){
				// Пропускаем знак равенства
				i++;
				// Пропускаем пробелы перед значением
				while((i < size) && ((text[i] == ' ') || (text[i] == '\t')))
					// Переходим к следующему символу
					i++;
				// Если значение в кавычках
				if((i < size) && (text[i] == '"')){
					// Пропускаем открывающую кавычку
					i++;
					// Читаем значение до закрывающей кавычки
					while((i < size) && (text[i] != '"')){
						// Если символ экранирован, берём следующий символ как есть
						if((text[i] == '\\') && ((i + 1) < size))
							// Пропускаем обратную косую черту
							i++;
						// Добавляем символ значения
						value.append(1, text[i++]);
					}
					// Пропускаем закрывающую кавычку
					if(i < size)
						// Переходим к следующему символу
						i++;
					// Пропускаем всё до следующей запятой
					while((i < size) && (text[i] != ','))
						// Переходим к следующему символу
						i++;
				// Если значение без кавычек (токен)
				} else {
					// Начало значения
					const size_t begin = i;
					// Ищем конец значения
					while((i < size) && (text[i] != ','))
						// Переходим к следующему символу
						i++;
					// Получаем значение параметра
					value = this->_fmk->transform(text.substr(begin, i - begin), fmk_t::transform_t::TRIM);
				}
			}
			// Если ключ получен
			if(!key.empty())
				// Добавляем параметр в список
				result.emplace_back(::move(key), ::move(value));
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Очищаем результат
		result.clear();
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(text), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения названия алгоритма хэширования для заголовков Digest авторизации
 *
 * @param hash алгоритм хэширования
 * @return     название алгоритма
 */
string awh::Authorization::algorithm(const hash_t hash) const noexcept {
	/**
	 * Определяем алгоритм хэширования
	 */
	switch(static_cast <uint8_t> (hash)){
		// Если алгоритм хэширования SHA1
		case static_cast <uint8_t> (hash_t::SHA1): return "SHA1";
		// Если алгоритм хэширования SHA224
		case static_cast <uint8_t> (hash_t::SHA224): return "SHA224";
		// Если алгоритм хэширования SHA256 (название по RFC 7616)
		case static_cast <uint8_t> (hash_t::SHA256): return "SHA-256";
		// Если алгоритм хэширования SHA384
		case static_cast <uint8_t> (hash_t::SHA384): return "SHA384";
		// Если алгоритм хэширования SHA512
		case static_cast <uint8_t> (hash_t::SHA512): return "SHA512";
	}
	// Выводим алгоритм по умолчанию
	return "MD5";
}
/**
 * @brief Метод определения алгоритма хэширования по названию из заголовка Digest авторизации
 *
 * @param name название алгоритма
 * @param hash полученный алгоритм хэширования
 * @return     результат определения
 */
bool awh::Authorization::algorithm(const string & name, hash_t & hash) const noexcept {
	// Название алгоритма без дефисов (SHA-256 и SHA256 — один алгоритм)
	string key = "";
	// Переходим по всем символам названия
	for(auto & c : name){
		// Если символ не является дефисом
		if(c != '-')
			// Добавляем символ в верхнем регистре
			key.append(1, static_cast <char> (::toupper(static_cast <unsigned char> (c))));
	}
	// Если алгоритм MD5
	if(key.compare("MD5") == 0)
		// Устанавливаем алгоритм MD5
		hash = hash_t::MD5;
	// Если алгоритм SHA1
	else if(key.compare("SHA1") == 0)
		// Устанавливаем алгоритм SHA1
		hash = hash_t::SHA1;
	// Если алгоритм SHA224
	else if(key.compare("SHA224") == 0)
		// Устанавливаем алгоритм SHA224
		hash = hash_t::SHA224;
	// Если алгоритм SHA256
	else if(key.compare("SHA256") == 0)
		// Устанавливаем алгоритм SHA256
		hash = hash_t::SHA256;
	// Если алгоритм SHA384
	else if(key.compare("SHA384") == 0)
		// Устанавливаем алгоритм SHA384
		hash = hash_t::SHA384;
	// Если алгоритм SHA512
	else if(key.compare("SHA512") == 0)
		// Устанавливаем алгоритм SHA512
		hash = hash_t::SHA512;
	// Алгоритм не поддерживается (например, MD5-sess или SHA-512-256)
	else return false;
	// Сообщаем, что алгоритм определён
	return true;
}
/**
 * @brief Метод получени типа авторизации
 *
 * @return тип авторизации
 */
awh::Authorization::type_t awh::Authorization::type() const noexcept {
	// Выводим тип авторизации
	return this->_type;
}
/**
 * @brief Метод установки типа авторизации
 *
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest авторизации
 */
void awh::Authorization::type(const type_t type, const hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->_type = type;
	// Устанавливаем алгоритм шифрования для авторизации Digest
	this->_digest.hash = hash;
}
