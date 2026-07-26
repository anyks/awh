/**
 * @file: bearer.cpp
 * @date: 2026-07-14
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/auth/bearer.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод проверки учётных данных (только для сервера)
 *
 * @return результат проверки
 */
bool awh::http::Bearer::check() noexcept {
	// На стороне клиента проверка не требуется
	if(this->_owner == auth_t::owner_t::CLIENT)
		// Подтверждаем корректность
		return true;
	// Если токен доступа разобран и функция проверки установлена
	if(!this->_params.token.empty() && (this->_params.callback.checkToken != nullptr))
		// Выполняем проверку токена доступа через внешнюю функцию
		return this->_params.callback.checkToken(this->_params.token);
	// Сообщаем о неудачной проверке
	return false;
}
/**
 * @brief Метод разбора входящего заголовка авторизации
 *
 * @param header значение заголовка (клиент: вызов, сервер: учётные данные)
 * @return       результат разбора
 */
bool awh::http::Bearer::parse(const string_view header) noexcept {
	// Результат работы функции
	bool result = false;
	// Если заголовок передан
	if(!header.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Название схемы авторизации
			const string type = "Bearer";
			// Полезная нагрузка заголовка после названия схемы
			string payload = "";
			// Если схема BEARER не распознана - завершаем разбор
			if(!this->schemePayload(header, type, payload))
				// Сообщаем о неудачном разборе
				return result;
			// На стороне сервера разбираем токен доступа клиента
			if(this->_owner == auth_t::owner_t::SERVER){
				// Удаляем крайние пробелы у токена доступа
				this->_fmk->transform(payload, fmk_t::transform_t::TRIM);
				// Если токен доступа получен
				if((result = !payload.empty()))
					// Устанавливаем токен доступа
					this->_params.token = ::move(payload);
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
 */
string awh::http::Bearer::header(const bool full) noexcept {
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
				// Если токен доступа установлен
				if(!this->_params.token.empty()){
					// Формируем значение заголовка авторизации
					result = this->_fmk->format("Bearer %s", this->_params.token.c_str());
					// Если требуется вывести заголовок вместе с его именем
					if(full)
						// Дополняем результат именем заголовка
						result = this->_fmk->format("%s: %s\r\n", this->name().c_str(), result.c_str());
				}
			} break;
			// На стороне сервера формируем вызов авторизации (WWW-Authenticate)
			case static_cast <uint8_t> (auth_t::owner_t::SERVER): {
				// Формируем значение вызова авторизации
				result = this->_fmk->format("Bearer realm=\"%s\"", this->_params.digest.realm.c_str());
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
 */
awh::http::Bearer::Bearer(const auth_t::owner_t owner, auth_t::params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept :
 auth_t::scheme_t(owner, params, crypto, fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::http::Bearer::~Bearer() noexcept {}
