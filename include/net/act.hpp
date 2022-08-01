/**
 * @file: act.hpp
 * @date: 2022-07-31
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

#ifndef __AWH_ACTUATOR__
#define __AWH_ACTUATOR__

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
	 * Actuator Класс для работы с каналом передачи данных
	 */
	typedef class Actuator {
		public:
			/**
			 * Тип активного приложения
			 */
			enum class type_t : uint8_t {
				NONE   = 0x00, // Приложение не инициализированно
				CLIENT = 0x01, // Приложение является клиентом
				SERVER = 0x02  // Приложение является сервером
			};
		public:
			/**
			 * KeepAlive Структура с параметрами для постоянного подключения
			 */
			typedef struct KeepAlive {
				int keepcnt;   // Максимальное количество попыток
				int keepidle;  // Интервал времени в секундах через которое происходит проверка подключения
				int keepintvl; // Интервал времени в секундах между попытками
				/**
				 * KeepAlive Конструктор
				 */
				KeepAlive() noexcept : keepcnt(3), keepidle(1), keepintvl(2) {}
			} __attribute__((packed)) alive_t;
			/**
			 * Прототип класса Context
			 */
			class Context;
			/**
			 * Sock Класс сетевого пространства
			 */
			typedef class Sock {
				public:
					/**
					 * Статус подключения
					 */
					enum class status_t : uint8_t {
						ACCEPTED     = 0x01, // Статус подключения (разрешено)
						CONNECTED    = 0x02, // Статус подключения (подключено)
						DISCONNECTED = 0x00  // Статус подключения (отключено)
					};
				private:
					/**
					 * Context Устанавливаем дружбу с классом контекста двигателя
					 */
					friend class Context;
				public:
					// Файловый дескриптор
					int fd;
					// Тип сокета (SOCK_STREAM / SOCK_DGRAM)
					int type;
					// Семейство сокета (AF_INET / AF_INET6 / AF_UNIX)
					int family;
					// Протокол сокета (IPPROTO_TCP / IPPROTO_UDP)
					int protocol;
				public:
					// Флаг разрешающий работу только с IPv6
					bool v6only;
				public:
					// Статус подключения
					status_t status;
				public:
					// Адрес интернет подключения и аппаратный
					string ip, mac;
				public:
					// Параметры постоянного подключения
					alive_t alive;
				private:
					// Создаем объект сети
					network_t nwk;
					// Объект для работы с сетевым интерфейсом
					ifnet_t ifnet;
					// Объект для работы с сокетами
					socket_t socket;
				public:
					// Список сетевых интерфейсов
					vector <string> network;
				public:
					// Объект SSL
					SSL * ssl;
				public:
				/**
				 * Если операционной системой не является Windows
				 */
				#if !defined(_WIN32) && !defined(_WIN64)
					// Параметры подключения для unix-сокета
					struct sockaddr_un uxsock;
				#endif
				public:
					// Параметры подключения клиента IPv4
					struct sockaddr_in client;
					// Параметры подключения сервера IPv4
					struct sockaddr_in server;
				public:
					// Параметры подключения клиента IPv6
					struct sockaddr_in6 client6;
					// Параметры подключения сервера IPv6
					struct sockaddr_in6 server6;
				public:
					// Создаём объект фреймворка
					const fmk_t * fmk = nullptr;
					// Создаём объект работы с логами
					const log_t * log = nullptr;
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
					/**
					 * connect Метод выполнения подключения сервера к клиенту для UDP
					 * @param sock объект подключения сервера
					 */
					bool connect(Sock & sock) noexcept;
				public:
					/**
					 * accept Метод согласования подключения
					 * @param sock объект подключения сервера
					 * @return     результат выполнения операции
					 */
					bool accept(Sock & sock) noexcept;
					/**
					 * accept Метод согласования подключения
					 * @param fd файловый дескриптор сервера
					 * @return   результат выполнения операции
					 */
					bool accept(const int fd) noexcept;
				public:
					/**
					 * init Метод инициализации адресного пространства сокета
					 * @param unixsocket адрес unxi-сокета в файловой системе
					 * @param type       тип приложения (клиент или сервер)
					 */
					void init(const string & unixsocket, const type_t type) noexcept;
					/**
					 * init Метод инициализации адресного пространства сокета
					 * @param ip   адрес для которого нужно создать сокет
					 * @param port порт сервера для которого нужно создать сокет
					 * @param type тип приложения (клиент или сервер)
					 * @return     параметры подключения к серверу
					 */
					void init(const string & ip, const u_int port, const type_t type) noexcept;
				public:
					/**
					 * Sock Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Sock(const fmk_t * fmk, const log_t * log) noexcept :
					 fd(-1), type(SOCK_STREAM), family(AF_INET),
					 protocol(IPPROTO_TCP), v6only(false),
					 status(status_t::DISCONNECTED), ip(""), mac(""),
					 nwk(fmk), ifnet(fmk, log), socket(log),
					 ssl(nullptr), fmk(fmk), log(log) {}
					/**
					 * ~Sock Деструктор
					 */
					~Sock() noexcept;
			} sock_t;
		private:
			/**
			 * Verify Структура параметров для валидации доменов
			 */
			typedef struct Verify {
				string host;          // Хост для валидации
				const Actuator * act; // Объект для работы с SSL
				/**
				 * Verify Конструктор
				 * @param act  основной родительский объект
				 * @param host хост для которого производится проверка
				 */
				Verify(const string & host = "", const Actuator * act = nullptr) noexcept : host(host), act(act) {}
			} verify_t;
		public:
			/**
			 * Context Класс контекста двигателя
			 */
			typedef class Context {
				private:
					/**
					 * Actuator Устанавливаем дружбу с родительским объектом актуатора
					 */
					friend class Actuator;
				private:
					// Флаг инициализации
					bool mode;
				private:
					// Тип активного приложения
					type_t type;
				private:
					BIO * bio;         // Объект BIO
					SSL * ssl;         // Объект SSL
					SSL_CTX * ctx;     // Контекст SSL
					sock_t * sock;     // Объект подключения
					verify_t * verify; // Параметры валидации домена
				private:
					// Создаём объект фреймворка
					const fmk_t * fmk = nullptr;
					// Создаём объект работы с логами
					const log_t * log = nullptr;
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
					 * wrapped Метод првоерки на активацию контекста
					 * @return результат проверки
					 */
					bool wrapped() const noexcept;
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
				public:
					/**
					 * Context Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Context(const fmk_t * fmk, const log_t * log) noexcept :
					 mode(false), type(type_t::NONE), sock(nullptr),
					 bio(nullptr), ssl(nullptr), ctx(nullptr),
					 verify(nullptr), log(log) {}
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
			bool verify = true;
		private:
			// Список алгоритмов шифрования
			string cipher = "";
		private:
			// Приватный ключ сертификата
			string privkey = "";
			// Основная цепочка сертификатов
			string fullchain = "";
		private:
			// Каталог с доверенными сертификатами (CA-файлами)
			string path = "";
			// Доверенный сертификат (CA-файл)
			mutable string trusted = SSL_CA_FILE;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
			// Создаём объект работы с URI
			const uri_t * uri = nullptr;
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
			 * verifyCookie Функция обратного вызова для проверки куков
			 * @param ssl    объект SSL
			 * @param cookie данные куков
			 * @param size   количество символов
			 * @return       результат проверки
			 */
			static int verifyCookie(SSL * ssl, u_char * cookie, u_int size) noexcept;
			/**
			 * generateCookie Функция обратного вызова для генерации куков
			 * @param ssl    объект SSL
			 * @param cookie данные куков
			 * @param size   количество символов
			 * @return       результат проверки
			 */
			static int generateCookie(SSL * ssl, u_char * cookie, u_int * size) noexcept;
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
			 * initTrustedStore Метод инициализации магазина доверенных сертификатов
			 * @param ctx объект контекста SSL
			 * @return    результат инициализации
			 */
			bool initTrustedStore(SSL_CTX * ctx) const noexcept;
		public:
			/**
			 * wrap Метод обертывания файлового дескриптора для сервера
			 * @param target контекст назначения
			 * @param source исходный контекст
			 * @return       объект SSL контекста
			 */
			void wrap(ctx_t & target, ctx_t & source) noexcept;
			/**
			 * wrap Метод обертывания файлового дескриптора для клиента
			 * @param target контекст назначения
			 * @param source исходный контекст
			 * @param url    параметры URL адреса для инициализации
			 * @return       объект SSL контекста
			 */
			void wrap(ctx_t & target, ctx_t & source, const uri_t::url_t & url) noexcept;
		public:
			/**
			 * wrap Метод обертывания файлового дескриптора для сервера
			 * @param target контекст назначения
			 * @param socket объект подключения
			 * @return       объект SSL контекста
			 */
			void wrap(ctx_t & target, sock_t * socket) noexcept;
			/**
			 * wrap Метод обертывания файлового дескриптора для клиента
			 * @param target контекст назначения
			 * @param socket объект подключения
			 * @param source исходный контекст
			 * @return       объект SSL контекста
			 */
			void wrap(ctx_t & target, sock_t * socket, const ctx_t & source) noexcept;
		public:
			/**
			 * wrap Метод обертывания файлового дескриптора для сервера
			 * @param target контекст назначения
			 * @param socket объект подключения
			 * @param mode   флаг выполнения обертывания файлового дескриптора
			 * @return       объект SSL контекста
			 */
			void wrap(ctx_t & target, sock_t * socket, const bool mode) noexcept;
			/**
			 * wrap Метод обертывания файлового дескриптора для клиента
			 * @param target контекст назначения
			 * @param socket объект подключения
			 * @param url    параметры URL адреса для инициализации
			 * @return       объект SSL контекста
			 */
			void wrap(ctx_t & target, sock_t * socket, const uri_t::url_t & url) noexcept;
		public:
			/**
			 * setVerify Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
			 * @param mode флаг состояния разрешения проверки
			 */
			void setVerify(const bool mode) noexcept;
			/**
			 * setCipher Метод установки алгоритмов шифрования
			 * @param cipher список алгоритмов шифрования для установки
			 */
			void setCipher(const vector <string> & cipher) noexcept;
			/**
			 * setTrusted Метод установки доверенного сертификата (CA-файла)
			 * @param trusted адрес доверенного сертификата (CA-файла)
			 * @param path    адрес каталога где находится сертификат (CA-файл)
			 */
			void setTrusted(const string & trusted, const string & path = "") noexcept;
			/**
			 * setCert Метод установки файлов сертификата
			 * @param chain файл цепочки сертификатов
			 * @param key   приватный ключ сертификата (если требуется)
			 */
			void setCertificate(const string & chain, const string & key = "") noexcept;
		public:
			/**
			 * Actuator Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			Actuator(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept;
			/**
			 * ~Actuator Деструктор
			 */
			~Actuator() noexcept;
	} act_t;
};

#endif // __AWH_ACTUATOR__
