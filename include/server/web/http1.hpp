/**
 * @file: http1.hpp
 * @date: 2023-10-01
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

#ifndef __AWH_WEB_HTTP1_SERVER__
#define __AWH_WEB_HTTP1_SERVER__

/**
 * Наши модули
 */
#include <scheme/web.hpp>
#include <server/web/web.hpp>
#include <server/web/ws1.hpp>

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
		 * Http2 Прототип класса HTTP/2 сервера
		 */
		class Http2;
		/**
		 * Http1 Класс HTTP-сервера
		 */
		typedef class AWHSHARED_EXPORT Http1 : public web_t {
			private:
				/**
				 * Http2 Устанавливаем дружбу с классом HTTP/2 сервера
				 */
				friend class Http2;
			private:
				// Флаг разрешения использования протокол Websocket
				bool _webSocket;
				// Флаг разрешения метода CONNECT на сервере
				bool _methodConnect;
			private:
				// Идентичность протокола
				http_t::identity_t _identity;
			private:
				// Объект работы с Websocket-сервером
				ws1_t _ws1;
			private:
				// Объект рабочего
				scheme::web_t _scheme;
			private:
				// Список активных агентов
				std::map <uint64_t, agent_t> _agents;
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
				 * callbacksEvents Метод отлавливания событий контейнера функций обратного вызова
				 * @param event событие контейнера функций обратного вызова
				 * @param idw   идентификатор функции обратного вызова
				 * @param name  название функции обратного вызова
				 * @param dump  дамп данных функции обратного вызова
				 */
				void callbacksEvents(const fn_t::event_t event, const uint64_t idw, const string & name, const fn_t::dump_t * dump) noexcept;
			private:
				/**
				 * websocket Метод инициализации Websocket протокола
				 * @param bid идентификатор брокера
				 * @param sid идентификатор схемы сети
				 */
				void websocket(const uint64_t bid, const uint16_t sid) noexcept;
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
				 * parser Метод извлечения объекта HTTP-парсера
				 * @param bid идентификатор брокера
				 * @return    объект HTTP-парсера
				 */
				const awh::http_t * parser(const uint64_t bid) const noexcept;
			public:
				/**
				 * trailers Метод получения запроса на передачу трейлеров
				 * @param bid идентификатор брокера
				 * @return    флаг запроса клиентом передачи трейлеров
				 */
				bool trailers(const uint64_t bid) const noexcept;
				/**
				 * trailer Метод установки трейлера
				 * @param bid идентификатор брокера
				 * @param key ключ заголовка
				 * @param val значение заголовка
				 */
				void trailer(const uint64_t bid, const string & key, const string & val) noexcept;
			public:
				/**
				 * init Метод инициализации WEB-сервера
				 * @param socket      unix-сокет для биндинга
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & socket, const vector <http_t::compressor_t> & compressors = {}) noexcept;
				/**
				 * init Метод инициализации WEB-сервера
				 * @param port        порт сервера
				 * @param host        хост сервера
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const uint32_t port, const string & host = "", const vector <http_t::compressor_t> & compressors = {}) noexcept;
			public:
				/**
				 * sendError Метод отправки сообщения об ошибке
				 * @param bid  идентификатор брокера
				 * @param mess отправляемое сообщение об ошибке
				 */
				void sendError(const uint64_t bid, const ws::mess_t & mess) noexcept;
			public:
				/**
				 * sendMessage Метод отправки сообщения клиенту
				 * @param bid     идентификатор брокера
				 * @param message передаваемое сообщения в бинарном виде
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const uint64_t bid, const vector <char> & message, const bool text = true) noexcept;
				/**
				 * sendMessage Метод отправки сообщения на сервер
				 * @param bid     идентификатор брокера
				 * @param message передаваемое сообщения в бинарном виде
				 * @param size    размер передаваемого сообещния
				 * @param text    данные передаются в текстовом виде
				 * @return        результат отправки сообщения
				 */
				bool sendMessage(const uint64_t bid, const char * message, const size_t size, const bool text = true) noexcept;
			public:
				/**
				 * send Метод отправки тела сообщения клиенту
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных передаваемых клиенту
				 * @param size   размер сообщения в байтах
				 * @param end    флаг последнего сообщения после которого поток закрывается
				 * @return       результат отправки данных указанному клиенту
				 */
				bool send(const uint64_t bid, const char * buffer, const size_t size, const bool end) noexcept;
				/**
				 * send Метод отправки заголовков клиенту
				 * @param bid     идентификатор брокера
				 * @param code    код сообщения для брокера
				 * @param mess    отправляемое сообщение об ошибке
				 * @param headers заголовки отправляемые клиенту
				 * @param end     размер сообщения в байтах
				 * @return        идентификатор нового запроса
				 */
				int32_t send(const uint64_t bid, const uint32_t code, const string & mess, const std::unordered_multimap <string, string> & headers, const bool end) noexcept;
			public:
				/**
				 * send Метод отправки данных в бинарном виде клиенту
				 * @param bid    идентификатор брокера
				 * @param buffer буфер бинарных данных передаваемых клиенту
				 * @param size   размер сообщения в байтах
				 * @return       результат отправки сообщения
				 */
				bool send(const uint64_t bid, const char * buffer, const size_t size) noexcept;
				/**
				 * send Метод отправки сообщения брокеру
				 * @param bid     идентификатор брокера
				 * @param code    код сообщения для брокера
				 * @param mess    отправляемое сообщение об ошибке
				 * @param entity  данные полезной нагрузки (тело сообщения)
				 * @param headers HTTP заголовки сообщения
				 */
				void send(const uint64_t bid, const uint32_t code = 200, const string & mess = "", const vector <char> & entity = {}, const std::unordered_multimap <string, string> & headers = {}) noexcept;
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
				void subprotocols(const std::set <string> & subprotocols) noexcept;
				/**
				 * subprotocol Метод получения списка выбранных сабпротоколов
				 * @param bid идентификатор брокера
				 * @return    список выбранных сабпротоколов
				 */
				const std::set <string> & subprotocols(const uint64_t bid) const noexcept;
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
				void mode(const std::set <flag_t> & flags) noexcept;
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
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param bid  идентификатор брокера
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const uint64_t bid, const bool mode) noexcept;
			public:
				/**
				 * core Метод установки сетевого ядра
				 * @param core объект сетевого ядра
				 */
				void core(const server::core_t * core) noexcept;
			public:
				/**
				 * identity Метод установки идентичности протокола модуля
				 * @param identity идентичность протокола модуля
				 */
				void identity(const http_t::identity_t identity) noexcept;
			public:
				/**
				 * waitMessage Метод ожидания входящих сообщений
				 * @param sec интервал времени в секундах
				 */
				void waitMessage(const time_t sec) noexcept;
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
				Http1(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Http1 Деструктор
				 */
				~Http1() noexcept {}
		} http1_t;
	};
};

#endif // __AWH_WEB_HTTP1_SERVER__
