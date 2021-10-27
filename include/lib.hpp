/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Версия приложения
#define AWH_VERSION "1.3.5"
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
// Для всех остальных OS
#else
	// Локаль словаря по умолчанию
	#define AWH_LOCALE "en_US.UTF-8"
#endif

/** ПАРАМЕТРЫ ПРОЧИЕ **/
// Временная зона сервера
#define TIME_ZONE "UTC"
// Формат даты и времени
#define DATE_FORMAT "%m/%d/%Y %H:%M:%S"
// Интервал времени для получения данных с сервера в секундах
#define READ_TIMEOUT 30
// Интервал времени для записи данных на сервере в секундах
#define WRITE_TIMEOUT 0
// Размер бинарного буфера
#define BUFFER_SIZE 0xFA000
// Размер чанка буфера для чтения из файла
#define BUFFER_CHUNK 0x8000
// Максимальное время подключения
#define MAX_TIME_CONNECT 60000
// Интервал времени на проверку подключения с сервером в милисекундах
#define PERSIST_INTERVAL 120000
// Интервал времени на получение сообщений с сервера в милисекундах
#define CONNECT_TIMEOUT 1800000
// Максимальный размер файла логов в байтах
#define MAX_SIZE_LOGFILE 0xFA000
// Размер буфера на чтение
#define BUFFER_SIZE_RCV 0x40000000
// Размер буфера на запись
#define BUFFER_SIZE_SND 0x40000000

/**
 * HTTP заголовки по умолчанию
 */
// Заголовок Accept-Language по умолчанию
#define HTTP_HEADER_ACCEPTLANGUAGE "*"
// Заголовок Connection по умолчанию
#define HTTP_HEADER_CONNECTION "keep-alive"
// Заголовок Content-Type по умолчанию
#define HTTP_HEADER_CONTENTTYPE "text/html"
// Заголовок Accept-Encoding по умолчанию
#define HTTP_HEADER_ACCEPTENCODING "gzip, deflate, br"
// Заголовок User-Agent по умолчанию
#define HTTP_HEADER_AGENT "Mozilla/5.0 (Macintosh; Intel Mac OS X 11_1_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.192 Safari/537.36"
// Заголовок Accept по умолчанию
#define HTTP_HEADER_ACCEPT "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9"

/** ПАРАМЕТРЫ СЕТИ **/
// Версия HTTP протокола
#define HTTP_VERSION 1.1
// Адреса серверов DNS резолвера
#define IPV4_RESOLVER {"8.8.8.8", "8.8.4.4"}
#define IPV6_RESOLVER {"2001:4860:4860::8888", "2001:4860:4860::8844"}

/** ПАРАМЕТРЫ СЕРВЕРА */
// Порт сервера по умолчанию
#define SERVER_PORT 8080
// Хост сервера по умолчанию
#define SERVER_HOST "127.0.0.1"
// Максимальное количество запросов к серверу на одно подключение
#define SERVER_MAX_REQUESTS 100
// Количество максимальных подключений к серверу
#define SERVER_TOTAL_CONNECT 100

/**
 * Файловые пути CA-файла SSL сертификата
 */
// Если операционной системой является Windows
#if defined(_WIN32) || defined(_WIN64)
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "%ProgramFiles%\\OpenSSL-Win64\\bin\\PEM\\cert.pem"
// Если операционной системой является MacOS X
#elif __APPLE__ || __MACH__
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/usr/local/etc/openssl/cert.pem"
// Если операционной системой является Linux
#elif __linux__
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/etc/ssl/certs/certSIGN_ROOT_CA.pem"
// Если операционной системой является FreeBSD
#elif __FreeBSD__
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE "/usr/local/openssl/cert.pem"
// Для всех остальных Unix-подобных операционных систем
#elif __unix || __unix__
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE ""
// Для всех остальных операционных систем
#else
	// Адрес файла корневого сертификата
	#define SSL_CA_FILE ""
#endif
