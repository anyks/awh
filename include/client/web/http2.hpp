/**
 * @file: http2.hpp
 * @date: 2023-09-18
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

#ifndef __AWH_WEB_HTTP2_CLIENT__
#define __AWH_WEB_HTTP2_CLIENT__

/**
 * Наши модули
 */
#include <client/web/web.hpp>
#include <client/web/ws2.hpp>
#include <client/web/http1.hpp>

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
		 * Http2 Класс HTTP2-клиента
		 */
		typedef class Http2 : public web2_t {
			private:
				/**
				 * Worker Структура активного воркера
				 */
				typedef struct Worker {
					bool update;             // Флаг обновления параметров запроса
					http_t http;             // Объект для работы с HTTP
					fn_t callback;           // Объект функций обратного вызова
					agent_t agent;           // Агент воркера
					engine_t::proto_t proto; // Активный прототип интернета
					/**
					 * Worker Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Worker(const fmk_t * fmk, const log_t * log) noexcept :
					 update(false), http(fmk, log), callback(log),
					 agent(agent_t::HTTP), proto(engine_t::proto_t::HTTP1_1) {}
					/**
					 * ~Worker Деструктор
					 */
					~Worker() noexcept {}
				} worker_t;
			private:
				// Объект для работы c WebSocket
				ws2_t _ws2;
				// Объект для работы с HTTP/1.1 клиентом
				http1_t _http1;
			private:
				// Объект для работы с HTTP-протколом
				http_t _http;
			private:
				// Количество активных ядер
				ssize_t _threads;
				// Размер фрейма WebSocket
				size_t _frameSize;
			private:
				// Список активных воркеров
				map <int32_t, unique_ptr <worker_t>> _workers;
				// Список активых запросов
				map <int32_t, unique_ptr <request_t>> _requests;
			private:
				/**
				 * connectCallback Метод обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * disconnectCallback Метод обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void disconnectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * readCallback Метод обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void readCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
				/**
				 * writeCallback Метод обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param sid    идентификатор схемы сети
				 * @param core   объект сетевого ядра
				 */
				void writeCallback(const char * buffer, const size_t size, const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * persistCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 */
				void persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * frameSignal Метод обратного вызова при получении фрейма заголовков сервера HTTP/2
				 * @param sid   идентификатор потока
				 * @param type  тип полученного фрейма
				 * @param flags флаг полученного фрейма
				 * @return      статус полученных данных
				 */
				int frameSignal(const int32_t sid, const uint8_t type, const uint8_t flags) noexcept;
				/**
				 * chunkSignal Метод обратного вызова при получении чанка с сервера HTTP/2
				 * @param sid    идентификатор потока
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				int chunkSignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept;
			private:
				/**
				 * beginSignal Метод начала получения фрейма заголовков HTTP/2 сервера
				 * @param sid идентификатор потока
				 * @return    статус полученных данных
				 */
				int beginSignal(const int32_t sid) noexcept;
				/**
				 * closedSignal Метод завершения работы потока
				 * @param sid   идентификатор потока
				 * @param error флаг ошибки HTTP/2 если присутствует
				 * @return      статус полученных данных
				 */
				int closedSignal(const int32_t sid, const uint32_t error) noexcept;
				/**
				 * headerSignal Метод обратного вызова при получении заголовка HTTP/2 сервера
				 * @param sid идентификатор потока
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				int headerSignal(const int32_t sid, const string & key, const string & val) noexcept;
			private:
				/**
				 * redirect Метод выполнения редиректа если требуется
				 * @param aid  идентификатор адъютанта
				 * @param sid  идентификатор схемы сети
				 * @param core объект сетевого ядра
				 * @return     результат выполнения редиректа
				 */
				bool redirect(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept;
			private:
				/**
				 * flush Метод сброса параметров запроса
				 */
				void flush() noexcept;
			private:
				/**
				 * update Метод обновления параметров запроса для переадресации
				 * @param request параметры запроса на удалённый сервер
				 * @return        предыдущий идентификатор потока, если произошла переадресация
				 */
				int32_t update(request_t & request) noexcept;
			private:
				/**
				 * prepare Метод выполнения препарирования полученных данных
				 * @param sid  идентификатор потока
				 * @param aid  идентификатор адъютанта
				 * @param core объект сетевого ядра
				 * @return     результат препарирования
				 */
				status_t prepare(const int32_t sid, const uint64_t aid, client::core_t * core) noexcept;
			private:
				/**
				 * stream Метод вывода статус потока
				 * @param sid  идентификатор потока
				 * @param mode активный статус потока
				 */
				void stream(const int32_t sid, const mode_t mode) noexcept;
			public:
				/**
				 * sendError Метод отправки сообщения об ошибке
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const ws::mess_t & mess) noexcept;
				/**
				 * send Метод отправки сообщения на сервер
				 * @param agent   агент воркера
				 * @param request параметры запроса на удалённый сервер
				 * @return        идентификатор отправленного запроса
				 */
				int32_t send(const agent_t agent, const request_t & request) noexcept;
				/**
				 * send Метод отправки сообщения на сервер
				 * @param message буфер сообщения в бинарном виде
				 * @param size    размер сообщения в байтах
				 * @param utf8    данные передаются в текстовом виде
				 */
				void send(const char * message, const size_t size, const bool utf8 = true) noexcept;
			public:
				/**
				 * pause Метод установки на паузу клиента
				 */
				void pause() noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const mode_t)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие получения ошибок
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const u_int, const string &)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const vector <char> &, const bool)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова для перехвата полученных чанков
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const uint64_t, const vector <char> &, const awh::http_t *)> callback) noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие получения ошибки
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const log_t::flag_t, const http::error_t, const string &)> callback) noexcept;
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
				 * on Метод установки функции вывода полученного чанка бинарных данных с сервера
				 * @param callback функция обратного вызова
				 */
				void on(function <void (const int32_t, const vector <char> &)> callback) noexcept;
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
				 * sub Метод получения выбранного сабпротокола
				 * @return выбранный сабпротокол
				 */
				const string & sub() const noexcept;
				/**
				 * sub Метод установки сабпротокола поддерживаемого сервером
				 * @param sub сабпротокол для установки
				 */
				void sub(const string & sub) noexcept;
				/**
				 * subs Метод установки списка сабпротоколов поддерживаемых сервером
				 * @param subs сабпротоколы для установки
				 */
				void subs(const vector <string> & subs) noexcept;
			public:
				/**
				 * extensions Метод извлечения списка расширений
				 * @return список поддерживаемых расширений
				 */
				const vector <vector <string>> & extensions() const noexcept;
				/**
				 * extensions Метод установки списка расширений
				 * @param extensions список поддерживаемых расширений
				 */
				void extensions(const vector <vector <string>> & extensions) noexcept;
			public:
				/**
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
				/**
				 * segmentSize Метод установки размеров сегментов фрейма
				 * @param size минимальный размер сегмента
				 */
				void segmentSize(const size_t size) noexcept;
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
				/**
				 * core Метод установки сетевого ядра
				 * @param core объект сетевого ядра
				 */
				void core(const client::core_t * core) noexcept;
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
				 * multiThreads Метод активации многопоточности
				 * @param threads количество потоков для активации
				 * @param mode    флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const size_t threads = 0, const bool mode = true) noexcept;
			public:
				/**
				 * proxy Метод установки прокси-сервера
				 * @param uri    параметры прокси-сервера
				 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
				 */
				void proxy(const string & uri, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
				/**
				 * authTypeProxy Метод установки типа авторизации прокси-сервера
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest-авторизации
				 */
				void authTypeProxy(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
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
				 * Http2 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Http2(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Http2 Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Http2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Http2 Деструктор
				 */
				~Http2() noexcept {}
		} http2_t;
	};
};

#endif // __AWH_WEB_HTTP2_CLIENT__
