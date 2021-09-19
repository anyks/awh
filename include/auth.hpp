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
#include <unordered_map>

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
			enum class type_t : u_short {NONE, BASIC, DIGEST};
			/**
			 * Алгоритм шифрования для авторизации Digest
			 */
			enum class algorithm_t : u_short {MD5, SHA1, SHA256, SHA512};
		private:
			/**
			 * Digest Структура параметров дайджест авторизации
			 */
			typedef struct Digest {
				string nc;             // Счётчик 16-го секретного кода клиента
				string uri;            // Параметры HTTP запроса
				string qop;            // Тип авторизации (auth, auth-int)
				string realm;          // Название сервера или e-mail
				string nonce;          // Уникальный ключ клиента выдаваемый сервером
				string opaque;         // Временный ключ сессии сервера
				string cnonce;         // 16-й секретный код клиента
				string response;       // Ответ клиента
				time_t timestamp;      // Штамп времени последнего создания nonce
				algorithm_t algorithm; // Алгоритм шифрования (MD5, SHA1, SHA256, SHA512)
				/**
				 * Digest Конструктор
				 */
				Digest() : nc("00000000"), uri(""), qop("auth"), realm(AWH_HOST), nonce(""), opaque(""), cnonce(""), response(""), timestamp(0), algorithm(algorithm_t::MD5) {}
			} digest_t;
		private:
			bool server;                          // Флаг работы в режиме сервера
			type_t type;                          // Тип авторизации
			digest_t digest;                      // Параметры Digest авторизации
			string username;                      // Логин пользователя
			string password;                      // Пароль пользователя
			unordered_map <string, string> users; // Список пользователей для Basic авторизации
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
		public:
			/**
			 * check Метод проверки авторизации
			 * @param auth параметры авторизации
			 * @return     результат проверки авторизации
			 */
			const bool check(Authorization & auth) noexcept;
			/**
			 * check Метод проверки авторизации
			 * @param username логин пользователя
			 * @param password пароль пользователя
			 * @return         результат проверки авторизации
			 */
			const bool check(const string & username, const string & password) noexcept;
			/**
			 * check Метод проверки авторизации
			 * @param username логин пользователя для проверки
			 * @param nc       счётчик HTTP запроса
			 * @param uri      параметры HTTP запроса
			 * @param cnonce   ключ сгенерированный клиентом
			 * @param response хэш ответа клиента
			 * @return         результат проверки авторизации
			 */
			const bool check(const string & username, const string & nc, const string & uri, const string & cnonce, const string & response) noexcept;
		public:
			/**
			 * clearUsers Метод очистки списка пользователей
			 */
			void clearUsers() noexcept;
			/**
			 * setUsers Метод добавления списка пользователей
			 * @param users список пользователей для добавления
			 */
			void setUsers(const unordered_map <string, string> & users) noexcept;
		public:
			/**
			 * setUri Метод установки параметров HTTP запроса
			 * @param uri строка параметров HTTP запроса
			 */
			void setUri(const string & uri) noexcept;
			/**
			 * setRealm Метод установки название сервера
			 * @param realm название сервера
			 */
			void setRealm(const string & realm) noexcept;
			/**
			 * setNonce Метод установки уникального ключа клиента выданного сервером
			 * @param nonce уникальный ключ клиента
			 */
			void setNonce(const string & nonce) noexcept;
			/**
			 * setHeader Метод установки параметров авторизации из заголовков
			 * @param header заголовок HTTP с параметрами авторизации
			 */
			void setHeader(const string & header) noexcept;
			/**
			 * setOpaque Метод установки временного ключа сессии сервера
			 * @param opaque временный ключ сессии сервера
			 */
			void setOpaque(const string & opaque) noexcept;
			/**
			 * setLogin Метод установки логина пользователя
			 * @param username логин пользователя для установки
			 */
			void setLogin(const string & username) noexcept;
			/**
			 * setPassword Метод установки пароля пользователя
			 * @param password пароль пользователя для установки
			 */
			void setPassword(const string & password) noexcept;
			/**
			 * setFramework Метод установки объекта фреймворка
			 * @param fmk объект фреймворка для установки
			 * @param log объект для работы с логами
			 */
			void setFramework(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * setType Метод установки типа авторизации
			 * @param type      тип авторизации
			 * @param algorithm алгоритм шифрования для Digest авторизации
			 */
			void setType(const type_t type, const algorithm_t algorithm = algorithm_t::MD5) noexcept;
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
			 * getLogin Метод получения имени пользователя
			 * @return установленное имя пользователя
			 */
			const string & getLogin() const noexcept;
			/**
			 * getPassword Метод получения пароля пользователя
			 * @return установленный пароль пользователя
			 */
			const string & getPassword() const noexcept;
		public:
			/**
			 * header Метод получения строки авторизации HTTP заголовка
			 * @return строка авторизации
			 */
			const string header() noexcept;
			/**
			 * response Метод создания ответа на дайджест авторизацию
			 * @param digest   параметры дайджест авторизации
			 * @param username логин пользователя для проверки
			 * @param password пароль пользователя для проверки
			 * @return         ответ в 16-м виде
			 */
			const string response(const string & username, const string & password, const digest_t & digest) const noexcept;
		public:
			/**
			 * Authorization Конструктор
			 * @param fmk    объект фреймворка
			 * @param log    объект для работы с логами
			 * @param server флаг работы в режиме сервера
			 */
			Authorization(const fmk_t * fmk = nullptr, const log_t * log = nullptr, const bool server = false) : fmk(fmk), log(log), server(server) {}
	} auth_t;
};

#endif // __AWH_AUTH__
