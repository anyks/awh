/**
 * @file: ntp.hpp
 * @date: 2023-08-17
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

#ifndef __AWH_NTP_CLIENT__
#define __AWH_NTP_CLIENT__

/**
 * Стандартные модули
 */
#include <mutex>
#include <thread>
#include <cstdio>
#include <string>
#include <vector>
#include <random>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <unordered_set>

/**
 * Для операционной системы не являющейся OS Windows
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#define SOCKET int32_t
	#define INVALID_SOCKET -1
#endif

/**
 * Для операционной системы OS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
	#include <ws2tcpip.h>
/**
 * Для операционной системы не являющейся OS Windows
 */
#else
	#include <netdb.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

/**
 * Если используется модуль IDN и операционная система не MS Windows
 */
#if defined(AWH_IDN) && !defined(_WIN32) && !defined(_WIN64)
	#include <idn2.h>
#endif

/**
 * Наши модули
 */
#include <sys/fs.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/hold.hpp>
#include <net/net.hpp>
#include <net/dns.hpp>
#include <net/socket.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Прототип класса NTP-клиента
	 */
	class NTP;
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * NTP Класс NTP-клиента
	 */
	typedef class AWHSHARED_EXPORT NTP {
		private:
			// Дельта штампа времени (количество секунд между 1900 и 1970 годами)
			static constexpr uint64_t NTP_TIMESTAMP_DELTA {2208988800ull};
		private:
			/**
			 * Статус работы NTP-резолвера
			 */
			enum class status_t : uint8_t {
				NONE     = 0x00, // Событие не установлено
				CLEAR    = 0x01, // Событие отчистки параметров NTP-клиента
				REQUEST  = 0x02, // Событие запуска запроса к NTP-клиенту
				NET_SET  = 0x03, // Событие установки параметров сети
				NTS_SET  = 0x04, // Событие установки NTP-сервера
				NTSS_SET = 0x05, // Событие установки NTP-серверов
				NTSS_REP = 0x06  // Событие замены NTP-серверов
			};
		private:
			/**
			 * Шаблон формата данных NTP-сервера
			 * @tparam T размерность буфера NTP-сервера
			 */
			template <uint8_t T>
			/**
			 * Server Структура сервера имён
			 */
			struct Server {
				uint32_t port;     // Порт сервера
				uint32_t ip[T]; // Буфер IP-адреса
				/**
				 * Server Конструктор
				 */
				Server() noexcept : port(123), ip{0} {}
			};
			/**
			 * Шаблон формата данных NTP-сервера
			 * @tparam T размерность буфера NTP-сервера
			 */
			template <uint8_t T>
			// Создаём тип данных работы с NTP-серверами
			using server_t = Server <T>;
		private:
			private:
			/**
			 * Peer Структура подключения
			 */
			typedef struct Peer {
				socklen_t size;                 // Размер объекта подключения
				struct sockaddr_storage client; // Параметры подключения клиента
				struct sockaddr_storage server; // Параметры подключения сервера
				/**
				 * Peer Конструктор
				 */
				Peer() noexcept : size(0), client{}, server{} {}
			} peer_t;
			/**
			 * Packet Структура пакетов NTP-запроса
			 */
			typedef struct Packet {
				/**
				 * Восемь бит (li, vn и mode):
				 * li:   Два бита, индикатор прыжка
				 * vn:   Три бита, номер версии протокола
				 * mode: Три бита, клиент выберет режим 3 для клиента
				 */
				uint8_t mode;
				// Уровень страты местных часов
				uint8_t stratum;
				// Максимальный интервал между последовательными сообщениями
				uint8_t poll;
				// Точность местных часов
				uint8_t precision;
				// Общее время задержки туда и обратно
				uint32_t rootDelay;
				// Максимально-ощутимая ошибка от основного источника синхронизации
				uint32_t rootDispersion;
				// Идентификатор опорных часов
				uint32_t refId;
				// Отметка времени в секундах
				uint32_t refTimeStampSec;
				// Эталонная отметка времени в долях секунды
				uint32_t refTimeStampSecFrac;
				// Исходная метка времени в секундах
				uint32_t origTimeStampSec;
				// Исходная временная метка в долях секунды
				uint32_t origTimeStampSecFrac;
				// Полученная метка времени в секундах
				uint32_t receivedTimeStampSec;
				// Полученная временная метка в долях секунды
				uint32_t receivedTimeStampSecFrac;
				// Передача метки времени в секундах
				uint32_t transmitedTimeStampSec;
				// Отметки времени в долях секунды для передачи
				uint32_t transmitedTimeStampSecFrac;
				/**
				 * Packet Конструктор
				 */
				Packet() noexcept :
				 mode(0), stratum(0), poll(0),
				 precision(0), rootDelay(0),
				 rootDispersion(0), refId(0),
				 refTimeStampSec(0), refTimeStampSecFrac(0),
				 origTimeStampSec(0), origTimeStampSecFrac(0),
				 receivedTimeStampSec(0), receivedTimeStampSecFrac(0),
				 transmitedTimeStampSec(0), transmitedTimeStampSecFrac(0) {}
			} __attribute__((packed)) packet_t;
			/**
			 * Worker Класс воркера резолвинга
			 */
			typedef class AWHSHARED_EXPORT Worker {
				private:
					/**
					 * NTP Устанавливаем дружбу с классом NTP-клиента
					 */
					friend class NTP;
				private:
					// Файловый дескриптор (сокет)
					SOCKET _fd;
				private:
					// Флаг запуска резолвера
					bool _mode;
				private:
					// Тип протокола интернета AF_INET или AF_INET6
					int32_t _family;
				private:
					// Объект для работы с подключениями
					peer_t _peer;
					// Объект для работы с сокетами
					socket_t _socket;
				private:
					// Список сетевых интерфейсов
					vector <string> _network;
				private:
					// Объект NTP-клиента
					const NTP * _self;
				private:
					/**
					 * host Метод извлечения хоста компьютера
					 * @return хост компьютера с которого производится запрос
					 */
					string host() const noexcept;
				public:
					/**
					 * close Метод закрытия подключения
					 */
					void close() noexcept;
				public:
					/**
					 * cancel Метод отмены выполнения запроса
					 */
					void cancel() noexcept;
				public:
					/**
					 * request Метод выполнения запроса
					 * @return полученный UnixTimeStamp
					 */
					uint64_t request() noexcept;
				private:
					/**
					 * Метод отправки запроса на удалённый сервер NTP
					 * @param from адрес компьютера с которого выполняется запрос
					 * @param to   адрес NTP-сервера на который выполняется запрос
					 * @return     полученный UnixTimeStamp
					 */
					uint64_t send(const string & from, const string & to) noexcept;
				public:
					/**
					 * Worker Конструктор
					 * @param family тип протокола интернета AF_INET или AF_INET6
					 * @param self   объект NTP-клиента
					 */
					Worker(const int32_t family, const NTP * self) noexcept :
					 _fd(INVALID_SOCKET), _mode(false), _family(family),
					 _socket(self->_fmk, self->_log), _self(self) {}
					/**
					 * ~Worker Деструктор
					 */
					~Worker() noexcept;
			} worker_t;
		private:
			// Объект IP-адресов
			net_t _net;
			// Объект DNS-резолвера
			dns_t _dns;
		private:
			// Таймаут ожидания выполнения запроса (в секундах)
			uint8_t _timeout;
		private:
			// Мютекс для блокировки потока
			recursive_mutex _mtx;
		private:
			// Выполняем инициализацию генератора
			random_device _randev;
		private:
			// Статус работы NTP-клиента
			stack <status_t> _status;
		private:
			// Список используемых адресов
			unordered_set <string> _using;
		private:
			// Создаём воркер для IPv4
			unique_ptr <worker_t> _workerIPv4;
			// Создаём воркер для IPv6
			unique_ptr <worker_t> _workerIPv6;
		private:
			// Адреса серверов имён NTP для IPv4
			vector <server_t <1>> _serversIPv4;
			// Адреса серверов имён NTP для IPv6
			vector <server_t <4>> _serversIPv6;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * clear Метод очистки данных NTP-клиента
			 * @return результат работы функции
			 */
			bool clear() noexcept;
		public:
			/**
			 * cancel Метод отмены выполнения запроса
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 */
			void cancel(const int32_t family) noexcept;
		public:
			/**
			 * shuffle Метод пересортировки серверов NTP
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 */
			void shuffle(const int32_t family) noexcept;
		public:
			/**
			 * timeout Метод установки времени ожидания выполнения запроса
			 * @param sec интервал времени выполнения запроса в секундах
			 */
			void timeout(const uint8_t sec) noexcept;
		public:
			/**
			 * ns Метод добавления DNS-серверов
			 * @param servers адреса DNS-серверов
			 */
			void ns(const vector <string> & servers) noexcept;
		public:
			/**
			 * server Метод получения данных NTP-сервера
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @return       запрошенный NTP-сервер
			 */
			string server(const int32_t family) noexcept;
		public:
			/**
			 * server Метод добавления NTP-сервера
			 * @param server адрес NTP-сервера
			 */
			void server(const string & server) noexcept;
			/**
			 * server Метод добавления NTP-сервера
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param server адрес NTP-сервера
			 */
			void server(const int32_t family, const string & server) noexcept;
		public:
			/**
			 * servers Метод добавления NTP-серверов
			 * @param servers адреса NTP-серверов
			 */
			void servers(const vector <string> & servers) noexcept;
			/**
			 * servers Метод добавления NTP-серверов
			 * @param family  тип интернет-протокола AF_INET, AF_INET6
			 * @param servers адреса NTP-серверов
			 */
			void servers(const int32_t family, const vector <string> & servers) noexcept;
		public:
			/**
			 * replace Метод замены существующих NTP-серверов
			 * @param servers адреса NTP-серверов
			 */
			void replace(const vector <string> & servers = {}) noexcept;
			/**
			 * replace Метод замены существующих NTP-серверов
			 * @param family  тип интернет-протокола AF_INET, AF_INET6
			 * @param servers адреса NTP-серверов
			 */
			void replace(const int32_t family, const vector <string> & servers = {}) noexcept;
		public:
			/**
			 * network Метод установки адреса сетевых плат, с которых нужно выполнять запросы
			 * @param network IP-адреса сетевых плат
			 */
			void network(const vector <string> & network) noexcept;
			/**
			 * network Метод установки адреса сетевых плат, с которых нужно выполнять запросы
			 * @param family  тип интернет-протокола AF_INET, AF_INET6
			 * @param network IP-адреса сетевых плат
			 */
			void network(const int32_t family, const vector <string> & network) noexcept;
		public:
			/**
			 * request Метод выполнение получения времени с NTP-сервера
			 * @return полученный UnixTimeStamp
			 */
			uint64_t request() noexcept;
			/**
			 * request Метод выполнение получения времени с NTP-сервера
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @return       полученный UnixTimeStamp
			 */
			uint64_t request(const int32_t family) noexcept;
		public:
			/**
			 * NTP Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			NTP(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~NTP Деструктор
			 */
			~NTP() noexcept;
	} ntp_t;
};

#endif // __AWH_NTP_CLIENT__
