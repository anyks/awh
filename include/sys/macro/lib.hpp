/**
 * @file lib.hpp
 * @date 2025-10-25
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл конфигурации библиотеки — версия, название и контактные данные проекта,
 *        локаль по умолчанию для каждой операционной системы и настройки режима отладки,
 *        включая выбор безопасного приведения типов
 *
 * \~english
 * @brief Header file of the configuration of the library — the version, the name and the contact data of the project,
 *        the default locale for every operating system and the settings of the debug mode,
 *        including the choice of the safe type cast
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Если название библиотеки не определено
 */
#if !defined(AWH_NAME)
	/**
	 * @brief Устанавливаем название библиотеки
	 *
	 */
	#define AWH_NAME "ANYKS-WEB"
#endif
/**
 * Если короткое название библиотеки не определено
 */
#if !defined(AWH_SHORT_NAME)
	/**
	 * @brief Устанавливаем короткое название библиотеки
	 *
	 */
	#define AWH_SHORT_NAME "AWH"
#endif
/**
 * Если версия библиотеки не определена
 */
#if !defined(AWH_VERSION)
	/**
	 * @brief Устанавливаем версию библиотеки
	 *
	 */
	#define AWH_VERSION "5.0.0"
#endif
/**
 * Если хост библиотеки не определён
 */
#if !defined(AWH_HOST)
	/**
	 * @brief Устанавливаем хост библиотеки
	 *
	 */
	#define AWH_HOST "anyks.com"
#endif
/**
 * Если адрес сайта библиотеки не определён
 */
#if !defined(AWH_SITE)
	/**
	 * @brief Устанавливаем адрес сайта библиотеки
	 *
	 */
	#define AWH_SITE "https://anyks.com"
#endif
/**
 * Если адрес электронной почты библиотеки не определён
 */
#if !defined(AWH_EMAIL)
	/**
	 * @brief Устанавливаем адрес электронной почты библиотеки
	 *
	 */
	#define AWH_EMAIL "info@anyks.com"
#endif
/**
 * Если контактный адрес библиотеки не определён
 */
#if !defined(AWH_CONTACT)
	/**
	 * @brief Устанавливаем контактный адрес библиотеки
	 *
	 */
	#define AWH_CONTACT "https://t.me/forman"
#endif
/**
 * Если локаль библиотеки не определена
 */
#if !defined(AWH_LOCALE)
	/**
	 * Для операционной системы MS Windows
	 */
	#if defined(_WIN32) || defined(_WIN64)
		/**
		 * @brief Устанавливаем локаль словаря по умолчанию
		 *
		 */
		#define AWH_LOCALE "C"
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		/**
		 * @brief Устанавливаем локаль словаря по умолчанию
		 *
		 */
		#define AWH_LOCALE "en_US.UTF-8"
	#endif
#endif

/**
 * Параметры библиотеки
 */

// Временная зона сервера
#define TIME_ZONE "UTC"
// Формат даты и времени
#define DATE_FORMAT "%m/%d/%Y %H:%M:%S"
// Интервал времени для получения данных с сервера в секундах
#define READ_TIMEOUT 60
// Интервал времени для записи данных на сервере в секундах
#define WRITE_TIMEOUT 1
// Интервал времени для подключение на сервере
#define CONNECT_TIMEOUT 5
// Интервал времени на проверку подключения в милисекундах
#define PING_INTERVAL 120000
// Максимальное количество потоков сервера
#define MAX_COUNT_THREADS 1000
// Время жизни ключа сессии DIGEST авторизации в милисекундах
#define DIGEST_ALIVE_NONCE 1800000
// Максимальный размер файла логов в байтах
#define MAX_SIZE_LOGFILE 0xFA000

/**
 * Размеры буферов данных
 */

// Максимальный размер передаваемого буфера полезной нагрузки
#define AWH_PAYLOAD_SIZE 0xFFFFF
// Максимальный размер окна всех хранимых данных полезной нагрузки
#define AWH_WINDOW_SIZE 0x7FFFFFFF
// Размер чанка буфера для чтения из файла
#define AWH_CHUNK_SIZE 0x8000
// Размер бинарных данных
#define AWH_DATA_SIZE 0x10000
// Размер бинарного буфера
#define AWH_BUFFER_SIZE 0xFA000
// Размер буфера на чтение
#define AWH_BUFFER_SIZE_RCV 0x8000
// Размер буфера на запись
#define AWH_BUFFER_SIZE_SND 0x8000

/**
 * Если максимальный размер HTTP-тела запроса/ответа установлено
 */
#if !defined(AWH_MAX_BODY_SIZE)
	/**
	 * Устанавливаем максимальный размер HTTP-тела 10Mb
	 */
	#define AWH_MAX_BODY_SIZE 0xA00000
#endif

/**
 * HTTP заголовки по умолчанию
 */

// Заголовок Accept-Language по умолчанию
#define HTTP_HEADER_ACCEPTLANGUAGE "*"
// Заголовок Connection по умолчанию
#define HTTP_HEADER_CONNECTION "keep-alive"
// Заголовок Content-Type по умолчанию
#define HTTP_HEADER_CONTENTTYPE "text/html"
// Заголовок User-Agent по умолчанию
#define HTTP_HEADER_AGENT "Mozilla/5.0 (Macintosh; Intel Mac OS X 11_1_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.192 Safari/537.36"
// Заголовок Accept по умолчанию
#define HTTP_HEADER_ACCEPT "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9"

/**
 * Параметры сервера
 */

// Порт сервера по умолчанию
#define SERVER_PORT 80
// Порт сервера защищённый
#define SERVER_SEC_PORT 443
// Порт прокси-сервера по умолчанию
#define SERVER_PROXY_PORT 3128
// Порт прокси-сервера защищённый
#define SERVER_PROXY_SEC_PORT 3129
// Хост сервера по умолчанию
#define SERVER_HOST "127.0.0.1"
// Максимальное количество запросов к серверу на одно подключение
#define SERVER_MAX_REQUESTS 0
// Количество максимальных подключений к серверу
#define SERVER_TOTAL_CONNECT 1000

/**
 * Файловые пути хранения SSL CA-файла
 */

/**
 * Для операционной системы MS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "%ProgramFiles%\\OpenSSL-Win64\\bin\\PEM\\cert.pem"
/**
 * Для операционной системы macOS
 */
#elif defined(__APPLE__) || defined(__MACH__)
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/usr/local/etc/ca-certificates/cert.pem"
/**
 * Для операционной системы Linux
 */
#elif defined(__linux__)
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/etc/ssl/certs/certSIGN_ROOT_CA.pem"
/**
 * Для операционной системы FreeBSD
 */
#elif defined(__FreeBSD__)
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/usr/local/openssl/cert.pem"
/**
 * Для операционной системы OpenBSD
 */
#elif defined(__OpenBSD__)
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/usr/local/openssl/cert.pem"
/**
 * Для операционной системы NetBSD
 */
#elif defined(__NetBSD__)
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/usr/src/crypto/external/bsd/openssl/dist/apps/cert.pem"
/**
 * Для операционной системы Sun Solaris
 */
#elif defined(__sun__)
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/etc/certs/CA/certSIGN_ROOT_CA.pem"
/**
 * Для всех остальных Unix-подобных операционных систем
 */
#elif defined(__unix) || defined(__unix__)
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE ""
/**
 * Для всех остальных операционных систем
 */
#else
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE ""
#endif
