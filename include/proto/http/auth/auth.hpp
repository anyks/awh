/**
 * @file auth.hpp
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
 * \~russian
 * @brief Заголовочный файл модуля HTTP-авторизации — класс http::Authorization, объединяющий схемы авторизации,
 *        генерацию и проверку заголовков, управление nonce и защиту от replay-атак на стороне клиента и сервера
 *
 * \~english
 * @brief Header file of the module of the HTTP authorization — the class http::Authorization uniting the schemes of the authorization,
 *        the generation and the checking of the headers, the control of the nonce and the protection from the replay attacks on the side of the client and of the server
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_AUTH__
#define __AWH_AUTH__

/**
 * Стандартные заголовочные файлы
 */
#include <list>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>
#include <functional>
#include <string_view>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../../sys/fmk.hpp"
#include "../../../sys/log.hpp"
#include "../../../cryptography/crypto.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../../sys/macro_push.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён HTTP-протокола
	 *
	 *
	 * \~english
	 * @brief HTTP protocol namespace
	 *
	 * \~
	 */
	namespace http {
		/**
		 * \~russian
		 * @brief Единый класс HTTP-авторизации (клиент/сервер)
		 *
		 * @details Один класс обслуживает обе стороны обмена: поведение методов
		 *          parse()/header()/check() определяется флагом owner_t (CLIENT/SERVER),
		 *          а конкретная схема (Basic/Digest/Bearer/HMAC) выбирается методом type()
		 *          и реализуется внутренней стратегией scheme_t.
		 *
		 * @par Жизненный цикл и состояние
		 * Метод type() при каждой смене схемы автоматически вызывает reset() и сбрасывает
		 * временное состояние (счётчики Digest, таблицы replay, разобранные поля HMAC).
		 * Учётные данные (user/pass/token), ключ подписи (key/keyId), realm, uri, method,
		 * entity и callbacks при этом сохраняются.
		 *
		 * Метод reset() можно вызывать вручную между запросами на одном объекте auth_t,
		 * если требуется начать новый цикл авторизации без смены схемы (например, после
		 * ошибки parse() или при повторном использовании соединения).
		 *
		 * @par Рекомендации по размещению auth_t
		 * - Клиент: один auth_t на HTTP-сессию или TCP-соединение.
		 * - Сервер Basic/Bearer: допустим один shared auth_t на воркер (состояние не
		 *   накапливается между запросами).
		 * - Сервер Digest/HMAC: рекомендуется auth_t на соединение или запрос, так как
		 *   схемы хранят nonce/opaque, таблицы replay (lncs, usedNonces) и выданные ключи.
		 *
		 * @par Пример использования (клиент)
		 * @par Пример использования (сервер)
		 * @par Пример использования (клиент, подпись запроса HMAC, RFC 9421)
		 * @par Пример использования (сервер, проверка HMAC, RFC 9421)
		 * @par Пример использования (клиент, Digest qop=auth-int)
		 *
		 * @code{.cpp}
		 * // Создаём модуль авторизации на стороне клиента
		 * auth_t auth(auth_t::owner_t::CLIENT, fmk, log);
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
		 * @code{.cpp}
		 * // Создаём модуль авторизации на стороне сервера
		 * auth_t auth(auth_t::owner_t::SERVER, fmk, log);
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
		 * @code{.cpp}
		 * // Создаём модуль авторизации на стороне клиента
		 * auth_t auth(auth_t::owner_t::CLIENT, fmk, log);
		 * // Выбираем схему подписи HMAC с алгоритмом SHA-256
		 * auth.type(auth_t::type_t::HMAC, auth_t::hash_t::SHA256);
		 * // Указываем секретный ключ и его идентификатор
		 * auth.key("secret-shared-key");
		 * auth.keyId("test-key");
		 * // Перечисляем покрываемые подписью компоненты запроса (порядок важен)
		 * auth.component("@method", "POST");
		 * auth.component("@authority", "example.com");
		 * auth.component("@path", "/foo");
		 * // Опционально: задать параметры подписи до формирования заголовков
		 * const uint64_t now = fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS);
		 * auth.signCreated(now);
		 * auth.signExpires(now + 300); // срок действия 5 минут (если нужен)
		 * auth.signNonce("unique-request-id"); // одноразовое значение (опционально)
		 * // Получаем набор заголовков подписи (Signature-Input и Signature)
		 * vector <pair <string, string>> headers;
		 * auth.headers(headers);
		 * @endcode
		 *
		 * @code{.cpp}
		 * auth_t auth(auth_t::owner_t::SERVER, fmk, log);
		 * auth.type(auth_t::type_t::HMAC, auth_t::hash_t::SHA256);
		 * // Допуск расхождения часов клиента и сервера (по умолчанию 60 с)
		 * auth.mode.clockSkew(120);
		 * // Максимальный возраст подписи без expires (рекомендуется в production, если клиент
		 * // не передаёт expires; 0 — без ограничения)
		 * auth.mode.signMaxAge(300);
		 * // Восстанавливаем значения компонентов из принятого HTTP-запроса
		 * auth.component("@method", request.method());
		 * auth.component("@authority", request.authority());
		 * auth.component("@path", request.path());
		 * auth.callbackExtractKey([](const string & keyId) -> string {
		 *     return keystore.secret(keyId);
		 * });
		 * // Разбираем заголовки подписи клиента
		 * auth.parse("Signature-Input", request.header("Signature-Input"));
		 * auth.parse("Signature", request.header("Signature"));
		 * if(!auth.check())
		 *     response.status(401);
		 * @endcode
		 *
		 * @code{.cpp}
		 * auth_t auth(auth_t::owner_t::CLIENT, fmk, log);
		 * auth.type(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
		 * auth.user("login");
		 * auth.pass("secret");
		 * auth.method("POST");
		 * auth.uri("/api/resource");
		 * auth.parse(wwwAuthenticate); // вызов с qop="auth-int"
		 * auth.entity(requestBody);    // обязательно: тело запроса для расчёта HA2
		 * const string & credentials = auth.header();
		 * @endcode
		 *
		 * \~english
		 * @brief Single class of the HTTP authorization (a client/a server)
		 * @details One class serves both sides of the exchange: the behaviour of the methods
		 *          parse()/header()/check() is determined by the flag owner_t (CLIENT/SERVER),
		 *          while the particular scheme (Basic/Digest/Bearer/HMAC) is chosen by the method type()
		 *          and is implemented by the internal strategy scheme_t.
		 * @par Life cycle and state
		 * The method type() at every change of the scheme automatically calls reset() and resets
		 * the temporary state (the counters of Digest, the tables of the replay, the parsed fields of HMAC).
		 * The credentials (user/pass/token), the key of the signature (key/keyId), realm, uri, method,
		 * entity and the callbacks are thereby preserved.
		 * The method reset() may be called manually between the requests on a single object auth_t,
		 * if it is required to begin a new cycle of the authorization without a change of the scheme (for example, after
		 * an error of parse() or at a repeated use of the connection).
		 * @par Recommendations on the placement of auth_t
		 * - A client: one auth_t per HTTP session or TCP connection.
		 * - A server of Basic/Bearer: a single shared auth_t per worker is admissible (the state is not
		 *   accumulated between the requests).
		 * - A server of Digest/HMAC: an auth_t per connection or request is recommended, as
		 *   the schemes store the nonce/opaque, the tables of the replay (lncs, usedNonces) and the issued keys.
		 * @par Example of the use (a client)
		 * @par Example of the use (a server)
		 * @par Example of the use (a client, a signature of a request by HMAC, RFC 9421)
		 * @par Example of the use (a server, a checking of HMAC, RFC 9421)
		 * @par Example of the use (a client, Digest qop=auth-int)
		 *
		 * @code{.cpp}
		 * // Creating the module of the authorization on the side of the client
		 * auth_t auth(auth_t::owner_t::CLIENT, fmk, log);
		 * // Choosing the DIGEST scheme with the SHA-256 algorithm
		 * auth.type(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
		 * // If needed, turning on the session mode of the algorithm (SHA-256-sess)
		 * auth.session(true);
		 * // Setting the credentials and the parameters of the request
		 * auth.user("login");
		 * auth.pass("secret");
		 * auth.uri("/api/resource");
		 * auth.method("GET");
		 * // Parsing the challenge of the authorization received from the server (the WWW-Authenticate header)
		 * auth.parse(wwwAuthenticate);
		 * // Building the header of the credentials for the request (the value of the Authorization header)
		 * const string & credentials = auth.header();
		 * @endcode
		 *
		 * @code{.cpp}
		 * // Creating the module of the authorization on the side of the server
		 * auth_t auth(auth_t::owner_t::SERVER, fmk, log);
		 * // Choosing the DIGEST scheme with the SHA-256 algorithm
		 * auth.type(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
		 * // If needed, turning on the session mode of the algorithm (SHA-256-sess)
		 * auth.session(true);
		 * // Setting the name of the server (realm) and the HTTP method of the request
		 * auth.realm("anyks.com");
		 * auth.method("GET");
		 * // Registering the function of the extraction of the password of a user by their login
		 * auth.callbackExtractPass([](const string & user) -> string {
		 *     return db.password(user);
		 * });
		 * // Parsing the credentials of the client (the Authorization header)
		 * auth.parse(authorization);
		 * // Checking the credentials, on a failure sending the client a challenge of the authorization
		 * if(!auth.check())
		 *     response.header("WWW-Authenticate", auth.header());
		 * @endcode
		 *
		 * @code{.cpp}
		 * // Creating the module of the authorization on the side of the client
		 * auth_t auth(auth_t::owner_t::CLIENT, fmk, log);
		 * // Choosing the HMAC scheme of the signature with the SHA-256 algorithm
		 * auth.type(auth_t::type_t::HMAC, auth_t::hash_t::SHA256);
		 * // Setting the secret key and its identifier
		 * auth.key("secret-shared-key");
		 * auth.keyId("test-key");
		 * // Listing the components of the request covered by the signature (the order matters)
		 * auth.component("@method", "POST");
		 * auth.component("@authority", "example.com");
		 * auth.component("@path", "/foo");
		 * // Optionally: to set the parameters of the signature before the building of the headers
		 * const uint64_t now = fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS);
		 * auth.signCreated(now);
		 * auth.signExpires(now + 300); // the term of the validity is 5 minutes (if it is needed)
		 * auth.signNonce("unique-request-id"); // a one-time value (optionally)
		 * // Getting the set of the headers of the signature (Signature-Input and Signature)
		 * vector <pair <string, string>> headers;
		 * auth.headers(headers);
		 * @endcode
		 *
		 * @code{.cpp}
		 * auth_t auth(auth_t::owner_t::SERVER, fmk, log);
		 * auth.type(auth_t::type_t::HMAC, auth_t::hash_t::SHA256);
		 * // The tolerance of the divergence of the clocks of the client and of the server (60 s by default)
		 * auth.mode.clockSkew(120);
		 * // The maximum age of a signature without expires (is recommended in production if the client
		 * // does not pass expires; 0 — without a limitation)
		 * auth.mode.signMaxAge(300);
		 * // Restoring the values of the components from the received HTTP request
		 * auth.component("@method", request.method());
		 * auth.component("@authority", request.authority());
		 * auth.component("@path", request.path());
		 * auth.callbackExtractKey([](const string & keyId) -> string {
		 *     return keystore.secret(keyId);
		 * });
		 * // Parsing the headers of the signature of the client
		 * auth.parse("Signature-Input", request.header("Signature-Input"));
		 * auth.parse("Signature", request.header("Signature"));
		 * if(!auth.check())
		 *     response.status(401);
		 * @endcode
		 *
		 * @code{.cpp}
		 * auth_t auth(auth_t::owner_t::CLIENT, fmk, log);
		 * auth.type(auth_t::type_t::DIGEST, auth_t::hash_t::SHA256);
		 * auth.user("login");
		 * auth.pass("secret");
		 * auth.method("POST");
		 * auth.uri("/api/resource");
		 * auth.parse(wwwAuthenticate); // a challenge with qop="auth-int"
		 * auth.entity(requestBody);    // obligatory: the body of the request for the calculation of HA2
		 * const string & credentials = auth.header();
		 * @endcode
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Authorization {
			public:
				/**
				 * \~russian
				 * @brief Псевдоним типа хэш-суммы (переиспользуем алгоритмы модуля криптографии)
				 *
				 * \~english
				 * @brief Alias of the type of the hash sum (we reuse the algorithms of the module of the cryptography)
				 *
				 * \~
				 */
				using hash_t = crypto_t::hash_t;
			public:
				/**
				 * \~russian
				 * @brief Сторона, от имени которой работает модуль
				 *
				 * \~english
				 * @brief Side on behalf of which the module works
				 *
				 * \~
				 */
				enum class owner_t : uint8_t {
					NONE   = 0x00, // Сторона не установлена
					CLIENT = 0x01, // Клиент (отправляет учётные данные, читает вызов сервера)
					SERVER = 0x02  // Сервер (проверяет учётные данные, отправляет вызов клиенту)
				};
				/**
				 * \~russian
				 * @brief Тип (схема) авторизации
				 *
				 * \~english
				 * @brief Type (scheme) of the authorization
				 *
				 * \~
				 */
				enum class type_t : uint8_t {
					NONE   = 0x00, // Авторизация не установлена
					HMAC   = 0x01, // Авторизация подписью запроса HMAC
					BASIC  = 0x02, // BASIC авторизация (RFC 7617)
					DIGEST = 0x03, // DIGEST авторизация (RFC 7616)
					BEARER = 0x04  // BEARER/Token авторизация (RFC 6750)
				};
				/**
				 * \~russian
				 * @brief Режим строгости проверки учётных данных (сервер)
				 *
				 * @details SIMPLE — совместимый режим: допускается Digest legacy RFC 2069 (без qop),
				 *          ключи параметров HMAC Signature-Input сверяются без учёта регистра.
				 *          STRICT — строгое соответствие RFC:
				 *          - Digest: отклоняется отсутствие qop (RFC 7616), обязательны cnonce и opaque,
				 *            алгоритм из учётных данных должен совпадать с настроенным (защита от downgrade);
				 *          - HMAC: параметры подписи сверяются байт-точно (RFC 9421), алгоритм должен
				 *            совпадать с настроенным, а при отсутствии expires применяется ограниченный
				 *            срок жизни подписи по умолчанию.
				 *
				 * \~english
				 * @brief Mode of the strictness of the checking of the credentials (a server)
				 * @details SIMPLE — the compatible mode: the legacy Digest of RFC 2069 (without a qop) is admitted,
				 *          the keys of the parameters of the HMAC Signature-Input are compared without the account of the case.
				 *          STRICT — a strict correspondence to the RFC:
				 *          - Digest: the absence of a qop is rejected (RFC 7616), the cnonce and the opaque are obligatory,
				 *            the algorithm from the credentials is obliged to coincide with the configured one (a protection from a downgrade);
				 *          - HMAC: the parameters of the signature are compared octet-exactly (RFC 9421), the algorithm is obliged
				 *            to coincide with the configured one, while at the absence of an expires a limited
				 *            lifetime of the signature by default is applied.
				 *
				 * \~
				 */
				enum class mode_t : uint8_t {
					SIMPLE = 0x00, // Простой (совместимый) режим
					STRICT = 0x01  // Строгий режим (жёсткое соответствие RFC)
				};
			public:
				/**
				 * \~russian
				 * @brief Структура режима работы схемы авторизации
				 *
				 * @details Поля mode.nonceMaxAge, mode.signMaxAge и mode.signStrictMaxAge
				 *          используются схемами Digest/HMAC для ограничения срока жизни nonce/signature.
				 *
				 * \~english
				 * @brief Structure of the mode of the work of the scheme of the authorization
				 * @details The fields mode.nonceMaxAge, mode.signMaxAge and mode.signStrictMaxAge
				 *          are used by the schemes Digest/HMAC for the limitation of the lifetime of a nonce/signature.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Mode_Digest {
					// Флаг наличия параметра qop (RFC 7616, иначе legacy RFC 2069)
					bool qop;
					// Флаг использования сессионного алгоритма (-sess)
					bool sess;
					// Флаг режима qop=auth-int (требует entity-body)
					bool authInt;
					// Штамп времени последней генерации nonce в секундах (сервер)
					uint64_t stamp;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Mode_Digest() noexcept;
				} mode_digest_t;
				/**
				 * \~russian
				 * @brief Структура параметров Digest-авторизации
				 *
				 * @details Поля mode.sess, mode.qop, mode.authInt, nc, nonce, opaque, lncs и др. используются
				 *          внутренне схемой Digest. На сервере lncs хранит последние принятые
				 *          значения nc по ключу «логин + nonce» для защиты от replay-атак.
				 *          Таблица сбрасывается при выдаче нового nonce и при reset()/type().
				 *
				 * \~english
				 * @brief Structure of the parameters of the Digest authorization
				 * @details The fields mode.sess, mode.qop, mode.authInt, nc, nonce, opaque, lncs and others are used
				 *          internally by the scheme Digest. At a server lncs stores the last accepted
				 *          values of nc by the key «a login + a nonce» for the protection from the replay attacks.
				 *          The table is reset at the issue of a new nonce and at reset()/type().
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Digest {
					// Флаги состояния авторизации (для внутреннего использования схемой Digest)
					mode_digest_t mode;
					// Счётчик запросов клиента (nonce count)
					string nc;
					// Параметры HTTP-запроса (request-uri)
					string uri;
					// Тип защиты (quality of protection), по умолчанию "auth" (RFC 7616)
					string qop;
					// Название сервера или realm
					string realm;
					// Уникальный ключ, выдаваемый сервером
					string nonce;
					// Уникальный ключ, фактически выданный сервером (для сверки при проверке)
					string issued;
					// Тело запроса (entity-body) для qop=auth-int
					string entity;
					// Временный ключ сессии сервера
					string opaque;
					// Уникальный ключ, генерируемый клиентом
					string cnonce;
					// Результат ответа клиента
					string response;
					// Фактически выданный сервером opaque (для сверки при проверке)
					string issuedOpaque;
					// Порядок ключей lncs для LRU-вытеснения (старейший — в начале)
					list <string> lncsOrder;
					// Последние принятые счётчики запросов по паре «логин + nonce» (значение nc + позиция в LRU-очереди)
					unordered_map <string, pair <string, list <string>::iterator>> lncs;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Digest() noexcept;
				} digest_t;
			public:
				/**
				 * \~russian
				 * @brief Структура даты подписи HMAC (RFC 9421)
				 *
				 * @details Поля created и expires включаются в Signature-Input и участвуют
				 *          в расчёте подписи. На сервере usedNonces хранит уже принятые значения nonce для защиты от повторного использования подписи.
				 *
				 * \~english
				 * @brief Structure of the date of an HMAC signature (RFC 9421)
				 * @details The fields created and expires are included into the Signature-Input and participate
				 *          in the calculation of the signature. At a server usedNonces stores the already accepted values of the nonce for the protection from a repeated use of a signature.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Sign_Date {
					// Штамп времени создания подписи в секундах
					uint64_t created;
					// Штамп времени истечения подписи в секундах (0 — не задано)
					uint64_t expires;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Sign_Date() noexcept;
				} sign_date_t;
				/**
				 * \~russian
				 * @brief Структура параметров авторизации подписью HMAC (RFC 9421)
				 *
				 * @details Поля created, expires и nonce включаются в Signature-Input и участвуют
				 *          в расчёте подписи. На сервере usedNonces хранит уже принятые значения
				 *          nonce для защиты от повторного использования подписи.
				 *
				 * \~english
				 * @brief Structure of the parameters of the authorization by an HMAC signature (RFC 9421)
				 * @details The fields created, expires and nonce are included into the Signature-Input and participate
				 *          in the calculation of the signature. At a server usedNonces stores the already accepted values
				 *          of the nonce for the protection from a repeated use of a signature.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Sign {
					// Параметры даты подписи (created/expires)
					sign_date_t date;
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
					// Метка подписи из заголовка Signature-Input (для сверки с Signature)
					string inputLabel;
					// Порядок покрываемых подписью компонентов (имена)
					vector <string> covered;
					// Порядок принятых nonce для LRU-вытеснения (старейший — в начале)
					list <string> usedNoncesOrder;
					// Значения покрываемых компонентов (имя -> значение)
					vector <pair <string, string>> components;
					// Индекс компонентов по имени в нижнем регистре (для быстрого поиска)
					unordered_map <string, size_t> componentIndex;
					// Уже принятые одноразовые значения подписи (nonce -> позиция в LRU-очереди, защита от replay на сервере)
					unordered_map <string, list <string>::iterator> usedNonces;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Sign() noexcept;
				} sign_t;
				/**
				 * \~russian
				 * @brief Структура обратных вызовов для проверки учётных данных (сервер)
				 *
				 * @details Функции обратных вызовов вызываются схемой авторизации на сервере
				 *          для проверки учётных данных, извлечения пароля пользователя или
				 *          секретного ключа подписи по идентификатору.
				 *          Функции должны быть зарегистрированы через методы callback*() до вызова parse().
				 *
				 * \~english
				 * @brief Structure of the callbacks for the checking of the credentials (a server)
				 * @details The callback functions are called by the scheme of the authorization at a server
				 *          for the checking of the credentials, the extraction of the password of a user or
				 *          of the secret key of the signature by an identifier.
				 *          The functions are obliged to be registered through the methods callback*() before the call of parse().
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Callback {
					/**
					 * \~russian
					 * @brief Внешняя функция проверки токена доступа (BEARER, сервер)
					 *
					 * @param token токен доступа
					 * @return      результат проверки токена (true - токен действителен, false - токен недействителен)
					 *
					 * \~english
					 * @brief External function of the checking of a token of the access (BEARER, a server)
					 * @param token token of the access
					 * @return      result of the checking of the token (true - the token is valid, false - the token is invalid)
					 *
					 * \~
					 */
					function <bool (const string &)> checkToken;
					/**
					 * \~russian
					 * @brief Внешняя функция извлечения секретного ключа по идентификатору (HMAC, сервер)
					 *
					 * @param keyId идентификатор ключа
					 * @return      секретный ключ (пустая строка — ключ не найден)
					 *
					 * \~english
					 * @brief External function of the extraction of a secret key by an identifier (HMAC, a server)
					 * @param keyId identifier of the key
					 * @return      secret key (an empty string — the key is not found)
					 *
					 * \~
					 */
					function <string (const string &)> extractKey;
					/**
					 * \~russian
					 * @brief Внешняя функция извлечения пароля по логину (DIGEST, сервер)
					 *
					 * @param user логин пользователя
					 * @return     пароль пользователя (пустая строка — пользователь не найден)
					 *
					 * \~english
					 * @brief External function of the extraction of a password by a login (DIGEST, a server)
					 * @param user login of the user
					 * @return     password of the user (an empty string — the user is not found)
					 *
					 * \~
					 */
					function <string (const string &)> extractPass;
					/**
					 * \~russian
					 * @brief Внешняя функция проверки пары «логин/пароль» (BASIC, сервер)
					 *
					 * @param user логин пользователя
					 * @param pass пароль пользователя
					 * @return     результат проверки пары (true - пара действительна, false - пара недействительна)
					 *
					 * \~english
					 * @brief External function of the checking of a pair «a login/a password» (BASIC, a server)
					 * @param user login of the user
					 * @param pass password of the user
					 * @return     result of the checking of the pair (true - the pair is valid, false - the pair is invalid)
					 *
					 * \~
					 */
					function <bool (const string &, const string &)> checkUser;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Callback() noexcept;
				} callback_t;
			public:
				/**
				 * \~russian
				 * @brief Структура режима работы параметров авторизации
				 *
				 * @details Поля clockSkew задаёт допуск (в секундах) при проверке created/expires
				 *          подписи HMAC на сервере. Значение по умолчанию — 60 секунд.
				 *          signMaxAge ограничивает срок жизни подписи без expires (0 — без лимита).
				 *
				 * \~english
				 * @brief Structure of the mode of the work of the parameters of the authorization
				 * @details The field clockSkew sets the tolerance (in seconds) at the checking of the created/expires
				 *          of an HMAC signature at a server. The value by default is 60 seconds.
				 *          signMaxAge limits the lifetime of a signature without an expires (0 — without a limit).
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Mode_Params {
					// Флаг работы через прокси (Proxy-Authorization/Proxy-Authenticate)
					bool proxy;
					// Режим строгости проверки учётных данных на сервере (по умолчанию SIMPLE)
					mode_t validation;
					// Допустимое расхождение локальных часов при проверке HMAC (секунды, по умолчанию 60)
					uint64_t clockSkew;
					// Максимальный возраст HMAC-подписи без expires (секунды, 0 — не ограничен)
					uint64_t signMaxAge;
					// Максимальный возраст Digest-nonce (секунды, 0 — без ограничения по времени, по умолчанию 1800)
					uint64_t nonceMaxAge;
					// Максимальный возраст HMAC-подписи без expires в строгом режиме (секунды, 0 — не ограничен, по умолчанию 300)
					uint64_t signStrictMaxAge;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Mode_Params() noexcept;
				} mode_params_t;
				/**
				 * \~russian
				 * @brief Структура общих параметров авторизации
				 *
				 * @details clockSkew задаёт допуск (в секундах) при проверке created/expires
				 *          подписи HMAC на сервере. Значение по умолчанию — 60 секунд.
				 *          signMaxAge ограничивает срок жизни подписи без expires (0 — без лимита).
				 *
				 * \~english
				 * @brief Structure of the common parameters of the authorization
				 * @details clockSkew sets the tolerance (in seconds) at the checking of the created/expires
				 *          of an HMAC signature at a server. The value by default is 60 seconds.
				 *          signMaxAge limits the lifetime of a signature without an expires (0 — without a limit).
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Params {
					// Режим работы параметров авторизации
					mode_params_t mode;
					// Логин пользователя
					string user;
					// Пароль пользователя
					string pass;
					// Токен доступа (для BEARER)
					string token;
					// HTTP-метод запроса (для расчёта ответа DIGEST, по умолчанию GET)
					string method;
					// Текущий алгоритм хэширования (может переопределяться из заголовка Digest, по умолчанию MD5)
					hash_t hash;
					// Алгоритм хэширования, заданный через type() (для DIGEST/HMAC, по умолчанию MD5)
					hash_t scheme;
					// Параметры авторизации подписью HMAC
					sign_t sign;
					// Параметры Digest-авторизации
					digest_t digest;
					// Функции обратных вызовов для проверки учётных данных (сервер)
					callback_t callback;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Params() noexcept;
				} params_t;
			public:
				/**
				 * \~russian
				 * @brief Абстрактный интерфейс схемы авторизации (внутренняя стратегия)
				 *
				 * @details Конкретные схемы (Basic/Digest/Bearer/HMAC) реализуют интерфейс
				 *          scheme_t и делегируют ему методы parse()/header()/check().
				 *          Интерфейс scheme_t не должен использоваться напрямую.
				 *
				 * \~english
				 * @brief Abstract interface of a scheme of the authorization (an internal strategy)
				 * @details The particular schemes (Basic/Digest/Bearer/HMAC) implement the interface
				 *          scheme_t and delegate to it the methods parse()/header()/check().
				 *          The interface scheme_t is not obliged to be used directly.
				 *
				 * \~
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
						 * \~russian
						 * @brief Метод получения имени исходящего заголовка авторизации
						 *
						 * @details Клиент формирует заголовок учётных данных
						 *          (Authorization либо Proxy-Authorization для прокси),
						 *          сервер формирует заголовок вызова
						 *          (WWW-Authenticate либо Proxy-Authenticate для прокси)
						 *
						 * @return имя заголовка авторизации
						 *
						 * \~english
						 * @brief Method of getting the name of the outgoing header of the authorization
						 * @details A client forms the header of the credentials
						 *          (Authorization or Proxy-Authorization for a proxy),
						 *          a server forms the header of the challenge
						 *          (WWW-Authenticate or Proxy-Authenticate for a proxy)
						 * @return name of the header of the authorization
						 *
						 * \~
						 */
						string name() const noexcept;
						/**
						 * \~russian
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
						 * \~english
						 * @brief Method of the comparison of the strings in a constant time
						 * @details It is used for the comparison of the secrets and of the signatures. It is not delegated
						 *          to fmk_t::compare(), as that one completes prematurely and is not suitable
						 *          for the cryptographic comparisons.
						 * @param left  first string
						 * @param right second string
						 * @return      result of the comparison
						 *
						 * \~
						 */
						static bool secureCompare(const string_view left, const string_view right) noexcept;
						/**
						 * \~russian
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
						 * \~english
						 * @brief Method of the extraction of the payload after the name of the scheme of the authorization
						 * @details Checks that the header begins with the indicated scheme and after it
						 *          a space character follows (RFC 7235). For the trim and the comparison of the scheme
						 *          fmk_t is used.
						 * @param header  value of the header of the authorization
						 * @param scheme  name of the scheme (Basic, Bearer, Digest)
						 * @param payload payload after the scheme
						 * @return        result of the extraction
						 *
						 * \~
						 */
						bool schemePayload(const string_view header, const string_view scheme, string & payload) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод проверки учётных данных (только для сервера)
						 *
						 * @return результат проверки
						 *
						 *
						 * \~english
						 * @brief Method of checking the credentials (for the server only)
						 * @return result of the check
						 *
						 * \~
						 */
						virtual bool check() noexcept = 0;
					public:
						/**
						 * \~russian
						 * @brief Метод формирования исходящего заголовка авторизации
						 *
						 * @param full режим вывода вместе с именем заголовка
						 * @return     значение заголовка авторизации
						 *
						 *
						 * \~english
						 * @brief Method of forming an outgoing authorization header
						 * @param full mode of the output together with the name of the header
						 * @return     value of the authorization header
						 *
						 * \~
						 */
						virtual string header(const bool full = false) noexcept = 0;
						/**
						 * \~russian
						 * @brief Метод формирования набора исходящих заголовков авторизации
						 *
						 * @details Базовая реализация формирует один заголовок (имя -> значение).
						 *          Многозаголовочные схемы (HMAC) переопределяют метод.
						 *
						 * @param result контейнер для набора заголовков (имя -> значение)
						 *
						 * \~english
						 * @brief Method of the forming of the collection of the outgoing headers of the authorization
						 * @details The base implementation forms one header (a name -> a value).
						 *          The multi-header schemes (HMAC) redefine the method.
						 * @param result container for the collection of the headers (a name -> a value)
						 *
						 * \~
						 */
						virtual void headers(vector <pair <string, string>> & result) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Метод разбора входящего заголовка авторизации
						 *
						 * @param header значение заголовка (клиент: вызов, сервер: учётные данные)
						 * @return       результат разбора
						 *
						 *
						 * \~english
						 * @brief Method of parsing an incoming authorization header
						 * @param header value of the header (client: the challenge, server: the credentials)
						 * @return       result of the parsing
						 *
						 * \~
						 */
						virtual bool parse(const string_view header) noexcept = 0;
						/**
						 * \~russian
						 * @brief Метод разбора входящего заголовка авторизации с указанием имени
						 *
						 * @details Требуется многозаголовочным схемам (HMAC: Signature-Input и Signature).
						 *          Базовая реализация игнорирует имя и делегирует разбор значению.
						 *
						 * @param name   имя входящего заголовка
						 * @param header значение входящего заголовка
						 * @return       результат разбора
						 *
						 * \~english
						 * @brief Method of parsing an incoming header of the authorization with an indication of the name
						 * @details It is required by the multi-header schemes (HMAC: Signature-Input and Signature).
						 *          The base implementation ignores the name and delegates the parsing to the value.
						 * @param name   name of the incoming header
						 * @param header value of the incoming header
						 * @return       result of the parsing
						 *
						 * \~
						 */
						virtual bool parse(const string_view name, const string_view header) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param owner  сторона работы (клиент/сервер)
						 * @param params общие параметры авторизации
						 * @param crypto объект криптографии
						 * @param fmk    объект фреймворка
						 * @param log    объект для работы с логами
						 *
						 *
						 * \~english
						 * @brief Constructor
						 * @param owner  side of the work (client/server)
						 * @param params common parameters of the authorization
						 * @param crypto cryptography object
						 * @param fmk    framework object
						 * @param log    object for working with logs
						 *
						 * \~
						 */
						explicit Scheme(const owner_t owner, params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept;
						/**
						 * \~russian
						 * @brief Деструктор
						 *
						 *
						 * \~english
						 * @brief Destructor
						 *
						 * \~
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
				 * \~russian
				 * @brief Метод получения стороны работы модуля
				 *
				 * @return сторона работы (клиент/сервер)
				 *
				 * \~english
				 * @brief Method of getting the side of the work of the module
				 * @return side of the work (a client/a server)
				 *
				 * \~
				 */
				owner_t owner() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения типа авторизации
				 *
				 * @return тип авторизации
				 *
				 * \~english
				 * @brief Method of getting the type of the authorization
				 * @return type of the authorization
				 *
				 * \~
				 */
				type_t type() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки типа авторизации
				 *
				 * @details Перед активацией новой стратегии автоматически вызывается reset().
				 *          Сохранённые user/pass/token, key/keyId, realm, uri, method, entity
				 *          и callbacks не затрагиваются.
				 *
				 * @param type тип авторизации для установки
				 * @param hash алгоритм хэширования (для DIGEST/HMAC)
				 *
				 * \~english
				 * @brief Method of setting the type of the authorization
				 * @details Before the activation of a new strategy reset() is called automatically.
				 *          The stored user/pass/token, key/keyId, realm, uri, method, entity
				 *          and the callbacks are not affected.
				 * @param type type of the authorization for the setting
				 * @param hash algorithm of the hashing (for DIGEST/HMAC)
				 *
				 * \~
				 */
				void type(const type_t type, const hash_t hash = hash_t::MD5) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки логина пользователя
				 *
				 * @param user логин пользователя
				 *
				 * \~english
				 * @brief Method of setting the login of a user
				 * @param user login of the user
				 *
				 * \~
				 */
				void user(string_view user) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки пароля пользователя
				 *
				 * @details Для BASIC пароль передаётся как есть; символ «:» в пароле
				 *          не поддерживается (RFC 7617: user-pass = userid \":\" password).
				 *
				 * @param pass пароль пользователя
				 *
				 * \~english
				 * @brief Method of setting the password of a user
				 * @details For BASIC the password is transmitted as it is; the character «:» in a password
				 *          is not supported (RFC 7617: user-pass = userid \":\" password).
				 * @param pass password of the user
				 *
				 * \~
				 */
				void pass(string_view pass) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки токена доступа (BEARER)
				 *
				 * @param token токен доступа
				 *
				 * \~english
				 * @brief Method of setting a token of the access (BEARER)
				 * @param token token of the access
				 *
				 * \~
				 */
				void token(string_view token) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки режима работы через прокси
				 *
				 * @details Влияет на имена заголовков: Proxy-Authorization/Proxy-Authenticate
				 *          вместо Authorization/WWW-Authenticate
				 *
				 * @param mode флаг работы через прокси
				 *
				 * \~english
				 * @brief Method of setting the mode of the work through a proxy
				 * @details It influences the names of the headers: Proxy-Authorization/Proxy-Authenticate
				 *          instead of Authorization/WWW-Authenticate
				 * @param mode flag of the work through a proxy
				 *
				 * \~
				 */
				void proxy(const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима строгости проверки учётных данных
				 *
				 * @return режим строгости проверки (SIMPLE/STRICT)
				 *
				 * \~english
				 * @brief Method of getting the mode of the strictness of the checking of the credentials
				 * @return mode of the strictness of the checking (SIMPLE/STRICT)
				 *
				 * \~
				 */
				mode_t mode() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима строгости проверки учётных данных (сервер)
				 *
				 * @details SIMPLE (по умолчанию) — совместимый режим: сервер принимает Digest legacy
				 *          RFC 2069 (без qop), ключи параметров HMAC Signature-Input сверяются без
				 *          учёта регистра. STRICT — строгое соответствие RFC: для Digest обязательны
				 *          qop, cnonce, opaque и совпадение алгоритма со схемой; для HMAC параметры
				 *          сверяются байт-точно, алгоритм должен совпадать со схемой, а при отсутствии
				 *          expires применяется ограниченный срок жизни подписи по умолчанию.
				 *          Влияет только на проверку на стороне сервера.
				 *
				 * @param mode режим строгости проверки (SIMPLE/STRICT)
				 *
				 * \~english
				 * @brief Method of setting the mode of the strictness of the checking of the credentials (a server)
				 * @details SIMPLE (by default) — the compatible mode: the server accepts the legacy Digest
				 *          of RFC 2069 (without a qop), the keys of the parameters of the HMAC Signature-Input are compared without
				 *          the account of the case. STRICT — a strict correspondence to the RFC: for Digest the
				 *          qop, the cnonce, the opaque and a coincidence of the algorithm with the scheme are obligatory; for HMAC the parameters
				 *          are compared octet-exactly, the algorithm is obliged to coincide with the scheme, while at the absence of an
				 *          expires a limited lifetime of the signature by default is applied.
				 *          It influences only the checking on the side of the server.
				 * @param mode mode of the strictness of the checking (SIMPLE/STRICT)
				 *
				 * \~
				 */
				void mode(const mode_t mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки секретного ключа подписи (HMAC)
				 *
				 * @param key секретный ключ подписи
				 *
				 * \~english
				 * @brief Method of setting the secret key of the signature (HMAC)
				 * @param key secret key of the signature
				 *
				 * \~
				 */
				void key(string_view key) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки идентификатора ключа подписи (HMAC)
				 *
				 * @param keyId идентификатор ключа подписи
				 *
				 * \~english
				 * @brief Method of setting the identifier of the key of the signature (HMAC)
				 * @param keyId identifier of the key of the signature
				 *
				 * \~
				 */
				void keyId(string_view keyId) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки метки подписи (HMAC)
				 *
				 * @param label метка подписи (например, sig1)
				 *
				 * \~english
				 * @brief Method of setting the label of the signature (HMAC)
				 * @param label label of the signature (for example, sig1)
				 *
				 * \~
				 */
				void label(string_view label) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки одноразового значения подписи (HMAC)
				 *
				 * @details Вызывается на клиенте **до** headers()/header(). Значение включается
				 *          в Signature-Input и участвует в расчёте подписи. На сервере повторная
				 *          проверка подписи с тем же nonce отклоняется (защита от replay).
				 *
				 * @param nonce одноразовое значение
				 *
				 * \~english
				 * @brief Method of setting the one-time value of the signature (HMAC)
				 * @details It is called at a client **before** headers()/header(). The value is included
				 *          into the Signature-Input and participates in the calculation of the signature. At a server a repeated
				 *          checking of a signature with the same nonce is rejected (a protection from a replay).
				 * @param nonce one-time value
				 *
				 * \~
				 */
				void signNonce(string_view nonce) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки штампа времени создания подписи (HMAC)
				 *
				 * @details Вызывается на клиенте **до** headers()/header(). Значение попадает
				 *          в Signature-Input и участвует в канонической базе подписи.
				 *          Если передать 0, при формировании подписи будет использован текущий
				 *          штамп времени (fmk_t::timestamp).
				 *
				 * @param stamp штамп времени в секундах (0 — автоматически при формировании)
				 *
				 * \~english
				 * @brief Method of setting the time stamp of the creation of the signature (HMAC)
				 * @details It is called at a client **before** headers()/header(). The value gets
				 *          into the Signature-Input and participates in the canonical base of the signature.
				 *          If a 0 is passed, at the forming of the signature the current
				 *          time stamp will be used (fmk_t::timestamp).
				 * @param stamp time stamp in seconds (0 — automatically at the forming)
				 *
				 * \~
				 */
				void signCreated(const uint64_t stamp) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки штампа времени истечения подписи (HMAC)
				 *
				 * @details Вызывается на клиенте **до** headers()/header(). Сервер отклоняет
				 *          подпись, если текущее время превышает expires (с учётом clockSkew).
				 *          Значение 0 означает, что срок действия не ограничен.
				 *
				 * @param stamp штамп времени в секундах (0 — не задано)
				 *
				 * \~english
				 * @brief Method of setting the time stamp of the expiration of the signature (HMAC)
				 * @details It is called at a client **before** headers()/header(). The server rejects
				 *          a signature, if the current time exceeds the expires (with the account of the clockSkew).
				 *          A value of 0 means that the term of the validity is not limited.
				 * @param stamp time stamp in seconds (0 — not given)
				 *
				 * \~
				 */
				void signExpires(const uint64_t stamp) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод добавления покрываемого подписью компонента (HMAC)
				 *
				 * @details Порядок добавления компонентов сохраняется. Производные компоненты
				 *          начинаются с символа '@' (например, "@method", "@path", "@authority")
				 *
				 * @param name  имя компонента
				 * @param value значение компонента
				 *
				 * \~english
				 * @brief Method of adding a component covered by the signature (HMAC)
				 * @details The order of the addition of the components is preserved. The derived components
				 *          begin with the character '@' (for example, "@method", "@path", "@authority")
				 * @param name  name of the component
				 * @param value value of the component
				 *
				 * \~
				 */
				void component(string_view name, string_view value) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки параметров HTTP-запроса (DIGEST, клиент)
				 *
				 * @param uri параметры HTTP-запроса (request-uri)
				 *
				 * \~english
				 * @brief Method of setting the parameters of an HTTP request (DIGEST, a client)
				 * @param uri parameters of the HTTP request (request-uri)
				 *
				 * \~
				 */
				void uri(string_view uri) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки HTTP-метода запроса (DIGEST)
				 *
				 * @param method HTTP-метод запроса
				 *
				 * \~english
				 * @brief Method of setting the HTTP method of a request (DIGEST)
				 * @param method HTTP method of the request
				 *
				 * \~
				 */
				void method(string_view method) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки тела запроса (DIGEST, qop=auth-int)
				 *
				 * @details Для qop=auth-int необходимо явно передать entity-body запроса
				 *          до формирования или проверки учётных данных.
				 *
				 * @param entity тело HTTP-запроса (entity-body)
				 *
				 * \~english
				 * @brief Method of setting the body of a request (DIGEST, qop=auth-int)
				 * @details For qop=auth-int it is necessary to pass explicitly the entity-body of the request
				 *          before the forming or the checking of the credentials.
				 * @param entity body of the HTTP request (entity-body)
				 *
				 * \~
				 */
				void entity(string_view entity) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки названия сервера (realm)
				 *
				 * @param realm название сервера
				 *
				 * \~english
				 * @brief Method of setting the name of the server (realm)
				 * @param realm name of the server
				 *
				 * \~
				 */
				void realm(string_view realm) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки уникального ключа сервера (nonce)
				 *
				 * @param nonce уникальный ключ, выдаваемый сервером
				 *
				 * \~english
				 * @brief Method of setting the unique key of the server (nonce)
				 * @param nonce unique key issued by the server
				 *
				 * \~
				 */
				void nonce(string_view nonce) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки временного ключа сессии сервера (opaque)
				 *
				 * @param opaque временный ключ сессии сервера
				 *
				 * \~english
				 * @brief Method of setting the temporary key of the session of the server (opaque)
				 * @param opaque temporary key of the session of the server
				 *
				 * \~
				 */
				void opaque(string_view opaque) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки сессионного режима алгоритма Digest (-sess)
				 *
				 * @details При включении используется сессионный вариант расчёта HA1:
				 *          HA1 = H(H(user:realm:pass):nonce:cnonce), а в имени алгоритма
				 *          добавляется суффикс -sess (например, SHA-256-sess)
				 *
				 * @param mode флаг сессионного режима алгоритма
				 *
				 * \~english
				 * @brief Method of setting the session mode of the algorithm of Digest (-sess)
				 * @details At the enabling the session variety of the calculation of the HA1 is used:
				 *          HA1 = H(H(user:realm:pass):nonce:cnonce), while into the name of the algorithm
				 *          the suffix -sess is added (for example, SHA-256-sess)
				 * @param mode flag of the session mode of the algorithm
				 *
				 * \~
				 */
				void session(const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения допустимого расхождения локальных часов (HMAC)
				 *
				 * @return допуск в секундах (по умолчанию 60)
				 *
				 * \~english
				 * @brief Method of getting the admissible divergence of the local clocks (HMAC)
				 * @return tolerance in seconds (by default 60)
				 *
				 * \~
				 */
				uint64_t clockSkew() const noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of setting the admissible divergence of the local clocks (HMAC, seconds)
				 * @details It is used at a server at check(): a signature is accepted, if the
				 *          created is not more than by clockSkew seconds in the future relative to
				 *          the time of the server, while the expires is not more than by clockSkew seconds
				 *          in the past. The value by default is 60. Pass a 0 for a strict
				 *          checking without a tolerance.
				 * @param seconds tolerance at the checking of the created/expires (0 — only an exact coincidence)
				 *
				 * \~
				 */
				void clockSkew(const uint64_t seconds) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального возраста HMAC-подписи без expires
				 *
				 * @return лимит в секундах (0 — не ограничен)
				 *
				 * \~english
				 * @brief Method of getting the largest age of an HMAC signature without an expires
				 * @return limit in seconds (0 — not limited)
				 *
				 * \~
				 */
				uint64_t signMaxAge() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального возраста HMAC-подписи без expires (секунды)
				 *
				 * @details Используется на сервере при check(), если клиент не передал expires.
				 *          Подпись отклоняется, когда now > created + signMaxAge + clockSkew.
				 *          Значение 0 (по умолчанию) — ограничение не применяется.
				 *          Для production-серверов HMAC рекомендуется задавать ненулевой лимит.
				 *
				 * @param seconds максимальный возраст подписи (0 — без ограничения)
				 *
				 * \~english
				 * @brief Method of setting the largest age of an HMAC signature without an expires (seconds)
				 * @details It is used at a server at check(), if the client has not passed an expires.
				 *          A signature is rejected when now > created + signMaxAge + clockSkew.
				 *          A value of 0 (by default) — the limitation is not applied.
				 *          For the production servers of HMAC it is recommended to give a non-zero limit.
				 * @param seconds largest age of a signature (0 — without a limitation)
				 *
				 * \~
				 */
				void signMaxAge(const uint64_t seconds) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального возраста Digest-nonce
				 *
				 * @return лимит в секундах (0 — без ограничения по времени)
				 *
				 * \~english
				 * @brief Method of getting the largest age of a Digest nonce
				 * @return limit in seconds (0 — without a limitation in the time)
				 *
				 * \~
				 */
				uint64_t nonceMaxAge() const noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of setting the largest age of a Digest nonce (seconds)
				 * @details It is used at a server: a nonce is considered outdated, if since the moment
				 *          of its issue more than the indicated time has passed. An outdated nonce is rejected
				 *          at the checking and is reissued at the forming of a new challenge.
				 *          The value by default is 1800 (30 minutes). Pass a 0 to disable
				 *          the limitation in the lifetime of a nonce.
				 * @param seconds largest age of a nonce (0 — without a limitation)
				 *
				 * \~
				 */
				void nonceMaxAge(const uint64_t seconds) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального возраста HMAC-подписи без expires для строгого режима
				 *
				 * @return лимит в секундах (0 — не ограничен)
				 *
				 * \~english
				 * @brief Method of getting the largest age of an HMAC signature without an expires for the strict mode
				 * @return limit in seconds (0 — not limited)
				 *
				 * \~
				 */
				uint64_t signStrictMaxAge() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального возраста HMAC-подписи без expires для строгого режима (секунды)
				 *
				 * @details Используется на сервере при check() в строгом режиме (STRICT), если клиент
				 *          не передал expires и не задан общий signMaxAge. Значение по умолчанию — 300.
				 *          Передайте 0, чтобы отключить ограничение по умолчанию даже в строгом режиме.
				 *
				 * @param seconds максимальный возраст подписи в строгом режиме (0 — без ограничения)
				 *
				 * \~english
				 * @brief Method of setting the largest age of an HMAC signature without an expires for the strict mode (seconds)
				 * @details It is used at a server at check() in the strict mode (STRICT), if the client
				 *          has not passed an expires and no common signMaxAge is given. The value by default is 300.
				 *          Pass a 0 to disable the limitation by default even in the strict mode.
				 * @param seconds largest age of a signature in the strict mode (0 — without a limitation)
				 *
				 * \~
				 */
				void signStrictMaxAge(const uint64_t seconds) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки учётных данных (только для сервера)
				 *
				 * @details На стороне CLIENT всегда возвращает true (проверка не выполняется).
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking the credentials (only for a server)
				 * @details On the side of a CLIENT it always returns true (the checking is not performed).
				 * @return result of the checking
				 *
				 * \~
				 */
				bool check() noexcept;
			public:
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the reset of the temporary state of the scheme of the authorization
				 * @details Clears the state accumulated between the requests:
				 *          - Digest: mode.qop, mode.authInt, mode.stamp, nc, nonce, issued, opaque,
				 *            cnonce, response, issuedOpaque, lncs (realm, uri, entity, mode.sess are preserved);
				 *          - HMAC: date.created, date.expires, nonce, params, signature, covered,
				 *            components, componentIndex, usedNonces (key, keyId, label, tag are preserved).
				 *          It does not affect: the callbacks, user, pass, token, key, keyId, realm,
				 *          uri, method, entity, mode.proxy, mode.clockSkew, mode.signMaxAge, hash,
				 *          digest.mode.sess.
				 *          It is called automatically from type(). It makes sense to call it manually
				 *          at a repeated cycle of the authorization on the same auth_t without a change of the scheme.
				 *
				 * \~
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод формирования исходящего заголовка авторизации
				 *
				 * @details Клиент формирует учётные данные (Authorization),
				 *          сервер формирует вызов авторизации (WWW-Authenticate)
				 *
				 * @param full режим вывода вместе с именем заголовка
				 * @return     значение заголовка авторизации
				 *
				 * \~english
				 * @brief Method of the forming of the outgoing header of the authorization
				 * @details A client forms the credentials (Authorization),
				 *          a server forms the challenge of the authorization (WWW-Authenticate)
				 * @param full mode of the output together with the name of the header
				 * @return     value of the header of the authorization
				 *
				 * \~
				 */
				string header(const bool full = false) noexcept;
				/**
				 * \~russian
				 * @brief Метод формирования набора исходящих заголовков авторизации
				 *
				 * @details Универсальный способ получения заголовков: для одно-заголовочных схем
				 *          (Basic/Digest/Bearer) возвращается один элемент, для HMAC — два
				 *          (Signature-Input и Signature)
				 *
				 * @param result контейнер для набора заголовков (имя -> значение)
				 *
				 * \~english
				 * @brief Method of the forming of the collection of the outgoing headers of the authorization
				 * @details A universal way of getting the headers: for the single-header schemes
				 *          (Basic/Digest/Bearer) one element is returned, for HMAC — two
				 *          (Signature-Input and Signature)
				 * @param result container for the collection of the headers (a name -> a value)
				 *
				 * \~
				 */
				void headers(vector <pair <string, string>> & result) noexcept;
			public:
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of parsing an incoming header of the authorization
				 * @details At a server before the parsing the credentials of the previous
				 *          request are cleared (user/pass/token, response, nonce, nc and others), the hash
				 *          is restored to the value of type(), digest.mode.sess is reset.
				 *          For HMAC the clearing is performed only before the parsing of the Signature-Input
				 *          (a value with a «(»), so that a two-step parsing through parse(header)
				 *          would not erase the already parsed Signature-Input.
				 *          For Digest the realm given through realm() is not overwritten from the header
				 *          of the client; a non-coincidence of the realm is rejected at parse(). At a server
				 *          Digest requires a username and a response in the credentials.
				 * @param header value of the header (a client: the challenge of the server, a server: the credentials)
				 * @return       result of the parsing
				 *
				 * \~
				 */
				bool parse(const string_view header) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора входящего заголовка авторизации с указанием имени
				 *
				 * @details Требуется многозаголовочным схемам (HMAC: Signature-Input и Signature)
				 *
				 * @param name   имя входящего заголовка
				 * @param header значение входящего заголовка
				 * @return       результат разбора
				 *
				 * \~english
				 * @brief Method of parsing an incoming header of the authorization with an indication of the name
				 * @details It is required by the multi-header schemes (HMAC: Signature-Input and Signature)
				 * @param name   name of the incoming header
				 * @param header value of the incoming header
				 * @return       result of the parsing
				 *
				 * \~
				 */
				bool parse(const string_view name, const string_view header) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функции проверки токена доступа (BEARER, сервер)
				 *
				 * @param callback функция проверки токена доступа
				 *
				 * \~english
				 * @brief Method of setting the function of the checking of a token of the access (BEARER, a server)
				 * @param callback function of the checking of a token of the access
				 *
				 * \~
				 */
				void callbackCheckToken(function <bool (const string &)> callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции извлечения секретного ключа (HMAC, сервер)
				 *
				 * @param callback функция извлечения секретного ключа по идентификатору
				 *
				 * \~english
				 * @brief Method of setting the function of the extraction of a secret key (HMAC, a server)
				 * @param callback function of the extraction of a secret key by an identifier
				 *
				 * \~
				 */
				void callbackExtractKey(function <string (const string &)> callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции извлечения пароля (DIGEST, сервер)
				 *
				 * @param callback функция извлечения пароля по логину
				 *
				 * \~english
				 * @brief Method of setting the function of the extraction of a password (DIGEST, a server)
				 * @param callback function of the extraction of a password by a login
				 *
				 * \~
				 */
				void callbackExtractPass(function <string (const string &)> callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции проверки пары «логин/пароль» (BASIC, сервер)
				 *
				 * @param callback функция проверки пары «логин/пароль»
				 *
				 * \~english
				 * @brief Method of setting the function of the checking of a pair «a login/a password» (BASIC, a server)
				 * @param callback function of the checking of a pair «a login/a password»
				 *
				 * \~
				 */
				void callbackCheckUser(function <bool (const string &, const string &)> callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param owner сторона работы (клиент/сервер)
				 * @param fmk   объект фреймворка
				 * @param log   объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param owner side of the work (a client/a server)
				 * @param fmk   object of the framework
				 * @param log   object for the work with the logs
				 *
				 * \~
				 */
				explicit Authorization(const owner_t owner, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Authorization() noexcept;
		} auth_t;
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../../sys/macro_pop.hpp"

#endif // __AWH_AUTH__
