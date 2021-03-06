/**
 * @file: socks5.hpp
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

#ifndef __AWH_PROXY_SOCKS5_SERVER__
#define __AWH_PROXY_SOCKS5_SERVER__

/**
 * Наши модули
 */
#include <core/client.hpp>
#include <core/server.hpp>
#include <worker/socks5.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * ProxySocks5 Класс работы с SOCKS5 PROXY сервером
		 */
		typedef class ProxySocks5 {
			public:
				/**
				 * Режим работы клиента
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01,
					DISCONNECT = 0x02
				};
				/**
				 * Режим событие клиента
				 */
				enum class event_t : uint8_t {
					REQUEST  = 0x01,
					RESPONSE = 0x02
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					DEFER    = 0x01, // Флаг отложенных вызовов событий сокета
					NOINFO   = 0x02, // Флаг запрещающий вывод информационных сообщений
					WAITMESS = 0x04 // Флаг ожидания входящих сообщений
				};
			private:
				/**
				 * Core Объект биндинга TCP/IP
				 */
				typedef struct Core {
					client::core_t client; // Объект биндинга TCP/IP для клиента
					server::core_t server; // Объект биндинга TCP/IP для сервера
					/**
					 * Core Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Core(const fmk_t * fmk, const log_t * log) : client(fmk, log), server(fmk, log) {}
				} core_t;
			private:
				// Хости сервера
				string host = "";
				// Порт сервера
				u_int port = SERVER_PORT;
			private:
				// Объект биндинга TCP/IP
				core_t core;
				// Объект рабочего для сервера
				workerSocks5_t worker;
			private:
				// Функция обратного вызова для обработки авторизации
				function <bool (const string &, const string &, void *)> checkAuthFn = nullptr;
			private:
				// Список контекстов передаваемых объектов
				vector <void *> ctx = {
					nullptr, nullptr,
					nullptr, nullptr
				};
			private:
				// Создаём объект фреймворка
				const fmk_t * fmk = nullptr;
				// Создаём объект работы с логами
				const log_t * log = nullptr;
			private:
				// Функция обратного вызова, при запуске или остановки подключения к серверу
				function <void (const size_t, const mode_t, ProxySocks5 *, void *)> activeFn = nullptr;
				// Функция обратного вызова, при получении сообщения в бинарном виде с сервера
				function <bool (const size_t, const event_t, const char *, const size_t, ProxySocks5 *, void *)> binaryFn = nullptr;
			private:
				// Функция разрешения подключения клиента на сервере
				function <bool (const string &, const string &, ProxySocks5 *, void *)> acceptFn = nullptr;
			private:
				/**
				 * runCallback Функция обратного вызова при активации ядра сервера
				 * @param mode флаг запуска/остановки
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void runCallback(const bool mode, awh::core_t * core, void * ctx) noexcept;
			private:
				/**
				 * openServerCallback Функция обратного вызова при запуске работы
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void openServerCallback(const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * persistServerCallback Функция персистентного вызова
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void persistServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * connectClientCallback Функция обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void connectClientCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * connectServerCallback Функция обратного вызова при подключении к серверу
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void connectServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * disconnectClientCallback Функция обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void disconnectClientCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * disconnectServerCallback Функция обратного вызова при отключении от сервера
				 * @param aid  идентификатор адъютанта
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 */
				static void disconnectServerCallback(const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * acceptServerCallback Функция обратного вызова при проверке подключения клиента
				 * @param ip   адрес интернет подключения клиента
				 * @param mac  мак-адрес подключившегося клиента
				 * @param wid  идентификатор воркера
				 * @param core объект биндинга TCP/IP
				 * @param ctx  передаваемый контекст модуля
				 * @return     результат разрешения к подключению клиента
				 */
				static bool acceptServerCallback(const string & ip, const string & mac, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * readClientCallback Функция обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 * @param ctx    передаваемый контекст модуля
				 */
				static void readClientCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * readServerCallback Функция обратного вызова при чтении сообщения с клиента
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 * @param ctx    передаваемый контекст модуля
				 */
				static void readServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
				/**
				 * writeServerCallback Функция обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param aid    идентификатор адъютанта
				 * @param wid    идентификатор воркера
				 * @param core   объект биндинга TCP/IP
				 * @param ctx    передаваемый контекст модуля
				 */
				static void writeServerCallback(const char * buffer, const size_t size, const size_t aid, const size_t wid, awh::core_t * core, void * ctx) noexcept;
			public:
				/**
				 * init Метод инициализации WebSocket клиента
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const u_int port, const string & host = "") noexcept;
			public:
				/**
				 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова
				 */
				void on(void * ctx, function <void (const size_t, const mode_t, ProxySocks5 *, void *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие получения сообщений в бинарном виде
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова
				 */
				void on(void * ctx, function <bool (const size_t, const event_t, const char *, const size_t, ProxySocks5 *, void *)> callback) noexcept;
			public:
				/**
				 * on Метод добавления функции обработки авторизации
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова для обработки авторизации
				 */
				void on(void * ctx, function <bool (const string &, const string &, void *)> callback) noexcept;
				/**
				 * on Метод установки функции обратного вызова на событие активации клиента на сервере
				 * @param ctx      контекст для вывода в сообщении
				 * @param callback функция обратного вызова
				 */
				void on(void * ctx, function <bool (const string &, const string &, ProxySocks5 *, void *)> callback) noexcept;
			public:
				/**
				 * ip Метод получения IP адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес интернет подключения адъютанта
				 */
				const string & ip(const size_t aid) const noexcept;
				/**
				 * mac Метод получения MAC адреса адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    адрес устройства адъютанта
				 */
				const string & mac(const size_t aid) const noexcept;
			public:
				/**
				 * start Метод запуска клиента
				 */
				void start() noexcept;
				/**
				 * stop Метод остановки клиента
				 */
				void stop() noexcept;
			public:
				/**
				 * close Метод закрытия подключения клиента
				 * @param aid идентификатор адъютанта
				 */
				void close(const size_t aid) noexcept;
			public:
				/**
				 * setMode Метод установки флага модуля
				 * @param flag флаг модуля для установки
				 */
				void setMode(const u_short flag) noexcept;
				/**
				 * setTotal Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void setTotal(const u_short total) noexcept;
				/**
				 * setWaitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read  количество секунд для детекции по чтению
				 * @param write количество секунд для детекции по записи
				 */
				void setWaitTimeDetect(const time_t read, const time_t write) noexcept;
				/**
				 * setBytesDetect Метод детекции сообщений по количеству байт
				 * @param read  количество байт для детекции по чтению
				 * @param write количество байт для детекции по записи
				 */
				void setBytesDetect(const worker_t::mark_t read, const worker_t::mark_t write) noexcept;
			public:
				/**
				 * ProxySocks5 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				ProxySocks5(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~ProxySocks5 Деструктор
				 */
				~ProxySocks5() noexcept {}
		} proxySocks5_t;
	};
};

#endif // __AWH_PROXY_SOCKS5_SERVER__
