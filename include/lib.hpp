/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

// Версия приложения
#define AWH_VERSION "1.0.0"
// Короткое название библиотеки
#define AWH_SHORT_NAME "AWH"
// Название библиотеки
#define AWH_NAME "ANYKS - WebSocket/HTTP"
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

/** ПАРАМЕТРЫ СЛОВ **/
// Максимальная длина последовательности в символах
#define MAX_SEQ_LENGTH 50
// Максимальная длина слова
#define MAX_WORD_LENGTH 35
// Максимальная длина строки 103kb
#define MAX_STRING_BYTES 0x19C00

/** ПАРАМЕТРЫ ПРОЧИЕ **/
// Временная зона сервера
#define TIME_ZONE "UTC"
// Интервал времени для получения данных с сервера в секундах
#define READ_TIMEOUT 300
// Интервал времени на контроль подключения с сервером в милисекундах
#define CONNECT_TIMEOUT 1800000
// Размер чанка буфера для чтения из файла
#define BUFFER_CHUNK 0x19C00
// Размер бинарного буфера
#define BUFFER_SIZE 0x6400000
// User-Agent для REST запросов
#define USER_AGENT "Mozilla/5.0 (Macintosh; Intel Mac OS X 11_1_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.192 Safari/537.36"

/** ПАРАМЕТРЫ СЕТИ **/
// Версия HTTP протокола
#define HTTP_VERSION 1.1
// Адреса серверов DNS резолвера
#define IPV4_RESOLVER {"8.8.8.8", "8.8.4.4"}
#define IPV6_RESOLVER {"2001:4860:4860::8888", "2001:4860:4860::8844"}

/**
 * Файловые пути бота
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
