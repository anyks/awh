/**
 * @file: engine.hpp
 * @date: 2022-08-03
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

#ifndef __AWH_ENGINE__
#define __AWH_ENGINE__

/**
 * Настройка системы для высокой нагрузки:
 * url: https://romantelychko.com/blog/1300
 * количество занятых сокетов: lsof | grep 69080 | wc -l
 */

/**
 * Активация протокола SCTP:
 * 
 * Под Ubuntu Linux:
 * # sudo apt install libsctp-dev
 * # sudo modprobe sctp
 * # sudo sysctl -w net.sctp.auth_enable=1
 * 
 * Под FreeBSD:
 * # sudo kldload sctp
 */

/**
 * Стандартные модули
 */
#include <random>
#include <thread>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <sys/types.h>

/**
 * Для операционной системы не являющейся OS Windows
 */
#if !_WIN32 && !_WIN64
	#include <sys/un.h>
#endif

/**
 * Наши модули
 */
#include "if.hpp"
#include "uri.hpp"
#include "socket.hpp"
#include "../sys/fs.hpp"
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"

/**
 * Подключаем OpenSSL
 */
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>

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
	 * @brief Класс для работы с двигателем передачи данных
	 *
	 */
	typedef class AWHSHARED_EXPORT Engine {
		public:
			/**
			 * Режим работы
			 */
			enum class mode_t : uint8_t {
				NONE     = 0x00, // Экшен не установлен
				ENABLED  = 0x01, // Экшен активации
				DISABLED = 0x02  // Экшен деактивации
			};
			/**
			 * Основные поддерживаемые протоколы
			 */
			enum class proto_t : uint8_t {
				NONE    = 0x00, // Протокол не установлен
				RAW     = 0x01, // Протокол является бинарным
				SPDY1   = 0x02, // Протокол соответствует SPDY/1
				HTTP1   = 0x03, // Протокол соответствует HTTP/1
				HTTP1_1 = 0x04, // Протокол соответствует HTTP/1.1
				HTTP2   = 0x05, // Протокол соответствует HTTP/2
				HTTP3   = 0x06  // Протокол соответствует HTTP/3
			};
			/**
			 * Основные методы режимов работы
			 */
			enum class method_t : uint8_t {
				READ       = 0x01, // Метод чтения из сокета
				WRITE      = 0x02, // Метод записи в сокет
				ACCEPT     = 0x03, // Метод запроса на подключение
				CONNECT    = 0x04, // Метод подключения к серверу
				DISCONNECT = 0x00  // Метод отключения от сервера
			};
			/**
			 * Тип активного приложения
			 */
			enum class type_t : uint8_t {
				NONE   = 0x00, // Приложение не инициализированно
				CLIENT = 0x01, // Приложение является клиентом
				SERVER = 0x02  // Приложение является сервером
			};
		private:
			/**
			 * @brief Структура подключения
			 *
			 */
			typedef struct Peer {
				socklen_t size;                 // Размер объекта подключения
				struct sockaddr_storage client; // Параметры подключения клиента
				struct sockaddr_storage server; // Параметры подключения сервера
				/**
				 * @brief Конструктор
				 *
				 */
				Peer() noexcept : size(0), client{}, server{} {}
			} peer_t;
		public:
			/**
			 * @brief Структура с параметрами для постоянного подключения
			 *
			 */
			typedef struct KeepAlive {
				int32_t cnt;   // Максимальное количество попыток
				int32_t idle;  // Интервал времени в секундах через которое происходит проверка подключения
				int32_t intvl; // Интервал времени в секундах между попытками
				/**
				 * @brief Конструктор
				 *
				 */
				KeepAlive() noexcept : cnt(3), idle(1), intvl(2) {}
			} __attribute__((packed)) alive_t;
			/**
			 * @brief Класс сетевого пространства
			 *
			 */
			typedef class AWHSHARED_EXPORT Address {
				public:
					/**
					 * Статус подключения
					 */
					enum class status_t : uint8_t {
						ACCEPTED     = 0x01, // Статус подключения (разрешено)
						ATTACHED     = 0x03, // Статус подключения (прикреплено)
						CONNECTED    = 0x02, // Статус подключения (подключено)
						DISCONNECTED = 0x00  // Статус подключения (отключено)
					};
				private:
					/**
					 * @brief Устанавливаем дружбу с классом двигателя
					 *
					 */
					friend class Engine;
				public:
					// Сетевой сокет
					SOCKET sock;
				private:
					// Тип сокета (SOCK_STREAM | SOCK_DGRAM)
					int32_t _type;
					// Протокол сокета (IPPROTO_TCP | IPPROTO_UDP | IPPROTO_SCTP)
					int32_t _protocol;
				private:
					// Флаг асинхронного режима работы сокета
					bool _async;
					// Флаг инициализации шифрования
					bool _encrypted;
				public:
					// Статус подключения
					status_t status;
				public:
					// Порт клиента
					uint32_t port;
					// Адрес интернет подключения и аппаратный
					string ip, mac;
				public:
					// Параметры постоянного подключения
					alive_t alive;
				private:
					// Объект работы с файловой системой
					fs_t _fs;
					// Объект подключения
					peer_t _peer;
					// Объект для работы с сетевым интерфейсом
					ifnet_t _ifnet;
					// Объект для работы с сокетами
					socket_t _socket;
				private:
					// Выполняем инициализацию генератора
					std::random_device _randev;
				public:
					// Список сетевых интерфейсов
					vector <string> network;
				private:
					// Объект BIO
					BIO * _bio;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				private:
					/**
					 * Для операционной системы OS Windows
					 */
					#if _WIN32 || _WIN64
						// Объект данных запроса
						WSADATA _wsaData;
						// Флаг инициализации WinSocksAPI
						bool _winSockInit;
					#endif
				private:
					/**
					 * @brief Метод закрытия подключения
					 *
					 */
					bool close() noexcept;
					/**
					 * @brief Метод извлечения данных клиента
					 *
					 */
					void client() noexcept;
				public:
					/**
					 * @brief Метод активации прослушивания сокета
					 *
					 * @return результат выполнения операции
					 */
					bool list() noexcept;
					/**
					 * @brief Метод очистки параметров подключения
					 *
					 * @return результат выполнения операции
					 */
					bool clear() noexcept;
				public:
					/**
					 * @brief Метод выполнения подключения
					 *
					 * @return результат выполнения операции
					 */
					bool connect() noexcept;
				private:
					/**
					 * @brief Метод извлечения хоста компьютера
					 *
					 * @param family семейство сокета (AF_INET / AF_INET6)
					 * @return       хост компьютера с которого производится запрос
					 */
					string host(const int32_t family) const noexcept;
				public:
					/**
					 * @brief Метод прикрепления клиента к серверу
					 *
					 * @param addr объект подключения сервера
					 */
					bool attach(Address & addr) noexcept;
				public:
					/**
					 * @brief Метод согласования подключения
					 *
					 * @param addr объект подключения сервера
					 * @return     результат выполнения операции
					 */
					bool accept(Address & addr) noexcept;
					/**
					 * @brief Метод согласования подключения
					 *
					 * @param sock   сетевой сокет сервера
					 * @param family семейство сокета (AF_INET / AF_INET6 / AF_UNIX)
					 * @return       результат выполнения операции
					 */
					bool accept(const SOCKET sock, const int32_t family) noexcept;
				public:
					/**
					 * @brief Метод установки параметров сокета
					 *
					 * @param type     тип сокета (SOCK_STREAM / SOCK_DGRAM)
					 * @param protocol протокол сокета (IPPROTO_TCP / IPPROTO_UDP / IPPROTO_SCTP)
					 */
					void sonet(const int32_t type = SOCK_STREAM, const int32_t protocol = IPPROTO_TCP) noexcept;
				public:
					/**
					 * @brief Метод инициализации адресного пространства сокета
					 *
					 * @param unixsocket адрес unxi-сокета в файловой системе
					 * @param type       тип приложения (клиент или сервер)
					 */
					void init(const string & unixsocket, const type_t type) noexcept;
					/**
					 * @brief Метод инициализации адресного пространства сокета
					 *
					 * @param ip     адрес для которого нужно создать сокет
					 * @param port   порт сервера для которого нужно создать сокет
					 * @param family семейство сокета (AF_INET / AF_INET6 / AF_UNIX)
					 * @param type   тип приложения (клиент или сервер)
					 * @param onlyV6 флаг разрешающий использовать только IPv6 подключение
					 * @return       параметры подключения к серверу
					 */
					void init(const string & ip, const uint32_t port, const int32_t family, const type_t type, const bool onlyV6 = false) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Address(const fmk_t * fmk, const log_t * log) noexcept :
					 sock(INVALID_SOCKET), _type(SOCK_STREAM), _protocol(IPPROTO_TCP),
					 _async(false), _encrypted(false), status(status_t::DISCONNECTED),
					 port(0), ip{""}, mac{""}, _fs(fmk, log), _ifnet(fmk, log),
					 _socket(fmk, log), _bio(nullptr), _fmk(fmk), _log(log) {}
					/**
					 * @brief Деструктор
					 *
					 */
					~Address() noexcept;
			} addr_t;
		private:
			/**
			 * @brief Структура адресов файлов сертификатов
			 *
			 */
			typedef struct Cert {
				// Сертификат центра сертификации (CA-файл)
				string ca;
				// Сертификат отозванных сертификатов (CRL-файл)
				string crl;
				// Приватный ключ сертификата
				string key;
				// Основной сертификат или цепочка сертификатов
				string pem;
				// Каталог с сертификатами центра сертификации (CA-файлами)
				string path;
				/**
				 * @brief Конструктор
				 *
				 */
				Cert() noexcept : ca{SSL_CA_FILE}, crl{""}, key{""}, pem{""}, path{""} {}
			} cert_t;
			/**
			 * @brief Структура параметров для валидации доменов
			 *
			 */
			typedef struct Verify {
				string host;           // Хост для валидации
				const Engine * engine; // Объект для работы с двигателем
				/**
				 * @brief Конструктор
				 *
				 * @param engine основной родительский объект двигателя
				 * @param host   хост для которого производится проверка
				 */
				Verify(const string & host = "", const Engine * engine = nullptr) noexcept : host{host}, engine(engine) {}
			} verify_t;
		public:
			/**
			 * @brief Класс контекста двигателя
			 *
			 */
			typedef class AWHSHARED_EXPORT Context {
				private:
					/**
					 * @brief Устанавливаем дружбу с родительским объектом двигателя
					 *
					 */
					friend class Engine;
				private:
					// Флаг вывода информации об OpenSSL
					bool _verb;
					// Флаг инициализации шифрования
					bool _encrypted;
				private:
					// Тип активного приложения
					type_t _type;
				private:
					// Протокол подключения
					proto_t _proto;
				private:
					BIO * _bio;     // Объект BIO
					SSL * _ssl;     // Объект SSL
					SSL_CTX * _ctx; // Контекст SSL
					addr_t * _addr; // Объект подключения
				public:
					// Список поддерживаемых протоколов
					vector <uint8_t> protocols;
				private:
					// Параметры валидации домена
					std::unique_ptr <verify_t> _verify;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				private:
					// Объект CRL-файла сертификата
					const X509_CRL * _crl;
				private:
					/**
					 * @brief Метод вывода информации об ошибке
					 *
					 * @param status статус ошибки
					 * @return       результат работы фукнции
					 */
					int32_t error(const int32_t status) const noexcept;
				public:
					/**
					 * @brief Метод очистки контекста
					 *
					 */
					void clear() noexcept;
				public:
					/**
					 * @brief Метод вывода информации о сертификате
					 *
					 */
					void info() const noexcept;
				public:
					/**
					 * @brief Метод установки CRL-файла сертификата
					 *
					 * @param crl CRL-файл сертификат
					 */
					void crl(const X509_CRL * crl) noexcept;
				public:
					/**
					 * @brief Метод чтения данных из сокета
					 *
					 * @param buffer буфер данных для чтения
					 * @param size   размер буфера данных
					 * @return       количество считанных байт
					 */
					int64_t read(char * buffer, const size_t size) noexcept;
					/**
					 * @brief Метод записи данных в сокет
					 *
					 * @param buffer буфер данных для записи
					 * @param size   размер буфера данных
					 * @return       количество записанных байт
					 */
					int64_t write(const char * buffer, const size_t size) noexcept;
				public:
					/**
					 * @brief Метод проверки на то, является ли сокет заблокированным
					 *
					 * @return результат работы функции
					 */
					bool blocking() noexcept;
					/**
					 * @brief Метод активации/деактивации блокирующего сокета
					 *
					 * @param mode режим применимой операции
					 * @return     результат работы функции
					 */
					bool blocking(const mode_t mode) noexcept;
				public:
					/**
					 * @brief Метод отключения/включения алгоритма TCP/CORK
					 *
					 * @param mode режим применимой операции
					 * @return     результат выполенния операции
					 */
					bool cork(const mode_t mode) noexcept;
					/**
					 * @brief Метод отключения/включения алгоритма Нейгла
					 *
					 * @param mode режим применимой операции
					 * @return     результат выполенния операции
					 */
					bool nodelay(const mode_t mode) noexcept;
				public:
					/**
					 * @brief Метод извлечения поддерживаемого протокола
					 *
					 * @return поддерживаемый протокол подключения
					 */
					proto_t proto() const noexcept;
					/**
					 * @brief Метод установки поддерживаемого протокола
					 *
					 * @param proto устанавливаемый протокол
					 */
					void proto(const proto_t proto) noexcept;
				public:
					/**
					 * @brief Метод установки таймаута
					 *
					 * @param msec   количество миллисекунд
					 * @param method метод для установки таймаута
					 * @return       результат установки таймаута
					 */
					bool timeout(const uint32_t msec, const method_t method) noexcept;
				public:
					/**
					 * @brief Метод получения размеров буфера
					 *
					 * @param method метод для выполнения операции с буфером
					 * @return       размер буфера
					 */
					int32_t buffer(const method_t method) const noexcept;
					/**
					 * @brief Метод установки размеров буфера
					 *
					 * @param read  размер буфера на чтение
					 * @param write размер буфера на запись
					 * @return      результат работы функции
					 */
					bool buffer(const int32_t read, const int32_t write) noexcept;
				public:
					/**
					 * @brief Метод проверки количества находящихся байт в сокете
					 *
					 * @param method метод для выполнения операции
					 * @return       количество байт в сокете
					 */
					int32_t availability(const method_t method) const noexcept;
				private:
					/**
					 * @brief Метод выполнения выбора следующего протокола
					 *
					 * @param out     буфер назначения
					 * @param outSize размер буфера назначения
					 * @param in      буфер входящих данных
					 * @param inSize  размер буфера входящих данных
					 * @param key     ключ копирования
					 * @param keySize размер ключа для копирования
					 * @return        результат переключения протокола
					 */
					bool selectProto(uint8_t ** out, uint8_t * outSize, const uint8_t * in, uint32_t inSize, const char * key, uint32_t keySize) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Context(const fmk_t * fmk, const log_t * log) noexcept :
					 _verb(false), _encrypted(false),
					 _type(type_t::NONE), _proto(proto_t::RAW),
					 _bio(nullptr), _ssl(nullptr), _ctx(nullptr),
					 _addr(nullptr), _verify(nullptr), _fmk(fmk), _log(log), _crl(nullptr) {}
					/**
					 * @brief Деструктор
					 *
					 */
					~Context() noexcept;
			} ctx_t;
			/**
			 * Типы ошибок валидации
			 */
			enum class validate_t : uint8_t {
				NONE                 = 0x00, // Не установлено
				Error                = 0x01, // Ошибка валидации
				MatchFound           = 0x02, // Валидация пройдена
				NoSANPresent         = 0x03, // Сеть не распознана
				MatchNotFound        = 0x04, // Валидация не пройдена
				MalformedCertificate = 0x05  // Неверный сертификат
			};
		private:
			// Флаг проверки сертификата доменного имени
			bool _verify;
		private:
			// Объект работы с файловой системой
			fs_t _fs;
		private:
			// Список алгоритмов шифрования
			string _cipher;
		private:
			// Объект файлов сертификатов
			mutable cert_t _cert;
		private:
			// Флаг инициализации куков
			static bool _cookieInit;
			// Буфер для создания куков
			static uint8_t _cookies[16];
		private:
			// Объект CRL-файла сертификата
			X509_CRL * _crl;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
			// Объект работы с URI
			const uri_t * _uri;
		private:
			/**
			 * @brief Метод проверки на эквивалентность доменных имён
			 *
			 * @param first  первое доменное имя
			 * @param second второе доменное имя
			 * @return       результат проверки
			 */
			bool rawEqual(const string & first, const string & second) const noexcept;
			/**
			 * @brief Метод проверки на не эквивалентность доменных имён
			 *
			 * @param first  первое доменное имя
			 * @param second второе доменное имя
			 * @param max    количество начальных символов для проверки
			 * @return       результат проверки
			 */
			bool rawNequal(const string & first, const string & second, const size_t max = 0) const noexcept;
		private:
			/**
			 * @brief Метод проверки эквивалентности доменного имени с учетом шаблона
			 *
			 * @param host доменное имя
			 * @param patt шаблон домена
			 * @return     результат проверки
			 */
			bool hostmatch(const string & host, const string & patt) const noexcept;
			/**
			 * @brief Метод проверки доменного имени по шаблону
			 *
			 * @param host доменное имя
			 * @param patt шаблон домена
			 * @return     результат проверки
			 */
			bool certHostcheck(const string & host, const string & patt) const noexcept;
		private:
			/**
			 * Для операционной системы Linux или FreeBSD
			 */
			#if __linux__ || __FreeBSD__
				/**
				 * @brief Функция обработки нотификации SCTP
				 *
				 * @param bio    объект подключения BIO
				 * @param ctx    промежуточный передаваемый контекст
				 * @param buffer буфер передаваемых данных
				 */
				static void notificationsSCTP(BIO * bio, void * ctx, void * buffer) noexcept;
			#endif
		private:
			/**
			 * @brief Функция обратного вызова для проверки валидности сертификата
			 *
			 * @param ok   результат получения сертификата
			 * @param x509 данные сертификата
			 * @return     результат проверки
			 */
			static int32_t verifyCert(const int32_t ok, X509_STORE_CTX * x509) noexcept;
			/**
			 * @brief Функция обратного вызова для проверки валидности хоста
			 *
			 * @param x509 данные сертификата
			 * @param ctx  передаваемый контекст
			 * @return     результат проверки
			 */
			static int32_t verifyHost(X509_STORE_CTX * x509, void * ctx = nullptr) noexcept;
		private:
			/**
			 * OpenSSL собран без следующих переговорщиков по протоколам
			 */
			#ifndef OPENSSL_NO_NEXTPROTONEG
				/**
				 * @brief Функция обратного вызова сервера для переключения на следующий протокол
				 *
				 * @param ssl  объект SSL
				 * @param data данные буфера данных протокола
				 * @param len  размер буфера данных протокола
				 * @param ctx  передаваемый контекст
				 * @return     результат переключения протокола
				 */
				static int32_t nextProto(SSL * ssl, const uint8_t ** data, uint32_t * len, void * ctx = nullptr) noexcept;
				/**
				 * @brief Функция обратного вызова клиента для расширения NPN TLS. Выполняется проверка, что сервер объявил протокол HTTP/2, который поддерживает библиотека nghttp2.
				 *
				 * @param ssl     объект SSL
				 * @param out     буфер исходящего протокола
				 * @param outSize размер буфера исходящего протокола
				 * @param in      буфер входящего протокола
				 * @param inSize  размер буфера входящего протокола
				 * @param ctx     передаваемый контекст
				 * @return        результат выбора протокола
				 */
				static int32_t selectNextProtoClient(SSL * ssl, uint8_t ** out, uint8_t * outSize, const uint8_t * in, uint32_t inSize, void * ctx = nullptr) noexcept;
			#endif
			/**
			 * Если версия OpenSSL соответствует или выше версии 1.0.2
			 */
			#if OPENSSL_VERSION_NUMBER >= 0x10002000L
				/**
				 * @brief Функция обратного вызова сервера для расширения NPN TLS. Выполняется проверка, что сервер объявил протокол HTTP/2, который поддерживает библиотека nghttp2.
				 *
				 * @param ssl     объект SSL
				 * @param out     буфер исходящего протокола
				 * @param outSize размер буфера исходящего протокола
				 * @param in      буфер входящего протокола
				 * @param inSize  размер буфера входящего протокола
				 * @param ctx     передаваемый контекст
				 * @return        результат выбора протокола
				 */
				static int32_t selectNextProtoServer(SSL * ssl, const uint8_t ** out, uint8_t * outSize, const uint8_t * in, uint32_t inSize, void * ctx = nullptr) noexcept;
			#endif
		private:
			/**
			 * @brief Функция обратного вызова для генерации куков
			 *
			 * @param ssl    объект SSL
			 * @param cookie данные куков
			 * @param size   количество символов
			 * @return       результат проверки
			 */
			static int32_t generateCookie(SSL * ssl, uint8_t * cookie, uint32_t * size) noexcept;
			/**
			 * @brief Функция обратного вызова для проверки куков
			 *
			 * @param ssl    объект SSL
			 * @param cookie данные куков
			 * @param size   количество символов
			 * @return       результат проверки
			 */
			static int32_t verifyCookie(SSL * ssl, const uint8_t * cookie, uint32_t size) noexcept;
			/**
			 * @brief Функция обратного вызова для генерации куков
			 *
			 * @param ssl    объект SSL
			 * @param cookie данные куков
			 * @param size   количество символов
			 * @return       результат проверки
			 */
			static int32_t generateStatelessCookie(SSL * ssl, uint8_t * cookie, size_t * size) noexcept;
			/**
			 * @brief Функция обратного вызова для проверки куков
			 *
			 * @param ssl    объект SSL
			 * @param cookie данные куков
			 * @param size   количество символов
			 * @return       результат проверки
			 */
			static int32_t verifyStatelessCookie(SSL * ssl, const uint8_t * cookie, size_t size) noexcept;
		private:
			/**
			 * @brief Метод проверки доменного имени по данным из сертификата
			 *
			 * @param host доменное имя
			 * @param cert сертификат
			 * @return     результат проверки
			 */
			validate_t matchesCommonName(const string & host, const X509 * cert = nullptr) const noexcept;
			/**
			 * @brief Метод проверки доменного имени по списку доменных имён из сертификата
			 *
			 * @param host доменное имя
			 * @param cert сертификат
			 * @return     результат проверки
			 */
			validate_t matchSubjectName(const string & host, const X509 * cert = nullptr) const noexcept;
		private:
			/**
			 * @brief Метод проверки доменного имени
			 *
			 * @param host доменное имя
			 * @param cert сертификат
			 * @return     результат проверки
			 */
			validate_t validateHostname(const string & host, const X509 * cert = nullptr) const noexcept;
		private:
			/**
			 * @brief Метод инициализации магазина CA-файлов сертификатов
			 *
			 * @param ctx объект контекста SSL
			 * @return    результат инициализации
			 */
			bool storeCA(SSL_CTX * ctx) const noexcept;
			/**
			 * @brief Метод инициализации магазина CRL-файлов сертификатов
			 *
			 * @param ctx объект контекста SSL
			 * @return    результат инициализации
			 */
			bool storeCRL(SSL_CTX * ctx) const noexcept;
		public:
			/**
			 * @brief Метод ожидания рукопожатия
			 *
			 * @param target контекст назначения
			 * @return       результат проверки
			 */
			bool wait(ctx_t & target) noexcept;
		public:
			/**
			 * @brief Метод проверки на активацию режима шифрования
			 *
			 * @param ctx контекст подключения
			 * @return    результат проверки
			 */
			bool encrypted(ctx_t & ctx) const noexcept;
			/**
			 * @brief Метод установки флага режима шифрования
			 *
			 * @param mode флаг режима шифрования
			 * @param ctx  контекст подключения
			 */
			void encrypted(const bool mode, ctx_t & ctx) noexcept;
		public:
			/**
			 * @brief Метод извлечения активного протокола
			 *
			 * @param target контекст назначения
			 * @return       метод активного протокола
			 */
			proto_t proto(ctx_t & target) const noexcept;
		private:
			/**
			 * @brief Метод активации протокола HTTP
			 *
			 * @param target контекст назначения
			 */
			void httpUpgrade(ctx_t & target) const noexcept;
		public:
			/**
			 * @brief Метод прикрепления контекста клиента к контексту сервера
			 *
			 * @param target  контекст назначения
			 * @param address объект подключения
			 * @return        объект SSL контекста
			 */
			void attach(ctx_t & target, addr_t * address) noexcept;
		public:
			/**
			 * @brief Метод обертывания сетевого сокета для сервера
			 *
			 * @param target контекст назначения
			 * @param source исходный контекст
			 * @return       объект SSL контекста
			 */
			void wrap(ctx_t & target, ctx_t & source) noexcept;
			/**
			 * @brief Метод обертывания сетевого сокета для сервера
			 *
			 * @param target  контекст назначения
			 * @param address объект подключения
			 * @return        объект SSL контекста
			 */
			void wrap(ctx_t & target, addr_t * address) noexcept;
		public:
			/**
			 * @brief Метод обертывания сетевого сокета для клиента/сервера
			 *
			 * @param target  контекст назначения
			 * @param address объект подключения
			 * @param type    тип активного приложения
			 * @return        объект SSL контекста
			 */
			void wrap(ctx_t & target, addr_t * address, const type_t type) noexcept;
		public:
			/**
			 * @brief Метод обертывания сетевого сокета для клиента
			 *
			 * @param target контекст назначения
			 * @param source исходный контекст
			 * @param host   хост удалённого сервера
			 * @return       объект SSL контекста
			 */
			void wrap(ctx_t & target, ctx_t & source, const string & host) noexcept;
			/**
			 * @brief Метод обертывания сетевого сокета для клиента
			 *
			 * @param target  контекст назначения
			 * @param address объект подключения
			 * @param host    хост удалённого сервера
			 * @return        объект SSL контекста
			 */
			void wrap(ctx_t & target, addr_t * address, const string & host) noexcept;
		public:
			/**
			 * @brief Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
			 *
			 * @param mode флаг состояния разрешения проверки
			 */
			void verify(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод установки алгоритмов шифрования
			 *
			 * @param ciphers список алгоритмов шифрования для установки
			 */
			void ciphers(const vector <string> & ciphers) noexcept;
		public:
			/**
			 * @brief Метод установки CRL-файла отозванных сертификатов центром сертификации
			 *
			 * @param crl адрес CRL-файла отозванных сертификатов центром сертификации
			 */
			void crl(const string & crl) noexcept;
			/**
			 * @brief Метод установки сертификата центра сертификации (CA-файла)
			 *
			 * @param ca   адрес сертификата центра сертификации (CA-файла)
			 * @param path адрес каталога где находится сертификат (CA-файл)
			 */
			void ca(const string & ca, const string & path = "") noexcept;
			/**
			 * @brief Метод установки файлов сертификата
			 *
			 * @param pem файл цепочки сертификатов
			 * @param key приватный ключ сертификата (если требуется)
			 */
			void certificate(const string & pem, const string & key = "") noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			Engine(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Engine() noexcept;
	} engine_t;
};

#endif // __AWH_ENGINE__
