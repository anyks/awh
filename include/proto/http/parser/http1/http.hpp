/**
 * @file: http.hpp
 * @date: 2026-07-18
 * @license: LicenseRef-AWH-1.0
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
 * Подключаем заголовочные файлы проекта
 */
#include "../parser.hpp"
#include "../../headers.hpp"
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
				 * @brief Идентификатор единственного логического потока HTTP/1.x
				 *
				 * @note HTTP/1.x не мультиплексируется - идентификатор существует только
				 *       для универсальности сигнатур функций обратного вызова с HTTP/2
				 */
				static constexpr uint32_t STREAM_ID = (1);
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
				 * @brief Порог сигнала о готовности принимать данные тела (low-water)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t SEND_LOW_WATER = (64 * 1024);
				/**
				 * @brief Ёмкость выходного буфера отправки (high-water)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t SEND_HIGH_WATER = (256 * 1024);
				/**
				 * @brief Гранулярность порции pull-источника данных тела
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 */
				static constexpr size_t SOURCE_CHUNK_SIZE = (16 * 1024);
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
				typedef struct __AWH_SHARED_EXPORT__ Limits : parser_t::limits_t {
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
					explicit Limits() noexcept;
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
						typedef struct __AWH_SHARED_EXPORT__ Flags {
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
							explicit Flags() noexcept;
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
				 * @brief Тип функции обратного вызова о готовности принимать данные тела
				 *
				 * @details Вызывается когда выходной буфер опустился ниже low-water
				 *          (после частичного приёма в sendData можно отправлять дальше).
				 *          Идентификатор потока для HTTP/1.x всегда равен STREAM_ID -
				 *          сигнатура универсальна с HTTP/2.
				 *
				 * @param sid идентификатор потока (всегда STREAM_ID)
				 */
				using writable_callback_t = function <void (const uint32_t)>;
				/**
				 * @brief Тип функции обратного вызова записи исходящих байтов в сеть
				 *
				 * @details Если установлена - парсер сам отдаёт исходящие байты сетевому слою
				 *          сразу по мере формирования. Если не установлена - исходящие байты
				 *          накапливаются во внутреннем буфере (pull-модель: pending() +
				 *          consumePending()).
				 *
				 * @param buffer буфер исходящих данных
				 * @param size   размер исходящих данных
				 */
				using write_callback_t = function <void (const void *, const size_t)>;
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
				 *          Идентификатор потока для HTTP/1.x всегда равен STREAM_ID -
				 *          сигнатура универсальна с HTTP/2.
				 *
				 * @param sid   идентификатор потока (всегда STREAM_ID)
				 * @param phase фаза разбора HTTP-сообщения
				 * @param part  часть сообщения (заголовки, трейлеры, тело), NONE - сообщение целиком
				 * @return      результат обработки (true - продолжить разбор, false - прервать с ошибкой ABORTED)
				 */
				using phase_callback_t = function <bool (const uint32_t, const phase_t, const part_t)>;
				/**
				 * @brief Тип функции обратного вызова для обработки границ чанков (Transfer-Encoding: chunked)
				 *
				 * @details Нужен потребителям, которым важно кадрирование тела "чанк-в-чанк":
				 *          прозрачным прокси (ретрансляция с сохранением исходного кадрирования)
				 *          и протоколам с семантикой расширений чанков (например, подписи чанков
				 *          в AWS S3 aws-chunked). Последовательность событий для каждого чанка:
				 *          1. (BEGIN, size, extension) - строка размера чанка разобрана;
				 *          2. фрагменты данных чанка отдаются через data_callback_t;
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
				 * @brief Тип функции обратного вызова для обработки провайдера заголовков сообщения
				 *
				 * @details Вызывается по завершению блока заголовков, когда заголовки разобраны
				 *          и интерпретированы (выбран способ кадрирования тела) - тот же момент,
				 *          что END_HEADERS у HTTP/2. Для трейлеров провайдер передаётся как nullptr.
				 *          Идентификатор потока для HTTP/1.x всегда равен STREAM_ID -
				 *          сигнатура универсальна с HTTP/2.
				 *
				 * @param sid       идентификатор потока (всегда STREAM_ID)
				 * @param provider  объект провайдера заголовков сообщения (nullptr для трейлеров)
				 * @param endStream флаг завершения сообщения (тела не будет)
				 * @return          результат обработки (true - продолжить разбор, false - прервать с ошибкой ABORTED)
				 */
				using provider_callback_t = function <bool (const uint32_t, const provider_t *, const bool)>;
				/**
				 * @brief Тип функции обратного вызова для обработки фрагмента тела сообщения
				 *
				 * @details Буфер указывает во входные данные (zero-copy) и действителен ТОЛЬКО на время
				 *          вызова. Фрагменты отдаются по мере поступления данных из сети и не совпадают
				 *          с границами чанков. Флаг endStream выставляется на фрагменте, завершающем
				 *          тело фиксированного размера (Content-Length); для тел chunked и "до закрытия
				 *          соединения" конец тела в момент фрагмента неизвестен - завершение сигнализируется
				 *          провайдером трейлеров либо фазой (END, NONE). Идентификатор потока для HTTP/1.x
				 *          всегда равен STREAM_ID - сигнатура универсальна с HTTP/2.
				 *
				 * @param sid       идентификатор потока (всегда STREAM_ID)
				 * @param buffer    буфер данных тела сообщения
				 * @param size      размер данных тела сообщения
				 * @param endStream флаг завершения сообщения (фрагмент завершает тело)
				 * @return          результат обработки (true - продолжить разбор, false - прервать с ошибкой ABORTED)
				 */
				using data_callback_t = function <bool (const uint32_t, const void *, const size_t, const bool)>;
				/**
				 * @brief Тип pull-источника данных тела сообщения (для больших тел без лишней копии)
				 *
				 * @details Альтернатива sendData: парсер сам запрашивает у источника данные
				 *          ровно тогда, когда есть место в выходном буфере. Источник заполняет
				 *          буфер (не более cap байт), выставляет eof = true по достижении конца
				 *          тела и возвращает число записанных байт, либо -1 при ошибке.
				 *          Идентификатор потока для HTTP/1.x всегда равен STREAM_ID -
				 *          сигнатура универсальна с HTTP/2.
				 *
				 * @param sid    идентификатор потока (всегда STREAM_ID)
				 * @param buffer буфер для заполнения
				 * @param cap    ёмкость буфера
				 * @param eof    флаг достижения конца тела
				 * @return       число записанных байт либо -1 при ошибке
				 */
				using data_source_callback_t = function <int64_t (const uint32_t, uint8_t *, const size_t, bool &)>;
				/**
				 * @brief Тип функции обратного вызова для обработки заголовков или трейлеров сообщения
				 *
				 * @note Заголовки и трейлеры сообщения обрабатываются одинаково, поэтому используется
				 *       один и тот же тип функции обратного вызова. Название и значение заголовка
				 *       действительны ТОЛЬКО на время вызова. Идентификатор потока для HTTP/1.x
				 *       всегда равен STREAM_ID - сигнатура универсальна с HTTP/2.
				 *
				 * @param sid   идентификатор потока (всегда STREAM_ID)
				 * @param name  название заголовка
				 * @param value значение заголовка (без внешних OWS)
				 * @param part  часть сообщения (заголовки или трейлеры)
				 * @return      результат обработки (true - продолжить разбор, false - прервать с ошибкой ABORTED)
				 */
				using header_callback_t = function <bool (const uint32_t, const string_view, const string_view, const part_t)>;
			private:
				/**
				 * @brief Структура промежуточных параметров заголовка HTTP
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Header {
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
					explicit Header() noexcept;
				} header_t;
				/**
				 * @brief Структура для хранения статистики тела HTTP-сообщения
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Statistics_Body {
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
					explicit Statistics_Body() noexcept;
				} statistics_body_t;
				/**
				 * @brief Структура для хранения статистики заголовков HTTP
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Statistics_Headers {
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
					explicit Statistics_Headers() noexcept;
				} statistics_headers_t;
				/**
				 * @brief Структура для хранения флагов состояния парсера
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Flags {
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
					explicit Flags() noexcept;
				} flags_t;
				/**
				 * @brief Структура состояния отправки исходящего сообщения
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Sender {
					/**
					 * @brief Способ кадрирования тела исходящего сообщения
					 *
					 */
					enum class framing_t : uint8_t {
						NONE     = 0x00, // Заголовки ещё не отправлены либо тела нет
						RAW      = 0x01, // Сырое тело до закрытия соединения (HTTP/1.0)
						CHUNKED  = 0x02, // Кодировка chunked (Transfer-Encoding: chunked)
						IDENTITY = 0x03  // Фиксированный размер (Content-Length)
					};
					// Исходящее сообщение завершено (конец тела отправлен)
					bool endSent;
					// Достигнут конец тела pull-источника данных
					bool sourceEof;
					// Заголовки исходящего сообщения отправлены
					bool headersSent;
					// Сигнал writable уже подан для текущего провала буфера
					bool writableNotified;
					// Способ кадрирования тела исходящего сообщения
					framing_t framing;
					// Порог сигнала writable (low-water)
					size_t lowWater;
					// Ёмкость выходного буфера отправки (high-water)
					size_t highWater;
					// Префикс output, уже отданный сети (вместо erase(0,..))
					size_t outputPos;
					// Остаток тела до полного Content-Length (для кадрирования IDENTITY)
					uint64_t remaining;
					// Буфер исходящих байтов (заголовки + кадрированное тело)
					string output;
					// Pull-источник данных тела (если задан вместо sendData)
					data_source_callback_t source;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Sender() noexcept;
				} sender_t;
				/**
				 * @brief Структура для хранения функций обратного вызова
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Callbacks {
					/**
					 * @brief Функция обратного вызова для обработки фрагмента тела сообщения
					 *
					 */
					data_callback_t data;
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
					 * @brief Функция обратного вызова записи исходящих байтов в сеть
					 *
					 */
					write_callback_t write;
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
					 * @brief Функция обратного вызова о готовности принимать данные тела
					 *
					 */
					writable_callback_t writable;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Callbacks() noexcept;
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
				// Объект состояния отправки исходящего сообщения
				sender_t _sender;
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
				 * @brief Метод передачи исходящих байтов сетевому слою через функцию обратного вызова записи
				 *
				 * @details Если функция записи не установлена - байты остаются во внутреннем
				 *          буфере до выборки через pending()/consumePending().
				 */
				void flush() noexcept;
				/**
				 * @brief Метод получения логического объёма ещё не отправленных исходящих байтов
				 *
				 * @return объём не отправленных исходящих байтов
				 */
				size_t outputPending() const noexcept;
			private:
				/**
				 * @brief Метод дозагрузки выходного буфера из pull-источника данных (если он задан)
				 *
				 * @details Источник пишет напрямую в выходной буфер (без промежуточной копии),
				 *          кадрирование тела применяется к каждой полученной порции.
				 *
				 * @return число полученных от источника байт тела
				 */
				size_t refillFromSource() noexcept;
			private:
				/**
				 * @brief Метод прокачки pull-источника данных в сеть
				 *
				 * @details В push-режиме (установлена функция обратного вызова записи) качает
				 *          источник до конца тела либо до временного отсутствия данных.
				 *          В pull-режиме наполняет выходной буфер до high-water однократно -
				 *          досылка происходит по мере выборки consumePending().
				 */
				void pumpSource() noexcept;
				/**
				 * @brief Метод сигнализации о готовности принимать данные (один раз на провал буфера)
				 *
				 */
				void maybeNotifyWritable() noexcept;
			private:
				/**
				 * @brief Метод завершения тела исходящего сообщения (финализация кадрирования)
				 *
				 */
				void finishBody() noexcept;
				/**
				 * @brief Метод кадрирования и записи порции тела в выходной буфер
				 *
				 * @param buffer буфер данных тела
				 * @param size   размер данных тела
				 */
				void frameBody(const void * buffer, const size_t size) noexcept;
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
				/**
				 * @brief Метод вызова функции обратного вызова обработки провайдера заголовков сообщения
				 *
				 * @param provider  объект провайдера заголовков сообщения (nullptr для трейлеров)
				 * @param endStream флаг завершения сообщения (тела не будет)
				 * @return          результат обработки (false - разбор прерван с ошибкой ABORTED)
				 */
				bool fireProvider(const provider_t * provider, const bool endStream) noexcept;
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
				string_view errorName() const noexcept override;
				/**
				 * @brief Метод получения человекочитаемого названия кода ошибки
				 *
				 * @param error код ошибки разбора
				 * @return      название кода ошибки
				 */
				static string_view errorName(const error_t error) noexcept;
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
				 * @brief Метод сброса состояния отправки для следующего сообщения в том же соединении
				 *
				 * @details Готовит отправитель к следующему сообщению (keep-alive): сбрасывает
				 *          кадрирование, источник данных и флаги, но НЕ трогает неотправленный
				 *          остаток выходного буфера. Состояние разбора не затрагивается -
				 *          для него используется reset().
				 */
				void resetSender() noexcept;
			public:
				/**
				 * @brief Метод назначения pull-источника данных тела сообщения
				 *
				 * @param source pull-источник данных тела
				 */
				void dataSource(data_source_callback_t source) noexcept;
			public:
				/**
				 * @brief Метод настройки порогов выходного буфера отправки
				 *
				 * @param high ёмкость выходного буфера отправки (high-water)
				 * @param low  порог сигнала writable (low-water)
				 */
				void sendWaterMarks(const size_t high, const size_t low) noexcept;
			public:
				/**
				 * @brief Метод отправки блока заголовков (запрос/ответ/трейлеры) исходящего сообщения
				 *
				 * @details Способ кадрирования тела выбирается по заголовкам контейнера:
				 *          - установлен Content-Length - тело фиксированного размера (IDENTITY);
				 *          - Content-Length отсутствует и endStream == false - добавляется
				 *            Transfer-Encoding: chunked (для HTTP/1.0 - сырое тело до закрытия
				 *            соединения, chunked в HTTP/1.0 не существует);
				 *          - endStream == true - тела нет, заголовки отправляются как есть.
				 *          Контейнер без провайдера в режиме chunked интерпретируется как
				 *          трейлеры (завершает тело последним чанком) - та же семантика,
				 *          что у HTTP/2.
				 *
				 * @param headers   контейнер заголовков (провайдер контейнера задаёт стартовую строку)
				 * @param endStream флаг завершения сообщения (тела не будет)
				 */
				void sendHeaders(const headers_t & headers, const bool endStream) noexcept;
				/**
				 * @brief Метод передачи части тела сообщения для отправки (push-модель, bounded buffer)
				 *
				 * @details Копирует в выходной буфер столько байт, сколько влезает до high-water,
				 *          и возвращает это число (0..size). Если вернулось меньше size - буфер
				 *          заполнен: приостановите выдачу и дождитесь функции обратного вызова
				 *          writable. Кадрирование тела (chunked/identity) парсер применяет сам.
				 *
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream флаг завершения сообщения
				 * @return          число принятых байт (0..size)
				 */
				size_t sendData(const void * buffer, const size_t size, const bool endStream) noexcept;
			public:
				/**
				 * @brief Метод получения ещё не отправленных исходящих байтов (pull-модель)
				 *
				 * @details View действителен до следующего вызова любого метода парсера.
				 *          После записи в сокет освободите отправленную часть методом
				 *          consumePending(). При установленной функции обратного вызова
				 *          записи буфер опустошается автоматически.
				 *
				 * @return ещё не отправленные исходящие байты (zero-copy view во внутренний буфер)
				 */
				string_view pending() const noexcept;
				/**
				 * @brief Метод освобождения отправленных байтов из исходящего буфера (амортизированно O(1))
				 *
				 * @param size число отправленных байт
				 */
				void consumePending(const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки функции обратного вызова для обработки фрагмента тела сообщения
				 *
				 * @param callback функция обратного вызова для обработки фрагмента тела сообщения
				 */
				void on(data_callback_t callback) noexcept;
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
				 * @brief Метод установки функции обратного вызова записи исходящих байтов в сеть
				 *
				 * @param callback функция обратного вызова записи исходящих байтов в сеть
				 */
				void on(write_callback_t callback) noexcept;
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
				/**
				 * @brief Метод установки функции обратного вызова о готовности принимать данные тела
				 *
				 * @param callback функция обратного вызова о готовности принимать данные тела
				 */
				void on(writable_callback_t callback) noexcept;
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
