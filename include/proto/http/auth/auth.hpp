/**
 * @file: auth.hpp
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
		 * @par Пример использования (сервер)
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
		 * @par Пример использования (клиент, подпись запроса HMAC, RFC 9421)
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
		 * @par Пример использования (сервер, проверка HMAC, RFC 9421)
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
		 * @par Пример использования (клиент, Digest qop=auth-int)
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
				/**
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
				 */
				enum class mode_t : uint8_t {
					SIMPLE = 0x00, // Простой (совместимый) режим
					STRICT = 0x01  // Строгий режим (жёсткое соответствие RFC)
				};
			public:
				/**
				 * @brief Структура режима работы схемы авторизации
				 *
				 * @details Поля mode.nonceMaxAge, mode.signMaxAge и mode.signStrictMaxAge
				 *          используются схемами Digest/HMAC для ограничения срока жизни nonce/signature.
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
					 * @brief Конструктор
					 *
					 */
					explicit Mode_Digest() noexcept;
				} mode_digest_t;
				/**
				 * @brief Структура параметров Digest-авторизации
				 *
				 * @details Поля mode.sess, mode.qop, mode.authInt, nc, nonce, opaque, lncs и др. используются
				 *          внутренне схемой Digest. На сервере lncs хранит последние принятые
				 *          значения nc по ключу «логин + nonce» для защиты от replay-атак.
				 *          Таблица сбрасывается при выдаче нового nonce и при reset()/type().
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
					 * @brief Конструктор
					 *
					 */
					explicit Digest() noexcept;
				} digest_t;
			public:
				/**
				 * @brief Структура даты подписи HMAC (RFC 9421)
				 *
				 * @details Поля created и expires включаются в Signature-Input и участвуют
				 *          в расчёте подписи. На сервере usedNonces хранит уже принятые значения nonce для защиты от повторного использования подписи.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Sign_Date {
					// Штамп времени создания подписи в секундах
					uint64_t created;
					// Штамп времени истечения подписи в секундах (0 — не задано)
					uint64_t expires;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Sign_Date() noexcept;
				} sign_date_t;
				/**
				 * @brief Структура параметров авторизации подписью HMAC (RFC 9421)
				 *
				 * @details Поля created, expires и nonce включаются в Signature-Input и участвуют
				 *          в расчёте подписи. На сервере usedNonces хранит уже принятые значения
				 *          nonce для защиты от повторного использования подписи.
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
					 * @brief Конструктор
					 *
					 */
					explicit Sign() noexcept;
				} sign_t;
				/**
				 * @brief Структура обратных вызовов для проверки учётных данных (сервер)
				 *
				 * @details Функции обратных вызовов вызываются схемой авторизации на сервере
				 *          для проверки учётных данных, извлечения пароля пользователя или
				 *          секретного ключа подписи по идентификатору.
				 *          Функции должны быть зарегистрированы через методы callback*() до вызова parse().
				 */
				typedef struct __AWH_SHARED_EXPORT__ Callback {
					/**
					 * @brief Внешняя функция проверки токена доступа (BEARER, сервер)
					 *
					 * @param token токен доступа
					 * @return      результат проверки токена (true - токен действителен, false - токен недействителен)
					 */
					function <bool (const string &)> checkToken;
					/**
					 * @brief Внешняя функция извлечения секретного ключа по идентификатору (HMAC, сервер)
					 *
					 * @param keyId идентификатор ключа
					 * @return      секретный ключ (пустая строка — ключ не найден)
					 */
					function <string (const string &)> extractKey;
					/**
					 * @brief Внешняя функция извлечения пароля по логину (DIGEST, сервер)
					 *
					 * @param user логин пользователя
					 * @return     пароль пользователя (пустая строка — пользователь не найден)
					 */
					function <string (const string &)> extractPass;
					/**
					 * @brief Внешняя функция проверки пары «логин/пароль» (BASIC, сервер)
					 *
					 * @param user логин пользователя
					 * @param pass пароль пользователя
					 * @return     результат проверки пары (true - пара действительна, false - пара недействительна)
					 */
					function <bool (const string &, const string &)> checkUser;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Callback() noexcept;
				} callback_t;
			public:
				/**
				 * @brief Структура режима работы параметров авторизации
				 *
				 * @details Поля clockSkew задаёт допуск (в секундах) при проверке created/expires
				 *          подписи HMAC на сервере. Значение по умолчанию — 60 секунд.
				 *          signMaxAge ограничивает срок жизни подписи без expires (0 — без лимита).
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
					 * @brief Конструктор
					 *
					 */
					explicit Mode_Params() noexcept;
				} mode_params_t;
				/**
				 * @brief Структура общих параметров авторизации
				 *
				 * @details clockSkew задаёт допуск (в секундах) при проверке created/expires
				 *          подписи HMAC на сервере. Значение по умолчанию — 60 секунд.
				 *          signMaxAge ограничивает срок жизни подписи без expires (0 — без лимита).
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
					 * @brief Конструктор
					 *
					 */
					explicit Params() noexcept;
				} params_t;
			public:
				/**
				 * @brief Абстрактный интерфейс схемы авторизации (внутренняя стратегия)
				 *
				 * @details Конкретные схемы (Basic/Digest/Bearer/HMAC) реализуют интерфейс
				 *          scheme_t и делегируют ему методы parse()/header()/check().
				 *          Интерфейс scheme_t не должен использоваться напрямую.
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
						 */
						static bool secureCompare(const string_view left, const string_view right) noexcept;
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
						 */
						bool schemePayload(const string_view header, const string_view scheme, string & payload) const noexcept;
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
				 * @details Перед активацией новой стратегии автоматически вызывается reset().
				 *          Сохранённые user/pass/token, key/keyId, realm, uri, method, entity
				 *          и callbacks не затрагиваются.
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
				 * @details Для BASIC пароль передаётся как есть; символ «:» в пароле
				 *          не поддерживается (RFC 7617: user-pass = userid \":\" password).
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
				 * @brief Метод получения режима строгости проверки учётных данных
				 *
				 * @return режим строгости проверки (SIMPLE/STRICT)
				 */
				mode_t mode() const noexcept;
				/**
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
				 */
				void mode(const mode_t mode) noexcept;
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
			public:
				/**
				 * @brief Метод установки одноразового значения подписи (HMAC)
				 *
				 * @details Вызывается на клиенте **до** headers()/header(). Значение включается
				 *          в Signature-Input и участвует в расчёте подписи. На сервере повторная
				 *          проверка подписи с тем же nonce отклоняется (защита от replay).
				 *
				 * @param nonce одноразовое значение
				 */
				void signNonce(string_view nonce) noexcept;
				/**
				 * @brief Метод установки штампа времени создания подписи (HMAC)
				 *
				 * @details Вызывается на клиенте **до** headers()/header(). Значение попадает
				 *          в Signature-Input и участвует в канонической базе подписи.
				 *          Если передать 0, при формировании подписи будет использован текущий
				 *          штамп времени (fmk_t::timestamp).
				 *
				 * @param stamp штамп времени в секундах (0 — автоматически при формировании)
				 */
				void signCreated(const uint64_t stamp) noexcept;
				/**
				 * @brief Метод установки штампа времени истечения подписи (HMAC)
				 *
				 * @details Вызывается на клиенте **до** headers()/header(). Сервер отклоняет
				 *          подпись, если текущее время превышает expires (с учётом clockSkew).
				 *          Значение 0 означает, что срок действия не ограничен.
				 *
				 * @param stamp штамп времени в секундах (0 — не задано)
				 */
				void signExpires(const uint64_t stamp) noexcept;
			public:
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
				/**
				 * @brief Метод установки тела запроса (DIGEST, qop=auth-int)
				 *
				 * @details Для qop=auth-int необходимо явно передать entity-body запроса
				 *          до формирования или проверки учётных данных.
				 *
				 * @param entity тело HTTP-запроса (entity-body)
				 */
				void entity(string_view entity) noexcept;
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
				 * @brief Метод получения допустимого расхождения локальных часов (HMAC)
				 *
				 * @return допуск в секундах (по умолчанию 60)
				 */
				uint64_t clockSkew() const noexcept;
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
				 */
				void clockSkew(const uint64_t seconds) noexcept;
			public:
				/**
				 * @brief Метод получения максимального возраста HMAC-подписи без expires
				 *
				 * @return лимит в секундах (0 — не ограничен)
				 */
				uint64_t signMaxAge() const noexcept;
				/**
				 * @brief Метод установки максимального возраста HMAC-подписи без expires (секунды)
				 *
				 * @details Используется на сервере при check(), если клиент не передал expires.
				 *          Подпись отклоняется, когда now > created + signMaxAge + clockSkew.
				 *          Значение 0 (по умолчанию) — ограничение не применяется.
				 *          Для production-серверов HMAC рекомендуется задавать ненулевой лимит.
				 *
				 * @param seconds максимальный возраст подписи (0 — без ограничения)
				 */
				void signMaxAge(const uint64_t seconds) noexcept;
			public:
				/**
				 * @brief Метод получения максимального возраста Digest-nonce
				 *
				 * @return лимит в секундах (0 — без ограничения по времени)
				 */
				uint64_t nonceMaxAge() const noexcept;
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
				 */
				void nonceMaxAge(const uint64_t seconds) noexcept;
			public:
				/**
				 * @brief Метод получения максимального возраста HMAC-подписи без expires для строгого режима
				 *
				 * @return лимит в секундах (0 — не ограничен)
				 */
				uint64_t signStrictMaxAge() const noexcept;
				/**
				 * @brief Метод установки максимального возраста HMAC-подписи без expires для строгого режима (секунды)
				 *
				 * @details Используется на сервере при check() в строгом режиме (STRICT), если клиент
				 *          не передал expires и не задан общий signMaxAge. Значение по умолчанию — 300.
				 *          Передайте 0, чтобы отключить ограничение по умолчанию даже в строгом режиме.
				 *
				 * @param seconds максимальный возраст подписи в строгом режиме (0 — без ограничения)
				 */
				void signStrictMaxAge(const uint64_t seconds) noexcept;
			public:
				/**
				 * @brief Метод проверки учётных данных (только для сервера)
				 *
				 * @details На стороне CLIENT всегда возвращает true (проверка не выполняется).
				 *
				 * @return результат проверки
				 */
				bool check() noexcept;
			public:
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
				 */
				void reset() noexcept;
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
