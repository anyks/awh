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
#include "core.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief клиентское пространство имён
	 *
	 */
	namespace client {
		/**
		 * @brief Класс работы с авторизацией клиента
		 *
		 */
		typedef class AWHSHARED_EXPORT Auth : public auth_t {
			public:
				/**
				 * @brief Структура данных авторизации
				 *
				 */
				typedef struct Data {
					const type_t * type;     // Тип авторизации
					const digest_t * digest; // Параметры Digest авторизации
					const string * user;     // Логин пользователя
					const string * pass;     // Пароль пользователя
					/**
					 * @brief Конструктор
					 *
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
				 * @brief Метод извлечения данных авторизации
				 *
				 * @return данные модуля авторизации
				 */
				data_t data() const noexcept;
				/**
				 * @brief Метод установки данных авторизации
				 *
				 * @param data данные авторизации для установки
				 */
				void data(const data_t & data) noexcept;
			public:
				/**
				 * @brief Метод установки параметров HTTP запроса
				 *
				 * @param uri строка параметров HTTP запроса
				 */
				void uri(const string & uri) noexcept;
			public:
				/**
				 * @brief Метод установки логина пользователя
				 *
				 * @param user логин пользователя для установки
				 */
				void user(const string & user) noexcept;
				/**
				 * @brief Метод установки пароля пользователя
				 *
				 * @param pass пароль пользователя для установки
				 */
				void pass(const string & pass) noexcept;
			public:
				/**
				 * @brief Метод установки параметров авторизации из заголовков
				 *
				 * @param header заголовок HTTP с параметрами авторизации
				 */
				void header(const string & header) noexcept;
			public:
				/**
				 * @brief Метод получения строки авторизации HTTP-заголовка
				 *
				 * @param method метод HTTP-запроса
				 * @return       строка авторизации
				 */
				string auth(const string & method) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Auth(const fmk_t * fmk, const log_t * log) noexcept : auth_t(fmk, log), _user{""}, _pass{""} {}
		} auth_t;
	};
};

#endif // __AWH_AUTH_CLIENT__
