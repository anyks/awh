/**
 * @file http.hpp
 * @brief Публичный API высокопроизводительного инкрементального парсера HTTP/1.1.
 *
 * Дизайн намеренно «процедурный»: никаких объектов с методами и наследованием.
 * Используются только namespace + свободные (static) функции и POD-подобные
 * структуры данных (struct без методов). Это упрощает встраивание парсера в
 * горячие сетевые циклы и делает поведение полностью предсказуемым.
 *
 * Сборка: см. подробный комментарий в начале файла http.cpp.
 */

#ifndef AWH_EXPERIENCE_HTTP_HPP
#define AWH_EXPERIENCE_HTTP_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace awh {
	namespace http {
		/**
		 * @brief Тип разбираемого сообщения.
		 */
		enum class Type : uint8_t {
			REQUEST,  // HTTP-запрос  (request-line: METHOD SP target SP HTTP/x.y)
			RESPONSE  // HTTP-ответ   (status-line: HTTP/x.y SP code SP reason)
		};

		/**
		 * @brief Распознанный HTTP-метод (для удобства; полное имя в Message::methodName).
		 *
		 * Помимо основных методов HTTP/1.1 распознаются методы WebDAV (RFC 4918)
		 * и ряд расширений (RFC 5789, UPnP, ICAL и др.). Любой нераспознанный, но
		 * синтаксически корректный метод даёт UNKNOWN, при этом Message::methodName
		 * всегда содержит оригинальную строку.
		 */
		enum class Method : uint8_t {
			UNKNOWN,
			// основные (RFC 7231) + PATCH (RFC 5789)
			GET, HEAD, POST, PUT, DELETE_, CONNECT, OPTIONS, TRACE, PATCH,
			// WebDAV (RFC 4918) и расширения версионирования (RFC 3253)
			COPY, LOCK, MKCOL, MOVE, PROPFIND, PROPPATCH, SEARCH, UNLOCK,
			BIND, REBIND, UNBIND, ACL, REPORT, MKACTIVITY, CHECKOUT, MERGE,
			// прочие распространённые расширения
			MSEARCH, NOTIFY, SUBSCRIBE, UNSUBSCRIBE, PURGE, MKCALENDAR,
			LINK, UNLINK, PRI, SOURCE
		};

		/**
		 * @brief Результат вызова execute().
		 */
		enum class Status : uint8_t {
			OK,        // данные приняты, но сообщение ещё не завершено — нужно ещё байтов
			COMPLETE,  // одно сообщение полностью разобрано (в буфере могут идти следующие)
			ERROR      // ошибка разбора/безопасности — подробности в Parser::error
		};

		/**
		 * @brief Код ошибки разбора.
		 */
		enum class Error : uint16_t {
			NONE = 0,
			INTERNAL,                 // внутренняя ошибка состояния
			INVALID_METHOD,           // недопустимый символ в методе
			INVALID_TARGET,           // недопустимый символ в request-target
			INVALID_VERSION,          // неверная строка версии (HTTP/x.y)
			INVALID_STATUS,           // неверный статус-код ответа
			INVALID_HEADER_TOKEN,     // недопустимый символ в имени заголовка / obs-fold
			INVALID_HEADER_VALUE,     // недопустимый символ в значении заголовка
			INVALID_CONTENT_LENGTH,   // Content-Length не число / некорректен
			INVALID_TRANSFER_ENCODING,// некорректный Transfer-Encoding (chunked не последний и т.п.)
			INVALID_CHUNK_SIZE,       // неверный размер чанка
			INVALID_CHUNK_TERMINATOR, // нет CRLF после данных чанка
			INVALID_EOL,              // ожидался LF после CR
			INVALID_CONSTANT,         // ожидался литеральный символ (например, в "HTTP/")
			CONTENT_LENGTH_CONFLICT,  // CL+TE или несколько разных Content-Length (request smuggling)
			HEADER_OVERFLOW,          // превышен лимит размера заголовков
			URL_OVERFLOW,             // превышен лимит длины request-line
			BODY_OVERFLOW,            // превышен лимит размера тела
			CHUNK_OVERFLOW,           // превышен лимит размера чанка
			TOO_MANY_HEADERS,         // превышено число заголовков
			ABORTED                   // разбор прерван пользовательским callback'ом
		};

		/**
		 * @brief Одна пара «заголовок: значение» (хранится как есть, значение уже без OWS).
		 */
		struct Header {
			std::string name;
			std::string value;
		};

		/**
		 * @brief Ограничения безопасности. Значения по умолчанию подобраны консервативно.
		 */
		struct Limits {
			size_t   maxRequestLine  = 8 * 1024;            // макс. длина request-line/status-line
			size_t   maxHeaderName   = 1 * 1024;            // макс. длина имени заголовка
			size_t   maxHeaderValue  = 16 * 1024;           // макс. длина значения заголовка
			size_t   maxHeaderCount  = 128;                 // макс. число заголовков
			size_t   maxHeadersTotal = 64 * 1024;           // суммарный размер всех заголовков
			uint64_t maxBodySize     = 64ull * 1024 * 1024; // макс. размер тела
			uint64_t maxChunkSize    = 1ull * 1024 * 1024 * 1024; // макс. размер одного чанка
		};

		/**
		 * @brief Разобранное сообщение. Заполняется по мере парсинга.
		 */
		struct Message {
			Type type = Type::REQUEST;

			// --- request-line ---
			Method      method = Method::UNKNOWN;
			std::string methodName;   // оригинальное имя метода
			std::string target;       // request-target как есть (origin/absolute/authority/asterisk)

			// --- status-line ---
			uint16_t    statusCode = 0;
			std::string reason;

			// --- версия протокола ---
			uint8_t versionMajor = 0;
			uint8_t versionMinor = 0;

			// --- заголовки и трейлеры ---
			std::vector<Header> headers;
			std::vector<Header> trailers;

			// --- тело ---
			std::string body;

			// --- семантика передачи ---
			bool     chunked         = false; // тело передаётся chunked
			bool     keepAlive       = true;  // соединение переиспользуемое
			bool     hasContentLength = false;
			uint64_t contentLength   = 0;
			bool     complete        = false; // сообщение полностью разобрано
		};

		struct Parser; // предварительное объявление для типов callback'ов

		// --- Типы callback'ов (указатели на функции => нулевые накладные расходы) ---
		// Все возвращают bool: true — продолжать разбор, false — прервать (Error::ABORTED).

		/// Событие без данных (начало/конец сообщения, конец заголовков и т.п.).
		using HookCb = bool (*)(Parser & p, void * user);
		/// Событие с непрерывным фрагментом данных (zero-copy: указатель во входной буфер!).
		using DataCb = bool (*)(Parser & p, void * user, const char * data, size_t len);
		/// Событие «поле: значение» (имя/значение валидны на время вызова).
		using FieldCb = bool (*)(Parser & p, void * user,
		                         const char * name, size_t nameLen,
		                         const char * value, size_t valueLen);
		/// Событие со статус-строкой ответа.
		using StatusCb = bool (*)(Parser & p, void * user,
		                          uint16_t code, const char * reason, size_t reasonLen);
		/// Событие с размером (заголовок чанка).
		using SizeCb = bool (*)(Parser & p, void * user, uint64_t size);

		/**
		 * @brief Набор callback'ов для потокового (streaming) разбора.
		 *
		 * Любой указатель может быть nullptr. Особенно полезен onBody: он отдаёт
		 * фрагменты тела как std::string_view прямо во входной буфер (zero-copy),
		 * что позволяет обрабатывать тела любого размера без буферизации.
		 * Указатели в callback'ах действительны ТОЛЬКО на время вызова.
		 */
		struct Handler {
			HookCb   onMessageBegin    = nullptr; // начало нового сообщения
			DataCb   onTarget          = nullptr; // request-target (для запросов)
			StatusCb onStatus          = nullptr; // статус-строка (для ответов)
			FieldCb  onHeader          = nullptr; // очередной заголовок
			HookCb   onHeadersComplete = nullptr; // заголовки закончились
			DataCb   onBody            = nullptr; // фрагмент тела (zero-copy)
			SizeCb   onChunkHeader     = nullptr; // заголовок очередного чанка
			HookCb   onChunkComplete   = nullptr; // чанк дочитан
			FieldCb  onTrailer         = nullptr; // очередной трейлер
			HookCb   onMessageComplete = nullptr; // сообщение полностью разобрано
		};

		/**
		 * @brief Состояние парсера. Поля после message — внутренние; снаружи не меняйте.
		 *
		 * Структура — обычный контейнер данных (никаких методов). Все операции
		 * выполняются свободными функциями ниже.
		 */
		struct Parser {
			Limits  limits;             // настраиваемые лимиты
			Message message;            // результат разбора
			Error   error = Error::NONE;

			// Установите true, если этот ответ соответствует запросу методом HEAD
			// (тогда тело не читается, даже при наличии Content-Length).
			bool responseToHead = false;

			// --- Потоковый режим (опционально) ---
			// Необязательный набор callback'ов. Если задан, парсер вызывает их по ходу
			// разбора. Указатель и userData сохраняются при reset() (но обнуляются init()).
			const Handler * handler = nullptr;
			void *          userData = nullptr;
			// Управление накоплением результата в Message:
			//   storeBody    — складывать тело в Message::body (выключите для чистого
			//                  zero-copy стриминга гигабайтных тел через onBody);
			//   storeHeaders — складывать заголовки/трейлеры в Message::headers/trailers.
			bool storeBody    = true;
			bool storeHeaders = true;

			// ----------------- внутреннее состояние (не трогать) -----------------
			uint16_t state = 0;
			Type     type  = Type::REQUEST;

			std::string curName;   // накопитель имени текущего заголовка
			std::string curValue;  // накопитель значения текущего заголовка

			size_t   headerCount      = 0;
			size_t   headersTotalBytes = 0;
			size_t   lineBytes        = 0;

			uint64_t bytesRemaining = 0; // остаток тела/чанка
			uint64_t chunkSize      = 0;
			uint32_t chunkDigits    = 0;

			bool     clSeen        = false;
			uint64_t clValue       = 0;
			bool     teSeen        = false;
			bool     teChunkedFinal = false;
			bool     teInvalid     = false;
			bool     connClose     = false;
			bool     connKeepAlive = false;
			bool     inTrailers    = false;
		};

		/**
		 * @brief Полная инициализация парсера под заданный тип сообщения.
		 */
		void init(Parser & p, Type type) noexcept;

		/**
		 * @brief Сброс парсера для разбора следующего сообщения в том же соединении.
		 *        Сохраняет limits, type и флаг responseToHead.
		 */
		void reset(Parser & p) noexcept;

		/**
		 * @brief Скормить парсеру очередную порцию байтов.
		 *
		 * Функция потребляет столько байтов, сколько смогла, и возвращает их число.
		 * Разбор останавливается ровно на границе завершённого сообщения, поэтому
		 * для конвейерных (pipelined) сообщений вызывайте reset() и затем execute()
		 * на оставшемся хвосте буфера.
		 *
		 * Признак конца потока (EOF) — вызов с len == 0; нужен для ответов без
		 * Content-Length и без chunked (тело «до закрытия соединения»).
		 *
		 * @param p      парсер
		 * @param data   указатель на данные (может быть nullptr при len == 0)
		 * @param len    число доступных байтов
		 * @param status [out] итоговый статус разбора
		 * @return число потреблённых байтов из [data, data+len)
		 */
		size_t execute(Parser & p, const char * data, size_t len, Status & status) noexcept;

		/**
		 * @brief Человекочитаемое имя метода.
		 */
		const char * methodName(Method m) noexcept;

		/**
		 * @brief Человекочитаемое имя кода ошибки.
		 */
		const char * errorName(Error e) noexcept;

		/**
		 * @brief Поиск заголовка по имени без учёта регистра (или nullptr).
		 */
		const Header * findHeader(const Message & m, const char * name) noexcept;
	}
}

#endif // AWH_EXPERIENCE_HTTP_HPP
