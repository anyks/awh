/**
 * @file: auth.hpp
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
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_AUTH__
#define __AWH_AUTH__

/**
 * Стандартные заголовочные файлы
 */
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>
#include <functional>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../../sys/fmk.hpp"
#include "../../../sys/log.hpp"
#include "../../../sys/crypto.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён HTTP-протокола
	 *
	 */
	namespace http {
		/**
		 * @brief Единый класс HTTP-авторизации (клиент/сервер)
		 *
		 * @details Один класс обслуживает обе стороны обмена: поведение методов
		 *          parse()/header()/check() определяется флагом owner_t (CLIENT/SERVER),
		 *          а конкретная схема (Basic/Digest/Bearer/HMAC) выбирается методом type()
		 *          и реализуется внутренней стратегией scheme_t.
		 *
		 * @par Пример использования (клиент)
		 * @code{.cpp}
		 * // Создаём модуль авторизации на стороне клиента
		 * auth_t auth(fmk, log, auth_t::owner_t::CLIENT);
		 * // Выбираем схему DIGEST с алгоритмом SHA-256
		 * auth.type(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
		 * // При необходимости включаем сессионный режим алгоритма (SHA-256-sess)
		 * auth.session(true);
		 * // Указываем учётные данные и параметры запроса
		 * auth.user("login");
		 * auth.pass("secret");
		 * auth.uri("/api/resource");
		 * auth.method("GET");
		 * // Разбираем вызов авторизации, полученный от сервера (заголовок WWW-Authenticate)
		 * auth.parse(wwwAuthenticate);
		 * // Формируем заголовок учётных данных для запроса (значение заголовка Authorization)
		 * const string & credentials = auth.header();
		 * @endcode
		 *
		 * @par Пример использования (сервер)
		 * @code{.cpp}
		 * // Создаём модуль авторизации на стороне сервера
		 * auth_t auth(fmk, log, auth_t::owner_t::SERVER);
		 * // Выбираем схему DIGEST с алгоритмом SHA-256
		 * auth.type(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
		 * // При необходимости включаем сессионный режим алгоритма (SHA-256-sess)
		 * auth.session(true);
		 * // Указываем название сервера (realm) и HTTP-метод запроса
		 * auth.realm("anyks.com");
		 * auth.method("GET");
		 * // Регистрируем функцию извлечения пароля пользователя по его логину
		 * auth.callbackExtractPass([](const string & user) -> string {
		 *     return db.password(user);
		 * });
		 * // Разбираем учётные данные клиента (заголовок Authorization)
		 * auth.parse(authorization);
		 * // Проверяем учётные данные, при неудаче отправляем клиенту вызов авторизации
		 * if(!auth.check())
		 *     response.header("WWW-Authenticate", auth.header());
		 * @endcode
		 *
		 * @par Пример использования (клиент, подпись запроса HMAC, RFC 9421)
		 * @code{.cpp}
		 * // Создаём модуль авторизации на стороне клиента
		 * auth_t auth(fmk, log, auth_t::owner_t::CLIENT);
		 * // Выбираем схему подписи HMAC с алгоритмом SHA-256
		 * auth.type(auth_t::type_t::HMAC, auth_t::hash_t::SHA256);
		 * // Указываем секретный ключ и его идентификатор
		 * auth.key("secret-shared-key");
		 * auth.keyId("test-key");
		 * // Перечисляем покрываемые подписью компоненты запроса (порядок важен)
		 * auth.component("@method", "POST");
		 * auth.component("@authority", "example.com");
		 * auth.component("@path", "/foo");
		 * // Получаем набор заголовков подписи (Signature-Input и Signature)
		 * vector <pair <string, string>> headers;
		 * auth.headers(headers);
		 * @endcode
		 */
		typedef class __AWH_SHARED_EXPORT__ Authorization {
			public:
				/**
				 * @brief Псевдоним типа хэш-суммы (переиспользуем алгоритмы модуля криптографии)
				 *
				 */
				using hash_t = crypto_t::hash_t;
			public:
				/**
				 * @brief Сторона, от имени которой работает модуль
				 *
				 */
				enum class owner_t : uint8_t {
					NONE   = 0x00, // Сторона не установлена
					CLIENT = 0x01, // Клиент (отправляет учётные данные, читает вызов сервера)
					SERVER = 0x02  // Сервер (проверяет учётные данные, отправляет вызов клиенту)
				};
				/**
				 * @brief Тип (схема) авторизации
				 *
				 */
				enum class type_t : uint8_t {
					NONE   = 0x00, // Авторизация не установлена
					HMAC   = 0x01, // Авторизация подписью запроса HMAC
					BASIC  = 0x02, // BASIC авторизация (RFC 7617)
					DIGEST = 0x03, // DIGEST авторизация (RFC 7616)
					BEARER = 0x04  // BEARER/Token авторизация (RFC 6750)
				};
			public:
				/**
				 * @brief Структура параметров Digest-авторизации
				 *
				 */
				typedef struct Digest {
					// Флаг использования сессионного алгоритма (-sess)
					bool sess;
					// Штамп времени последней генерации nonce в секундах (сервер)
					uint64_t stamp;
					// Счётчик запросов клиента (nonce count)
					string nc;
					// Последний принятый сервером счётчик запросов (защита от повторов)
					string lnc;
					// Параметры HTTP-запроса (request-uri)
					string uri;
					// Тип защиты (quality of protection)
					string qop;
					// Название сервера или realm
					string realm;
					// Уникальный ключ, выдаваемый сервером
					string nonce;
					// Уникальный ключ, фактически выданный сервером (для сверки при проверке)
					string issued;
					// Временный ключ сессии сервера
					string opaque;
					// Уникальный ключ, генерируемый клиентом
					string cnonce;
					// Результат ответа клиента
					string response;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Digest() noexcept :
					 sess(false), stamp(0),
					 nc{"00000000"}, lnc{"00000000"},
					 uri{""}, qop{"auth"}, realm{""},
					 nonce{""}, issued{""}, opaque{""},
					 cnonce{""}, response{""} {}
				} digest_t;
				/**
				 * @brief Структура параметров авторизации подписью HMAC (RFC 9421)
				 *
				 */
				typedef struct Sign {
					// Штамп времени создания подписи в секундах
					uint64_t created;
					// Штамп времени истечения подписи в секундах (0 — не задано)
					uint64_t expires;
					// Секретный ключ подписи (HMAC)
					string key;
					// Тег приложения (опционально)
					string tag;
					// Идентификатор ключа подписи
					string keyId;
					// Метка подписи (например, sig1)
					string label;
					// Одноразовое значение подписи (опционально)
					string nonce;
					// Сырое значение параметров подписи (@signature-params из Signature-Input)
					string params;
					// Разобранная подпись клиента в формате BASE64 (сервер)
					string signature;
					// Порядок покрываемых подписью компонентов (имена)
					vector <string> covered;
					// Значения покрываемых компонентов (имя -> значение)
					vector <pair <string, string>> components;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Sign() noexcept :
					 created(0), expires(0),
					 key{""}, tag{""},
					 keyId{""}, label{"sig1"},
					 nonce{""}, params{""}, signature{""} {}
				} sign_t;
				/**
				 * @brief Структура обратных вызовов для проверки учётных данных (сервер)
				 *
				 */
				typedef struct Callback {
					// Внешняя функция проверки токена доступа (BEARER, сервер)
					function <bool (const string &)> checkToken;
					// Внешняя функция извлечения секретного ключа по идентификатору (HMAC, сервер)
					function <string (const string &)> extractKey;
					// Внешняя функция извлечения пароля по логину (DIGEST, сервер)
					function <string (const string &)> extractPass;
					// Внешняя функция проверки пары «логин/пароль» (BASIC, сервер)
					function <bool (const string &, const string &)> checkUser;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Callback() noexcept :
					 checkToken(nullptr), extractKey(nullptr),
					 extractPass(nullptr), checkUser(nullptr) {}
				} callback_t;
				/**
				 * @brief Структура общих параметров авторизации
				 *
				 */
				typedef struct Params {
					// Флаг работы через прокси (Proxy-Authorization/Proxy-Authenticate)
					bool proxy;
					// Логин пользователя
					string user;
					// Пароль пользователя
					string pass;
					// Токен доступа (для BEARER)
					string token;
					// HTTP-метод запроса (для расчёта ответа DIGEST)
					string method;
					// Алгоритм хэширования (для DIGEST/HMAC)
					hash_t hash;
					// Параметры авторизации подписью HMAC
					sign_t sign;
					// Параметры Digest-авторизации
					digest_t digest;
					// Функции обратных вызовов для проверки учётных данных (сервер)
					callback_t callback;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Params() noexcept :
					 proxy(false), user{""}, pass{""},
					 token{""}, method{"GET"}, hash(hash_t::MD5) {}
				} params_t;
			public:
				/**
				 * @brief Абстрактный интерфейс схемы авторизации (внутренняя стратегия)
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Scheme {
					protected:
						// Сторона работы (клиент/сервер)
						owner_t _owner;
						// Общие параметры авторизации (принадлежат фасаду)
						params_t & _params;
					protected:
						// Объект фреймворка
						const fmk_t * _fmk;
						// Объект работы с логами
						const log_t * _log;
						// Объект криптографии (хэш/HMAC/BASE64)
						const crypto_t * _crypto;
					protected:
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
						string name() const noexcept;
					public:
						/**
						 * @brief Метод проверки учётных данных (только для сервера)
						 *
						 * @return результат проверки
						 */
						virtual bool check() noexcept = 0;
					public:
						/**
						 * @brief Метод формирования исходящего заголовка авторизации
						 *
						 * @param full режим вывода вместе с именем заголовка
						 * @return     значение заголовка авторизации
						 */
						virtual string header(const bool full = false) noexcept = 0;
						/**
						 * @brief Метод формирования набора исходящих заголовков авторизации
						 *
						 * @details Базовая реализация формирует один заголовок (имя -> значение).
						 *          Многозаголовочные схемы (HMAC) переопределяют метод.
						 *
						 * @param result контейнер для набора заголовков (имя -> значение)
						 */
						virtual void headers(vector <pair <string, string>> & result) noexcept;
					public:
						/**
						 * @brief Метод разбора входящего заголовка авторизации
						 *
						 * @param header значение заголовка (клиент: вызов, сервер: учётные данные)
						 * @return       результат разбора
						 */
						virtual bool parse(const string_view header) noexcept = 0;
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
						virtual bool parse(const string_view name, const string_view header) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param owner  сторона работы (клиент/сервер)
						 * @param params общие параметры авторизации
						 * @param crypto объект криптографии
						 * @param fmk    объект фреймворка
						 * @param log    объект для работы с логами
						 */
						explicit Scheme(const owner_t owner, params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept;
						/**
						 * @brief Деструктор
						 *
						 */
						virtual ~Scheme() noexcept;
				} scheme_t;
			protected:
				// Тип (схема) авторизации
				type_t _type;
				// Сторона работы модуля (клиент/сервер)
				owner_t _owner;
			protected:
				// Общие параметры авторизации
				params_t _params;
			protected:
				// Объект криптографии
				crypto_t _crypto;
				// Активная стратегия выбранной схемы авторизации
				unique_ptr <scheme_t> _scheme;
			protected:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод получения стороны работы модуля
				 *
				 * @return сторона работы (клиент/сервер)
				 */
				owner_t owner() const noexcept;
			public:
				/**
				 * @brief Метод получения типа авторизации
				 *
				 * @return тип авторизации
				 */
				type_t type() const noexcept;
				/**
				 * @brief Метод установки типа авторизации
				 *
				 * @param type тип авторизации для установки
				 * @param hash алгоритм хэширования (для DIGEST/HMAC)
				 */
				void type(const type_t type, const hash_t hash = hash_t::MD5) noexcept;
			public:
				/**
				 * @brief Метод установки логина пользователя
				 *
				 * @param user логин пользователя
				 */
				void user(string_view user) noexcept;
				/**
				 * @brief Метод установки пароля пользователя
				 *
				 * @param pass пароль пользователя
				 */
				void pass(string_view pass) noexcept;
				/**
				 * @brief Метод установки токена доступа (BEARER)
				 *
				 * @param token токен доступа
				 */
				void token(string_view token) noexcept;
			public:
				/**
				 * @brief Метод установки режима работы через прокси
				 *
				 * @details Влияет на имена заголовков: Proxy-Authorization/Proxy-Authenticate
				 *          вместо Authorization/WWW-Authenticate
				 *
				 * @param mode флаг работы через прокси
				 */
				void proxy(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод установки секретного ключа подписи (HMAC)
				 *
				 * @param key секретный ключ подписи
				 */
				void key(string_view key) noexcept;
				/**
				 * @brief Метод установки идентификатора ключа подписи (HMAC)
				 *
				 * @param keyId идентификатор ключа подписи
				 */
				void keyId(string_view keyId) noexcept;
				/**
				 * @brief Метод установки метки подписи (HMAC)
				 *
				 * @param label метка подписи (например, sig1)
				 */
				void label(string_view label) noexcept;
				/**
				 * @brief Метод добавления покрываемого подписью компонента (HMAC)
				 *
				 * @details Порядок добавления компонентов сохраняется. Производные компоненты
				 *          начинаются с символа '@' (например, "@method", "@path", "@authority")
				 *
				 * @param name  имя компонента
				 * @param value значение компонента
				 */
				void component(string_view name, string_view value) noexcept;
			public:
				/**
				 * @brief Метод установки параметров HTTP-запроса (DIGEST, клиент)
				 *
				 * @param uri параметры HTTP-запроса (request-uri)
				 */
				void uri(string_view uri) noexcept;
				/**
				 * @brief Метод установки HTTP-метода запроса (DIGEST)
				 *
				 * @param method HTTP-метод запроса
				 */
				void method(string_view method) noexcept;
			public:
				/**
				 * @brief Метод установки названия сервера (realm)
				 *
				 * @param realm название сервера
				 */
				void realm(string_view realm) noexcept;
				/**
				 * @brief Метод установки уникального ключа сервера (nonce)
				 *
				 * @param nonce уникальный ключ, выдаваемый сервером
				 */
				void nonce(string_view nonce) noexcept;
				/**
				 * @brief Метод установки временного ключа сессии сервера (opaque)
				 *
				 * @param opaque временный ключ сессии сервера
				 */
				void opaque(string_view opaque) noexcept;
			public:
				/**
				 * @brief Метод установки сессионного режима алгоритма Digest (-sess)
				 *
				 * @details При включении используется сессионный вариант расчёта HA1:
				 *          HA1 = H(H(user:realm:pass):nonce:cnonce), а в имени алгоритма
				 *          добавляется суффикс -sess (например, SHA-256-sess)
				 *
				 * @param mode флаг сессионного режима алгоритма
				 */
				void session(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод проверки учётных данных (только для сервера)
				 *
				 * @return результат проверки
				 */
				bool check() noexcept;
			public:
				/**
				 * @brief Метод формирования исходящего заголовка авторизации
				 *
				 * @details Клиент формирует учётные данные (Authorization),
				 *          сервер формирует вызов авторизации (WWW-Authenticate)
				 *
				 * @param full режим вывода вместе с именем заголовка
				 * @return     значение заголовка авторизации
				 */
				string header(const bool full = false) noexcept;
				/**
				 * @brief Метод формирования набора исходящих заголовков авторизации
				 *
				 * @details Универсальный способ получения заголовков: для одно-заголовочных схем
				 *          (Basic/Digest/Bearer) возвращается один элемент, для HMAC — два
				 *          (Signature-Input и Signature)
				 *
				 * @param result контейнер для набора заголовков (имя -> значение)
				 */
				void headers(vector <pair <string, string>> & result) noexcept;
			public:
				/**
				 * @brief Метод разбора входящего заголовка авторизации
				 *
				 * @param header значение заголовка (клиент: вызов сервера, сервер: учётные данные)
				 * @return       результат разбора
				 */
				bool parse(const string_view header) noexcept;
				/**
				 * @brief Метод разбора входящего заголовка авторизации с указанием имени
				 *
				 * @details Требуется многозаголовочным схемам (HMAC: Signature-Input и Signature)
				 *
				 * @param name   имя входящего заголовка
				 * @param header значение входящего заголовка
				 * @return       результат разбора
				 */
				bool parse(const string_view name, const string_view header) noexcept;
			public:
				/**
				 * @brief Метод установки функции проверки токена доступа (BEARER, сервер)
				 *
				 * @param callback функция проверки токена доступа
				 */
				void callbackCheckToken(function <bool (const string &)> callback) noexcept;
				/**
				 * @brief Метод установки функции извлечения секретного ключа (HMAC, сервер)
				 *
				 * @param callback функция извлечения секретного ключа по идентификатору
				 */
				void callbackExtractKey(function <string (const string &)> callback) noexcept;
				/**
				 * @brief Метод установки функции извлечения пароля (DIGEST, сервер)
				 *
				 * @param callback функция извлечения пароля по логину
				 */
				void callbackExtractPass(function <string (const string &)> callback) noexcept;
				/**
				 * @brief Метод установки функции проверки пары «логин/пароль» (BASIC, сервер)
				 *
				 * @param callback функция проверки пары «логин/пароль»
				 */
				void callbackCheckUser(function <bool (const string &, const string &)> callback) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param owner сторона работы (клиент/сервер)
				 * @param fmk   объект фреймворка
				 * @param log   объект для работы с логами
				 */
				explicit Authorization(const owner_t owner, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Authorization() noexcept;
		} auth_t;
	};
};

#endif // __AWH_AUTH__
