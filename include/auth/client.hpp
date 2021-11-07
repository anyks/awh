/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_AUTH_CLIENT__
#define __AWH_AUTH_CLIENT__

/**
 * Наши модули
 */
#include <auth/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * AuthClient Класс работы с авторизацией клиента
	 */
	typedef class AuthClient : public auth_t {
		private:
			// Логин пользователя
			string user = "";
			// Пароль пользователя
			string pass = "";
		public:
			/**
			 * setUri Метод установки параметров HTTP запроса
			 * @param uri строка параметров HTTP запроса
			 */
			void setUri(const string & uri) noexcept;
		public:
			/**
			 * setUser Метод установки логина пользователя
			 * @param user логин пользователя для установки
			 */
			void setUser(const string & user) noexcept;
			/**
			 * setPass Метод установки пароля пользователя
			 * @param pass пароль пользователя для установки
			 */
			void setPass(const string & pass) noexcept;
		public:
			/**
			 * setHeader Метод установки параметров авторизации из заголовков
			 * @param header заголовок HTTP с параметрами авторизации
			 */
			void setHeader(const string & header) noexcept;
			/**
			 * getHeader Метод получения строки авторизации HTTP заголовка
			 * @param method метод HTTP запроса
			 * @param mode   режим вывода только значения заголовка
			 * @return       строка авторизации
			 */
			const string getHeader(const string & method, const bool mode = false) noexcept;
		public:
			/**
			 * AuthClient Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			AuthClient(const fmk_t * fmk, const log_t * log) noexcept : auth_t(fmk, log) {}
	} authCli_t;
};

#endif // __AWH_AUTH_CLIENT__
