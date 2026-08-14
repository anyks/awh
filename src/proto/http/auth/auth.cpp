/**
 * @file auth.cpp
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
 * @brief Реализация модуля HTTP-авторизации — выбор и переключение схем авторизации,
 *        генерация и проверка заголовков на стороне клиента и сервера, выдача и учёт nonce,
 *        контроль срока жизни и защита от replay-атак
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартный заголовочный файл
 */
#include <cctype>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <encoding/ascii.hpp>
#include <proto/http/auth/auth.hpp>
#include <proto/http/auth/hmac.hpp>
#include <proto/http/auth/basic.hpp>
#include <proto/http/auth/digest.hpp>
#include <proto/http/auth/bearer.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Инкапсулируем статические параметры в пространство имён
 *
 */
namespace {
	/**
	 * @brief Функция очистки разобранных учётных данных предыдущего запроса (сервер)
	 *
	 * @param params общие параметры авторизации
	 *
	 */
	void clearParsedCredentials(auth_t::params_t & params) noexcept {
		/**
		 * Очищаем учётные данные предыдущего запроса
		 */
		params.user.clear();
		params.pass.clear();
		params.token.clear();
		/**
		 * Очищаем разобранные поля HMAC (key, components сохраняются)
		 */
		params.sign.tag.clear();
		params.sign.nonce.clear();
		params.sign.keyId.clear();
		params.sign.params.clear();
		params.sign.covered.clear();
		params.sign.inputLabel.clear();
		params.sign.signature.clear();
		params.sign.date.created = 0;
		params.sign.date.expires = 0;
		params.sign.label        = "sig1";
		/**
		 * Очищаем разобранные поля Digest (issued, issuedOpaque, lncs, mode.stamp, realm сохраняются)
		 */
		params.digest.uri.clear();
		params.digest.nonce.clear();
		params.digest.cnonce.clear();
		params.digest.opaque.clear();
		params.digest.response.clear();
		params.digest.mode.qop     = false;
		params.digest.mode.sess    = false;
		params.digest.mode.authInt = false;
		params.digest.qop          = "auth";
		params.digest.nc           = "00000000";
		params.hash                = params.scheme;
	}
};

/**
 * @brief Конструктор
 *
 */
awh::http::Authorization::Mode_Digest::Mode_Digest() noexcept :
 qop(false), sess(false), authInt(false), stamp(0) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Authorization::Digest::Digest() noexcept :
 nc{"00000000"}, uri{""},
 qop{"auth"}, realm{""},
 nonce{""}, issued{""},
 entity{""}, opaque{""},
 cnonce{""}, response{""},
 issuedOpaque{""} {}

/**
 * @brief Конструктор
 *
 */
awh::http::Authorization::Sign_Date::Sign_Date() noexcept : created(0), expires(0) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Authorization::Sign::Sign() noexcept :
 key{""}, tag{""},
 keyId{""}, label{"sig1"},
 nonce{""}, params{""},
 signature{""}, inputLabel{""} {}

/**
 * @brief Конструктор
 *
 */
