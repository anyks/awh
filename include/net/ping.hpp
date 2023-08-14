/**
 * @file: ping.hpp
 * @date: 2023-08-11
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

#ifndef __AWH_PING__
#define __AWH_PING__

/**
 * Стандартная библиотека
 */
#include <mutex>
#include <ctime>
#include <chrono>
#include <thread>
#include <cstdio>
#include <string>
#include <vector>
#include <limits>
#include <cstdlib>
#include <algorithm>
#include <functional>

/**
 * Если операционной системой является Nix-подобная
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#define SOCKET int
	#define INVALID_SOCKET -1
#endif

/**
 * Если операционной системой является MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
	#include <ws2tcpip.h>
/**
 * Если операционной системой является Nix-подобная
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
 * Наши модули
 */
#include <sys/fs.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/if.hpp>
#include <net/net.hpp>
#include <net/dns.hpp>
#include <net/socket.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Ping Класс пинга
	 */
	typedef class Ping {
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
				Peer() noexcept : size(0) {}
			} peer_t;
		private:
			/**
			 * IcmpHeader Структура заголовков ICMP
			 */
			struct IcmpHeader {
				uint8_t type;      // Тип запроса
				uint8_t code;      // Код запроса
				uint16_t checksum; // Контрольная сумма
				/**
				 * Объединение структур запроса
				 */
				union {
					/**
					 * echo Структура отправляемого запроса
					 */
					struct {
						uint16_t identifier = 0; // Идентификатор запроса
						uint16_t sequence   = 0; // Номер последовательности
						uint64_t payload    = 0; // Тело полезной нагрузки
					} echo;
					/**
					 * pointer Структура указателя запроса
					 */
					struct ICMP_PACKET_POINTER_HEADER {
						// Указатель пакета
						uint8_t pointer = 0;
					} pointer;
					/**
					 * redirect Структура адреса ответа
					 */
					struct ICMP_PACKET_REDIRECT_HEADER {
						// Адрес ответа IPv4
						uint32_t gatewayAddress = 0;
					} redirect;
					/**
					 * redirect Структура адреса ответа
					 */
					struct ICMP6_PACKET_REDIRECT_HEADER {
						// Адрес ответа IPv6
						uint32_t gatewayAddress[4] = {0,0,0,0};
					} redirect6;
				} meta;
			};
		private:
			// Файловый дескриптор (сокет)
			SOCKET _fd;
		private:
			// Флаг запуска работы
			bool _mode;
		private:
			// Флаг запрещающий вывод информации
			bool _noInfo;
		private:
			// Объект для работы с сетью
			net_t _net;
			// Объект для работы с DNS-резолвером
			dns_t _dns;
			// Объект для работы с подключениями
			peer_t _peer;
			// Объект для работы с сетевым интерфейсом
			ifnet_t _ifnet;
			// Объект для работы с сокетами
			socket_t _socket;
		private:
			// Мютекс для блокировки потока
			recursive_mutex _mtx;
		private:
			// Выполняем инициализацию генератора
			random_device _randev;
		private:
			// Сдвиг по времени для выполнения пинга
			time_t _shifting;
		private:
			// Таймаут на чтение
			time_t _timeoutRead;
			// Таймаут на запись
			time_t _timeoutWrite;
		private:
			// Список сетевых интерфейсов для IPv4
			vector <string> _networkIPv4;
			// Список сетевых интерфейсов для IPv6
			vector <string> _networkIPv6;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			// Функция обратного вызова для работы в асинхронном режиме
			function <void (const time_t, const string &, Ping *)> _callback;
		private:
			/**
			 * host Метод извлечения хоста компьютера
			 * @param family семейство сокета (AF_INET / AF_INET6)
			 * @return       хост компьютера с которого производится запрос
			 */
			string host(const int family) const noexcept;
		private:
			/**
			 * checksum Метод подсчёта контрольной суммы
			 * @param buffer буфер данных для подсчёта
			 * @param size   размер данных для подсчёта
			 * @return       подсчитанная контрольная сумма
			 */
			uint16_t checksum(const void * buffer, const size_t size) noexcept;
		private:
			/**
			 * send Метод отправки запроса на сервер
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param index  индекс последовательности
			 * @return       количество прочитанных байт
			 */
			int64_t send(const int family, const size_t index) noexcept;
		public:
			/**
			 * close Метод закрытия подключения
			 */
			void close() noexcept;
		public:
			/**
			 * cancel Метод остановки запущенной работы
			 */
			void cancel() noexcept;
		public:
			/**
			 * working Метод проверки запуска работы модуля
			 * @return результат работы
			 */
			bool working() const noexcept;
		public:
			/**
			 * ping Метод запуска пинга хоста в асинхронном режиме
			 * @param host хост для выполнения пинга
			 */
			void ping(const string & host) noexcept;
		private:
			/**
			 * work Метод запуска пинга IP-адреса в асинхронном режиме
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param ip     адрес для выполнения пинга
			 */
			void work(const int family, const string & ip) noexcept;
		public:
			/**
			 * ping Метод запуска пинга хоста в синхронном режиме
			 * @param host  хост для выполнения пинга
			 * @param count количество итераций
			 */
			double ping(const string & host, const uint16_t count) noexcept;
			/**
			 * ping Метод запуска пинга IP-адреса в синхронном режиме
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param ip     адрес для выполнения пинга
			 * @param count  количество итераций
			 */
			double ping(const int family, const string & ip, const uint16_t count) noexcept;
		public:
			/**
			 * noInfo Метод запрещающий выводить информацию пинга в лог
			 * @param mode флаг для установки
			 */
			void noInfo(const bool mode) noexcept;
		public:
			/**
			 * shifting Метод установки сдвига по времени выполнения пинга в миллисекундах
			 * @param msec сдвиг по времени в миллисекундах
			 */
			void shifting(const time_t msec) noexcept;
		public:
			/**
			 * ns Метод добавления серверов DNS
			 * @param servers параметры DNS-серверов
			 */
			void ns(const vector <string> & servers) noexcept;
		public:
			/**
			 * network Метод установки адреса сетевых плат, с которых нужно выполнять запросы
			 * @param network IP-адреса сетевых плат
			 */
			void network(const vector <string> & network) noexcept;
		public:
			/**
			 * timeout Метод установки таймаутов в миллисекундах
			 * @param read  таймаут на чтение
			 * @param write таймаут на запись
			 */
			void timeout(const time_t read, const time_t write) noexcept;
		public:
			/**
			 * on Метод установки функции обратного вызова, для работы в асинхронном режиме
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const time_t, const string &, Ping *)> callback) noexcept;
		public:
			/**
			 * Ping Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Ping(const fmk_t * fmk, const log_t * log) noexcept :
			 _fd(INVALID_SOCKET), _mode(false), _noInfo(false), _dns(fmk, log), _ifnet(fmk, log), _socket(log),
			 _shifting(3000), _timeoutRead(5000), _timeoutWrite(15000), _fmk(fmk), _log(log), _callback(nullptr) {}
			/**
			 * ~Ping Деструктор
			 */
			~Ping() noexcept {}
	} ping_t;
};

#endif // __AWH_PING__
