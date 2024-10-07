/**
 * @file: ws1.hpp
 * @date: 2023-10-03
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

#ifndef __AWH_WEB_WS1_SERVER__
#define __AWH_WEB_WS1_SERVER__

/**
 * Наши модули
 */
#include <ws/frame.hpp>
#include <ws/server.hpp>
#include <scheme/ws.hpp>
#include <sys/threadpool.hpp>
#include <server/web/web.hpp>

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
		 * Http1 Прототип класса HTTP/1.1 сервера
		 */
		class Http1;
		/**
		 * Http2 Прототип класса HTTP/2 сервера
		 */
		class Http2;
		/**
		 * Websocket2 Прототип класса Websocket/2 сервера
		 */
		class Websocket2;
		/**
		 * Websocket1 Класс Websocket-сервера
		 */
		typedef class AWHSHARED_EXPORT Websocket1 : public web_t {
			private:
				/**
				 * Http1 Устанавливаем дружбу с классом HTTP/1.1 сервера
				 */
				friend class Http1;
				/**
				 * Http2 Устанавливаем дружбу с классом HTTP/2 сервера
				 */
				friend class Http2;
				/**
				 * Websocket2 Устанавливаем дружбу с классом Websocket/2 сервера
				 */
				friend class Websocket2;
			private:
				// Время ожидания понга
				time_t _waitPong;
			private:
				// Размер отправляемого сегмента
				size_t _frameSize;
			private:
				// Объект тредпула для работы с потоками
				thr_t _thr;
			private:
				// Объект рабочего
				scheme::ws_t _scheme;
			private:
				// Объект партнёра клиента
				scheme::ws_t::partner_t _client;
				// Объект партнёра сервера
				scheme::ws_t::partner_t _server;
			private:
				// Поддерживаемые сабпротоколы
				set <string> _subprotocols;
			private:
				// Список поддверживаемых расширений
				vector <vector <string>> _extensions;
			private:
				// Полученные HTTP заголовки
				unordered_multimap <string, string> _headers;
			private:
				/**
				 * connectEvents Метод обратного вызова при подключении к серверу
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void connectEvents(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * disconnectEvents Метод обратного вызова при отключении клиента
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void disconnectEvents(const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * readEvents Метод обратного вызова при чтении сообщения с клиента
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void readEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * writeEvents Метод обратного вызова при записи сообщение брокеру
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void writeEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * error Метод вывода сообщений об ошибках работы брокера
				 * @param bid     идентификатор брокера
				 * @param message сообщение с описанием ошибки
				 */
				void error(const uint64_t bid, const ws::mess_t & message) const noexcept;
				/**
				 * extraction Метод извлечения полученных данных
				 * @param bid    идентификатор брокера
				 * @param buffer данные в чистом виде полученные с сервера
				 * @param text   данные передаются в текстовом виде
				 */
				void extraction(const uint64_t bid, const vector <char> & buffer, const bool text) noexcept;
			private:
				/**
				 * pong Метод ответа на проверку о доступности сервера
				 * @param bid идентификатор брокера
				 * @param     message сообщение для отправки
				 */
				void pong(const uint64_t bid, const string & message = "") noexcept;
				/**
				 * ping Метод проверки доступности сервера
				 * @param bid идентификатор брокера
				 * @param     message сообщение для отправки
				 */
				void ping(const uint64_t bid, const string & message = "") noexcept;
			private:
				/**
				 * erase Метод удаления отключившихся брокеров
				 * @param bid идентификатор брокера
				 */
				void erase(const uint64_t bid = 0) noexcept;
			private:
				/**
				 * pinging Метод таймера выполнения пинга клиента
				 * @param tid идентификатор таймера
				 */
				void pinging(const uint16_t tid) noexcept;
			public:
				/**
				 * init Метод инициализации Websocket-сервера
				 * @param socket      unix-сокет для биндинга
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & socket, const vector <http_t::compressor_t> & compressors = {http_t::compressor_t::DEFLATE}) noexcept;
				/**
				 * init Метод инициализации Websocket-сервера
				 * @param port        порт сервера
				 * @param host        хост сервера
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const uint32_t port, const string & host = "", const vector <http_t::compressor_t> & compressors = {http_t::compressor_t::DEFLATE}) noexcept;
			public:
				/**
				 * sendError Метод отправки сообщения об ошибке
				 * @param bid  идентификатор брокера
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const uint64_t bid, const ws::mess_t & mess) noexcept;
				/**
				 * sendMessage Метод отправки сообщения клиенту
				 * @param bid     идентификатор брокера
				 * @param message передаваемое сообщения в бинарном виде
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const uint64_t bid, const vector <char> & message, const bool text = true) noexcept;
			public:
				/**
				 * send Метод отправки данных в бинарном виде клиенту
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных передаваемых клиенту
				 * @param size   размер сообщения в байтах
				 * @return       результат отправки сообщения
				 */
				bool send(const uint64_t bid, const char * buffer, const size_t size) noexcept;
			public:
				/**
				 * callbacks Метод установки функций обратного вызова
				 * @param callbacks функции обратного вызова
				 */
				void callbacks(const fn_t & callbacks) noexcept;
			public:
				/**
				 * port Метод получения порта подключения брокера
				 * @param bid идентификатор брокера
				 * @return    порт подключения брокера
				 */
				uint32_t port(const uint64_t bid) const noexcept;
				/**
				 * agent Метод извлечения агента клиента
				 * @param bid идентификатор брокера
				 * @return    агент к которому относится подключённый клиент
				 */
				agent_t agent(const uint64_t bid) const noexcept;
				/**
				 * ip Метод получения IP-адреса брокера
				 * @param bid идентификатор брокера
				 * @return    адрес интернет подключения брокера
				 */
				const string & ip(const uint64_t bid) const noexcept;
				/**
				 * mac Метод получения MAC-адреса брокера
				 * @param bid идентификатор брокера
				 * @return    адрес устройства брокера
				 */
				const string & mac(const uint64_t bid) const noexcept;
			public:
				/**
				 * stop Метод остановки сервера
				 */
				void stop() noexcept;
				/**
				 * start Метод запуска сервера
				 */
				void start() noexcept;
			public:
				/**
				 * close Метод закрытия подключения брокера
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
			public:
				/**
				 * waitPong Метод установки времени ожидания ответа WebSocket-клиента
				 * @param sec время ожидания в секундах
				 */
				void waitPong(const time_t sec) noexcept;
				/**
				 * pingInterval Метод установки интервала времени выполнения пингов
				 * @param sec интервал времени выполнения пингов в секундах
				 */
				void pingInterval(const time_t sec) noexcept;
			public:
				/**
				 * subprotocol Метод установки поддерживаемого сабпротокола
				 * @param subprotocol сабпротокол для установки
				 */
				void subprotocol(const string & subprotocol) noexcept;
				/**
				 * subprotocols Метод установки списка поддерживаемых сабпротоколов
				 * @param subprotocols сабпротоколы для установки
				 */
				void subprotocols(const set <string> & subprotocols) noexcept;
				/**
				 * subprotocol Метод получения списка выбранных сабпротоколов
				 * @param bid идентификатор брокера
				 * @return    список выбранных сабпротоколов
				 */
				const set <string> & subprotocols(const uint64_t bid) const noexcept;
			public:
				/**
				 * extensions Метод установки списка расширений
				 * @param extensions список поддерживаемых расширений
				 */
				void extensions(const vector <vector <string>> & extensions) noexcept;
				/**
				 * extensions Метод извлечения списка расширений
				 * @param bid идентификатор брокера
				 * @return    список поддерживаемых расширений
				 */
				const vector <vector <string>> & extensions(const uint64_t bid) const noexcept;
			public:
				/**
				 * multiThreads Метод активации многопоточности
				 * @param count количество потоков для активации
				 * @param mode  флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const uint16_t count = 0, const bool mode = true) noexcept;
			public:
				/**
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const uint16_t total) noexcept;
				/**
				 * segmentSize Метод установки размеров сегментов фрейма
				 * @param size минимальный размер сегмента
				 */
				void segmentSize(const size_t size) noexcept;
				/**
				 * compressors Метод установки списка поддерживаемых компрессоров
				 * @param compressors список поддерживаемых компрессоров
				 */
				void compressors(const vector <http_t::compressor_t> & compressors) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
			public:
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const bool mode) noexcept;
				/**
				 * alive Метод установки времени жизни подключения
				 * @param sec время жизни подключения
				 */
				void alive(const time_t sec) noexcept;
			public:
				/**
				 * core Метод установки сетевого ядра
				 * @param core объект сетевого ядра
				 */
				void core(const server::core_t * core) noexcept;
			public:
				/**
				 * setHeaders Метод установки списка заголовков
				 * @param headers список заголовков для установки
				 */
				void setHeaders(const unordered_multimap <string, string> & headers) noexcept;
			public:
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read  количество секунд для детекции по чтению
				 * @param write количество секунд для детекции по записи
				 */
				void waitTimeDetect(const time_t read, const time_t write) noexcept;
			public:
				/**
				 * crypted Метод получения флага шифрования
				 * @param bid идентификатор брокера
				 * @return    результат проверки
				 */
				bool crypted(const uint64_t bid) const noexcept;
				/**
				 * encrypt Метод активации шифрования для клиента
				 * @param bid  идентификатор брокера
				 * @param mode флаг активации шифрования
				 */
				void encrypt(const uint64_t bid, const bool mode) noexcept;
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
				Websocket1(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Websocket1 Деструктор
				 */
				~Websocket1() noexcept;
		} ws1_t;
	};
};

#endif // __AWH_WEB_WS1_SERVER__
