/**
 * @file: parser.hpp
 * @date: 2026-07-18
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

#ifndef __AWH_HTTP_PARSER__
#define __AWH_HTTP_PARSER__

/**
 * Стандартные заголовочные файлы
 */
#include <cstddef>
#include <cstdint>

/**
 * Подключаем наши заголовочные файлы
 */
#include "../http.hpp"
#include "../headers.hpp"
#include "../../../sys/buffer.hpp"
#include "../../../sys/global.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён HTTP-протокола
	 *
	 */
	namespace http {
        /**
		 * @brief Класс HTTP-парсера
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Parser {
			public:
				/**
				 * @brief Максимальная длина строки заголовка чанка (size + chunk-ext)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t MAX_CHUNK_LINE = (16 * 1024);
				/**
				 * @brief Максимальная длина имени заголовка
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t MAX_HEADER_NAME = (1 * 1024);
				/**
				 * @brief Максимальная длина request-line/status-line
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t MAX_REQUEST_LINE = (8 * 1024);
				/**
				 * @brief Максимальная длина значения заголовка
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t MAX_HEADER_VALUE = (16 * 1024);
				/**
				 * @brief Максимальное число заголовков
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t MAX_HEADER_COUNT = (128);
				/**
				 * @brief Суммарный размер всех заголовков
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t MAX_HEADERS_TOTAL = (64 * 1024);
				/**
				 * @brief Верхняя граница предвыделения body по Content-Length
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t MAX_BODY_PREALLOC = (256 * 1024);
				/**
				 * @brief Максимальный размер тела
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr uint64_t MAX_BODY_SIZE = (64ull * 1024 * 1024);
				/**
				 * @brief Максимальный размер одного чанка
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr uint64_t MAX_CHUNK_SIZE = (1ull * 1024 * 1024 * 1024);
			public:
				/**
				 * @brief Результат итогового статуса разбора HTTP-сообщения
				 *
				 */
				enum class status_t : uint8_t {
					NONE	 = 0x00, // Статус не определён
					OK       = 0x01, // Данные приняты, но сообщение ещё не завершено — нужно ещё байтов
					ERROR    = 0x02, // Ошибка разбора/безопасности
					COMPLETE = 0x03  // Одно сообщение полностью разобрано (в буфере могут идти следующие)
				};
				/**
				 * @brief Код ошибки разбора HTTP-парсера
				 *
				 */
				enum class error_t : uint16_t {
					NONE                      = 0x00, // Ошибок нет
					INTERNAL                  = 0x01, // Внутренняя ошибка состояния
					INVALID_EOL               = 0x02, // Ожидался LF после CR
					INVALID_METHOD            = 0x03, // Недопустимый символ в методе
					INVALID_TARGET            = 0x04, // Недопустимый символ в request-target
					INVALID_STATUS            = 0x05, // Неверный статус-код ответа
					INVALID_VERSION           = 0x06, // Неверная строка версии (HTTP/x.y)
					INVALID_CONSTANT          = 0x07, // Ожидался литеральный символ (например, в "HTTP/")
					INVALID_CHUNK_SIZE        = 0x08, // Неверный размер чанка
					INVALID_HEADER_TOKEN      = 0x09, // Недопустимый символ в имени заголовка / obs-fold
					INVALID_HEADER_VALUE      = 0x0A, // Недопустимый символ в значении заголовка
					INVALID_CONTENT_LENGTH    = 0x0B, // Content-Length не число / Некорректен
					INVALID_CHUNK_TERMINATOR  = 0x0C, // Нет CRLF после данных чанка
					INVALID_TRANSFER_ENCODING = 0x0D, // Некорректный Transfer-Encoding (chunked не последний и т.п.)
					ABORTED                   = 0x0E, // Разбор прерван пользовательским callback'ом
					URL_OVERFLOW              = 0x0F, // Превышен лимит длины request-line
					BODY_OVERFLOW             = 0x10, // Превышен лимит размера тела
					CHUNK_OVERFLOW            = 0x11, // Превышен лимит размера чанка
					HEADER_OVERFLOW           = 0x12, // Превышен лимит размера заголовков
					TOO_MANY_HEADERS          = 0x13, // Превышено число заголовков
					CONTENT_LENGTH_CONFLICT   = 0x14  // CL+TE или несколько разных Content-Length (request smuggling)
				};
			public:
				/**
				 * @brief Структура флагов состояния парсера
				 *
				 */
				typedef struct Flags {
					// Тело передаётся chunked
					bool chunked;
					// Сообщение полностью разобрано
					bool complete;
					// Соединение переиспользуемое
					bool keepAlive;
					// В сообщении есть заголовок Content-Length
					bool hasContentLength;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Flags() noexcept : 
					 chunked(false), complete(false),
					 keepAlive(true), hasContentLength(false) {}
				} flags_t;
				/**
				 * @brief Структура поддерживаемых версий протокола HTTP
				 *
				 */
				typedef struct Version {
					// Старшая версия протокола HTTP
					uint8_t major;
					// Младшая версия протокола HTTP
					uint8_t minor;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Version() noexcept = default;
					/**
					 * @brief Конструктор
					 *
					 * @param major Старшая версия HTTP
					 * @param minor Младшая версия HTTP
					 */
					explicit Version(uint8_t major, uint8_t minor) noexcept :
					 major(major), minor(minor) {}
				} version_t;
			public:
				/**
				 * @brief Структура тела HTTP-сообщения
				 *
				 */
				typedef struct Body {
					// Размер тела сообщения
					uint64_t size;
					// Буфер для хранения данных
					buffer_t body;
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					explicit Body(const fmk_t * fmk, const log_t * log) noexcept :
					 size(0), body(fmk, log) {}
				} body_t;
			public:
				/**
				 * @brief Структура ограничений безопасности
				 *
				 */
				typedef struct Limits {
					// Максимальная длина строки заголовка чанка (size + chunk-ext)
					size_t maxChunkLine;
					// Максимальная длина имени заголовка
					size_t maxHeaderName;
					// Максимальная длина request-line/status-line
					size_t maxRequestLine;
					// Максимальная длина значения заголовка
					size_t maxHeaderValue;
					// Максимальное число заголовков
					size_t maxHeaderCount;
					// Суммарный размер всех заголовков
					size_t maxHeadersTotal;
					// Верхняя граница предвыделения body по Content-Length
					size_t maxBodyPrealloc;
					// Максимальный размер тела
					uint64_t maxBodySize;
					// Максимальный размер одного чанка
					uint64_t maxChunkSize;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Limits() noexcept :
					 maxChunkLine(MAX_CHUNK_LINE),
					 maxHeaderName(MAX_HEADER_NAME),
					 maxRequestLine(MAX_REQUEST_LINE),
					 maxHeaderValue(MAX_HEADER_VALUE),
					 maxHeaderCount(MAX_HEADER_COUNT),
					 maxHeadersTotal(MAX_HEADERS_TOTAL),
					 maxBodyPrealloc(MAX_BODY_PREALLOC),
					 maxBodySize(MAX_BODY_SIZE),
					 maxChunkSize(MAX_CHUNK_SIZE) {}
				} limits_t;
			public:
				/**
				 * @brief Класс разобранного сообщения
				 * 
				 * @note Заполняется по мере парсинга
				 */
				typedef class __AWH_SHARED_EXPORT__ Message {
					public:
						// Флаги состояния парсера
						flags_t flags;
						// Буфер для хранения данных
						body_t body;
						// Контейнер заголовков
						headers_t headers;
						// Контейнер трейлеров
						headers_t trailers;
						// Версия протокола HTTP
						version_t version;
					public:
						/**
						 * @brief Оператор перемещающего присваивания параметров сообщения
						 *
						 * @param message объект сообщения для перемещения
						 * @return        текущее сообщение
						 */
						Message & operator = (Message && message) noexcept;
						/**
						 * @brief Оператор присваивания параметров сообщения
						 *
						 * @param message объект сообщения для копирования
						 * @return        текущее сообщение
						 */
						Message & operator = (const Message & message) noexcept;
					public:
						/**
						 * @brief Оператор сравнения
						 *
						 * @param message объект сообщения для сравнения
						 * @return        результат сравнения
						 */
						bool operator == (const Message & message) noexcept;
					public:
						/**
						 * @brief Конструктор перемещения
						 *
						 * @param message объект сообщения для перемещения
						 */
						Message(Message && message) noexcept;
						/**
						 * @brief Конструктор копирования
						 *
						 * @param message объект сообщения для копирования
						 */
						Message(const Message & message) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						explicit Message(const fmk_t * fmk, const log_t * log) noexcept;
				} message_t;
			protected:
				// Код ошибки разбора
				error_t _error;
			protected:
				// Направление потока данных
				direct_t _direct;
			protected:
				// Настраиваемые лимиты
				limits_t _limits;
				// Результат разбора
				message_t _message;
			protected:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод очистки всех данных парсера
				 *
				 */
				void clear() noexcept;
			public:
				/**
				 * @brief Метод получения кода ошибки разбора
				 *
				 * @return код ошибки
				 */
				error_t error() const noexcept;
			public:
				/**
				 * @brief Метод получения направления потока данных
				 *
				 * @return направление потока данных
				 */
				direct_t direct() const noexcept;
			public:
				/**
				 * @brief Метод получения лимитов безопасности
				 *
				 * @return лимиты безопасности
				 */
				const limits_t & limits() const noexcept;
				/**
				 * @brief Метод установки лимитов безопасности
				 *
				 * @param limits лимиты безопасности
				 */
				void limits(const limits_t & limits) noexcept;
			public:
				/**
				 * @brief Метод получения разобранного сообщения
				 *
				 * @return разобранное сообщение
				 */
				const message_t & message() noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param direct направление потока данных
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 */
				explicit Parser(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				virtual ~Parser() noexcept;
        } parser_t;
    };
};

#endif // __AWH_HTTP_PARSER__
