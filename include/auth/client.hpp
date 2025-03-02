/**
 * @file: client.hpp
 * @date: 2022-09-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_AUTH_CLIENT__
#define __AWH_AUTH_CLIENT__

/**
 * Наши модули
 */
#include <auth/core.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * AuthClient Класс работы с авторизацией клиента
		 */
		typedef class AWHSHARED_EXPORT Auth : public auth_t {
			public:
				/**
				 * Data Структура данных авторизации
				 */
				typedef struct Data {
					const type_t * type;     // Тип авторизации
					const digest_t * digest; // Параметры Digest авторизации
					const string * user;     // Логин пользователя
					const string * pass;     // Пароль пользователя
					/**
					 * Data Конструктор
					 */
					Data() noexcept :
					 type(nullptr), digest(nullptr),
					 user(nullptr), pass(nullptr) {}
				} data_t;
			private:
				// Логин пользователя
				string _user;
				// Пароль пользователя
				string _pass;
			public:
				/**
				 * data Метод извлечения данных авторизации
				 * @return данные модуля авторизации
				 */
				data_t data() const noexcept;
				/**
				 * data Метод установки данных авторизации
				 * @param data данные авторизации для установки
				 */
				void data(const data_t & data) noexcept;
			public:
				/**
				 * uri Метод установки параметров HTTP запроса
				 * @param uri строка параметров HTTP запроса
				 */
				void uri(const string & uri) noexcept;
			public:
				/**
				 * user Метод установки логина пользователя
				 * @param user логин пользователя для установки
				 */
				void user(const string & user) noexcept;
				/**
				 * pass Метод установки пароля пользователя
				 * @param pass пароль пользователя для установки
				 */
				void pass(const string & pass) noexcept;
			public:
				/**
				 * header Метод установки параметров авторизации из заголовков
				 * @param header заголовок HTTP с параметрами авторизации
				 */
				void header(const string & header) noexcept;
			public:
				/**
				 * auth Метод получения строки авторизации HTTP-заголовка
				 * @param method метод HTTP-запроса
				 * @return       строка авторизации
				 */
				string auth(const string & method) noexcept;
			public:
				/**
				 * Auth Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Auth(const fmk_t * fmk, const log_t * log) noexcept : auth_t(fmk, log), _user{""}, _pass{""} {}
		} auth_t;
	};
};

#endif // __AWH_AUTH_CLIENT__
