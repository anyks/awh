/**
 * @file basic.cpp
 * @date 2026-07-14
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Реализация схемы BASIC-авторизации (RFC 7617) —
 *        формирование и разбор заголовка Authorization с логином и паролем в кодировке Base64
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/auth/basic.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод проверки учётных данных (только для сервера)
 *
 * @return результат проверки
 *
 */
bool awh::http::Basic::check() noexcept {
	// На стороне клиента проверка не требуется
	if(this->_owner == auth_t::owner_t::CLIENT)
		// Подтверждаем корректность
		return true;
	// Если учётные данные разобраны и функция проверки установлена
	if(!this->_params.user.empty() && (this->_params.callback.checkUser != nullptr))
		// Выполняем проверку пары «логин/пароль» через внешнюю функцию
		return this->_params.callback.checkUser(this->_params.user, this->_params.pass);
	// Сообщаем о неудачной проверке
	return false;
}
/**
 * @brief Метод разбора входящего заголовка авторизации
 *
 * @param header значение заголовка (клиент: вызов, сервер: учётные данные)
 * @return       результат разбора
 *
 */
bool awh::http::Basic::parse(const string_view header) noexcept {
	// Результат работы функции
	bool result = false;
	// Если заголовок передан
	if(!header.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Название схемы авторизации
			const string type = "Basic";
			// Полезная нагрузка заголовка после названия схемы
			string payload = "";
			// Если схема BASIC не распознана - завершаем разбор
			if(!this->schemePayload(header, type, payload))
				// Сообщаем о неудачном разборе
				return result;
			// На стороне сервера разбираем учётные данные клиента
			if(this->_owner == auth_t::owner_t::SERVER){
				// Удаляем крайние пробелы у полезной нагрузки
				this->_fmk->transform(payload, fmk_t::transform_t::TRIM);
				// Если полезная нагрузка получена
				if(!payload.empty()){
					// Выполняем декодирование учётных данных из BASE64
					const string & credentials = this->_crypto->decrypt <string> (payload, crypto_t::hash_t::NONE, crypto_t::cipher_t::BASE64);
					// Если учётные данные декодированы
					if(!credentials.empty()){
						// Выполняем поиск разделителя «логин:пароль»
						const size_t sep = credentials.find(":");
						// Если разделитель найден
						if((result = (sep != string::npos))){
							// Извлекаем логин пользователя
							this->_params.user = credentials.substr(0, sep);
							// Извлекаем пароль пользователя
							this->_params.pass = credentials.substr(sep + 1);
						}
					}
				}
			// На стороне клиента подтверждаем распознавание схемы вызова
			} else result = true;
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(header), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим в лог сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод формирования исходящего заголовка авторизации
 *
 * @param full режим вывода вместе с именем заголовка
 * @return     значение заголовка авторизации
 *
 */
string awh::http::Basic::header(const bool full) noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * В зависимости от стороны работы формируем заголовок авторизации
		 */
		switch(static_cast <uint8_t> (this->_owner)){
			// На стороне клиента формируем учётные данные (Authorization)
			case static_cast <uint8_t> (auth_t::owner_t::CLIENT): {
				// Если логин и пароль установлены
				if(!this->_params.user.empty() && !this->_params.pass.empty()){
					// Формируем строку «логин:пароль»
					const string credentials = this->_params.user + ":" + this->_params.pass;
					// Выполняем кодирование учётных данных в BASE64
					const string & base64 = this->_crypto->encrypt <string> (credentials, crypto_t::hash_t::NONE, crypto_t::cipher_t::BASE64);
					// Формируем значение заголовка авторизации
					result = this->_fmk->format("Basic %s", base64.c_str());
					// Если требуется вывести заголовок вместе с его именем
					if(full)
						// Дополняем результат именем заголовка
						result = this->_fmk->format("%s: %s\r\n", this->name().c_str(), result.c_str());
				}
			} break;
			// На стороне сервера формируем вызов авторизации (WWW-Authenticate)
			case static_cast <uint8_t> (auth_t::owner_t::SERVER): {
				// Формируем значение вызова авторизации
				result = this->_fmk->format("Basic realm=\"%s\"", this->_params.digest.realm.c_str());
				// Если требуется вывести заголовок вместе с его именем
				if(full)
					// Дополняем результат именем заголовка
					result = this->_fmk->format("%s: %s\r\n", this->name().c_str(), result.c_str());
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(full), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим в лог сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param owner  сторона работы (клиент/сервер)
 * @param params общие параметры авторизации
 * @param crypto объект криптографии
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 *
 */
awh::http::Basic::Basic(const auth_t::owner_t owner, auth_t::params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept :
 auth_t::scheme_t(owner, params, crypto, fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::http::Basic::~Basic() noexcept {}
