/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_AUTH__
#define __AWH_AUTH__

/**
 * Стандартная библиотека
 */
#include <string>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <time.h>
// Если - это Unix
#else
	#include <ctime>
#endif

/**
 * Наши модули
 */
#include <fmk.hpp>
#include <log.hpp>
#include <base64.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Authorization Класс работы с авторизацией
	 */
	typedef class Authorization {
		public:
			/**
			 * Тип авторизации
			 */
			enum class type_t : uint8_t {NONE, BASIC, DIGEST};
			/**
			 * Алгоритм шифрования для авторизации Digest
			 */
			enum class alg_t : uint8_t {MD5, SHA1, SHA256, SHA512};
		protected:
			/**
			 * Digest Структура параметров дайджест авторизации
			 */
			typedef struct Digest {
				alg_t alg;     // Алгоритм шифрования (MD5, SHA1, SHA256, SHA512)
				time_t stamp;  // Штамп времени последнего создания nonce
				string nc;     // Счётчик 16-го секретного кода клиента
				string uri;    // Параметры HTTP запроса
				string qop;    // Тип авторизации (auth, auth-int)
				string resp;   // Результат ответа клиента
				string realm;  // Название сервера или e-mail
				string nonce;  // Уникальный ключ клиента выдаваемый сервером
				string opaque; // Временный ключ сессии сервера
				string cnonce; // 16-й секретный код клиента
				/**
				 * Digest Конструктор
				 */
				Digest() :
					nc("00000000"), uri(""),
					qop("auth"), realm(AWH_HOST),
					nonce(""), opaque(""),
					cnonce(""), resp(""),
					stamp(0), alg(alg_t::MD5) {}
			} digest_t;
		protected:
			// Тип авторизации
			type_t type;
			// Параметры Digest авторизации
			digest_t digest;
		protected:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
		public:
			/**
			 * getType Метод получени типа авторизации
			 * @return тип авторизации
			 */
			const type_t getType() const noexcept;
			/**
			 * getDigest Метод получения параметров Digest авторизации
			 * @return параметры Digest авторизации
			 */
			const digest_t & getDigest() const noexcept;
		public:
			/**
			 * setHeader Метод установки параметров авторизации из заголовков
			 * @param header заголовок HTTP с параметрами авторизации
			 */
			virtual void setHeader(const string & header) noexcept = 0;
			/**
			 * getHeader Метод получения строки авторизации HTTP заголовка
			 * @param mode режим вывода только значения заголовка
			 * @return     строка авторизации
			 */
			virtual const string getHeader(const bool mode = false) noexcept = 0;
		protected:
			/**
			 * response Метод создания ответа на дайджест авторизацию
			 * @param digest параметры дайджест авторизации
			 * @param user   логин пользователя для проверки
			 * @param pass   пароль пользователя для проверки
			 * @return       ответ в 16-м виде
			 */
			const string response(const string & user, const string & pass, const digest_t & digest) const noexcept;
		public:
			/**
			 * setType Метод установки типа авторизации
			 * @param type тип авторизации
			 * @param alg  алгоритм шифрования для Digest авторизации
			 */
			void setType(const type_t type, const alg_t alg = alg_t::MD5) noexcept;
		public:
			/**
			 * Authorization Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Authorization(const fmk_t * fmk, const log_t * log) noexcept : type(type_t::NONE), fmk(fmk), log(log) {}
			/*
			 * ~Authorization Деструктор
			 */
			virtual ~Authorization() noexcept {}
	} auth_t;
};

#endif // __AWH_AUTH__
