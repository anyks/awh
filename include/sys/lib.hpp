/**
 * @file: lib.hpp
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

#ifndef __AWH_CONFIG__
#define __AWH_CONFIG__

// Версия приложения
#define AWH_VERSION "4.3.1"
// Короткое название библиотеки
#define AWH_SHORT_NAME "AWH"
// Название библиотеки
#define AWH_NAME "ANYKS-WEB"
// Основной хост системы
#define AWH_HOST "anyks.com"
// Адрес сайта автора
#define AWH_SITE "https://anyks.com"
// Адрес электронной почты
#define AWH_EMAIL "info@anyks.com"
// Телеграм-контакт
#define AWH_CONTACT "https://t.me/forman"

/**
 * Определяем сепаратор
 */
#if defined(_WIN32) || defined(_WIN64)
	// Локаль словаря по умолчанию
	#define AWH_LOCALE "C"
/**
 * Для всех остальных операционных систем
 */
#else
	// Локаль словаря по умолчанию
	#define AWH_LOCALE "en_US.UTF-8"
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
#define AWH_BUFFER_SIZE_RCV 0x10624D
// Размер буфера на запись
#define AWH_BUFFER_SIZE_SND 0x10624D

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
 * Параметры сети
 */

// Версия HTTP протокола
#define HTTP_VERSION 1.1
// Адреса серверов IPv4 DNS-резолвера
#define IPV4_RESOLVER {"8.8.8.8", "8.8.4.4"}
// Адреса серверов IPv6 DNS-резолвера
#define IPV6_RESOLVER {"[2001:4860:4860::8888]", "[2001:4860:4860::8844]"}

/**
 * Адреса NTP серверов
 */

// Адреса серверов IPv4 NTP-клиента
#define IPV4_NTP { \
	"ntp0.ntp-servers.net", \
	"ntp1.ntp-servers.net", \
	"ntp2.ntp-servers.net", \
	"ntp3.ntp-servers.net", \
	"ntp4.ntp-servers.net", \
	"ntp5.ntp-servers.net", \
	"ntp6.ntp-servers.net", \
	"ntp7.ntp-servers.net", \
	"pool.ntp.org", \
	"0.pool.ntp.org", \
	"1.pool.ntp.org", \
	"2.pool.ntp.org", \
	"3.pool.ntp.org" \
}
// Адреса серверов IPv6 NTP-клиента
#define IPV6_NTP { \
	"ntp0.ntp-servers.net", \
	"ntp2.ntp-servers.net", \
	"ntp3.ntp-servers.net", \
	"ntp4.ntp-servers.net", \
	"ntp6.ntp-servers.net", \
	"2.pool.ntp.org" \
}

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
 * Если операционной системой является Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "%ProgramFiles%\\OpenSSL-Win64\\bin\\PEM\\cert.pem"
/**
 * Если операционной системой является MacOS X
 */
#elif __APPLE__ || __MACH__
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/usr/local/etc/openssl/cert.pem"
/**
 * Если операционной системой является Linux
 */
#elif __linux__
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/etc/ssl/certs/certSIGN_ROOT_CA.pem"
/**
 * Если операционной системой является FreeBSD
 */
#elif __FreeBSD__
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/usr/local/openssl/cert.pem"
/**
 * Для всех остальных Unix-подобных операционных систем
 */
#elif __unix || __unix__
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE ""
/**
 * Для всех остальных операционных систем
 */
#else
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE ""
#endif

#endif // __AWH_CONFIG__
