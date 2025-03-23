/**
 * @file: socks5.hpp
 * @date: 2022-09-03
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

#ifndef __AWH_PROXY_SOCKS5_SERVER__
#define __AWH_PROXY_SOCKS5_SERVER__

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/queue.hpp>
#include <core/client.hpp>
#include <core/server.hpp>
#include <scheme/socks5.hpp>

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
		 * ProxySocks5 Класс работы с SOCKS5 сервером
		 */
		typedef class AWHSHARED_EXPORT ProxySocks5 {
			private:
				/**
				 * Брокеры учавствующие в передаче данных
				 */
				enum class broker_t : uint8_t {
					CLIENT = 0x01, // Агент является клиентом
					SERVER = 0x02  // Агент является сервером
				};
			public:
				/**
				 * Режим работы клиента
				 */
				enum class mode_t : uint8_t {
					CONNECT    = 0x01, // Режим подключения
					DISCONNECT = 0x02  // Режим отключения
				};
				/**
				 * Режим событие клиента
				 */
				enum class event_t : uint8_t {
					REQUEST  = 0x01, // Режим запроса
					RESPONSE = 0x02  // Режим ответа
				};
				/**
				 * Основные флаги приложения
				 */
				enum class flag_t : uint8_t {
					// Флаг запрещающий вывод информационных сообщений
					NOT_INFO = 0x01
				};
			private:
				/**
				 * Структура параметров клиента
				 */
				typedef struct Settings {
					bool verify;         // Флаг выполнение верификации доменного имени
					node_t::ssl_t ssl;   // Параметры SSL-шифрования
					vector <string> ips; // Список IP-адресов компьютера с которых разрешено выходить в интернет
					/**
					 * Settings Конструктор
					 */
					Settings() noexcept : verify(false) {}
				} settings_t;
			private:
				// Порт сервера
				uint32_t _port;
				// Хости сервера
				string _host;
				// unix-сокет сервера
				string _socket;
			private:
				// Объект DNS-резолвера
				dns_t _dns;
				// Объект IP-адресов
				net_t _net;
				// Хранилище функций обратного вызова
				fn_t _callbacks;
				// Объект параметров клиента
				settings_t _settings;
			private:
				// Объект биндинга TCP/IP для сервера
				server::core_t _core;
			private:
				// Объект работы таймера
				timer_t _timer;
			private:
				// Объект рабочего для сервера
				scheme::socks5_t _scheme;
			private:
				// Максимальный размер памяти для хранений полезной нагрузки всех брокеров
				size_t _memoryAvailableSize;
				// Максимальный размер хранимой полезной нагрузки для одного брокера
				size_t _brokerAvailableSize;
			private:
				// Буферы отправляемой полезной нагрузки
				map <uint64_t, unique_ptr <queue_t>> _payloads;
				// Список активных клиентов
				map <uint64_t, unique_ptr <client::core_t>> _clients;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			private:
				/**
				 * openEvents Метод обратного вызова при запуске работы
				 * @param sid идентификатор схемы сети
				 */
				void openEvents(const uint16_t sid) noexcept;
			private:
				/**
				 * acceptEvents Метод обратного вызова при проверке подключения клиента
				 * @param ip   адрес интернет подключения клиента
				 * @param mac  мак-адрес подключившегося клиента
				 * @param port порт подключившегося брокера
				 * @param sid  идентификатор схемы сети
				 * @return     результат разрешения к подключению клиента
				 */
				bool acceptEvents(const string & ip, const string & mac, const uint32_t port, const uint16_t sid) noexcept;
			private:
				/**
				 * connectClientEvents Метод обратного вызова при подключении
				 * @param broker брокер вызвавший событие
				 * @param bid1   идентификатор брокера сервера
				 * @param bid2   идентификатор брокера клиента
				 * @param sid    идентификатор схемы сети
				 */
				void connectEvents(const broker_t broker, const uint64_t bid1, const uint64_t bid2, const uint16_t sid) noexcept;
				/**
				 * disconnectClientEvents Метод обратного вызова при отключении
				 * @param broker брокер вызвавший событие
				 * @param bid1   идентификатор брокера сервера
				 * @param bid2   идентификатор брокера клиента
				 * @param sid    идентификатор схемы сети
				 */
				void disconnectEvents(const broker_t broker, const uint64_t bid1, const uint64_t bid2, const uint16_t sid) noexcept;
			private:
				/**
				 * readClientEvents Метод обратного вызова при чтении сообщения
				 * @param broker брокер вызвавший событие
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер бинарного буфера содержащего сообщение
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void readEvents(const broker_t broker, const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
				/**
				 * writeServerEvents Метод обратного вызова при записи сообщения на клиенте
				 * @param broker брокер вызвавший событие
				 * @param buffer бинарный буфер содержащий сообщение
				 * @param size   размер записанных в сокет байт
				 * @param bid    идентификатор брокера
				 * @param sid    идентификатор схемы сети
				 */
				void writeEvents(const broker_t broker, const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept;
			private:
				/**
				 * available Метод получения событий освобождения памяти буфера полезной нагрузки
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param bid    идентификатор брокера
				 * @param size   размер буфера полезной нагрузки
				 * @param core   объект сетевого ядра
				 */
				void available(const broker_t broker, const uint64_t bid, const size_t size, awh::core_t * core) noexcept;
				/**
				 * unavailable Метод получения событий недоступности памяти буфера полезной нагрузки
				 * @param broker брокер для которого устанавливаются настройки (CLIENT/SERVER)
				 * @param bid    идентификатор брокера
				 * @param buffer буфер полезной нагрузки которую не получилось отправить
				 * @param size   размер буфера полезной нагрузки
				 */
				void unavailable(const broker_t broker, const uint64_t bid, const char * buffer, const size_t size) noexcept;
			private:
				/**
				 * erase Метод удаления отключённых клиентов
				 * @param tid идентификатор таймера
				 * @param bid идентификатор брокера
				 */
				void erase(const uint16_t tid, const uint64_t bid) noexcept;
			public:
				/**
				 * init Метод инициализации брокера
				 * @param socket unix-сокет для биндинга
				 */
				void init(const string & socket) noexcept;
				/**
				 * init Метод инициализации брокера
				 * @param port порт сервера
				 * @param host хост сервера
				 */
				void init(const uint32_t port = 1080, const string & host = "") noexcept;
			public:
				/**
				 * callbacks Метод установки функций обратного вызова
				 * @param callbacks функции обратного вызова
				 */
				void callbacks(const fn_t & callbacks) noexcept;
			public:
				/**
				 * callback Шаблон метода установки финкции обратного вызова
				 * @tparam A тип функции обратного вызова
				 */
				template <typename A>
				/**
				 * callback Метод установки функции обратного вызова
				 * @param idw идентификатор функции обратного вызова
				 * @param fn  функция обратного вызова для установки
				 */
				void callback(const uint64_t idw, function <A> fn) noexcept {
					// Если функция обратного вызова передана
					if((idw > 0) && (fn != nullptr))
						// Выполняем установку функции обратного вызова
						this->_callbacks.set <A> (idw, fn);
				}
				/**
				 * callback Шаблон метода установки финкции обратного вызова
				 * @tparam A тип функции обратного вызова
				 */
				template <typename A>
				/**
				 * callback Метод установки функции обратного вызова
				 * @param name название функции обратного вызова
				 * @param fn   функция обратного вызова для установки
				 */
				void callback(const string & name, function <A> fn) noexcept {
					// Если функция обратного вызова передана
					if(!name.empty() && (fn != nullptr))
						// Выполняем установку функции обратного вызова
						this->_callbacks.set <A> (name, fn);
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
				 * close Метод закрытия подключения
				 * @param bid идентификатор брокера
				 */
				void close(const uint64_t bid) noexcept;
			public:
				/**
				 * memoryAvailableSize Метод получения максимального рамзера памяти для хранения полезной нагрузки всех брокеров
				 * @return размер памяти для хранения полезной нагрузки всех брокеров
				 */
				size_t memoryAvailableSize() const noexcept;
				/**
				 * memoryAvailableSize Метод установки максимального рамзера памяти для хранения полезной нагрузки всех брокеров
				 * @param size размер памяти для хранения полезной нагрузки всех брокеров
				 */
				void memoryAvailableSize(const size_t size) noexcept;
			public:
				/**
				 * brokerAvailableSize Метод получения максимального размера хранимой полезной нагрузки для одного брокера
				 * @return размер хранимой полезной нагрузки для одного брокера
				 */
				size_t brokerAvailableSize() const noexcept;
				/**
				 * brokerAvailableSize Метод установки максимального размера хранимой полезной нагрузки для одного брокера
				 * @param size размер хранимой полезной нагрузки для одного брокера
				 */
				void brokerAvailableSize(const size_t size) noexcept;
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
				 * total Метод установки максимального количества одновременных подключений
				 * @param total максимальное количество одновременных подключений
				 */
				void total(const uint16_t total) noexcept;
			public:
				/**
				 * clusterAutoRestart Метод установки флага перезапуска процессов
				 * @param mode флаг перезапуска процессов
				 */
				void clusterAutoRestart(const bool mode) noexcept;
				/**
				 * cluster Метод установки количества процессов кластера
				 * @param mode флаг активации/деактивации кластера
				 * @param size количество рабочих процессов
				 */
				void cluster(const awh::scheme_t::mode_t mode, const uint16_t size = 0) noexcept;
			public:
				/**
				 * mode Метод установки флагов модуля
				 * @param flags список флагов модуля для установки
				 */
				void mode(const set <flag_t> & flags) noexcept;
			public:
				/**
				 * ipV6only Метод установки флага использования только сети IPv6
				 * @param mode флаг для установки
				 */
				void ipV6only(const bool mode) noexcept;
				/**
				 * sonet Метод установки типа сокета подключения
				 * @param sonet тип сокета подключения (TCP / UDP / SCTP)
				 */
				void sonet(const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
				/**
				 * family Метод установки типа протокола интернета
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 */
				void family(const scheme_t::family_t family = scheme_t::family_t::IPV4) noexcept;
				/**
				 * keepAlive Метод установки жизни подключения
				 * @param cnt   максимальное количество попыток
				 * @param idle  интервал времени в секундах через которое происходит проверка подключения
				 * @param intvl интервал времени в секундах между попытками
				 */
				void keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept;
				/**
				 * bandwidth Метод установки пропускной способности сети
				 * @param bid   идентификатор брокера
				 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
				 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
				 */
				void bandwidth(const uint64_t bid, const string & read = "", const string & write = "") noexcept;
				/**
				 * network Метод установки параметров сети
				 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
				 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
				 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
				 * @param sonet  тип сокета подключения (TCP / UDP)
				 */
				void network(const vector <string> & ips = {}, const vector <string> & ns = {}, const scheme_t::family_t family = scheme_t::family_t::IPV4, const scheme_t::sonet_t sonet = scheme_t::sonet_t::TCP) noexcept;
			public:
				/**
				 * signalInterception Метод активации перехвата сигналов
				 * @param mode флаг активации
				 */
				void signalInterception(const scheme_t::mode_t mode) noexcept;
			public:
				/**
				 * ssl Метод установки параметров SSL-шифрования
				 * @param ssl объект параметров SSL-шифрования
				 */
				void ssl(const node_t::ssl_t & ssl) noexcept;
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
		} proxy_socks5_t;
	};
};

#endif // __AWH_PROXY_SOCKS5_SERVER__
