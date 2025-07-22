/**
 * @file: ws2.hpp
 * @date: 2023-09-14
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

#ifndef __AWH_WEB_WS2_CLIENT__
#define __AWH_WEB_WS2_CLIENT__

/**
 * Наши модули
 */
#include "ws1.hpp"
#include "web.hpp"
#include "../../ws/frame.hpp"
#include "../../ws/client.hpp"
#include "../../sys/threadpool.hpp"

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
		 * Http2 Прототип класса HTTP/2 клиента
		 */
		class Http2;
		/**
		 * Websocket2 Класс Websocket-клиента
		 */
		typedef class AWHSHARED_EXPORT Websocket2 : public web2_t {
			private:
				/**
				 * Http2 Устанавливаем дружбу с классом HTTP/2 клиента
				 */
				friend class Http2;
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
				// Количество активных ядер
				uint16_t _threads;
			private:
				// Время ожидания понга
				uint32_t _waitPong;
				// Контрольная точка ответа на пинг
				uint64_t _respPong;
			private:
				// Объект работы с Websocket-клиентом HTTP/1.1
				ws1_t _ws1;
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
				// Хранилище функций обратного вызова для вывода результата
				callback_t _callback;
			private:
				// Формат шифрования
				hash_t::cipher_t _cipher;
			private:
				// Активный прототип интернета
				engine_t::proto_t _proto;
			private:
				// Метод компрессии данных
				http_t::compressor_t _compressor;
			private:
				// Данные фрагметрированного сообщения
				vector <char> _fragments;
				// Полученные HTTP заголовки
				std::unordered_multimap <string, string> _headers;
			private:
				/**
				 * send Метод отправки запроса на удалённый сервер
				 * @param bid идентификатор брокера
				 */
				void send(const uint64_t bid) noexcept;
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
				 * writeEvent Метод обратного вызова при записи сообщения на клиенте
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void writeEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * callbackEvent Метод отлавливания событий контейнера функций обратного вызова
				 * @param event событие контейнера функций обратного вызова
				 * @param fid   идентификатор функции обратного вызова
				 * @param fn    функция обратного вызова в чистом виде
				 */
				void callbackEvent(const callback_t::event_t event, const uint64_t fid, const callback_t::fn_t & fn) noexcept;
			private:
				/**
				 * chunkSignal Метод обратного вызова при получении чанка с сервера HTTP/2
				 * @param sid    идентификатор потока
				 * @param buffer буфер данных который содержит полученный чанк
				 * @param size   размер полученного буфера данных чанка
				 * @return       статус полученных данных
				 */
				int32_t chunkSignal(const int32_t sid, const uint8_t * buffer, const size_t size) noexcept;
				/**
				 * frameSignal Метод обратного вызова при получении фрейма заголовков сервера HTTP/2
				 * @param sid    идентификатор потока
				 * @param direct направление передачи фрейма
				 * @param type   тип полученного фрейма
				 * @param flags  флаги полученного фрейма
				 * @return       статус полученных данных
				 */
				int32_t frameSignal(const int32_t sid, const http2_t::direct_t direct, const http2_t::frame_t frame, const std::set <http2_t::flag_t> & flags) noexcept;
			private:
				/**
				 * closedSignal Метод завершения работы потока
				 * @param sid   идентификатор потока
				 * @param error флаг ошибки если присутствует
				 * @return      статус полученных данных
				 */
				int32_t closedSignal(const int32_t sid, const http2_t::error_t error) noexcept;
			private:
				/**
				 * beginSignal Метод начала получения фрейма заголовков HTTP/2 сервера
				 * @param sid идентификатор потока
				 * @return    статус полученных данных
				 */
				int32_t beginSignal(const int32_t sid) noexcept;
				/**
				 * headerSignal Метод обратного вызова при получении заголовка HTTP/2 сервера
				 * @param sid идентификатор потока
				 * @param key данные ключа заголовка
				 * @param val данные значения заголовка
				 * @return    статус полученных данных
				 */
				int32_t headerSignal(const int32_t sid, const string & key, const string & val) noexcept;
			private:
				/**
				 * answer Метод получение статуса ответа сервера
				 * @param sid    идентификатор потока
				 * @param rid    идентификатор запроса
				 * @param status статус ответа сервера
				 */
				void answer(const int32_t sid, const uint64_t rid, const awh::http_t::status_t status) noexcept;
			private:
				/**
				 * redirect Метод выполнения редиректа если требуется
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 * @return    результат выполнения редиректа
				 */
				bool redirect(const uint64_t bid, const uint16_t sid) noexcept;
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
				 * callback Метод установки функций обратного вызова
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
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
				const std::unordered_set <string> & subprotocols() const noexcept;
				/**
				 * subprotocols Метод установки списка поддерживаемых сабпротоколов
				 * @param subprotocols сабпротоколы для установки
				 */
				void subprotocols(const std::unordered_set <string> & subprotocols) noexcept;
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
				 * core Метод установки сетевого ядра
				 * @param core объект сетевого ядра
				 */
				void core(const client::core_t * core) noexcept;
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const std::set <flag_t> & flags) noexcept;
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
				void setHeaders(const std::unordered_multimap <string, string> & headers) noexcept;
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
				 * @param family семейстово интернет протоколов (IPV4 / IPV6 / IPC)
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
				 * Websocket2 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Websocket2(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * Websocket2 Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Websocket2(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Websocket2 Деструктор
				 */
				~Websocket2() noexcept;
		} ws2_t;
	};
};

#endif // __AWH_WEB_WS2_CLIENT__
