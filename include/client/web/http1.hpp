/**
 * @file: http1.hpp
 * @date: 2023-09-12
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_WEB_HTTP1_CLIENT__
#define __AWH_WEB_HTTP1_CLIENT__

/**
 * Наши модули
 */
#include <client/web/web.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * client клиентское пространство имён
	 */
	namespace client {
		/**
		 * Http2 Прототип класса HTTP/2 клиента
		 */
		class Http2;
		/**
		 * Http1 Класс HTTP-клиента
		 */
		typedef class Http1 : public web_t {
			private:
				/**
				 * Http2 Устанавливаем дружбу с классом HTTP/2 клиента
				 */
				friend class Http2;
			private:
				// Флаг открытия подключения
				bool _mode;
			private:
				// Объект для работы с HTTP-протколом
				http_t _http;
			private:
				// Объект функций обратного вызова для вывода результата
				fn_t _resultCallback;
			private:
				// Список активых запросов
				map <int32_t, request_t> _requests;
			private:
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectCallback Метод обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * response Метод получения ответа сервера
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 */
				void response(const u_int code, const string & message) noexcept;
			private:
				/**
				 * header Метод получения заголовка
				 * @param key   ключ заголовка
				 * @param value значение заголовка
				 */
				void header(const string & key, const string & value) noexcept;
				/**
				 * headers Метод получения заголовков
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 * @param headers заголовки ответа сервера
				 */
				void headers(const u_int code, const string & message, const unordered_multimap <string, string> & headers) noexcept;
			private:
				/**
				 * chunking Метод обработки получения чанков
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				void chunking(const vector <char> & chunk, const awh::http_t * http) noexcept;
			private:
				/**
				 * flush Метод сброса параметров запроса
				 */
				void flush() noexcept;
			private:
				/**
				 * prepare Метод выполнения препарирования полученных данных
				 * @param id   идентификатор запроса
				 * @param aid  идентификатор адъютанта
				 * @param core объект сетевого ядра
				 * @return     результат препарирования
				 */
				status_t prepare(const int32_t id, const size_t aid, client::core_t * core) noexcept;
			private:
				/** 
				 * submit Метод выполнения удалённого запроса на сервер
				 * @param request объект запроса на удалённый сервер
				 */
				void submit(const request_t & request) noexcept;
			public:
				/**
				 * send Метод отправки сообщения на сервер
				 * @param request параметры запроса на удалённый сервер
				 * @return        идентификатор отправленного запроса
				 */
				int32_t send(const request_t & request) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова для перехвата полученных чанков
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const vector <char> &, const awh::http_t *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функция обратного вызова активности потока
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const mode_t)> callback) noexcept;
				/**
				 * on Метод выполнения редиректа с одного потока на другой (необходим для совместимости с HTTP/2)
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const int32_t)> callback) noexcept;
				/**
				 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const u_int, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного заголовка с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const string &, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученного тела данных с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const u_int, const string &, const vector <char> &)> callback) noexcept;
				/**
				 * on Метод установки функции вывода полученных заголовков с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept;
			public:
				/**
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
				/**
				 * user Метод установки параметров авторизации
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				void user(const string & login, const string & password) noexcept;
			public:
				/**
				 * userAgent Метод установки User-Agent для HTTP запроса
				 * @param userAgent агент пользователя для HTTP запроса
				 */
				void userAgent(const string & userAgent) noexcept;
				/**
				 * serv Метод установки данных сервиса
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void serv(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
			public:
				/**
				 * crypto Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				void crypto(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * Http1 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Http1(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Http1 Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Http1(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Http1 Деструктор
				 */
				~Http1() noexcept {}
		} http1_t;
	};
};

#endif // __AWH_WEB_HTTP1_CLIENT__
