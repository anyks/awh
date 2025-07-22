/**
 * @file: ws.hpp
 * @date: 2023-10-04
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

#ifndef __AWH_WEBSOCKET_SERVER__
#define __AWH_WEBSOCKET_SERVER__

/**
 * Наши модули
 */
#include "web/ws2.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * Websocket Класс работы с Websocket-сервером
		 */
		typedef class AWHSHARED_EXPORT Websocket {
			private:
				// Объект DNS-резолвера
				dns_t _dns;
				// Объект работы с протоколом Websocket/2
				server::ws2_t _ws;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * proto Метод извлечения поддерживаемого протокола подключения
				 * @param bid идентификатор брокера
				 * @return    поддерживаемый протокол подключения (HTTP1_1, HTTP2)
				 */
				engine_t::proto_t proto(const uint64_t bid) const noexcept;
			public:
				/**
				 * init Метод инициализации Websocket-сервера
				 * @param socket      unix-сокет для биндинга
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & socket, const vector <http_t::compressor_t> & compressors = {}) noexcept;
				/**
				 * init Метод инициализации Websocket-сервера
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
				 * callback Метод установки функций обратного вызова
				 * @param callback функции обратного вызова
				 */
				void callback(const callback_t & callback) noexcept;
			public:
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param T    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param name  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const char * name, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(name != nullptr)
						// Выполняем установку функции обратного вызова
						return this->_ws.on <T> (name, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param T    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param name  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const string & name, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(!name.empty())
						// Выполняем установку функции обратного вызова
						return this->_ws.on <T> (name, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param T    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename T, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param fid  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const uint64_t fid, Args... args) noexcept -> uint64_t {
					// Если мы получили название функции обратного вызова
					if(fid > 0)
						// Выполняем установку функции обратного вызова
						return this->_ws.on <T> (fid, args...);
					// Выводим результат по умолчанию
					return 0;
				}
				/**
				 * @tparam Шаблон метода подключения финкции обратного вызова
				 * @param A    тип идентификатора функции
				 * @param B    тип функции обратного вызова
				 * @param Args аргументы функции обратного вызова
				 */
				template <typename A, typename B, class... Args>
				/**
				 * on Метод подключения финкции обратного вызова
				 * @param fid  идентификатор функкции обратного вызова
				 * @param args аргументы функции обратного вызова
				 * @return     идентификатор добавленной функции обратного вызова
				 */
				auto on(const A fid, Args... args) noexcept -> uint64_t {
					// Если мы получили на вход число
					if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
						// Выполняем установку функции обратного вызова
						return this->_ws.on <B> (static_cast <uint64_t> (fid), args...);
					// Выводим результат по умолчанию
					return 0;
				}
			public:
				/**
				 * port Метод получения порта подключения брокера
				 * @param bid идентификатор брокера
				 * @return    порт подключения брокера
				 */
				uint32_t port(const uint64_t bid) const noexcept;
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
				void waitPong(const uint16_t sec) noexcept;
				/**
				 * pingInterval Метод установки интервала времени выполнения пингов
				 * @param sec интервал времени выполнения пингов в секундах
				 */
				void pingInterval(const uint16_t sec) noexcept;
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
				void subprotocols(const std::unordered_set <string> & subprotocols) noexcept;
				/**
				 * subprotocol Метод получения списка выбранных сабпротоколов
				 * @param bid идентификатор брокера
				 * @return    список выбранных сабпротоколов
				 */
				const std::unordered_set <string> & subprotocols(const uint64_t bid) const noexcept;
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
				void multiThreads(const uint16_t threads = 0, const bool mode = true) noexcept;
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
			public:
				/**
				 * hosts Метод загрузки файла со списком хостов
				 * @param filename адрес файла для загрузки
				 */
				void hosts(const string & filename) noexcept;
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
				void mode(const std::set <web_t::flag_t> & flags) noexcept;
			public:
				/**
				 * realm Метод установки название сервера
				 * @param realm название сервера
				 */
				void realm(const string & realm) noexcept;
				/**
				 * opaque Метод установки временного ключа сессии сервера
				 * @param opaque временный ключ сессии сервера
				 */
				void opaque(const string & opaque) noexcept;
			public:
				/**
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
			public:
				/**
				 * alive Метод установки долгоживущего подключения
				 * @param mode флаг долгоживущего подключения
				 */
				void alive(const bool mode) noexcept;
			public:
				/**
				 * setHeaders Метод установки списка заголовков
				 * @param headers список заголовков для установки
				 */
				void setHeaders(const std::unordered_multimap <string, string> & headers) noexcept;
			public:
				/**
				 * waitMessage Метод ожидания входящих сообщений
				 * @param sec интервал времени в секундах
				 */
				void waitMessage(const uint16_t sec) noexcept;
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read  количество секунд для детекции по чтению
				 * @param write количество секунд для детекции по записи
				 */
				void waitTimeDetect(const uint16_t read, const uint16_t write) noexcept;
			public:
				/**
				 * addOrigin Метод добавления разрешённого источника
				 * @param origin разрешённый источнико
				 */
				void addOrigin(const string & origin) noexcept;
				/**
				 * setOrigin Метод установки списка разрешённых источников
				 * @param origins список разрешённых источников
				 */
				void setOrigin(const vector <string> & origins) noexcept;
			public:
				/**
				 * addAltSvc Метод добавления альтернативного сервиса
				 * @param origin название альтернативного сервиса
				 * @param field  поле альтернативного сервиса
				 */
				void addAltSvc(const string & origin, const string & field) noexcept;
				/**
				 * setAltSvc Метод установки списка разрешённых источников
				 * @param origins список альтернативных сервисов
				 */
				void setAltSvc(const std::unordered_multimap <string, string> & origins) noexcept;
			public:
				/**
				 * settings Модуль установки настроек протокола HTTP/2
				 * @param settings список настроек протокола HTTP/2
				 */
				void settings(const std::map <awh::http2_t::settings_t, uint32_t> & settings = {}) noexcept;
			public:
				/**
				 * ident Метод установки идентификации сервера
				 * @param id   идентификатор сервиса
				 * @param name название сервиса
				 * @param ver  версия сервиса
				 */
				void ident(const string & id, const string & name, const string & ver) noexcept;
			public:
				/**
				 * authType Метод установки типа авторизации
				 * @param type тип авторизации
				 * @param hash алгоритм шифрования для Digest авторизации
				 */
				void authType(const auth_t::type_t type = auth_t::type_t::BASIC, const auth_t::hash_t hash = auth_t::hash_t::MD5) noexcept;
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
				 * Websocket Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Websocket(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Websocket Деструктор
				 */
				~Websocket() noexcept {}
		} websocket_t;
	};
};

#endif // __AWH_WEBSOCKET_SERVER__
