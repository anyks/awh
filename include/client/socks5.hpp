/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_SOCKS5_CLIENT__
#define __AWH_SOCKS5_CLIENT__

/**
 * Наши модули
 */
#include <core/socks5.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Socks5Client Класс клиента для работы с socks5 прокси-сервером
	 */
	typedef class Socks5Client : public socks5_t {
		private:
			// Логин пользователя
			string login = "";
			// Пароль пользователя
			string password = "";
		public:
			/**
			 * reqCmd Метод получения бинарного буфера запроса
			 * @return бинарный буфер запроса
			 */
			vector <char> reqCmd() const noexcept;
			/**
			 * reqAuth Метод получения бинарного буфера авторизации на сервере
			 * @return бинарный буфер запроса
			 */
			vector <char> reqAuth() const noexcept;
			/**
			 * reqMethods Метод получения бинарного буфера опроса методов подключения
			 * @return бинарный буфер запроса
			 */
			vector <char> reqMethods() const noexcept;
		public:
			/**
			 * parse Метод парсинга входящих данных
			 * @param buffer бинарный буфер входящих данных
			 * @param size   размер бинарного буфера входящих данных
			 */
			void parse(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * reset Метод сброса собранных данных
			 */
			void reset() noexcept;
		public:
			/**
			 * clearUsers Метод очистки списка пользователей
			 */
			void clearUsers() noexcept;
			/**
			 * setUser Метод установки параметров авторизации
			 * @param login    логин пользователя для авторизации на сервере
			 * @param password пароль пользователя для авторизации на сервере
			 */
			void setUser(const string & login, const string & password) noexcept;
		public:
			/**
			 * Socks5Client Конструктор
			 * @param uri объект для работы с URI
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Socks5Client(const uri_t * uri, const fmk_t * fmk, const log_t * log) noexcept : socks5_t(uri, fmk, log) {}
			/**
			 * ~Socks5Client Деструктор
			 */
			~Socks5Client() noexcept {}
	} s5cli_t;
};

#endif // __AWH_SOCKS5_CLIENT__
