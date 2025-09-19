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
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_WEB_WS1_SERVER__
#define __AWH_WEB_WS1_SERVER__

/**
 * Наши модули
 */
#include "web.hpp"
#include "../../ws/frame.hpp"
#include "../../ws/server.hpp"
#include "../../scheme/ws.hpp"
#include "../../sys/threadpool.hpp"

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
	 * @brief серверное пространство имён
	 *
	 */
	namespace server {
		/**
		 * @brief Прототип класса HTTP/1.1 сервера
		 *
		 */
		class Http1;
		/**
		 * @brief Прототип класса HTTP/2 сервера
		 *
		 */
		class Http2;
		/**
		 * @brief Прототип класса Websocket/2 сервера
		 *
		 */
		class Websocket2;
		/**
		 * @brief Класс Websocket-сервера
		 *
		 */
		typedef class AWHSHARED_EXPORT Websocket1 : public web_t {
			private:
				/**
				 * @brief Устанавливаем дружбу с классом HTTP/1.1 сервера
				 *
				 */
				friend class Http1;
				/**
				 * @brief Устанавливаем дружбу с классом HTTP/2 сервера
				 *
				 */
				friend class Http2;
				/**
				 * @brief Устанавливаем дружбу с классом Websocket/2 сервера
				 *
				 */
				friend class Websocket2;
			private:
				// Размер отправляемого сегмента
				size_t _frameSize;
			private:
				// Время ожидания понга
				uint32_t _waitPong;
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
				// Список поддверживаемых расширений
				vector <vector <string>> _extensions;
			private:
				// Поддерживаемые сабпротоколы
				std::unordered_set <string> _subprotocols;
			private:
				// Полученные HTTP заголовки
				std::unordered_multimap <string, string> _headers;
			private:
				/**
				 * @brief Метод обратного вызова при подключении к серверу
				 *
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void connectEvents(const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * @brief Метод обратного вызова при отключении клиента
				 *
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void disconnectEvents(const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * @brief Метод обратного вызова при чтении сообщения с клиента
				 *
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void readEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * @brief Метод обратного вызова при записи сообщение брокеру
				 *
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void writeEvents(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * @brief Метод вывода сообщений об ошибках работы брокера
				 *
				 * @param bid     идентификатор брокера
				 * @param message сообщение с описанием ошибки
				 */
				void error(const uint64_t bid, const ws::mess_t & message) const noexcept;
				/**
				 * @brief Метод извлечения полученных данных
				 *
				 * @param bid    идентификатор брокера
				 * @param buffer данные в чистом виде полученные с сервера
				 * @param text   данные передаются в текстовом виде
				 */
				void extraction(const uint64_t bid, const vector <char> & buffer, const bool text) noexcept;
			private:
				/**
				 * @brief Метод ответа на проверку о доступности сервера
				 *
				 * @param bid    идентификатор брокера
				 * @param buffer бинарный буфер отправляемого сообщения
				 * @param size   размер бинарного буфера отправляемого сообщения
				 */
				void pong(const uint64_t bid, const void * buffer = nullptr, const size_t size = 0) noexcept;
				/**
				 * @brief Метод проверки доступности сервера
				 *
				 * @param bid    идентификатор брокера
				 * @param buffer бинарный буфер отправляемого сообщения
				 * @param size   размер бинарного буфера отправляемого сообщения
				 */
				void ping(const uint64_t bid, const void * buffer = nullptr, const size_t size = 0) noexcept;
			private:
				/**
				 * @brief Метод удаления отключившихся брокеров
				 *
				 * @param bid идентификатор брокера
				 */
				void erase(const uint64_t bid = 0) noexcept;
			private:
				/**
				 * @brief Метод таймера выполнения пинга клиента
				 *
				 * @param tid идентификатор таймера
				 */
				void pinging(const uint16_t tid) noexcept;
			public:
				/**
				 * @brief Метод инициализации Websocket-сервера
				 *
				 * @param socket      unix-сокет для биндинга
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & socket, const vector <http_t::compressor_t> & compressors = {http_t::compressor_t::DEFLATE}) noexcept;
				/**
				 * @brief Метод инициализации Websocket-сервера
				 *
				 * @param port        порт сервера
				 * @param host        хост сервера
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const uint32_t port, const string & host = "", const vector <http_t::compressor_t> & compressors = {http_t::compressor_t::DEFLATE}) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения об ошибке
				 *
				 * @param bid  идентификатор брокера
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const uint64_t bid, const ws::mess_t & mess) noexcept;
			public:
				/**
				 * @brief Метод отправки сообщения клиенту
				 *
				 * @param bid     идентификатор брокера
				 * @param message передаваемое сообщения в бинарном виде
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const uint64_t bid, const vector <char> & message, const bool text = true) noexcept;
				/**
				 * @brief Метод отправки сообщения на сервер
				 *
				 * @param bid     идентификатор брокера
				 * @param message передаваемое сообщения в бинарном виде
				 * @param size    размер передаваемого сообещния
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const uint64_t bid, const char * message, const size_t size, const bool text = true) noexcept;
			public:
				/**
				 * @brief Метод отправки данных в бинарном виде клиенту
				 *
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных передаваемых клиенту
				 * @param size   размер сообщения в байтах
				 * @return       результат отправки сообщения
				 */
				bool send(const uint64_t bid, const char * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки функций обратного вызова
				 *
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @brief Метод получения порта подключения брокера
				 *
				 * @param bid идентификатор брокера
				 * @return    порт подключения брокера
				 */
				uint32_t port(const uint64_t bid) const noexcept;
				/**
				 * @brief Метод извлечения агента клиента
				 *
				 * @param bid идентификатор брокера
				 * @return    агент к которому относится подключённый клиент
				 */
				agent_t agent(const uint64_t bid) const noexcept;
				/**
				 * @brief Метод получения IP-адреса брокера
				 *
				 * @param bid идентификатор брокера
				 * @return    адрес интернет подключения брокера
				 */
				const string & ip(const uint64_t bid) const noexcept;
				/**
				 * @brief Метод получения MAC-адреса брокера
				 *
				 * @param bid идентификатор брокера
				 * @return    адрес устройства брокера
				 */
				const string & mac(const uint64_t bid) const noexcept;
			public:
				/**
				 * @brief Метод остановки сервера
				 *
				 */
				void stop() noexcept;
				/**
				 * @brief Метод запуска сервера
				 *
				 */
				void start() noexcept;
			public:
				/**
				 * @brief Метод закрытия подключения брокера
				 *
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
			public:
				/**
				 * @brief Метод установки времени ожидания ответа WebSocket-клиента
				 *
				 * @param sec время ожидания в секундах
				 */
				void waitPong(const uint16_t sec) noexcept;
				/**
				 * @brief Метод установки интервала времени выполнения пингов
				 *
				 * @param sec интервал времени выполнения пингов в секундах
				 */
				void pingInterval(const uint16_t sec) noexcept;
			public:
				/**
				 * @brief Метод установки поддерживаемого сабпротокола
				 *
				 * @param subprotocol сабпротокол для установки
				 */
				void subprotocol(const string & subprotocol) noexcept;
				/**
				 * @brief Метод установки списка поддерживаемых сабпротоколов
				 *
				 * @param subprotocols сабпротоколы для установки
				 */
				void subprotocols(const std::unordered_set <string> & subprotocols) noexcept;
				/**
				 * @brief Метод получения списка выбранных сабпротоколов
				 *
				 * @param bid идентификатор брокера
				 * @return    список выбранных сабпротоколов
				 */
				const std::unordered_set <string> & subprotocols(const uint64_t bid) const noexcept;
			public:
				/**
				 * @brief Метод установки списка расширений
				 *
				 * @param extensions список поддерживаемых расширений
				 */
				void extensions(const vector <vector <string>> & extensions) noexcept;
				/**
				 * @brief Метод извлечения списка расширений
				 *
				 * @param bid идентификатор брокера
				 * @return    список поддерживаемых расширений
				 */
				const vector <vector <string>> & extensions(const uint64_t bid) const noexcept;
			public:
				/**
				 * @brief Метод активации многопоточности
				 *
				 * @param count количество потоков для активации
				 * @param mode  флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const uint16_t count = 0, const bool mode = true) noexcept;
			public:
				/**
				 * @brief Метод установки максимального количества одновременных подключений
				 *
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const uint16_t total) noexcept;
				/**
				 * @brief Метод установки размеров сегментов фрейма
				 *
				 * @param size минимальный размер сегмента
				 */
				void segmentSize(const size_t size) noexcept;
				/**
				 * @brief Метод установки списка поддерживаемых компрессоров
				 *
				 * @param compressors список поддерживаемых компрессоров
				 */
				void compressors(const vector <http_t::compressor_t> & compressors) noexcept;
				/**
				 * @brief Метод установки жизни подключения
				 *
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * @brief Метод установки флагов настроек модуля
				 *
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const std::set <flag_t> & flags) noexcept;
			public:
				/**
				 * @brief Метод установки долгоживущего подключения
				 *
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод установки сетевого ядра
				 *
				 * @param core объект сетевого ядра
				 */
				void core(const server::core_t * core) noexcept;
			public:
				/**
				 * @brief Метод установки списка заголовков
				 *
				 * @param headers список заголовков для установки
				 */
				void setHeaders(const std::unordered_multimap <string, string> & headers) noexcept;
			public:
				/**
				 * @brief Метод ожидания входящих сообщений
				 *
				 * @param sec интервал времени в секундах
				 */
				void waitMessage(const uint16_t sec) noexcept;
				/**
				 * @brief Метод детекции сообщений по количеству секунд
				 *
				 * @param read  количество секунд для детекции по чтению
				 * @param write количество секунд для детекции по записи
				 */
				void waitTimeDetect(const uint16_t read, const uint16_t write) noexcept;
			public:
				/**
				 * @brief Метод получения флага шифрования
				 *
				 * @param bid идентификатор брокера
				 * @return    результат проверки
				 */
				bool crypted(const uint64_t bid) const noexcept;
				/**
				 * @brief Метод активации шифрования для клиента
				 *
				 * @param bid  идентификатор брокера
				 * @param mode флаг активации шифрования
				 */
				void encrypt(const uint64_t bid, const bool mode) noexcept;
			public:
				/**
				 * @brief Метод активации шифрования
				 *
				 * @param mode флаг активации шифрования
				 */
				void encryption(const bool mode) noexcept;
				/**
				 * @brief Метод установки параметров шифрования
				 *
				 * @param pass   пароль шифрования передаваемых данных
				 * @param salt   соль шифрования передаваемых данных
				 * @param cipher размер шифрования передаваемых данных
				 */
				void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Websocket1(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Websocket1(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Websocket1() noexcept;
		} ws1_t;
	};
};

#endif // __AWH_WEB_WS1_SERVER__