awh::http::Authorization::Callback::Callback() noexcept :
 checkToken(nullptr), extractKey(nullptr),
 extractPass(nullptr), checkUser(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Authorization::Mode_Params::Mode_Params() noexcept :
 proxy(false),
 validation(mode_t::SIMPLE),
 clockSkew(60), signMaxAge(0),
 nonceMaxAge(1800), signStrictMaxAge(300) {}

/**
 * @brief Конструктор
 *
 */
awh::http::Authorization::Params::Params() noexcept :
 user{""}, pass{""},
 token{""}, method{"GET"},
 hash(hash_t::MD5), scheme(hash_t::MD5) {}

/**
 * @brief Метод получения имени исходящего заголовка авторизации
 *
 * @details Клиент формирует заголовок учётных данных
 *          (Authorization либо Proxy-Authorization для прокси),
 *          сервер формирует заголовок вызова
 *          (WWW-Authenticate либо Proxy-Authenticate для прокси)
 *
 * @return имя заголовка авторизации
 *
 */
string awh::http::Authorization::Scheme::name() const noexcept {
	// На стороне клиента формируем имя заголовка учётных данных
	if(this->_owner == owner_t::CLIENT)
		// Учитываем режим работы через прокси
		return (this->_params.mode.proxy ? "Proxy-Authorization" : "Authorization");
	// На стороне сервера формируем имя заголовка вызова с учётом прокси
	return (this->_params.mode.proxy ? "Proxy-Authenticate" : "WWW-Authenticate");
}
/**
 * @brief Метод сравнения строк в постоянном времени
 *
 * @details Используется для сравнения секретов и подписей. Не делегируется
 *          в fmk_t::compare(), так как тот завершается досрочно и не подходит
 *          для криптографических сверок.
 *
 * @param left  первая строка
 * @param right вторая строка
 * @return      результат сравнения
 *
 */
bool awh::http::Authorization::Scheme::secureCompare(const string_view left, const string_view right) noexcept {
	// Если длины строк не совпадают
	if(left.size() != right.size())
		// Сообщаем о несовпадении
		return false;
	// Накопитель различий между строками
	uint8_t diff = 0;
	/**
	 * Выполняем побайтовое сравнение
	 */
	for(size_t index = 0; index < left.size(); ++index)
		// Накапливаем различия символов
		diff |= static_cast <uint8_t> (left[index]) ^ static_cast <uint8_t> (right[index]);
	// Подтверждаем совпадение только при отсутствии различий
	return (diff == 0);
}
/**
 * @brief Метод извлечения полезной нагрузки после названия схемы авторизации
 *
 * @details Проверяет, что заголовок начинается с указанной схемы и после неё
 *          следует пробельный символ (RFC 7235). Для trim и сравнения схемы
 *          используется fmk_t.
 *
 * @param header  значение заголовка авторизации
 * @param scheme  название схемы (Basic, Bearer, Digest)
 * @param payload полезная нагрузка после схемы
 * @return        результат извлечения
 *
 */
bool awh::http::Authorization::Scheme::schemePayload(const string_view header, const string_view scheme, string & payload) const noexcept {
	// Если заголовок или схема не переданы
	if(header.empty() || scheme.empty())
		// Сообщаем о неудачном извлечении
		return false;
	// Копируем заголовок для нормализации через fmk_t
	string text(header);
	// Удаляем крайние пробелы у заголовка
	this->_fmk->transform(text, fmk_t::transform_t::TRIM);
	// Если длина заголовка недостаточна для схемы и полезной нагрузки
	if(text.size() < (scheme.size() + 1))
		// Сообщаем о неудачном извлечении
		return false;
	// Сравниваем название схемы без учёта регистра (RFC 7235)
	if(!this->_fmk->compare(text.substr(0, scheme.size()), scheme))
		// Сообщаем о неудачном извлечении
		return false;
	// Перемещаем индекс за название схемы
	size_t index = scheme.size();
	// После названия схемы должен быть пробельный символ
	if((index >= text.size()) || !awh::ascii::isSpace(text[index]))
		// Сообщаем о неудачном извлечении
		return false;
	/**
	 * Пропускаем пробелы после названия схемы
	 */
	while((index < text.size()) && awh::ascii::isSpace(text[index]))
		// Переходим к следующему символу
		++index;
	// Если полезная нагрузка отсутствует
	if(index >= text.size())
		// Сообщаем о неудачном извлечении
		return false;
	// Извлекаем полезную нагрузку
	payload = text.substr(index);
	// Подтверждаем успешное извлечение
	return true;
}
/**
 * @brief Метод формирования набора исходящих заголовков авторизации
 *
 * @details Базовая реализация формирует один заголовок (имя -> значение).
 *          Многозаголовочные схемы (HMAC) переопределяют метод.
 *
 * @param result контейнер для набора заголовков (имя -> значение)
 *
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
 *
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
 *
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
 *
 */
awh::http::Authorization::owner_t awh::http::Authorization::owner() const noexcept {
	// Выводим сторону работы модуля
	return this->_owner;
}
/**
 * @brief Метод получения типа авторизации
 *
 * @return тип авторизации
 *
 */
awh::http::Authorization::type_t awh::http::Authorization::type() const noexcept {
	// Выводим тип авторизации
	return this->_type;
}
/**
 * @brief Метод установки типа авторизации
 *
 * @details Перед активацией новой стратегии вызывает reset().
 *
 * @param type тип авторизации для установки
 * @param hash алгоритм хэширования (для DIGEST/HMAC)
 *
 */
void awh::http::Authorization::type(const type_t type, const hash_t hash) noexcept {
	// Временная стратегия выбранной схемы авторизации
	unique_ptr <scheme_t> scheme = nullptr;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем тип авторизации
		 */
		switch(static_cast <uint8_t> (type)){
			// Если тип авторизации HMAC
			case static_cast <uint8_t> (type_t::HMAC):
				// Создаём стратегию HMAC-авторизации
				scheme = make_unique <hmac_t> (this->_owner, this->_params, &this->_crypto, this->_fmk, this->_log);
			break;
			// Если тип авторизации BASIC
			case static_cast <uint8_t> (type_t::BASIC):
				// Создаём стратегию BASIC-авторизации
				scheme = make_unique <basic_t> (this->_owner, this->_params, &this->_crypto, this->_fmk, this->_log);
			break;
			// Если тип авторизации DIGEST
			case static_cast <uint8_t> (type_t::DIGEST):
				// Создаём стратегию DIGEST-авторизации
				scheme = make_unique <digest_scheme_t> (this->_owner, this->_params, &this->_crypto, this->_fmk, this->_log);
			break;
			// Если тип авторизации BEARER
			case static_cast <uint8_t> (type_t::BEARER):
				// Создаём стратегию BEARER-авторизации
				scheme = make_unique <bearer_t> (this->_owner, this->_params, &this->_crypto, this->_fmk, this->_log);
			break;
		}
		// Сбрасываем временное состояние только после успешного создания стратегии
		this->reset();
		// Устанавливаем тип авторизации
		this->_type = type;
		// Устанавливаем текущий алгоритм хэширования
		this->_params.hash = hash;
		// Устанавливаем алгоритм хэширования схемы
		this->_params.scheme = hash;
		// Активируем новую стратегию
		this->_scheme = ::move(scheme);
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
 *
 */
void awh::http::Authorization::user(string_view user) noexcept {
	// Устанавливаем логин пользователя
	this->_params.user = user;
}
/**
 * @brief Метод установки пароля пользователя
 *
 * @details Для BASIC пароль передаётся как есть; символ «:» в пароле
 *          не поддерживается (RFC 7617: user-pass = userid \":\" password).
 *
 * @param pass пароль пользователя
 *
 */
void awh::http::Authorization::pass(string_view pass) noexcept {
	// Устанавливаем пароль пользователя
	this->_params.pass = pass;
}
/**
 * @brief Метод установки токена доступа (BEARER)
 *
 * @param token токен доступа
 *
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
 *
 */
void awh::http::Authorization::proxy(const bool mode) noexcept {
	// Устанавливаем режим работы через прокси
	this->_params.mode.proxy = mode;
}
/**
 * @brief Метод получения режима строгости проверки учётных данных
 *
 * @return режим строгости проверки (SIMPLE/STRICT)
 *
 */
awh::http::Authorization::mode_t awh::http::Authorization::mode() const noexcept {
	// Выводим режим строгости проверки учётных данных
	return this->_params.mode.validation;
}
/**
 * @brief Метод установки режима строгости проверки учётных данных (сервер)
 *
 * @details SIMPLE (по умолчанию) — совместимый режим: сервер принимает Digest legacy
 *          RFC 2069 (без qop), ключи параметров HMAC Signature-Input сверяются без
 *          учёта регистра. STRICT — строгое соответствие RFC: Digest без qop
 *          отклоняется (RFC 7616), параметры подписи HMAC сверяются байт-точно
 *          (RFC 9421). Влияет только на проверку на стороне сервера.
 *
 * @param mode режим строгости проверки (SIMPLE/STRICT)
 *
 */
void awh::http::Authorization::mode(const mode_t mode) noexcept {
	// Устанавливаем режим строгости проверки учётных данных
	this->_params.mode.validation = mode;
}
/**
 * @brief Метод установки секретного ключа подписи (HMAC)
 *
 * @param key секретный ключ подписи
 *
 */
void awh::http::Authorization::key(string_view key) noexcept {
	// Устанавливаем секретный ключ подписи
	this->_params.sign.key = key;
}
/**
 * @brief Метод установки идентификатора ключа подписи (HMAC)
 *
 * @param keyId идентификатор ключа подписи
 *
 */
void awh::http::Authorization::keyId(string_view keyId) noexcept {
	// Устанавливаем идентификатор ключа подписи
	this->_params.sign.keyId = keyId;
}
/**
 * @brief Метод установки метки подписи (HMAC)
 *
 * @param label метка подписи (например, sig1)
 *
 */
void awh::http::Authorization::label(string_view label) noexcept {
	// Если метка подписи передана
	if(!label.empty())
		// Устанавливаем метку подписи
		this->_params.sign.label = label;
}
/**
 * @brief Метод установки одноразового значения подписи (HMAC)
 *
 * @details Вызывается на клиенте **до** headers()/header(). Значение включается
 *          в Signature-Input и участвует в расчёте подписи. На сервере повторная
 *          проверка подписи с тем же nonce отклоняется (защита от replay).
 *
 * @param nonce одноразовое значение
 *
 */
void awh::http::Authorization::signNonce(string_view nonce) noexcept {
	// Устанавливаем одноразовое значение подписи
	this->_params.sign.nonce = nonce;
}
/**
 * @brief Метод установки штампа времени создания подписи (HMAC)
 *
 * @details Вызывается на клиенте **до** headers()/header(). Значение попадает
 *          в Signature-Input и участвует в канонической базе подписи.
 *          Если передать 0, при формировании подписи будет использован текущий
 *          штамп времени (fmk_t::timestamp).
 *
 * @param stamp штамп времени в секундах (0 — автоматически при формировании)
 *
 */
void awh::http::Authorization::signCreated(const uint64_t stamp) noexcept {
	// Устанавливаем штамп времени создания подписи
	this->_params.sign.date.created = stamp;
}
/**
 * @brief Метод установки штампа времени истечения подписи (HMAC)
 *
 * @details Вызывается на клиенте **до** headers()/header(). Сервер отклоняет
 *          подпись, если текущее время превышает expires (с учётом clockSkew).
 *          Значение 0 означает, что срок действия не ограничен.
 *
 * @param stamp штамп времени в секундах (0 — не задано)
 *
 */
void awh::http::Authorization::signExpires(const uint64_t stamp) noexcept {
	// Устанавливаем штамп времени истечения подписи
	this->_params.sign.date.expires = stamp;
}
/**
 * @brief Метод добавления покрываемого подписью компонента (HMAC)
 *
 * @details Порядок добавления компонентов сохраняется. Производные компоненты
 *          начинаются с символа '@' (например, "@method", "@path", "@authority")
 *
 * @param name  имя компонента
 * @param value значение компонента
 *
 */
void awh::http::Authorization::component(string_view name, string_view value) noexcept {
	// Если имя компонента передано
	if(!name.empty()){
		// Формируем ключ компонента в нижнем регистре
		string key(name);
		// Приводим ключ компонента к нижнему регистру
		this->_fmk->transform(key, fmk_t::transform_t::LOWER_CASE);
		// Если компонент с таким именем уже добавлен
		if(const auto it = this->_params.sign.componentIndex.find(key); it != this->_params.sign.componentIndex.end())
			// Обновляем значение существующего компонента
			this->_params.sign.components.at(it->second).second = value;
		// Если компонент добавляется впервые
		else {
			// Добавляем компонент в список покрываемых подписью
			this->_params.sign.componentIndex.emplace(key, this->_params.sign.components.size());
			// Сохраняем компонент с исходным именем
			this->_params.sign.components.emplace_back(name, value);
		}
	}
}
/**
 * @brief Метод установки параметров HTTP-запроса (DIGEST, клиент)
 *
 * @param uri параметры HTTP-запроса (request-uri)
 *
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
 *
 */
void awh::http::Authorization::method(string_view method) noexcept {
	// Если HTTP-метод запроса передан
	if(!method.empty())
		// Устанавливаем HTTP-метод запроса
		this->_params.method = method;
}
/**
 * @brief Метод установки тела запроса (DIGEST, qop=auth-int)
 *
 * @details Для qop=auth-int необходимо явно передать entity-body запроса
 *          до формирования или проверки учётных данных.
 *
 * @param entity тело HTTP-запроса (entity-body)
 *
 */
void awh::http::Authorization::entity(string_view entity) noexcept {
	// Устанавливаем тело HTTP-запроса
	this->_params.digest.entity = entity;
}
/**
 * @brief Метод установки названия сервера (realm)
 *
 * @param realm название сервера
 *
 */
void awh::http::Authorization::realm(string_view realm) noexcept {
	// Устанавливаем название сервера
	this->_params.digest.realm = realm;
}
/**
 * @brief Метод установки уникального ключа сервера (nonce)
 *
 * @param nonce уникальный ключ, выдаваемый сервером
 *
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
 *
 */
void awh::http::Authorization::opaque(string_view opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty()){
		// Устанавливаем временный ключ сессии сервера
		this->_params.digest.opaque = opaque;
		// На стороне сервера фиксируем opaque как выданный (для сверки при проверке)
		if(this->_owner == owner_t::SERVER)
			// Запоминаем фактически выданный opaque
			this->_params.digest.issuedOpaque = opaque;
	}
}
/**
 * @brief Метод установки сессионного режима алгоритма Digest (-sess)
 *
 * @details При включении используется сессионный вариант расчёта HA1:
 *          HA1 = H(H(user:realm:pass):nonce:cnonce), а в имени алгоритма
 *          добавляется суффикс -sess (например, SHA-256-sess)
 *
 * @param mode флаг сессионного режима алгоритма
 *
 */
void awh::http::Authorization::session(const bool mode) noexcept {
	// Устанавливаем сессионный режим алгоритма Digest
	this->_params.digest.mode.sess = mode;
}
/**
 * @brief Метод получения допустимого расхождения локальных часов (HMAC)
 *
 * @return допуск в секундах (по умолчанию 60)
 *
 */
uint64_t awh::http::Authorization::clockSkew() const noexcept {
	// Выводим допуск расхождения локальных часов
	return this->_params.mode.clockSkew;
}
/**
 * @brief Метод установки допустимого расхождения локальных часов (HMAC, секунды)
 *
 * @details Используется на сервере при check(): подпись принимается, если
 *          created не более чем на clockSkew секунд в будущем относительно
 *          серверного времени, а expires не более чем на clockSkew секунд
 *          в прошлом. Значение по умолчанию — 60. Передайте 0 для строгой
 *          проверки без допуска.
 *
 * @param seconds допуск при проверке created/expires (0 — только точное совпадение)
 *
 */
void awh::http::Authorization::clockSkew(const uint64_t seconds) noexcept {
	// Устанавливаем допуск расхождения локальных часов
	this->_params.mode.clockSkew = seconds;
}
/**
 * @brief Метод получения максимального возраста HMAC-подписи без expires
 *
 * @return лимит в секундах (0 — не ограничен)
 *
 */
uint64_t awh::http::Authorization::signMaxAge() const noexcept {
	// Выводим максимальный возраст подписи без expires
	return this->_params.mode.signMaxAge;
}
/**
 * @brief Метод установки максимального возраста HMAC-подписи без expires (секунды)
 *
 * @details Используется на сервере при check(), если клиент не передал expires.
 *          Подпись отклоняется, когда now > created + signMaxAge + clockSkew.
 *          Значение 0 (по умолчанию) — ограничение не применяется.
 *          Для production-серверов HMAC рекомендуется задавать ненулевой лимит.
 *
 * @param seconds максимальный возраст подписи (0 — без ограничения)
 *
 */
void awh::http::Authorization::signMaxAge(const uint64_t seconds) noexcept {
	// Устанавливаем максимальный возраст подписи без expires
	this->_params.mode.signMaxAge = seconds;
}
/**
 * @brief Метод получения максимального возраста Digest-nonce
 *
 * @return лимит в секундах (0 — без ограничения по времени)
 *
 */
uint64_t awh::http::Authorization::nonceMaxAge() const noexcept {
	// Выводим максимальный возраст Digest-nonce
	return this->_params.mode.nonceMaxAge;
}
/**
 * @brief Метод установки максимального возраста Digest-nonce (секунды)
 *
 * @details Используется на сервере: nonce считается устаревшим, если с момента
 *          его выдачи прошло больше указанного времени. Устаревший nonce отклоняется
 *          при проверке и перевыпускается при формировании нового вызова.
 *          Значение по умолчанию — 1800 (30 минут). Передайте 0, чтобы отключить
 *          ограничение по времени жизни nonce.
 *
 * @param seconds максимальный возраст nonce (0 — без ограничения)
 *
 */
void awh::http::Authorization::nonceMaxAge(const uint64_t seconds) noexcept {
	// Устанавливаем максимальный возраст Digest-nonce
	this->_params.mode.nonceMaxAge = seconds;
}
/**
 * @brief Метод получения максимального возраста HMAC-подписи без expires для строгого режима
 *
 * @return лимит в секундах (0 — не ограничен)
 *
 */
uint64_t awh::http::Authorization::signStrictMaxAge() const noexcept {
	// Выводим максимальный возраст подписи без expires для строгого режима
	return this->_params.mode.signStrictMaxAge;
}
/**
 * @brief Метод установки максимального возраста HMAC-подписи без expires для строгого режима (секунды)
 *
 * @details Используется на сервере при check() в строгом режиме (STRICT), если клиент
 *          не передал expires и не задан общий signMaxAge. Значение по умолчанию — 300.
 *          Передайте 0, чтобы отключить ограничение по умолчанию даже в строгом режиме.
 *
 * @param seconds максимальный возраст подписи в строгом режиме (0 — без ограничения)
 *
 */
void awh::http::Authorization::signStrictMaxAge(const uint64_t seconds) noexcept {
	// Устанавливаем максимальный возраст подписи без expires для строгого режима
	this->_params.mode.signStrictMaxAge = seconds;
}
/**
 * @brief Метод проверки учётных данных (только для сервера)
 *
 * @details На стороне CLIENT всегда возвращает true (проверка не выполняется).
 *
 * @return результат проверки
 *
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
 * @brief Метод сброса временного состояния схемы авторизации
 *
 * @details Очищает накопленное между запросами состояние:
 *          - Digest: mode.qop, mode.authInt, mode.stamp, nc, nonce, issued, opaque,
 *            cnonce, response, issuedOpaque, lncs (realm, uri, entity, mode.sess сохраняются);
 *          - HMAC: date.created, date.expires, nonce, params, signature, covered,
 *            components, componentIndex, usedNonces (key, keyId, label, tag сохраняются).
 *
 *          Не затрагивает: callbacks, user, pass, token, key, keyId, realm,
 *          uri, method, entity, mode.proxy, mode.clockSkew, mode.signMaxAge, hash,
 *          digest.mode.sess.
 *
 *          Вызывается автоматически из type(). Имеет смысл вызывать вручную
 *          при повторном цикле авторизации на том же auth_t без смены схемы.
 *
 */
void awh::http::Authorization::reset() noexcept {
	/**
	 * Сбрасываем временное состояние Digest (realm, uri, entity и mode.sess сохраняются)
	 */
	auth_t::digest_t & digest = this->_params.digest;
	digest.lncs.clear();
	digest.nonce.clear();
	digest.issued.clear();
	digest.opaque.clear();
	digest.cnonce.clear();
	digest.response.clear();
	digest.lncsOrder.clear();
	digest.issuedOpaque.clear();
	digest.mode.stamp   = 0;
	digest.mode.qop     = false;
	digest.mode.authInt = false;
	digest.qop          = "auth";
	digest.nc           = "00000000";
	/**
	 * Сбрасываем временное состояние HMAC (ключ и keyId сохраняются)
	 */
	auth_t::sign_t & sign = this->_params.sign;
	sign.nonce.clear();
	sign.params.clear();
	sign.covered.clear();
	sign.signature.clear();
	sign.components.clear();
	sign.inputLabel.clear();
	sign.usedNonces.clear();
	sign.componentIndex.clear();
	sign.usedNoncesOrder.clear();
	sign.date.created = 0;
	sign.date.expires = 0;
}
/**
 * @brief Метод формирования исходящего заголовка авторизации
 *
 * @details Клиент формирует учётные данные (Authorization),
 *          сервер формирует вызов авторизации (WWW-Authenticate)
 *
 * @param full режим вывода вместе с именем заголовка
 * @return     значение заголовка авторизации
 *
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
 *
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
 * @details На сервере перед разбором очищаются учётные данные предыдущего
 *          запроса (user/pass/token, response, nonce, nc и др.), hash
 *          восстанавливается до значения type(), digest.mode.sess сбрасывается.
 *          Для HMAC очистка выполняется только перед разбором Signature-Input
 *          (значение с «(»), чтобы двухшаговый разбор через parse(header)
 *          не затирал уже разобранный Signature-Input.
 *          Для Digest realm, заданный через realm(), не перезаписывается из заголовка
 *          клиента; несовпадение realm отклоняется при parse(). На сервере
 *          Digest требует username и response в учётных данных.
 *
 * @param header значение заголовка (клиент: вызов сервера, сервер: учётные данные)
 * @return       результат разбора
 *
 */
bool awh::http::Authorization::parse(const string_view header) noexcept {
	// На сервере очищаем учётные данные предыдущего запроса перед разбором
	if((this->_owner == owner_t::SERVER) && (this->_scheme != nullptr)){
		// HMAC: очистка только перед Signature-Input (значение содержит список компонентов)
		if(this->_type == type_t::HMAC){
			// Если заголовок содержит «(», значит это Signature-Input
			if(header.find('(') != string_view::npos)
				// Очищаем разобранные учётные данные предыдущего запроса
				::clearParsedCredentials(this->_params);
		// Если тип авторизации не HMAC, очищаем учётные данные всегда
		} else ::clearParsedCredentials(this->_params);
	}
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
 *
 */
bool awh::http::Authorization::parse(const string_view name, const string_view header) noexcept {
	// На сервере очищаем stale-данные в начале разбора Signature-Input
	if((this->_owner == owner_t::SERVER) && (this->_scheme != nullptr)){
		// Устанавливаем имя заголовка в нижнем регистре для сравнения
		string field(name);
		// Приводим имя заголовка к нижнему регистру
		this->_fmk->transform(field, fmk_t::transform_t::LOWER_CASE);
		// Если это заголовок Signature-Input, очищаем разобранные учётные данные
		if(field.compare("signature-input") == 0)
			// Очищаем разобранные учётные данные предыдущего запроса
			::clearParsedCredentials(this->_params);
	}
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
 *
 */
void awh::http::Authorization::callbackCheckToken(function <bool (const string &)> callback) noexcept {
	// Устанавливаем функцию проверки токена доступа
	this->_params.callback.checkToken = ::move(callback);
}
/**
 * @brief Метод установки функции извлечения секретного ключа (HMAC, сервер)
 *
 * @param callback функция извлечения секретного ключа по идентификатору
 *
 */
void awh::http::Authorization::callbackExtractKey(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию извлечения секретного ключа
	this->_params.callback.extractKey = ::move(callback);
}
/**
 * @brief Метод установки функции извлечения пароля (DIGEST, сервер)
 *
 * @param callback функция извлечения пароля по логину
 *
 */
void awh::http::Authorization::callbackExtractPass(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию извлечения пароля
	this->_params.callback.extractPass = ::move(callback);
}
/**
 * @brief Метод установки функции проверки пары «логин/пароль» (BASIC, сервер)
 *
 * @param callback функция проверки пары «логин/пароль»
 *
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
 *
 */
awh::http::Authorization::Authorization(const owner_t owner, const fmk_t * fmk, const log_t * log) noexcept :
 _type(type_t::NONE), _owner(owner), _crypto(fmk, log), _scheme(nullptr), _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::http::Authorization::~Authorization() noexcept {}
