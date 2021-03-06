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

/*
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
	#define ASN1_STRING_get0_data ASN1_STRING_data
#endif
*/

/**
 * Стандартная библиотека
 */
#include <cstdio>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <event2/bufferevent_ssl.h>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
	#include <ws2tcpip.h>
//	#define snprintf _snprintf
//	#define strcasecmp _stricmp
// Если - это Unix
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
				Verify(const string & host = "", const ASSL * ssl = nullptr) : host(host), ssl(ssl) {}
			} verify_t;
		public:
			/**
			 * Context Структура контекста
			 */
			typedef struct Context {
				bool mode;         // Флаг инициализации
				SSL * ssl;         // Параметры SSL
				SSL_CTX * ctx;     // Контекст SSL
				verify_t * verify; // Параметры валидации домена
				/**
				 * Context Конструктор
				 */
				Context() : mode(false), ssl(nullptr), ctx(nullptr), verify(nullptr) {}
			} ctx_t;
			/**
			 * Типы ошибок валидации
			 */
			enum class validate_t : uint8_t {
				Error,
				MatchFound,
				NoSANPresent,
				MatchNotFound,
				MalformedCertificate
			};
		private:
			// Флаг проверки сертификата доменного имени
			bool verify = true;
		private:
			// Приватный ключ сертификата
			string key = "";
			// Корневой сертификат
			string cert = "";
			// Файл цепочки сертификатов
			string chain = "";
			// Каталог с CA-файлами
			string capath = "";
			// Название CA-файла
			string cafile = SSL_CA_FILE;
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
		public:
			/**
			 * validateHostname Метод проверки доменного имени
			 * @param host доменное имя
			 * @param cert сертификат
			 * @return     результат проверки
			 */
			const validate_t validateHostname(const string & host, const X509 * cert = nullptr) const noexcept;
		public:
			/**
			 * clear Метод очистки контекста
			 * @param ctx контекст для очистки
			 */
			void clear(ctx_t & ctx) const noexcept;
		public:
			/**
			 * init Метод инициализации контекста для сервера
			 * @return объект SSL контекста
			 */
			ctx_t init() noexcept;
			/**
			 * init Метод инициализации контекста для сервера
			 * @param url Параметры URL адреса для инициализации
			 * @return    объект SSL контекста
			 */
			ctx_t init(const uri_t::url_t & url) noexcept;
		public:
			/**
			 * setVerify Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
			 * @param mode флаг состояния разрешения проверки
			 */
			void setVerify(const bool mode) noexcept;
			/**
			 * setCA Метод установки CA-файла корневого SSL сертификата
			 * @param cafile адрес CA-файла
			 * @param capath адрес каталога где находится CA-файл
			 */
			void setCA(const string & cafile, const string & capath = "") noexcept;
			/**
			 * setCert Метод установки файлов сертификата
			 * @param cert  корневой сертификат
			 * @param key   приватный ключ сертификата
			 * @param chain файл цепочки сертификатов
			 */
			void setCert(const string & cert, const string & key, const string & chain = "") noexcept;
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
