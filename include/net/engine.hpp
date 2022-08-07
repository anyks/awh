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
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_ENGINE__
#define __AWH_ENGINE__

/**
 * Отключаем Deprecated для Apple
 */
#if defined(__APPLE__) && defined(__clang__)
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

/**
 * Стандартная библиотека
 */
#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <errno.h>
#include <sys/types.h>

/**
 * Если операционной системой является Linux
 */
#ifdef  __linux__
	#include <netinet/sctp.h>
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
	#include <sys/un.h>
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
#include <net/uri.hpp>
#include <net/socket.hpp>

/**
 * Подключаем OpenSSL
 */
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>

// Устанавливаем область видимости
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Engine Класс для работы с двигателем передачи данных
	 */
	typedef class Engine {
		public:
			/**
			 * Основные методы режимов работы
			 */
			enum class method_t : uint8_t {
				READ    = 0x01, // Метод чтения из сокета
				WRITE   = 0x02, // Метод записи в сокет
				CONNECT = 0x03  // Метод подключения к серверу
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
		public:
			/**
			 * KeepAlive Структура с параметрами для постоянного подключения
			 */
			typedef struct KeepAlive {
				int cnt;   // Максимальное количество попыток
				int idle;  // Интервал времени в секундах через которое происходит проверка подключения
				int intvl; // Интервал времени в секундах между попытками
				/**
				 * KeepAlive Конструктор
				 */
				KeepAlive() noexcept : cnt(3), idle(1), intvl(2) {}
			} __attribute__((packed)) alive_t;
			/**
			 * Address Класс сетевого пространства
			 */
			typedef class Address {
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
					 * Engine Устанавливаем дружбу с классом двигателя
					 */
					friend class Engine;
				public:
					// Файловый дескриптор
					int fd;
				private:
					// Тип сокета (SOCK_STREAM / SOCK_DGRAM)
					int _type;
					// Протокол сокета (IPPROTO_TCP / IPPROTO_UDP / IPPROTO_SCTP)
					int _protocol;
				private:
					// Флаг инициализации шифрования TLS
					bool _tls;
				public:
					// Статус подключения
					status_t status;
				public:
					// Порт клиента
					u_int port;
					// Адрес интернет подключения и аппаратный
					string ip, mac;
				public:
					// Параметры постоянного подключения
					alive_t alive;
				private:
					// Объект подключения
					peer_t _peer;
					// Создаем объект сети
					network_t _nwk;
					// Объект для работы с сетевым интерфейсом
					ifnet_t _ifnet;
					// Объект для работы с сокетами
					socket_t _socket;
				public:
					// Список сетевых интерфейсов
					vector <string> network;
				private:
					// Создаём объект фреймворка
					const fmk_t * _fmk;
					// Создаём объект работы с логами
					const log_t * _log;
				private:
					/**
					 * client Метод извлечения данных клиента
					 */
					void client() noexcept;
				public:
					/**
					 * list Метод активации прослушивания сокета
					 * @return результат выполнения операции
					 */
					bool list() noexcept;
					/**
					 * close Метод закрытия подключения
					 * @return результат выполнения операции
					 */
					bool close() noexcept;
				public:
					/**
					 * connect Метод выполнения подключения
					 * @return результат выполнения операции
					 */
					bool connect() noexcept;
				public:
					/**
					 * attach Метод прикрепления клиента к серверу
					 * @param addr объект подключения сервера
					 */
					bool attach(Address & addr) noexcept;
				public:
					/**
					 * accept Метод согласования подключения
					 * @param addr объект подключения сервера
					 * @return     результат выполнения операции
					 */
					bool accept(Address & addr) noexcept;
					/**
					 * accept Метод согласования подключения
					 * @param fd     файловый дескриптор сервера
					 * @param family семейство сокета (AF_INET / AF_INET6 / AF_UNIX)
					 * @return       результат выполнения операции
					 */
					bool accept(const int fd, const int family) noexcept;
				public:
					/**
					 * sonet Метод установки параметров сокета
					 * @param type     тип сокета (SOCK_STREAM / SOCK_DGRAM)
					 * @param protocol протокол сокета (IPPROTO_TCP / IPPROTO_UDP)
					 */
					void sonet(const int type = SOCK_STREAM, const int protocol = IPPROTO_TCP) noexcept;
				public:
					/**
					 * init Метод инициализации адресного пространства сокета
					 * @param unixsocket адрес unxi-сокета в файловой системе
					 * @param type       тип приложения (клиент или сервер)
					 */
					void init(const string & unixsocket, const type_t type) noexcept;
					/**
					 * init Метод инициализации адресного пространства сокета
					 * @param ip     адрес для которого нужно создать сокет
					 * @param port   порт сервера для которого нужно создать сокет
					 * @param family семейство сокета (AF_INET / AF_INET6 / AF_UNIX)
					 * @param type   тип приложения (клиент или сервер)
					 * @param onlyV6 флаг разрешающий использовать только IPv6 подключение
					 * @return       параметры подключения к серверу
					 */
					void init(const string & ip, const u_int port, const int family, const type_t type, const bool onlyV6 = false) noexcept;
				public:
					/**
					 * Address Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Address(const fmk_t * fmk, const log_t * log) noexcept :
					 fd(-1), _type(SOCK_STREAM), _protocol(IPPROTO_TCP), _tls(false),
					 status(status_t::DISCONNECTED), port(0), ip(""), mac(""),
					 _nwk(fmk), _ifnet(fmk, log), _socket(log), _fmk(fmk), _log(log) {}
					/**
					 * ~Address Деструктор
					 */
					~Address() noexcept;
			} addr_t;
		private:
			/**
			 * Verify Структура параметров для валидации доменов
			 */
			typedef struct Verify {
				string host;           // Хост для валидации
				const Engine * engine; // Объект для работы с двигателем
				/**
				 * Verify Конструктор
				 * @param engine основной родительский объект двигателя
				 * @param host   хост для которого производится проверка
				 */
				Verify(const string & host = "", const Engine * engine = nullptr) noexcept : host(host), engine(engine) {}
			} verify_t;
		public:
			/**
			 * Context Класс контекста двигателя
			 */
			typedef class Context {
				private:
					/**
					 * Engine Устанавливаем дружбу с родительским объектом двигателя
					 */
					friend class Engine;
				private:
					// Флаг инициализации шифрования TLS
					bool _tls;
					// Флаг вывода информации об OpenSSL
					bool _verb;
				private:
					// Тип активного приложения
					type_t _type;
				private:
					BIO * _bio;         // Объект BIO
					SSL * _ssl;         // Объект SSL
					SSL_CTX * _ctx;     // Контекст SSL
					addr_t * _addr;     // Объект подключения
					verify_t * _verify; // Параметры валидации домена
				private:
					// Создаём объект фреймворка
					const fmk_t * _fmk;
					// Создаём объект работы с логами
					const log_t * _log;
				private:
					/**
					 * error Метод вывода информации об ошибке
					 * @param status статус ошибки
					 */
					void error(const int status) const noexcept;
				public:
					/**
					 * clear Метод очистки контекста
					 */
					void clear() noexcept;
				public:
					/**
					 * info Метод вывода информации о сертификате
					 */
					void info() const noexcept;
				public:
					/**
					 * read Метод чтения данных из сокета
					 * @param buffer буфер данных для чтения
					 * @param size   размер буфера данных
					 * @return       количество считанных байт
					 */
					int64_t read(char * buffer, const size_t size) noexcept;
					/**
					 * write Метод записи данных в сокет
					 * @param buffer буфер данных для записи
					 * @param size   размер буфера данных
					 * @return       количество записанных байт
					 */
					int64_t write(const char * buffer, const size_t size) noexcept;
				public:
					/**
					 * block Метод установки блокирующего сокета
					 * @return результат работы функции
					 */
					int block() noexcept;
					/**
					 * noblock Метод установки неблокирующего сокета
					 * @return результат работы функции
					 */
					int noblock() noexcept;
					/**
					 * isblock Метод проверки на то, является ли сокет заблокированным
					 * @return результат работы функции
					 */
					int isblock() noexcept;
					/**
					 * timeout Метод установки таймаута
					 * @param msec   количество миллисекунд
					 * @param method метод для установки таймаута
					 * @return       результат установки таймаута
					 */
					int timeout(const time_t msec, const method_t method) noexcept;
					/**
					 * buffer Метод установки размеров буфера
					 * @param read  размер буфера на чтение
					 * @param write размер буфера на запись
					 * @param total максимальное количество подключений
					 * @return      результат работы функции
					 */
					int buffer(const int read, const int write, const u_int total) noexcept;
				public:
					/**
					 * Context Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Context(const fmk_t * fmk, const log_t * log) noexcept :
					 _tls(false), _verb(false), _type(type_t::NONE),
					 _bio(nullptr), _ssl(nullptr), _ctx(nullptr),
					 _addr(nullptr), _verify(nullptr), _fmk(fmk), _log(log) {}
					/**
					 * ~Context Деструктор
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
			// Список алгоритмов шифрования
			string _cipher;
		private:
			// Основной сертификат или цепочка сертификатов
			string _chain;
			// Приватный ключ сертификата
			string _privkey;
		private:
			// Каталог с доверенными сертификатами (CA-файлами)
			string _path;
			// Доверенный сертификат (CA-файл)
			mutable string _ca;
		private:
			// Флаг инициализации куков
			static bool _cookieInit;
			// Буфер для создания куков
			static u_char _cookies[16];
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
			// Создаём объект работы с URI
			const uri_t * _uri;
		private:
			/**
			 * rawEqual Метод проверки на эквивалентность доменных имён
			 * @param first  первое доменное имя
			 * @param second второе доменное имя
			 * @return       результат проверки
			 */
			const bool rawEqual(const string & first, const string & second) const noexcept;
			/**
			 * rawNequal Метод проверки на не эквивалентность доменных имён
			 * @param first  первое доменное имя
			 * @param second второе доменное имя
			 * @param max    количество начальных символов для проверки
			 * @return       результат проверки
			 */
			const bool rawNequal(const string & first, const string & second, const size_t max = 0) const noexcept;
		private:
			/**
			 * hostmatch Метод проверки эквивалентности доменного имени с учетом шаблона
			 * @param host доменное имя
			 * @param patt шаблон домена
			 * @return     результат проверки
			 */
			const bool hostmatch(const string & host, const string & patt) const noexcept;
			/**
			 * certHostcheck Метод проверки доменного имени по шаблону
			 * @param host доменное имя
			 * @param patt шаблон домена
			 * @return     результат проверки
			 */
			const bool certHostcheck(const string & host, const string & patt) const noexcept;
		private:
			/**
			 * verifyCert Функция обратного вызова для проверки валидности сертификата
			 * @param ok   результат получения сертификата
			 * @param x509 данные сертификата
			 * @return     результат проверки
			 */
			static int verifyCert(const int ok, X509_STORE_CTX * x509) noexcept;
			/**
			 * verifyHost Функция обратного вызова для проверки валидности хоста
			 * @param x509 данные сертификата
			 * @param ctx  передаваемый контекст
			 * @return     результат проверки
			 */
			static int verifyHost(X509_STORE_CTX * x509, void * ctx = nullptr) noexcept;
		private:
			/**
			 * generateCookie Функция обратного вызова для генерации куков
			 * @param ssl    объект SSL
			 * @param cookie данные куков
			 * @param size   количество символов
			 * @return       результат проверки
			 */
			static int generateCookie(SSL * ssl, u_char * cookie, u_int * size) noexcept;
			/**
			 * verifyCookie Функция обратного вызова для проверки куков
			 * @param ssl    объект SSL
			 * @param cookie данные куков
			 * @param size   количество символов
			 * @return       результат проверки
			 */
			static int verifyCookie(SSL * ssl, const u_char * cookie, u_int size) noexcept;
			/**
			 * generateCookie Функция обратного вызова для генерации куков
			 * @param ssl    объект SSL
			 * @param cookie данные куков
			 * @param size   количество символов
			 * @return       результат проверки
			 */
			static int generateStatelessCookie(SSL * ssl, u_char * cookie, size_t * size) noexcept;
			/**
			 * verifyCookie Функция обратного вызова для проверки куков
			 * @param ssl    объект SSL
			 * @param cookie данные куков
			 * @param size   количество символов
			 * @return       результат проверки
			 */
			static int verifyStatelessCookie(SSL * ssl, const u_char * cookie, size_t size) noexcept;
		private:
			/**
			 * matchesCommonName Метод проверки доменного имени по данным из сертификата
			 * @param host доменное имя
			 * @param cert сертификат
			 * @return     результат проверки
			 */
			const validate_t matchesCommonName(const string & host, const X509 * cert = nullptr) const noexcept;
			/**
			 * matchSubjectName Метод проверки доменного имени по списку доменных имён из сертификата
			 * @param host доменное имя
			 * @param cert сертификат
			 * @return     результат проверки
			 */
			const validate_t matchSubjectName(const string & host, const X509 * cert = nullptr) const noexcept;
		private:
			/**
			 * validateHostname Метод проверки доменного имени
			 * @param host доменное имя
			 * @param cert сертификат
			 * @return     результат проверки
			 */
			const validate_t validateHostname(const string & host, const X509 * cert = nullptr) const noexcept;
		private:
			/**
			 * storeCA Метод инициализации магазина доверенных сертификатов
			 * @param ctx объект контекста SSL
			 * @return    результат инициализации
			 */
			bool storeCA(SSL_CTX * ctx) const noexcept;
		public:
			/**
			 * isTLS Метод проверки на активацию режима шифрования
			 * @param ctx контекст подключения
			 * @return    результат проверки
			 */
			bool isTLS(ctx_t & ctx) const noexcept;
		public:
			/**
			 * wait Метод ожидания рукопожатия
			 * @param target контекст назначения
			 */
			void wait(ctx_t & target) noexcept;
		public:
			/**
			 * attach Метод прикрепления контекста клиента к контексту сервера
			 * @param target  контекст назначения
			 * @param address объект подключения
			 * @return        объект SSL контекста
			 */
			void attach(ctx_t & target, addr_t * address) noexcept;
		public:
			/**
			 * wrap Метод обертывания файлового дескриптора для сервера
			 * @param target  контекст назначения
			 * @param address объект подключения
			 * @param type    тип активного приложения
			 * @return        объект SSL контекста
			 */
			void wrap(ctx_t & target, addr_t * address, const type_t type) noexcept;
		public:
			/**
			 * wrapServer Метод обертывания файлового дескриптора для сервера
			 * @param target контекст назначения
			 * @param source исходный контекст
			 * @return       объект SSL контекста
			 */
			void wrapServer(ctx_t & target, ctx_t & source) noexcept;
			/**
			 * wrapServer Метод обертывания файлового дескриптора для сервера
			 * @param target  контекст назначения
			 * @param address объект подключения
			 * @return        объект SSL контекста
			 */
			void wrapServer(ctx_t & target, addr_t * address) noexcept;
		public:
			/**
			 * wrapClient Метод обертывания файлового дескриптора для клиента
			 * @param target контекст назначения
			 * @param source исходный контекст
			 * @param url    параметры URL адреса для инициализации
			 * @return       объект SSL контекста
			 */
			void wrapClient(ctx_t & target, ctx_t & source, const uri_t::url_t & url) noexcept;
			/**
			 * wrapClient Метод обертывания файлового дескриптора для клиента
			 * @param target  контекст назначения
			 * @param address объект подключения
			 * @param url     параметры URL адреса для инициализации
			 * @return        объект SSL контекста
			 */
			void wrapClient(ctx_t & target, addr_t * address, const uri_t::url_t & url) noexcept;
		public:
			/**
			 * verifyEnable Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
			 * @param mode флаг состояния разрешения проверки
			 */
			void verifyEnable(const bool mode) noexcept;
			/**
			 * ciphers Метод установки алгоритмов шифрования
			 * @param ciphers список алгоритмов шифрования для установки
			 */
			void ciphers(const vector <string> & ciphers) noexcept;
			/**
			 * ca Метод установки доверенного сертификата (CA-файла)
			 * @param trusted адрес доверенного сертификата (CA-файла)
			 * @param path    адрес каталога где находится сертификат (CA-файл)
			 */
			void ca(const string & trusted, const string & path = "") noexcept;
			/**
			 * setCert Метод установки файлов сертификата
			 * @param chain файл цепочки сертификатов
			 * @param key   приватный ключ сертификата (если требуется)
			 */
			void certificate(const string & chain, const string & key = "") noexcept;
		public:
			/**
			 * Engine Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			Engine(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept;
			/**
			 * ~Engine Деструктор
			 */
			~Engine() noexcept;
	} engine_t;
};

#endif // __AWH_ENGINE__
