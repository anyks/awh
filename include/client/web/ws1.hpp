/**
 * @file: ws1.hpp
 * @date: 2023-09-13
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

#ifndef __AWH_WEB_WS1_CLIENT__
#define __AWH_WEB_WS1_CLIENT__

/**
 * Наши модули
 */
#include <ws/frame.hpp>
#include <ws/client.hpp>
#include <sys/threadpool.hpp>
#include <client/web/web.hpp>

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
		 * Http1 Прототип класса HTTP/1.1 клиента
		 */
		class Http1;
		/**
		 * Http2 Прототип класса HTTP/2 клиента
		 */
		class Http2;
		/**
		 * Websocket2 Прототип класса Websocket/2 клиента
		 */
		class Websocket2;
		/**
		 * Websocket1 Класс Websocket-клиента
		 */
		typedef class AWHSHARED_EXPORT Websocket1 : public web_t {
			private:
				/**
				 * Http1 Устанавливаем дружбу с классом HTTP/1.1 клиента
				 */
				friend class Http1;
				/**
				 * Http2 Устанавливаем дружбу с классом HTTP/2 клиента
				 */
				friend class Http2;
				/**
				 * Websocket2 Устанавливаем дружбу с классом Websocket/2 клиента
				 */
				friend class Websocket2;
			private:
				/**
				 * Allow Структура флагов разрешения обменом данных
				 */
				typedef struct Allow {
					bool send;    // Флаг разрешения отправки данных
					bool receive; // Флаг разрешения чтения данных
					/**
					 * Allow Конструктор
					 */
					Allow() noexcept : send(true), receive(true) {}
				} __attribute__((packed)) allow_t;
				/**
				 * Partner Структура партнёра
				 */
				typedef struct Partner {
					int16_t wbit;  // Размер скользящего окна
					bool takeover; // Флаг скользящего контекста сжатия
					/**
					 * Partner Конструктор
					 */
					Partner() noexcept : wbit(0), takeover(false) {}
				} __attribute__((packed)) partner_t;
				/**
				 * Frame Объект фрейма Websocket
				 */
				typedef struct Frame {
					size_t size;                  // Минимальный размер сегмента
					ws::frame_t methods;          // Методы работы с фреймом Websocket
					ws::frame_t::opcode_t opcode; // Полученный опкод сообщения
					/**
					 * Frame Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Frame(const fmk_t * fmk, const log_t * log) noexcept :
					 size(AWH_CHUNK_SIZE), methods(fmk, log),
					 opcode(ws::frame_t::opcode_t::TEXT) {}
				} frame_t;
			private:
				// Идентификатор подключения
				int32_t _sid;
			private:
				// Идентификатор запроса
				uint64_t _rid;
			private:
				// Флаг разрешения вывода информационных сообщений
				bool _verb;
				// Флаг завершения работы клиента
				bool _close;
				// Флаг выполненного рукопожатия
				bool _shake;
				// Флаг фриза работы клиента
				bool _freeze;
				// Флаг шифрования сообщений
				bool _crypted;
				// Флаг переданных сжатых данных
				bool _inflate;
			private:
				// Время ожидания понга
				uint32_t _waitPong;
				// Контрольная точка ответа на пинг
				uint64_t _respPong;
			private:
				// Объект тредпула для работы с потоками
				thr_t _thr;
				// Объект для работы с HTTP-протколом
				ws_t _http;
				// Объект хэширования
				hash_t _hash;
				// Объект фрейма Websocket
				frame_t _frame;
				// Объект разрешения обмена данными
				allow_t _allow;
				// Объект отправляемого сообщения
				ws::mess_t _mess;
			private:
				// Объект партнёра клиента
				partner_t _client;
				// Объект партнёра сервера
				partner_t _server;
			private:
				// Формат шифрования
				hash_t::cipher_t _cipher;
			private:
				// Хранилище функций обратного вызова для вывода результата
				fn_t _resultCallback;
			private:
				// Метод компрессии данных
				http_t::compressor_t _compressor;
			private:
				// Данные фрагметрированного сообщения
				vector <char> _fragmes;
				// Полученные HTTP заголовки
				unordered_multimap <string, string> _headers;
			private:
				/**
				 * connectEvent Метод обратного вызова при подключении к серверу
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void connectEvent(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * disconnectEvent Метод обратного вызова при отключении от сервера
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void disconnectEvent(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * readEvent Метод обратного вызова при чтении сообщения с сервера
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void readEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * writeCallback Метод обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * redirect Метод выполнения редиректа если требуется
				 * @return результат выполнения редиректа
				 */
				bool redirect() noexcept;
			private:
				/**
				 * response Метод получения ответа сервера
				 * @param bid     идентификатор брокера
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 */
				void response(const uint64_t bid, const uint32_t code, const string & message) noexcept;
			private:
				/**
				 * header Метод получения заголовка
				 * @param bid   идентификатор брокера
				 * @param key   ключ заголовка
				 * @param value значение заголовка
				 */
				void header(const uint64_t bid, const string & key, const string & value) noexcept;
				/**
				 * headers Метод получения заголовков
				 * @param bid     идентификатор брокера
				 * @param code    код ответа сервера
				 * @param message сообщение ответа сервера
				 * @param headers заголовки ответа сервера
				 */
				void headers(const uint64_t bid, const uint32_t code, const string & message, const unordered_multimap <string, string> & headers) noexcept;
			private:
				/**
				 * chunking Метод обработки получения чанков
				 * @param bid   идентификатор брокера
				 * @param chunk бинарный буфер чанка
				 * @param http  объект модуля HTTP
				 */
				void chunking(const uint64_t bid, const vector <char> & chunk, const awh::http_t * http) noexcept;
			private:
				/**
				 * eventCallback Метод отлавливания событий контейнера функций обратного вызова
				 * @param event событие контейнера функций обратного вызова
				 * @param idw   идентификатор функции обратного вызова
				 * @param name  название функции обратного вызова
				 * @param dump  дамп данных функции обратного вызова
				 */
				void eventCallback(const fn_t::event_t event, const uint64_t idw, const string & name, const fn_t::dump_t * dump) noexcept;
			private:
				/**
				 * flush Метод сброса параметров запроса
				 */
				void flush() noexcept;
			private:
				/**
				 * pinging Метод таймера выполнения пинга удалённого сервера
				 * @param tid идентификатор таймера
				 */
				void pinging(const uint16_t tid) noexcept;
			private:
				/**
				 * ping Метод проверки доступности сервера
				 * @param buffer бинарный буфер отправляемого сообщения
				 * @param size   размер бинарного буфера отправляемого сообщения
				 */
				void ping(const void * buffer = nullptr, const size_t size = 0) noexcept;
				/**
				 * pong Метод ответа на проверку о доступности сервера
				 * @param buffer бинарный буфер отправляемого сообщения
				 * @param size   размер бинарного буфера отправляемого сообщения
				 */
				void pong(const void * buffer = nullptr, const size_t size = 0) noexcept;
			private:
				/**
				 * prepare Метод выполнения препарирования полученных данных
				 * @param sid идентификатор запроса
				 * @param bid идентификатор брокера
				 * @return    результат препарирования
				 */
				status_t prepare(const int32_t sid, const uint64_t bid) noexcept;
			private:
				/**
				 * error Метод вывода сообщений об ошибках работы клиента
				 * @param message сообщение с описанием ошибки
				 */
				void error(const ws::mess_t & message) const noexcept;
				/**
				 * extraction Метод извлечения полученных данных
				 * @param buffer данные в чистом виде полученные с сервера
				 * @param text   данные передаются в текстовом виде
				 */
				void extraction(const vector <char> & buffer, const bool text) noexcept;
			public:
				/**
				 * sendError Метод отправки сообщения об ошибке
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const ws::mess_t & mess) noexcept;
			public:
				/**
				 * sendMessage Метод отправки сообщения на сервер
				 * @param message передаваемое сообщения в бинарном виде
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const vector <char> & message, const bool text = true) noexcept;
				/**
				 * sendMessage Метод отправки сообщения на сервер
				 * @param message передаваемое сообщения в бинарном виде
				 * @param size    размер передаваемого сообещния
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const char * message, const size_t size, const bool text = true) noexcept;
			public:
				/**
				 * send Метод отправки данных в бинарном виде серверу
				 * @param buffer буфер бинарных данных передаваемых серверу
				 * @param size   размер сообщения в байтах
				 * @return       результат отправки сообщения
				 */
				bool send(const char * buffer, const size_t size) noexcept;
			public:
				/**
				 * pause Метод установки на паузу клиента
				 */
				void pause() noexcept;
				/**
				 * stop Метод остановки клиента
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска клиента
				 */
				void start() noexcept;
			public:
				/**
				 * waitPong Метод установки времени ожидания ответа WebSocket-сервера
				 * @param sec время ожидания в секундах
				 */
				void waitPong(const uint16_t sec) noexcept;
				/**
				 * pingInterval Метод установки интервала времени выполнения пингов
				 * @param sec интервал времени выполнения пингов в секундах
				 */
				void pingInterval(const uint16_t sec) noexcept;
			public:
				/**
				 * callbacks Метод установки функций обратного вызова
				 * @param callbacks функции обратного вызова
				 */
				void callbacks(const fn_t & callbacks) noexcept;
			public:
				/**
				 * subprotocol Метод установки поддерживаемого сабпротокола
				 * @param subprotocol сабпротокол для установки
				 */
				void subprotocol(const string & subprotocol) noexcept;
				/**
				 * subprotocol Метод получения списка выбранных сабпротоколов
				 * @return список выбранных сабпротоколов
				 */
				const unordered_set <string> & subprotocols() const noexcept;
				/**
				 * subprotocols Метод установки списка поддерживаемых сабпротоколов
				 * @param subprotocols сабпротоколы для установки
				 */
				void subprotocols(const unordered_set <string> & subprotocols) noexcept;
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
				 * setHeaders Метод установки списка заголовков
				 * @param headers список заголовков для установки
				 */
				void setHeaders(const unordered_multimap <string, string> & headers) noexcept;
			public:
				/**
				 * userAgent Метод установки User-Agent для HTTP-запроса
				 * @param userAgent агент пользователя для HTTP-запроса
				 */
				void userAgent(const string & userAgent) noexcept;
				/**
				 * ident Метод установки идентификации клиента
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void ident(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * multiThreads Метод активации многопоточности
				 * @param count количество потоков для активации
				 * @param mode  флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const uint16_t count = 0, const bool mode = true) noexcept;
			public:
				/**
				 * proxy Метод активации/деактивации прокси-склиента
				 * @param work флаг активации/деактивации прокси-клиента
				 */
				void proxy(const client::scheme_t::work_t work) noexcept;
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
				 * crypted Метод получения флага шифрования
				 * @return результат проверки
				 */
				bool crypted() const noexcept;
			public:
				/**
				 * encryption Метод активации шифрования
				 * @param mode флаг активации шифрования
				 */
				void encryption(const bool mode) noexcept;
				/**
				 * encryption Метод установки параметров шифрования
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * Websocket1 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Websocket1(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Websocket1 Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Websocket1(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Websocket1 Деструктор
				 */
				~Websocket1() noexcept;
		} ws1_t;
	};
};

#endif // __AWH_WEB_WS1_CLIENT__
