/**
 * @file: ssl.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_SSL__
#define __AWH_SSL__

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
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

/**
 * Наши модули
 */
#include <sys/fs.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
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
	 * ASSL Класс для работы с зашифрованными данными
	 */
	typedef class ASSL {
		private:
			/**
			 * Тип активного приложения
			 */
			enum class type_t : uint8_t {NONE, CLIENT, SERVER};
		private:
			/**
			 * Verify Структура параметров для валидации доменов
			 */
			typedef struct Verify {
				string host;      // Хост для валидации
				const ASSL * ssl; // Объект для работы с SSL
				/**
				 * Verify Конструктор
				 * @param ssl  основной родительский объект
				 * @param host хост для которого производится проверка
				 */
				Verify(const string & host = "", const ASSL * ssl = nullptr) noexcept : host(host), ssl(ssl) {}
			} verify_t;
		public:
			/**
			 * Context Класс контекста
			 */
			typedef class Context {
				private:
					/**
					 *ASSL Устанавливаем дружбу с родительским объектом
					 */
					friend class ASSL;
				private:
					// Файловый дескриптор (сокет)
					int fd;
					// Флаг инициализации
					bool mode;
				private:
					// Тип активного приложения
					type_t type;
				private:
					// Объект для работы с сокетами
					socket_t socket;
				private:
					BIO * bio;     // Объект BIO
					SSL * ssl;     // Объект SSL
					SSL_CTX * ctx; // Контекст SSL
				private:
					// Параметры валидации домена
					verify_t * verify;
				private:
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
					 * get Метод получения файлового дескриптора
					 * @return файловый дескриптор
					 */
					int get() const noexcept;
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
					int64_t read(char * buffer, const size_t size) const noexcept;
					/**
					 * write Метод записи данных в сокет
					 * @param buffer буфер данных для записи
					 * @param size   размер буфера данных
					 * @return       количество записанных байт
					 */
					int64_t write(const char * buffer, const size_t size) const noexcept;
				public:
					/**
					 * blocking Метод установки блокирующего сокета
					 * @return результат работы функции
					 */
					int blocking() noexcept;
					/**
					 * nonBlocking Метод установки неблокирующего сокета
					 * @return результат работы функции
					 */
					int nonBlocking() noexcept;
					/**
					 * isBlocking Метод проверки сокета блокирующий режим
					 * @return результат работы функции
					 */
					int isBlocking() const noexcept;
				public:
					/**
					 * bufferSize Метод установки размеров буфера
					 * @param read  размер буфера на чтение
					 * @param write размер буфера на запись
					 * @param total максимальное количество подключений
					 * @return      результат работы функции
					 */
					int bufferSize(const int read = 0, const int write = 0, const u_int total = 0) const noexcept;
				public:
					/**
					 * Context Конструктор
					 * @param log объект для работы с логами
					 */
					Context(const log_t * log) noexcept :
					 fd(-1), mode(false), type(type_t::NONE),
					 socket(log), bio(nullptr), ssl(nullptr),
					 ctx(nullptr), verify(nullptr), log(log) {}
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
			 * verifyHost Функция обратного вызова для проверки валидности сертификата
			 * @param x509 данные сертификата
			 * @param ctx  передаваемый контекст
			 * @return     результат проверки
			 */
			static int verifyHost(X509_STORE_CTX * x509 = nullptr, void * ctx = nullptr) noexcept;
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
			 * clear Метод очистки контекста
			 * @param ctx контекст для очистки
			 */
			void clear(ctx_t & ctx) const noexcept;
		public:
			/**
			 * wrap Метод обертывания файлового дескриптора для сервера
			 * @param ctx контекст для очистки
			 * @return    объект SSL контекста
			 */
			ctx_t wrap(ctx_t & ctx) noexcept;
			/**
			 * wrap Метод обертывания файлового дескриптора для клиента
			 * @param ctx контекст для очистки
			 * @param url Параметры URL адреса для инициализации
			 * @return    объект SSL контекста
			 */
			ctx_t wrap(ctx_t & ctx, const uri_t::url_t & url) noexcept;
		public:
			/**
			 * wrap Метод обертывания файлового дескриптора для сервера
			 * @param fd   файловый дескриптор (сокет)
			 * @param mode флаг выполнения обертывания файлового дескриптора
			 * @return     объект SSL контекста
			 */
			ctx_t wrap(const int fd, const bool mode = true) noexcept;
			/**
			 * wrap Метод обертывания файлового дескриптора для клиента
			 * @param fd  файловый дескриптор (сокет)
			 * @param url Параметры URL адреса для инициализации
			 * @return    объект SSL контекста
			 */
			ctx_t wrap(const int fd, const uri_t::url_t & url) noexcept;
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
			 * ASSL Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			ASSL(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept;
			/**
			 * ~ASSL Деструктор
			 */
			~ASSL() noexcept;
	} ssl_t;
};

#endif // __AWH_SSL__
