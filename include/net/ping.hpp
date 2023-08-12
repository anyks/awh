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
#include <set>
#include <map>
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
	 * Ping Класс работы с примером клиента
	 */
	typedef class Ping {
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
				} meta;
			} __attribute__((packed));
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
			// Объект для работы с сокетами
			socket_t _socket;
		private:
			// Мютекс для блокировки потока
			recursive_mutex _mtx;
		private:
			// Размер объекта подключения
			socklen_t _socklen;
		private:
			// Параметры подключения сервера
			struct sockaddr_storage _addr;
		private:
			// Сдвиг по времени для выполнения пинга
			time_t _shifting;
		private:
			// Таймаут на чтение
			time_t _timeoutRead;
			// Таймаут на запись
			time_t _timeoutWrite;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			// Функция обратного вызова для работы в асинхронном режиме
			function <void (const double, const string &)> _callback;
		private:
			/**
			 * checksum Метод подсчёта контрольной суммы
			 * @param buffer буфер данных для подсчёта
			 * @param size   размер данных для подсчёта
			 * @return       подсчитанная контрольная сумма
			 */
			uint16_t checksum(const void * buffer, const size_t size) noexcept;
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
			 * ping Метод запуска пинга IP-адреса в асинхронном режиме
			 * @param host хост для выполнения пинга
			 */
			void ping(const string & host) noexcept;
			/**
			 * ping Метод запуска пинга IP-адреса в асинхронном режиме
			 * @param family тип интернет-протокола AF_INET, AF_INET6
			 * @param ip     адрес для выполнения пинга
			 */
			void ping(const int family, const string & ip) noexcept;
		public:
			/**
			 * ping Метод запуска пинга IP-адреса в синхронном режиме
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
			void on(function <void (const double, const string &)> callback) noexcept;
		public:
			/**
			 * Ping Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Ping(const fmk_t * fmk, const log_t * log) noexcept :
			 _fd(INVALID_SOCKET), _mode(false), _noInfo(false),
			 _net(fmk, log), _dns(fmk, log), _socket(log), _socklen(0),
			 _shifting(3000), _timeoutRead(5000), _timeoutWrite(15000),
			 _fmk(fmk), _log(log), _callback(nullptr) {}
			/**
			 * ~Ping Деструктор
			 */
			~Ping() noexcept {}
	} ping_t;
};

#endif // __AWH_PING__
