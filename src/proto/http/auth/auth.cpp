/**
 * @file: auth.cpp
 * @date: 2026-07-14
 * @license: GPL-3.0
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
 * Подключаем заголовочные файлы проекта
 */
#include <proto/http/auth/auth.hpp>
#include <proto/http/auth/basic.hpp>
#include <proto/http/auth/digest.hpp>
#include <proto/http/auth/bearer.hpp>
#include <proto/http/auth/hmac.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод получения имени исходящего заголовка авторизации
 *
 * @details Клиент формирует заголовок учётных данных
 *          (Authorization либо Proxy-Authorization для прокси),
 *          сервер формирует заголовок вызова
 *          (WWW-Authenticate либо Proxy-Authenticate для прокси)
 *
 * @return имя заголовка авторизации
 */
string awh::http::Authorization::Scheme::name() const noexcept {
	// На стороне клиента формируем имя заголовка учётных данных
	if(this->_owner == owner_t::CLIENT)
		// Учитываем режим работы через прокси
		return (this->_params.proxy ? "Proxy-Authorization" : "Authorization");
	// На стороне сервера формируем имя заголовка вызова с учётом прокси
	return (this->_params.proxy ? "Proxy-Authenticate" : "WWW-Authenticate");
}
/**
 * @brief Метод формирования набора исходящих заголовков авторизации
 *
 * @details Базовая реализация формирует один заголовок (имя -> значение).
 *          Многозаголовочные схемы (HMAC) переопределяют метод.
 *
 * @param result контейнер для набора заголовков (имя -> значение)
 */
void awh::http::Authorization::Scheme::headers(vector <pair <string, string>> & result) noexcept {
	// Формируем значение заголовка авторизации
	const string & value = this->header(false);
	// Если значение заголовка сформировано
	if(!value.empty())
		// Добавляем заголовок в набор
		result.emplace_back(this->name(), value);
}
/**
 * @brief Метод разбора входящего заголовка авторизации с указанием имени
 *
 * @details Требуется многозаголовочным схемам (HMAC: Signature-Input и Signature).
 *          Базовая реализация игнорирует имя и делегирует разбор значению.
 *
 * @param name   имя входящего заголовка
 * @param header значение входящего заголовка
 * @return       результат разбора
 */
