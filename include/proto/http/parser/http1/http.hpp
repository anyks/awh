/**
 * @file: http.hpp
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

#ifndef __AWH_HTTP_PARSER_HTTP1__
#define __AWH_HTTP_PARSER_HTTP1__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <functional>
#include <string_view>

/**
 * Подключаем наши заголовочные файлы
 */
#include "../parser.hpp"
#include "../../provider.hpp"
#include "../../../../sys/global.hpp"

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
		 * @brief Класс парсера HTTP/1.0 и HTTP/1.1
		 *
		 * @details Инкрементальный (streaming) парсер на базе байтового конечного автомата:
		 *          данные можно подавать любыми кусками, разрыв допустим в любом байте.
		 *          Парсер ничего не накапливает - все данные отдаются через функции обратного
		 *          вызова, стартовая строка складывается в провайдер заголовков сообщения.
		 *
		 * @note Контракт версий: принимаются только HTTP/1.0 и HTTP/1.1,
		 *       любая другая версия отвергается с ошибкой INVALID_VERSION.
		 */
		typedef class __AWH_SHARED_EXPORT__ Parser_HTTP : public parser_t {
			public:
				/**
				 * @brief Максимальная длина строки заголовка чанка (size + chunk-ext)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t MAX_CHUNK_LINE = (16 * 1024);
				/**
				 * @brief Максимальная длина request-line/status-line
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t MAX_REQUEST_LINE = (8 * 1024);
				/**
				 * @brief Максимальный размер одного чанка
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr uint64_t MAX_CHUNK_SIZE = (1ull * 1024 * 1024 * 1024);
			public:
				/**
				 * @brief Код ошибки разбора HTTP-парсера
				 *
				 */
				enum class error_t : uint8_t {
					NONE                      = 0x00, // Ошибок нет
					INTERNAL                  = 0x01, // Внутренняя ошибка состояния
					INVALID_EOL               = 0x02, // Ожидался LF после CR
					INVALID_METHOD            = 0x03, // Недопустимый символ в методе
					INVALID_TARGET            = 0x04, // Недопустимый символ в request-target
					INVALID_STATUS            = 0x05, // Неверный статус-код ответа
					INVALID_VERSION           = 0x06, // Неверная строка версии (HTTP/x.y)
					INVALID_CHUNK_SIZE        = 0x07, // Неверный размер чанка
					INVALID_HEADER_TOKEN      = 0x08, // Недопустимый символ в имени заголовка / obs-fold
					INVALID_HEADER_VALUE      = 0x09, // Недопустимый символ в значении заголовка
					INVALID_CONTENT_LENGTH    = 0x0A, // Content-Length не число / Некорректен
					INVALID_CHUNK_TERMINATOR  = 0x0B, // Нет CRLF после данных чанка
					INVALID_TRANSFER_ENCODING = 0x0C, // Некорректный Transfer-Encoding (chunked не последний и т.п.)
					ABORTED                   = 0x0D, // Разбор прерван пользовательским callback'ом
					URL_OVERFLOW              = 0x0E, // Превышен лимит длины request-line
					BODY_OVERFLOW             = 0x0F, // Превышен лимит размера тела
					CHUNK_OVERFLOW            = 0x10, // Превышен лимит размера чанка
					HEADER_OVERFLOW           = 0x11, // Превышен лимит размера заголовков
					TOO_MANY_HEADERS          = 0x12, // Превышено число заголовков
					CONTENT_LENGTH_CONFLICT   = 0x13, // CL+TE или несколько разных Content-Length (request smuggling)
					PREMATURE_EOF             = 0x14  // Соединение закрыто посреди незавершённого сообщения
				};
			public:
				/**
				 * @brief Структура ограничений безопасности парсера HTTP/1.x
				 *
				 * @details Расширяет общее ядро лимитов базового парсера лимитами,
				 *          специфичными для HTTP/1.x: стартовая строка и кадрирование
				 *          тела в кодировке chunked
				 */
				typedef struct Limits : parser_t::limits_t {
					// Максимальная длина строки заголовка чанка (size + chunk-ext)
					size_t maxChunkLine;
					// Максимальная длина request-line/status-line
					size_t maxRequestLine;
					// Максимальный размер одного чанка
					uint64_t maxChunkSize;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Limits() noexcept :
					 parser_t::limits_t(),
					 maxChunkLine(MAX_CHUNK_LINE),
					 maxRequestLine(MAX_REQUEST_LINE),
					 maxChunkSize(MAX_CHUNK_SIZE) {}
				} limits_t;
			public:
				/**
				 * @brief Класс разобранного сообщения
				 *
				 * @details Если Content-Length не установлен, то значение bodySize == -1.
				 *          Если Content-Length установлен, то значение поля bodySize >= 0.
				 *          Если указан Transfer-Encoding: chunked, то значение поля bodySize == -1.
				 */
				typedef class __AWH_SHARED_EXPORT__ Message {
					public:
						/**
						 * @brief Структура флагов состояния сообщения
						 *
						 */
						typedef struct Flags {
							// Тело передаётся chunked
							bool chunked;
							// Запрошено переключение протокола (Upgrade + Connection: upgrade, ответ 101 или успешный CONNECT)
							bool upgrade;
							// Сообщение полностью разобрано
							bool complete;
							// Соединение переиспользуемое
							bool keepAlive;
							// Клиент прислал заголовок [Expect: 100-continue] и ожидает промежуточный ответ до отправки тела
							bool expectContinue;
							/**
							 * @brief Конструктор
							 *
							 */
							explicit Flags() noexcept :
							 chunked(false), upgrade(false),
							 complete(false), keepAlive(true),
							 expectContinue(false) {}
						} flags_t;
					public:
						// Партиция текущего состояния парсера
						part_t part;
						// Фаза разбора HTTP-сообщения
						phase_t phase;
						// Флаги состояния сообщения
						flags_t flags;
						// Ожидаемый размер тела сообщения (Content-Length)
						int64_t bodySize;
						// Объект провайдера заголовков сообщения
						unique_ptr <provider_t> provider;
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
						/**
						 * @brief Оператор сравнения
						 *
						 * @param message объект сообщения для сравнения
						 * @return        результат сравнения
						 */
						bool operator != (const Message & message) noexcept;
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
						 */
						explicit Message() noexcept;
				} message_t;
			public:
				/**
				 * @brief Тип функции обратного вызова для обработки провайдера заголовков сообщения
				 *
				 * @note Вызывается один раз, сразу после разбора стартовой строки (request-line/status-line),
				 *       когда провайдер заполнен методом/URI либо кодом/сообщением и версией протокола.
				 *
				 * @param provider объект провайдера заголовков сообщения
				 * @return         результат обработки (true - продолжить разбор, false - прервать с ошибкой ABORTED)
				 */
				using provider_callback_t = function <bool (const provider_t *)>;
				/**
				 * @brief Тип функции обратного вызова для обработки тела сообщения
				 *
				 * @note Буфер указывает во входные данные (zero-copy) и действителен ТОЛЬКО на время вызова.
				 *       Фрагменты отдаются по мере поступления данных из сети и не совпадают с границами чанков.
				 *
				 * @param buffer буфер данных тела сообщения
				 * @param size   размер данных тела сообщения
				 * @return       результат обработки (true - продолжить разбор, false - прервать с ошибкой ABORTED)
				 */
				using body_callback_t = function <bool (const void *, const size_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки фазы разбора HTTP-сообщения
				 *
				 * @details Последовательность событий при разборе одного сообщения:
				 *          1. (BEGIN, NONE)    - начало разбора нового сообщения
				 *          2. (END, HEADERS)   - все заголовки разобраны и интерпретированы
				 *          3. (BEGIN, BODY)    - начало приёма тела (только если тело присутствует)
				 *          4. (END, BODY)      - тело полностью принято (только если тело присутствует)
				 *          5. (BEGIN, TRAILER) - начало разбора трейлеров (только для chunked)
				 *          6. (END, TRAILER)   - трейлеры разобраны (только для chunked)
				 *          7. (END, NONE)      - сообщение полностью разобрано
				 *
				 * @param phase фаза разбора HTTP-сообщения
				 * @param part  часть сообщения (заголовки, трейлеры, тело), NONE - сообщение целиком
				 * @return      результат обработки (true - продолжить разбор, false - прервать с ошибкой ABORTED)
				 */
				using phase_callback_t = function <bool (const phase_t, const part_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки границ чанков (Transfer-Encoding: chunked)
				 *
				 * @details Нужен потребителям, которым важно кадрирование тела "чанк-в-чанк":
				 *          прозрачным прокси (ретрансляция с сохранением исходного кадрирования)
				 *          и протоколам с семантикой расширений чанков (например, подписи чанков
				 *          в AWS S3 aws-chunked). Последовательность событий для каждого чанка:
				 *          1. (BEGIN, size, extension) - строка размера чанка разобрана;
				 *          2. фрагменты данных чанка отдаются через body_callback_t;
				 *          3. (END, size, "") - данные чанка дочитаны (принят завершающий CRLF).
				 *          Для последнего чанка (size == 0) вызывается только BEGIN - далее следуют
				 *          события трейлеров. Если функция обратного вызова не установлена,
				 *          расширения чанков не накапливаются (нулевые накладные расходы).
				 *
				 * @param phase     фаза разбора чанка (BEGIN - заголовок разобран, END - чанк дочитан)
				 * @param size      размер данных чанка
				 * @param extension сырые расширения чанка (содержимое после ';' без CRLF), действительны ТОЛЬКО на время вызова
				 * @return          результат обработки (true - продолжить разбор, false - прервать с ошибкой ABORTED)
				 */
				using chunk_callback_t = function <bool (const phase_t, const uint64_t, const string_view)>;
				/**
				 * @brief Тип функции обратного вызова для обработки заголовков или трейлеров сообщения
				 *
				 * @note Заголовки и трейлеры сообщения обрабатываются одинаково, поэтому используется
				 *       один и тот же тип функции обратного вызова. Название и значение заголовка
				 *       действительны ТОЛЬКО на время вызова.
				 *
				 * @param name  название заголовка
				 * @param value значение заголовка (без внешних OWS)
				 * @param part  часть сообщения (заголовки или трейлеры)
				 * @return      результат обработки (true - продолжить разбор, false - прервать с ошибкой ABORTED)
				 */
				using header_callback_t = function <bool (const string_view, const string_view, const part_t)>;
			private:
				/**
				 * @brief Структура промежуточных параметров заголовка HTTP
				 *
				 */
				typedef struct Header {
					// Накопитель имени текущего заголовка (также используется для имени метода запроса)
					string name;
					// Накопитель значения текущего заголовка
					string value;
					// Накопитель расширений текущего чанка (заполняется только при установленном chunk-callback'е)
					string chunkExt;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Header() noexcept :
					 name{""}, value{""}, chunkExt{""} {}
				} header_t;
				/**
				 * @brief Структура для хранения статистики тела HTTP-сообщения
				 *
				 */
				typedef struct Statistics_Body {
					// Общий размер принятого тела сообщения
					uint64_t bytes;
					// Счётчик цифр (hex-цифры размера чанка / цифры статус-кода)
					uint32_t digits;
					// Размер текущего чанка
					uint64_t chunkSize;
					// Значение заголовка Content-Length
					uint64_t contentLength;
					// Остаток непрочитанных данных тела/чанка
					uint64_t bytesRemaining;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Statistics_Body() noexcept :
					 bytes(0), digits(0), chunkSize(0),
					 contentLength(0), bytesRemaining(0) {}
				} statistics_body_t;
				/**
				 * @brief Структура для хранения статистики заголовков HTTP
				 *
				 */
				typedef struct Statistics_Headers {
					// Количество разобранных заголовков
					size_t count;
					// Суммарный размер разобранных заголовков
					size_t bytes;
					// Длина текущей стартовой строки (request-line/status-line)
					size_t lineBytes;
					// Длина текущей строки заголовка чанка (size + chunk-ext)
					size_t chunkLineBytes;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Statistics_Headers() noexcept :
					 count(0), bytes(0), lineBytes(0), chunkLineBytes(0) {}
				} statistics_headers_t;
				/**
				 * @brief Структура для хранения флагов состояния парсера
				 *
				 */
				typedef struct Flags {
					// Выполняется разбор трейлеров
					bool inTrailers;
					// Заголовок Upgrade получен
					bool upgradeSeen;
					// В заголовке Connection присутствует close
					bool connectionClose;
					// В заголовке Connection присутствует upgrade
					bool connectionUpgrade;
					// Заголовок Content-Length получен
					bool contentLengthSeen;
					// В заголовке Connection присутствует keep-alive
					bool connectionKeepAlive;
					// Заголовок Transfer-Encoding получен
					bool transferEncodingSeen;
					// Заголовок Transfer-Encoding некорректен (chunked не последний и т.п.)
					bool transferEncodingInvalid;
					// Последнее кодирование в Transfer-Encoding - chunked
					bool transferEncodingChunkedFinal;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Flags() noexcept :
					 inTrailers(false), upgradeSeen(false),
					 connectionClose(false), connectionUpgrade(false),
					 contentLengthSeen(false), connectionKeepAlive(false),
					 transferEncodingSeen(false), transferEncodingInvalid(false),
					 transferEncodingChunkedFinal(false) {}
				} flags_t;
				/**
				 * @brief Структура для хранения функций обратного вызова
				 *
				 */
				typedef struct Callbacks {
					/**
					 * @brief Функция обратного вызова для обработки тела сообщения
					 *
					 */
					body_callback_t body;
					/**
					 * @brief Функция обратного вызова для обработки фазы разбора HTTP-сообщения
					 *
					 */
					phase_callback_t phase;
					/**
					 * @brief Функция обратного вызова для обработки границ чанков
					 *
					 */
					chunk_callback_t chunk;
					/**
					 * @brief Функция обратного вызова для обработки заголовков или трейлеров сообщения
					 *
					 */
					header_callback_t header;
					/**
					 * @brief Функция обратного вызова для обработки провайдера заголовков сообщения
					 *
					 */
					provider_callback_t provider;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Callbacks() noexcept :
					 body(nullptr), phase(nullptr), chunk(nullptr),
					 header(nullptr), provider(nullptr) {}
				} callbacks_t;
			private:
				// Код ошибки разбора
				error_t _error;
			private:
				// Настраиваемые лимиты безопасности
				limits_t _limits;
				// Результат разбора сообщения
				message_t _message;
			private:
				// Флаги состояния парсера
				flags_t _flags;
			private:
				// Текущее состояние конечного автомата (значения определены в http.cpp)
				uint8_t _state;
			private:
				// Метод запроса, которому соответствует ожидаемый ответ (для направления RESPONSE)
				method_t _method;
			private:
				// Промежуточный объект заголовка HTTP
				header_t _header;
			private:
				// Объект функций обратного вызова
				callbacks_t _callbacks;
			private:
				// Статистика тела HTTP
				statistics_body_t _statsBody;
				// Статистика заголовков HTTP
				statistics_headers_t _statsHeaders;
			private:
				/**
				 * @brief Метод выбора способа кадрирования тела после завершения заголовков
				 *
				 */
				void beginBody() noexcept;
			private:
				/**
				 * @brief Метод завершения разбора текущего заголовка/трейлера
				 *
				 * @return результат обработки (false - разбор прерван)
				 */
				bool commitHeader() noexcept;
				/**
				 * @brief Метод завершения разбора стартовой строки (request-line/status-line)
				 *
				 * @return результат обработки (false - разбор прерван)
				 */
				bool commitStartLine() noexcept;
			private:
				/**
				 * @brief Метод завершения разбора всего сообщения
				 *
				 */
				void completeMessage() noexcept;
				/**
				 * @brief Метод завершения разбора строки размера чанка
				 *
				 */
				void chunkSizeComplete() noexcept;
				/**
				 * @brief Метод проверки отсутствия тела у ответа сервера (по статус-коду и методу запроса)
				 *
				 * @return результат проверки
				 */
				bool responseHasNoBody() const noexcept;
			private:
				/**
				 * @brief Метод фиксации ошибки разбора (код ошибки, итоговый статус и запись в лог)
				 *
				 * @param error код ошибки разбора
				 */
				void fail(const error_t error) noexcept;
			private:
				/**
				 * @brief Метод вызова функции обратного вызова обработки фазы разбора
				 *
				 * @param phase фаза разбора HTTP-сообщения
				 * @param part  часть сообщения
				 * @return      результат обработки (false - разбор прерван с ошибкой ABORTED)
				 */
				bool firePhase(const phase_t phase, const part_t part) noexcept;
				/**
				 * @brief Метод вызова функции обратного вызова обработки границ чанков
				 *
				 * @param phase фаза разбора чанка
				 * @param size  размер данных чанка
				 * @return      результат обработки (false - разбор прерван с ошибкой ABORTED)
				 */
				bool fireChunk(const phase_t phase, const uint64_t size) noexcept;
			private:
				/**
				 * @brief Метод интерпретации заголовка Connection
				 *
				 * @param begin начало значения заголовка
				 * @param end   конец значения заголовка
				 */
				void applyConnection(const char * begin, const char * end) noexcept;
				/**
				 * @brief Метод интерпретации заголовка Content-Length
				 *
				 * @param begin начало значения заголовка
				 * @param end   конец значения заголовка
				 * @return      результат интерпретации
				 */
				bool applyContentLength(const char * begin, const char * end) noexcept;
				/**
				 * @brief Метод интерпретации заголовка Transfer-Encoding (накопительно по нескольким заголовкам)
				 *
				 * @param begin начало значения заголовка
				 * @param end   конец значения заголовка
				 */
				void applyTransferEncoding(const char * begin, const char * end) noexcept;
			public:
				/**
				 * @brief Метод полной очистки всех данных парсера
				 *
				 * @details Помимо сброса состояния разбора возвращает лимиты безопасности
				 *          к значениям по умолчанию и удаляет установленные функции обратного вызова.
				 */
				void clear() noexcept override;
				/**
				 * @brief Метод сброса парсера для разбора следующего сообщения в том же соединении
				 *
				 * @details Дешёвый сброс между сообщениями (keep-alive/pipelining): сохраняет лимиты
				 *          безопасности и установленные функции обратного вызова, провайдер заголовков
				 *          не пересоздаётся, а очищается (переиспользуется выделенная память).
				 *          Метод запроса, установленный через method(), сбрасывается в NONE -
				 *          для направления RESPONSE выставляйте его заново перед каждым ответом.
				 */
				void reset() noexcept override;
			public:
				/**
				 * @brief Метод установки метода запроса, которому соответствует ожидаемый ответ
				 *
				 * @details Используется ТОЛЬКО для направления RESPONSE: парсер ответа сам не может
				 *          узнать, на какой запрос пришёл ответ, а метод запроса влияет на кадрирование
				 *          тела (ответ на HEAD содержит Content-Length, но тела не имеет; успешный 2xx
				 *          ответ на CONNECT открывает туннель и тела не имеет).
				 *          Сбрасывается в NONE при reset() - выставляйте заново перед каждым ответом
				 *          в keep-alive/конвейере.
				 *
				 * @param method метод запроса клиента
				 */
				void method(const method_t method) noexcept;
			public:
				/**
				 * @brief Метод клонирования объекта парсера
				 *
				 * @details Клон получает те же направление трафика, лимиты безопасности и функции
				 *          обратного вызова, но чистое состояние разбора ("фабрика с теми же настройками").
				 *
				 * @return копия объекта парсера
				 */
				unique_ptr <parser_t> clone() const noexcept override;
			public:
				/**
				 * @brief Метод уведомления парсера о завершении потока данных (закрытии соединения)
				 *
				 * @details Требуется для сообщений, у которых тело кадрируется закрытием соединения:
				 *          ответы HTTP/1.0 и ответы без Content-Length и без Transfer-Encoding: chunked.
				 *          У таких сообщений в протоколе нет маркера конца тела - конец определяется
				 *          только закрытием соединения удалённой стороной. Сетевой слой обязан вызвать
				 *          этот метод, когда соединение закрыто (получен FIN/EOF сокета):
				 *          - если парсер читает тело "до закрытия соединения" - сообщение помечается
				 *            завершённым (status() == COMPLETE);
				 *          - если парсер находится между сообщениями - ничего не происходит
				 *            (нормальное закрытие keep-alive соединения);
				 *          - если сообщение разобрано частично (заголовки или недочитанное тело
				 *            с Content-Length) - фиксируется ошибка PREMATURE_EOF (обрыв соединения).
				 */
				void eof() noexcept override;
				/**
				 * @brief Метод разбора данных
				 *
				 * @details Потребляет столько байтов, сколько смог, и возвращает их число.
				 *          Итоговый статус необходимо контролировать методом status():
				 *          - PARTIAL:  данные приняты, сообщение не завершено - нужно ещё байтов;
				 *          - COMPLETE: сообщение полностью разобрано, разбор остановлен ровно на границе
				 *                      сообщения - для конвейерных (pipelined) сообщений вызовите reset()
				 *                      и затем parse() на оставшемся хвосте буфера;
				 *          - ERROR:    ошибка разбора/безопасности - причина в методе error().
				 *
				 * @param buffer буфер данных для разбора
				 * @param size   размер данных для разбора
				 * @return       количество обработанных байт данных
				 */
				size_t parse(const void * buffer, const size_t size) noexcept override;
			public:
				/**
				 * @brief Метод получения кода ошибки разбора
				 *
				 * @return код ошибки
				 */
				error_t error() const noexcept;
				/**
				 * @brief Метод получения человекочитаемого названия текущей ошибки разбора
				 *
				 * @return название текущей ошибки разбора
				 */
				string errorName() const noexcept override;
				/**
				 * @brief Метод получения человекочитаемого названия кода ошибки
				 *
				 * @param error код ошибки разбора
				 * @return      название кода ошибки
				 */
				static string errorName(const error_t error) noexcept;
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
				const message_t & message() const noexcept;
			public:
				/**
				 * @brief Метод установки функции обратного вызова для обработки тела сообщения
				 *
				 * @param callback функция обратного вызова для обработки тела сообщения
				 */
				void on(body_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки фазы разбора HTTP-сообщения
				 *
				 * @param callback функция обратного вызова для обработки фазы разбора HTTP-сообщения
				 */
				void on(phase_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки границ чанков
				 *
				 * @param callback функция обратного вызова для обработки границ чанков
				 */
				void on(chunk_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки заголовков или трейлеров сообщения
				 *
				 * @param callback функция обратного вызова для обработки заголовков или трейлеров сообщения
				 */
				void on(header_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова для обработки провайдера заголовков сообщения
				 *
				 * @param callback функция обратного вызова для обработки провайдера заголовков сообщения
				 */
				void on(provider_callback_t callback) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param direct направление трафика (запрос/ответ)
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 */
				explicit Parser_HTTP(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * @brief Деструктор
				 *
				 */
				~Parser_HTTP() noexcept override;
		} parser_http_t;
	};
};

#endif // __AWH_HTTP_PARSER_HTTP1__
