/**
 * @file: server.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_SOCKS5_SERVER__
#define __AWH_SOCKS5_SERVER__

/**
 * Стандартная библиотека
 */
#include <functional>

/**
 * Наши модули
 */
#include <socks5/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/*
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * Socks5 Класс сервера для работы с socks5 прокси-сервером
		 */
		typedef class Socks5 : public awh::socks5_t {
			public:
				/**
				 * Server Структура параметров запрашиваемого сервера
				 */
				typedef struct Server {
					int family;  // Тип хоста сервера IPv4/IPv6
					u_int port;  // Порт сервера
					string host; // Хост сервера
					/**
					 * Server Конструктор
					 */
					Server() : family(AF_INET), port(80), host("") {}
				} serv_t;
			private:
				// Параметры запрашиваемого сервера
				serv_t server;
			private:
				// Список контекстов передаваемых объектов
				vector <void *> ctx = {nullptr};
			private:
				// Внешняя функция проверки авторизации
				function <bool (const string &, const string &, void *)> authFn = nullptr;
			public:
				/**
				 * getServer Метод извлечения параметров запрашиваемого сервера
				 * @return параметры запрашиваемого сервера
				 */
				const serv_t & getServer() const noexcept;
			public:
				/**
				 * resCmd Метод получения бинарного буфера ответа
				 * @param rep код ответа сервера
				 */
				void resCmd(const rep_t rep) const noexcept;
				/**
				 * resMethod Метод получения бинарного буфера выбора метода подключения
				 * @param methods методы авторизаций выбранныйе пользователем
				 */
				void resMethod(const vector <uint8_t> & methods) const noexcept;
				/**
				 * resAuth Метод получения бинарного буфера ответа на авторизацию клиента
				 * @param login    логин пользователя
				 * @param password пароль пользователя
				 */
				void resAuth(const string & login, const string & password) const noexcept;
			public:
				/**
				 * parse Метод парсинга входящих данных
				 * @param buffer бинарный буфер входящих данных
				 * @param size   размер бинарного буфера входящих данных
				 */
				void parse(const char * buffer = nullptr, const size_t size = 0) noexcept;
			public:
				/**
				 * reset Метод сброса собранных данных
				 */
				void reset() noexcept;
			public:
				/**
				 * setAuthCallback Метод добавления функции обработки авторизации
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void setAuthCallback(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept;
			public:
				/**
				 * Socks5 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 * @param uri объект для работы с URI
				 */
				Socks5(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : awh::socks5_t(fmk, log, uri) {}
				/**
				 * ~Socks5 Деструктор
				 */
				~Socks5() noexcept {}
		} socks5_t;
	};
};

#endif // __AWH_SOCKS5_SERVER__