bool awh::http::Authorization::Scheme::parse([[maybe_unused]] const string_view name, const string_view header) noexcept {
	// Делегируем разбор значению заголовка
	return this->parse(header);
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
awh::http::Authorization::Scheme::Scheme(const owner_t owner, params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept :
 _owner(owner), _params(params), _fmk(fmk), _log(log), _crypto(crypto) {}
/**
 * @brief Деструктор
 *
 */
awh::http::Authorization::Scheme::~Scheme() noexcept {}

/**
 * @brief Метод получения стороны работы модуля
 *
 * @return сторона работы (клиент/сервер)
 */
awh::http::Authorization::owner_t awh::http::Authorization::owner() const noexcept {
	// Выводим сторону работы модуля
	return this->_owner;
}
/**
 * @brief Метод получения типа авторизации
 *
 * @return тип авторизации
 */
awh::http::Authorization::type_t awh::http::Authorization::type() const noexcept {
	// Выводим тип авторизации
	return this->_type;
}
/**
 * @brief Метод установки типа авторизации
 *
 * @param type тип авторизации для установки
 * @param hash алгоритм хэширования (для DIGEST/HMAC)
 */
void awh::http::Authorization::type(const type_t type, const hash_t hash) noexcept {
	// Устанавливаем тип авторизации
	this->_type = type;
	// Устанавливаем алгоритм хэширования
	this->_params.hash = hash;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем тип авторизации
		 */
		switch(static_cast <uint8_t> (type)){
			// Если тип авторизации не установлен
			case static_cast <uint8_t> (type_t::NONE):
				// Сбрасываем активную стратегию
				this->_scheme = nullptr;
			break;
			// Если тип авторизации BASIC
			case static_cast <uint8_t> (type_t::BASIC):
				// Создаём стратегию BASIC-авторизации
				this->_scheme = make_unique <basic_t> (this->_owner, this->_params, &this->_crypto, this->_fmk, this->_log);
			break;
			// Если тип авторизации DIGEST
			case static_cast <uint8_t> (type_t::DIGEST):
				// Создаём стратегию DIGEST-авторизации
				this->_scheme = make_unique <digest_scheme_t> (this->_owner, this->_params, &this->_crypto, this->_fmk, this->_log);
			break;
			// Если тип авторизации BEARER
			case static_cast <uint8_t> (type_t::BEARER):
				// Создаём стратегию BEARER-авторизации
				this->_scheme = make_unique <bearer_t> (this->_owner, this->_params, &this->_crypto, this->_fmk, this->_log);
			break;
			// Если тип авторизации HMAC
			case static_cast <uint8_t> (type_t::HMAC):
				// Создаём стратегию HMAC-авторизации
				this->_scheme = make_unique <hmac_t> (this->_owner, this->_params, &this->_crypto, this->_fmk, this->_log);
			break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (type), static_cast <uint16_t> (hash)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим в лог сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод установки логина пользователя
 *
 * @param user логин пользователя
 */
void awh::http::Authorization::user(string_view user) noexcept {
	// Устанавливаем логин пользователя
	this->_params.user = user;
}
/**
 * @brief Метод установки пароля пользователя
 *
 * @param pass пароль пользователя
 */
void awh::http::Authorization::pass(string_view pass) noexcept {
	// Устанавливаем пароль пользователя
	this->_params.pass = pass;
}
/**
 * @brief Метод установки токена доступа (BEARER)
 *
 * @param token токен доступа
 */
void awh::http::Authorization::token(string_view token) noexcept {
	// Устанавливаем токен доступа
	this->_params.token = token;
}
/**
 * @brief Метод установки режима работы через прокси
 *
 * @details Влияет на имена заголовков: Proxy-Authorization/Proxy-Authenticate
 *          вместо Authorization/WWW-Authenticate
 *
 * @param mode флаг работы через прокси
 */
void awh::http::Authorization::proxy(const bool mode) noexcept {
	// Устанавливаем режим работы через прокси
	this->_params.proxy = mode;
}
/**
 * @brief Метод установки секретного ключа подписи (HMAC)
 *
 * @param key секретный ключ подписи
 */
void awh::http::Authorization::key(string_view key) noexcept {
	// Устанавливаем секретный ключ подписи
	this->_params.sign.key = key;
}
/**
 * @brief Метод установки идентификатора ключа подписи (HMAC)
 *
 * @param keyId идентификатор ключа подписи
 */
void awh::http::Authorization::keyId(string_view keyId) noexcept {
	// Устанавливаем идентификатор ключа подписи
	this->_params.sign.keyId = keyId;
}
/**
 * @brief Метод установки метки подписи (HMAC)
 *
 * @param label метка подписи (например, sig1)
 */
void awh::http::Authorization::label(string_view label) noexcept {
	// Если метка подписи передана
	if(!label.empty())
		// Устанавливаем метку подписи
		this->_params.sign.label = label;
}
/**
 * @brief Метод добавления покрываемого подписью компонента (HMAC)
 *
 * @details Порядок добавления компонентов сохраняется. Производные компоненты
 *          начинаются с символа '@' (например, "@method", "@path", "@authority")
 *
 * @param name  имя компонента
 * @param value значение компонента
 */
void awh::http::Authorization::component(string_view name, string_view value) noexcept {
	// Если имя компонента передано
	if(!name.empty())
		// Добавляем компонент в список покрываемых подписью
		this->_params.sign.components.emplace_back(name, value);
}
/**
 * @brief Метод установки параметров HTTP-запроса (DIGEST, клиент)
 *
 * @param uri параметры HTTP-запроса (request-uri)
 */
void awh::http::Authorization::uri(string_view uri) noexcept {
	// Если параметры HTTP-запроса переданы
	if(!uri.empty())
		// Устанавливаем параметры HTTP-запроса
		this->_params.digest.uri = uri;
}
/**
 * @brief Метод установки HTTP-метода запроса (DIGEST)
 *
 * @param method HTTP-метод запроса
 */
void awh::http::Authorization::method(string_view method) noexcept {
	// Если HTTP-метод запроса передан
	if(!method.empty())
		// Устанавливаем HTTP-метод запроса
		this->_params.method = method;
}
/**
 * @brief Метод установки названия сервера (realm)
 *
 * @param realm название сервера
 */
void awh::http::Authorization::realm(string_view realm) noexcept {
	// Устанавливаем название сервера
	this->_params.digest.realm = realm;
}
/**
 * @brief Метод установки уникального ключа сервера (nonce)
 *
 * @param nonce уникальный ключ, выдаваемый сервером
 */
void awh::http::Authorization::nonce(string_view nonce) noexcept {
	// Если уникальный ключ сервера передан
	if(!nonce.empty()){
		// Устанавливаем уникальный ключ сервера
		this->_params.digest.nonce = nonce;
		// На стороне сервера фиксируем ключ как выданный (для сверки при проверке)
		if(this->_owner == owner_t::SERVER)
			// Запоминаем вручную выданный сервером ключ
			this->_params.digest.issued = nonce;
	}
}
/**
 * @brief Метод установки временного ключа сессии сервера (opaque)
 *
 * @param opaque временный ключ сессии сервера
 */
void awh::http::Authorization::opaque(string_view opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty())
		// Устанавливаем временный ключ сессии сервера
		this->_params.digest.opaque = opaque;
}
/**
 * @brief Метод установки сессионного режима алгоритма Digest (-sess)
 *
 * @details При включении используется сессионный вариант расчёта HA1:
 *          HA1 = H(H(user:realm:pass):nonce:cnonce), а в имени алгоритма
 *          добавляется суффикс -sess (например, SHA-256-sess)
 *
 * @param mode флаг сессионного режима алгоритма
 */
void awh::http::Authorization::session(const bool mode) noexcept {
	// Устанавливаем сессионный режим алгоритма Digest
	this->_params.digest.sess = mode;
}
/**
 * @brief Метод проверки учётных данных (только для сервера)
 *
 * @return результат проверки
 */
bool awh::http::Authorization::check() noexcept {
	// Если активная стратегия установлена - делегируем проверку ей
	if(this->_scheme != nullptr)
		// Выполняем проверку учётных данных активной стратегией
		return this->_scheme->check();
	// Сообщаем о неудачной проверке
	return false;
}
/**
 * @brief Метод формирования исходящего заголовка авторизации
 *
 * @details Клиент формирует учётные данные (Authorization),
 *          сервер формирует вызов авторизации (WWW-Authenticate)
 *
 * @param full режим вывода вместе с именем заголовка
 * @return     значение заголовка авторизации
 */
string awh::http::Authorization::header(const bool full) noexcept {
	// Если активная стратегия установлена - делегируем формирование ей
	if(this->_scheme != nullptr)
		// Выполняем формирование заголовка активной стратегией
		return this->_scheme->header(full);
	// Возвращаем пустое значение заголовка
	return "";
}
/**
 * @brief Метод формирования набора исходящих заголовков авторизации
 *
 * @details Универсальный способ получения заголовков: для одно-заголовочных схем
 *          (Basic/Digest/Bearer) возвращается один элемент, для HMAC — два
 *          (Signature-Input и Signature)
 *
 * @param result контейнер для набора заголовков (имя -> значение)
 */
void awh::http::Authorization::headers(vector <pair <string, string>> & result) noexcept {
	// Если активная стратегия установлена - делегируем формирование ей
	if(this->_scheme != nullptr)
		// Выполняем формирование набора заголовков активной стратегией
		this->_scheme->headers(result);
}
/**
 * @brief Метод разбора входящего заголовка авторизации
 *
 * @param header значение заголовка (клиент: вызов сервера, сервер: учётные данные)
 * @return       результат разбора
 */
bool awh::http::Authorization::parse(const string_view header) noexcept {
	// Если активная стратегия установлена - делегируем разбор ей
	if(this->_scheme != nullptr)
		// Выполняем разбор заголовка активной стратегией
		return this->_scheme->parse(header);
	// Сообщаем о неудачном разборе
	return false;
}
/**
 * @brief Метод разбора входящего заголовка авторизации с указанием имени
 *
 * @details Требуется многозаголовочным схемам (HMAC: Signature-Input и Signature)
 *
 * @param name   имя входящего заголовка
 * @param header значение входящего заголовка
 * @return       результат разбора
 */
bool awh::http::Authorization::parse(const string_view name, const string_view header) noexcept {
	// Если активная стратегия установлена - делегируем разбор ей
	if(this->_scheme != nullptr)
		// Выполняем разбор заголовка активной стратегией с указанием имени
		return this->_scheme->parse(name, header);
	// Сообщаем о неудачном разборе
	return false;
}
/**
 * @brief Метод установки функции проверки токена доступа (BEARER, сервер)
 *
 * @param callback функция проверки токена доступа
 */
void awh::http::Authorization::callbackCheckToken(function <bool (const string &)> callback) noexcept {
	// Устанавливаем функцию проверки токена доступа
	this->_params.callback.checkToken = ::move(callback);
}
/**
 * @brief Метод установки функции извлечения секретного ключа (HMAC, сервер)
 *
 * @param callback функция извлечения секретного ключа по идентификатору
 */
void awh::http::Authorization::callbackExtractKey(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию извлечения секретного ключа
	this->_params.callback.extractKey = ::move(callback);
}
/**
 * @brief Метод установки функции извлечения пароля (DIGEST, сервер)
 *
 * @param callback функция извлечения пароля по логину
 */
void awh::http::Authorization::callbackExtractPass(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию извлечения пароля
	this->_params.callback.extractPass = ::move(callback);
}
/**
 * @brief Метод установки функции проверки пары «логин/пароль» (BASIC, сервер)
 *
 * @param callback функция проверки пары «логин/пароль»
 */
void awh::http::Authorization::callbackCheckUser(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию проверки пары «логин/пароль»
	this->_params.callback.checkUser = ::move(callback);
}
/**
 * @brief Конструктор
 *
 * @param owner сторона работы (клиент/сервер)
 * @param fmk   объект фреймворка
 * @param log   объект для работы с логами
 */
awh::http::Authorization::Authorization(const owner_t owner, const fmk_t * fmk, const log_t * log) noexcept :
 _type(type_t::NONE), _owner(owner), _crypto(fmk, log), _scheme(nullptr), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::http::Authorization::~Authorization() noexcept {}
