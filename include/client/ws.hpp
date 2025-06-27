/**
 * @file: ws.hpp
 * @date: 2023-09-25
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

#ifndef __AWH_WEBSOCKET_CLIENT__
#define __AWH_WEBSOCKET_CLIENT__

/**
 * Наши модули
 */
#include <client/web/ws2.hpp>

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
		 * Websocket Класс работы с Websocket-клиентом
		 */
		typedef class AWHSHARED_EXPORT Websocket {
			private:
				// Объект DNS-резолвера
				dns_t _dns;
				// Объект работы с протоколом HTTP/2
				client::ws2_t _ws;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
				// Объект сетевого ядра
				const client::core_t * _core;
			public:
				/**
				 * proto Метод извлечения поддерживаемого протокола подключения
				 * @return поддерживаемый протокол подключения (HTTP1_1, HTTP2)
				 */
				engine_t::proto_t proto() const noexcept;
			public:
				/**
				 * sendError Метод отправки сообщения об ошибке на сервер
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
				 * pause Метод установки на паузу клиента
				 */
				void pause() noexcept;
			public:
				/**
				 * init Метод инициализации клиента
				 * @param dest        адрес назначения удалённого сервера
				 * @param compressors список поддерживаемых компрессоров
				 */
				void init(const string & dest, const vector <awh::http_t::compressor_t> & compressors = {awh::http_t::compressor_t::DEFLATE}) noexcept;
			public:
				/**
				 * open Метод открытия подключения
				 */
				void open() noexcept;
			public:
				/**
				 * reset Метод принудительного сброса подключения
				 */
				void reset() noexcept;
			public:
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
				 * extensions Метод извлечения списка расширений Websocket
				 * @return список поддерживаемых расширений
				 */
				const vector <vector <string>> & extensions() const noexcept;
				/**
				 * extensions Метод установки списка расширений Websocket
				 * @param extensions список поддерживаемых расширений
				 */
				void extensions(const vector <vector <string>> & extensions) noexcept;
			public:
				/**
				 * bandwidth Метод установки пропускной способности сети
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandwidth(const string & read = "", const string & write = "") noexcept;
			public:
				/**
				 * chunk Метод установки размера чанка
				 * @param size размер чанка для установки
				 */
				void chunk(const size_t size) noexcept;
				/**
				 * segmentSize Метод установки размеров сегментов фрейма Websocket
				 * @param size минимальный размер сегмента
				 */
				void segmentSize(const size_t size) noexcept;
				/**
				 * attempts Метод установки общего количества попыток
				 * @param attempts общее количество попыток
				 */
				void attempts(const uint8_t attempts) noexcept;
			public:
				/**
				 * hosts Метод загрузки файла со списком хостов
				 * @param filename адрес файла для загрузки
				 */
				void hosts(const string & filename) noexcept;
				/**
				 * mode Метод установки флагов настроек модуля
				 * @param flags список флагов настроек модуля для установки
				 */
				void mode(const set <web_t::flag_t> & flags) noexcept;
				/**
				 * user Метод установки параметров авторизации
				 * @param login    логин пользователя для авторизации на сервере
				 * @param password пароль пользователя для авторизации на сервере
				 */
				void user(const string & login, const string & password) noexcept;
				/**
				 * compressors Метод установки списка поддерживаемых компрессоров
				 * @param compressors список поддерживаемых компрессоров
				 */
				void compressors(const vector <awh::http_t::compressor_t> & compressors) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
			public:
				/**
				 * multiThreads Метод активации многопоточности в Websocket
				 * @param count количество потоков для активации
				 * @param mode  флаг активации/деактивации мультипоточности
				 */
				void multiThreads(const uint16_t count = 0, const bool mode = true) noexcept;
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
				 * flushDNS Метод сброса кэша DNS-резолвера
				 * @return результат работы функции
				 */
				bool flushDNS() noexcept;
			public:
				/**
				 * timeoutDNS Метод установки времени ожидания выполнения запроса
				 * @param sec интервал времени выполнения запроса в секундах
				 */
				void timeoutDNS(const uint8_t sec) noexcept;
			public:
				/**
				 * prefixDNS Метод установки префикса переменной окружения для извлечения серверов имён
				 * @param prefix префикс переменной окружения для установки
				 */
				void prefixDNS(const string & prefix) noexcept;
			public:
				/**
				 * clearDNSBlackList Метод очистки чёрного списка
				 * @param domain доменное имя для которого очищается чёрный список
				 */
				void clearDNSBlackList(const string & domain) noexcept;
				/**
				 * delInDNSBlackList Метод удаления IP-адреса из чёрного списока
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для удаления из чёрного списка
				 */
				void delInDNSBlackList(const string & domain, const string & ip) noexcept;
				/**
				 * setToDNSBlackList Метод добавления IP-адреса в чёрный список
				 * @param domain доменное имя соответствующее IP-адресу
				 * @param ip     адрес для добавления в чёрный список
				 */
				void setToDNSBlackList(const string & domain, const string & ip) noexcept;
			public:
				/**
				 * cork Метод отключения/включения алгоритма TCP/CORK
				 * @param mode режим применимой операции
				 * @return     результат выполенния операции
				 */
				bool cork(const engine_t::mode_t mode) noexcept;
				/**
				 * nodelay Метод отключения/включения алгоритма Нейгла
				 * @param mode режим применимой операции
				 * @return     результат выполенния операции
				 */
				bool nodelay(const engine_t::mode_t mode) noexcept;
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
				 * waitMessage Метод ожидания входящих сообщений
				 * @param sec интервал времени в секундах
				 */
				void waitMessage(const uint16_t sec) noexcept;
				/**
				 * waitTimeDetect Метод детекции сообщений по количеству секунд
				 * @param read    количество секунд для детекции по чтению
				 * @param write   количество секунд для детекции по записи
				 * @param connect количество секунд для детекции по подключению
				 */
				void waitTimeDetect(const uint16_t read = READ_TIMEOUT, const uint16_t write = WRITE_TIMEOUT, const uint16_t connect = CONNECT_TIMEOUT) noexcept;
			public:
				/**
				 * network Метод установки параметров сети
				 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
				 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 */
				void network(const vector <string> & ips = {}, const vector <string> & ns = {}, const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
			public:
				/**
				 * Websocket Конструктор
				 * @param core объект сетевого ядра
				 * @param fmk  объект фреймворка
				 * @param log  объект для работы с логами
				 */
				Websocket(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * ~Websocket Деструктор
				 */
				~Websocket() noexcept {}
		} websocket_t;
	};
};

#endif // __AWH_WEBSOCKET_CLIENT__
