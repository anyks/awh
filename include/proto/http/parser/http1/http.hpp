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
 * \~russian
 * @brief Заголовочный файл парсера протокола HTTP/1.x — класс Parser_HTTP, выполняющий разбор стартовой строки,
 *        заголовков и тела с кадрированием chunked и Content-Length, контроль лимитов,
 *        сбор статистики и сборку исходящих сообщений
 *
 * \~english
 * @brief Header file of the parser of the HTTP/1.x protocol — the class Parser_HTTP performing the parsing of the starting line,
 *        of the headers and of the body with the framing chunked and Content-Length, the control of the limits,
 *        the collection of the statistics and the assembly of the outgoing messages
 *
 * \~
 *
 * @copyright: Copyright © 2026
 *
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
#include "../../../../container/buffer.hpp"
#include "../../../../sys/global.hpp"

/**
 * \~russian
 * @brief основное пространство имён
 *
 *
 * \~english
 * @brief main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён HTTP-протокола
	 *
	 *
	 * \~english
	 * @brief HTTP protocol namespace
	 *
	 * \~
	 */
	namespace http {
		/**
		 * \~russian
		 * @brief Класс парсера HTTP/1.0 и HTTP/1.1
		 *
		 * @details Инкрементальный (streaming) парсер на базе байтового конечного автомата:
		 *          данные можно подавать любыми кусками, разрыв допустим в любом байте.
		 *          Парсер ничего не накапливает - все данные отдаются через функции обратного
		 *          вызова, стартовая строка складывается в провайдер заголовков сообщения.
		 *
		 * @note Контракт версий: принимаются только HTTP/1.0 и HTTP/1.1,
		 *       любая другая версия отвергается с ошибкой INVALID_VERSION.
		 *
		 * \~english
		 * @brief Class of the parser of HTTP/1.0 and HTTP/1.1
		 * @details An incremental (streaming) parser on the base of an octet finite automaton:
		 *          the data may be supplied by any pieces, a break is admissible at any octet.
		 *          The parser accumulates nothing - all the data is issued through the callback
		 *          functions, the starting line is put into the provider of the headers of the message.
		 * @note The contract of the versions: only HTTP/1.0 and HTTP/1.1 are accepted,
		 *       any other version is rejected with the error INVALID_VERSION.
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Parser_HTTP : public parser_t {
			public:
				/**
				 * \~russian
				 * @brief Идентификатор единственного логического потока HTTP/1.x
				 *
				 * @note HTTP/1.x не мультиплексируется - идентификатор существует только
				 *       для универсальности сигнатур функций обратного вызова с HTTP/2
				 *
				 * \~english
				 * @brief Identifier of the single logical stream of HTTP/1.x
				 * @note HTTP/1.x is not multiplexed - the identifier exists only
				 *       for the universality of the signatures of the callback functions with HTTP/2
				 *
				 * \~
				 */
				static constexpr uint32_t STREAM_ID = (1);
				/**
				 * \~russian
				 * @brief Максимальная длина строки заголовка чанка (size + chunk-ext)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Largest length of the line of the header of a chunk (size + chunk-ext)
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t MAX_CHUNK_LINE = (16 * 1024);
				/**
				 * \~russian
				 * @brief Максимальная длина request-line/status-line
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Largest length of a request-line/status-line
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t MAX_REQUEST_LINE = (8 * 1024);
				/**
				 * \~russian
				 * @brief Порог сигнала о готовности принимать данные тела (low-water)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Threshold of the signal about the readiness to accept the data of the body (low-water)
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t SEND_LOW_WATER = (64 * 1024);
				/**
				 * \~russian
				 * @brief Ёмкость выходного буфера отправки (high-water)
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Capacity of the output buffer of the sending (high-water)
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t SEND_HIGH_WATER = (256 * 1024);
				/**
				 * \~russian
				 * @brief Гранулярность порции pull-источника данных тела
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Granularity of a portion of the pull source of the data of the body
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t SOURCE_CHUNK_SIZE = (16 * 1024);
				/**
				 * \~russian
				 * @brief Максимальный объём тела, выкачиваемый из pull-источника за одну прокачку
				 *
				 * @note Ограничивает время удержания управления при отправке больших тел -
				 *       остаток дозагружается вызовами resumeSource()
				 *
				 * \~english
				 * @brief Largest volume of the body pumped out of the pull source in a single pumping
				 * @note It limits the time of the holding of the control at the sending of the big bodies -
				 *       the remainder is loaded by the calls of resumeSource()
				 *
				 * \~
				 */
				static constexpr uint64_t SOURCE_PUMP_LIMIT = (1ull * 1024 * 1024);
				/**
				 * \~russian
				 * @brief Максимальный размер одного чанка
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Largest size of a single chunk
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr uint64_t MAX_CHUNK_SIZE = (1ull * 1024 * 1024 * 1024);
				/**
				 * \~russian
				 * @brief Требование строгого окончания строк CRLF (запрет одиночного LF)
				 *
				 * @note По умолчанию отключено ради совместимости с реальным трафиком
				 *
				 * \~english
				 * @brief Requirement of a strict ending of the lines by a CRLF (a prohibition of a single LF)
				 * @note By default it is disabled for the sake of the compatibility with the real traffic
				 *
				 * \~
				 */
				static constexpr bool STRICT_EOL = (false);
				/**
				 * \~russian
				 * @brief Запрет лишних пробелов внутри стартовой строки
				 *
				 * @note По умолчанию отключено ради совместимости с реальным трафиком
				 *
				 * \~english
				 * @brief Prohibition of the superfluous spaces inside the starting line
				 * @note By default it is disabled for the sake of the compatibility with the real traffic
				 *
				 * \~
				 */
				static constexpr bool STRICT_SPACES = (false);
				/**
				 * \~russian
				 * @brief Требование обязательного заголовка Host у запросов HTTP/1.1
				 *
				 * @note По умолчанию отключено - парсер не навязывает прикладную валидацию
				 *
				 * \~english
				 * @brief Requirement of an obligatory header Host at the requests of HTTP/1.1
				 * @note By default it is disabled - the parser does not impose an applied validation
				 *
				 * \~
				 */
				static constexpr bool REQUIRE_HOST = (false);
			public:
				/**
				 * \~russian
				 * @brief Код ошибки разбора HTTP-парсера
				 *
				 * \~english
				 * @brief Error code of the parsing of the HTTP parser
				 *
				 * \~
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
					INVALID_TRANSFER_ENCODING = 0x0C, // Некорректный Transfer-Encoding (chunked не последний, объявлен в HTTP/1.0 и т.п.)
					ABORTED                   = 0x0D, // Разбор прерван пользовательским callback'ом
					URL_OVERFLOW              = 0x0E, // Превышен лимит длины request-line
					BODY_OVERFLOW             = 0x0F, // Превышен лимит размера тела
					CHUNK_OVERFLOW            = 0x10, // Превышен лимит размера чанка
					HEADER_OVERFLOW           = 0x11, // Превышен лимит размера заголовков
					TOO_MANY_HEADERS          = 0x12, // Превышено число заголовков
					CONTENT_LENGTH_CONFLICT   = 0x13, // CL+TE или несколько разных Content-Length (request smuggling)
					PREMATURE_EOF             = 0x14, // Соединение закрыто посреди незавершённого сообщения
					INVALID_HOST              = 0x15  // Заголовок Host запроса отсутствует, продублирован либо несёт недопустимое значение
				};
			public:
				/**
				 * \~russian
				 * @brief Структура ограничений безопасности парсера HTTP/1.x
				 *
				 * @details Расширяет общее ядро лимитов базового парсера лимитами,
				 *          специфичными для HTTP/1.x: стартовая строка и кадрирование
				 *          тела в кодировке chunked, а также переключателями строгости
				 *          разбора. Переключатели строгости выключены по умолчанию -
				 *          парсер остаётся толерантным к реальному трафику, но фронтовым
				 *          (server-facing) и проксирующим сценариям их следует включать:
				 *          расхождение в трактовке одиночного LF и лишних пробелов между
				 *          двумя звеньями цепочки - классический вектор request smuggling
				 *
				 * @note Переключатели строгости описывают терпимость к неправильному
				 *       входу и от протокола работы не зависят. Роль узла задаётся
				 *       отдельно - методом proto: режим PROXY1 ужесточает не разбор,
				 *       а кадрирование, отвергая ответы, границу которых следующее
				 *       звено цепочки может определить иначе (объявленное кадрирование
				 *       у 1xx и 204, кодирование без завершающего chunked). Узел,
				 *       передающий сообщение дальше, обязан отвергать то, о чём
				 *       нельзя договориться, а конечному получателю достаточно
				 *       закрыть соединение
				 *
				 * @note Октеты, отброшенные при разборе, входят в лимиты наравне с
				 *       сохранёнными: ведущие OWS значения заголовка учитываются в
				 *       maxHeadersTotal, лишние пробелы стартовой строки - в
				 *       maxRequestLine, расширения чанка - в maxChunkLine. Лимиты
				 *       ограничивают принятый поток, а не то, что от него осталось:
				 *       иначе строка вида "X:" с потоком пробелов растягивалась бы
				 *       неограниченно, не приближая разбор к завершению и не расходуя
				 *       ни одного лимита
				 *
				 * \~english
				 * @brief Structure of the limitations of the safety of the parser of HTTP/1.x
				 * @details It extends the common core of the limits of the base parser by the limits
				 *          specific to HTTP/1.x: the starting line and the framing
				 *          of the body in the encoding chunked, and also by the switches of the strictness
				 *          of the parsing. The switches of the strictness are disabled by default -
				 *          the parser remains tolerant to the real traffic, but for the front
				 *          (server-facing) and the proxying scenarios they should be enabled:
				 *          a divergence in the treatment of a single LF and of the superfluous spaces between
				 *          two links of a chain is a classical vector of a request smuggling
				 * @note The switches of the strictness describe the tolerance to an incorrect
				 *       input and do not depend on the protocol of the work. The role of a node is set
				 *       separately - by the method proto: the mode PROXY1 toughens not the parsing
				 *       but the framing, rejecting the answers the boundary of which the next
				 *       link of the chain may determine differently (an announced framing
				 *       at 1xx and 204, an encoding without a concluding chunked). A node
				 *       passing a message onward is obliged to reject that about which
				 *       it is impossible to agree, while for a final receiver it suffices
				 *       to close the connection
				 * @note The octets discarded at the parsing enter into the limits on a par with
				 *       the preserved ones: the leading OWS of the value of a header are accounted in
				 *       maxHeadersTotal, the superfluous spaces of the starting line - in
				 *       maxRequestLine, the extensions of a chunk - in maxChunkLine. The limits
				 *       limit the accepted stream rather than that which is left of it:
				 *       otherwise a line of the form "X:" with a stream of the spaces would stretch
				 *       unlimitedly, not bringing the parsing closer to the completion and not spending
				 *       a single limit
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Limits : parser_t::limits_t {
					/**
					 * \~russian
					 * Требование строгого окончания строк CRLF (запрет одиночного LF)
					 *
					 * @note Заодно запрещает пустые строки перед стартовой строкой запроса,
					 *       которые толерантный режим пропускает по RFC 9112 §2.2
					 *
					 * \~english
					 * Requirement of a strict ending of the lines by a CRLF (a prohibition of a single LF)
					 * @note At the same time it prohibits the empty lines before the starting line of a request
					 *       which the tolerant mode skips by RFC 9112 §2.2
					 *
					 * \~
					 */
					bool strictEOL;
					// Запрет лишних пробелов внутри стартовой строки
					bool strictSpaces;
					// Требование обязательного заголовка Host у запросов HTTP/1.1
					bool requireHost;
					// Максимальная длина строки заголовка чанка (size + chunk-ext)
					size_t maxChunkLine;
					// Максимальная длина request-line/status-line
					size_t maxRequestLine;
					// Максимальный размер одного чанка
					uint64_t maxChunkSize;
					/**
					 * \~russian
					 * @brief Метод получения строгого набора ограничений
					 *
					 * @details Готовый пресет для узлов, принимающих трафик извне
					 *          (сервер, прокси, балансировщик): включает все переключатели
					 *          строгости разбора. Толерантность к одиночному LF, лишним
					 *          пробелам и отсутствию Host безопасна только когда обе
					 *          стороны соединения трактуют их одинаково, а на границе
					 *          сети это не гарантируется - расхождение с соседним звеном
					 *          цепочки и есть механизм request smuggling
					 *
					 * @return строгий набор ограничений
					 *
					 * \~english
					 * @brief Method of getting the strict collection of the limitations
					 * @details A ready preset for the nodes accepting the traffic from the outside
					 *          (a server, a proxy, a balancer): it enables all the switches
					 *          of the strictness of the parsing. The tolerance to a single LF, to the superfluous
					 *          spaces and to the absence of a Host is safe only when both
					 *          sides of the connection treat them identically, while at the boundary
					 *          of a network this is not guaranteed - a divergence with the neighbouring link
					 *          of the chain is exactly the mechanism of a request smuggling
					 * @return strict collection of the limitations
					 *
					 * \~
					 */
					static Limits strict() noexcept;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Limits() noexcept;
				} limits_t;
			public:
				/**
				 * \~russian
				 * @brief Класс разобранного сообщения
				 *
				 * @details Если Content-Length не установлен, то значение bodySize == -1.
				 *          Если Content-Length установлен, то значение поля bodySize >= 0.
				 *          Если указан Transfer-Encoding: chunked, то значение поля bodySize == -1.
				 *
				 * \~english
				 * @brief Class of a parsed message
				 * @details If the Content-Length is not set, then the value bodySize == -1.
				 *          If the Content-Length is set, then the value of the field bodySize >= 0.
				 *          If a Transfer-Encoding: chunked is indicated, then the value of the field bodySize == -1.
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Message {
					public:
						/**
						 * \~russian
						 * @brief Структура флагов состояния сообщения
						 *
						 * \~english
						 * @brief Structure of the flags of the state of a message
						 *
						 * \~
						 */
						typedef struct __AWH_SHARED_EXPORT__ Flags {
							// Тело передаётся chunked
							bool chunked;
							/**
							 * Запрошено переключение протокола
							 *
							 * У запроса - заголовок Upgrade вместе с токеном upgrade в Connection,
							 * у ответа - код [101 Switching Protocols] либо успешный ответ на CONNECT.
							 * В запросе HTTP/1.0 заголовок Upgrade игнорируется (RFC 9110 §7.8):
							 * отправитель этой версии смену протокола на соединении, где о ней не
							 * договаривались, не поймёт
							 *
							 */
							bool upgrade;
							// Сообщение полностью разобрано
							bool complete;
							/**
							 * Соединение переиспользуемое
							 *
							 * Признак говорит только о том, что соединение не помечено к закрытию
							 * заголовком Connection и версией протокола. Он не учитывает смену
							 * протокола: после ответа 101 и после успешного 2xx на CONNECT
							 * соединение остаётся открытым, но HTTP-сообщений по нему больше нет,
							 * и признак сохраняет значение true. Решение о переиспользовании
							 * принимается по обоим признакам - сначала upgrade, затем keepAlive.
							 * Сводить их в один нельзя: false закрыл бы туннель, который вызывающая
							 * сторона обязана передать другому протоколу, а true оставил бы
							 * соединение под конвейер HTTP, которого в нём уже нет
							 */
							bool keepAlive;
							/**
							 * Клиент прислал заголовок [Expect: 100-continue] и ожидает промежуточный
							 * ответ до отправки тела
							 *
							 * В запросе HTTP/1.0 ожидание игнорируется (RFC 9110 §10.1.1): промежуточный
							 * ответ отправитель этой версии прочитает как окончательный и уйдёт в
							 * рассинхронизацию
							 *
							 */
							bool expectContinue;
							/**
							 * \~russian
							 * @brief Конструктор
							 *
							 *
							 * \~english
							 * @brief Constructor
							 *
							 * \~
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
						 * \~russian
						 * @brief Оператор перемещающего присваивания параметров сообщения
						 *
						 * @param message объект сообщения для перемещения
						 * @return        текущее сообщение
						 *
						 * \~english
						 * @brief Operator of the moving assignment of the parameters of a message
						 * @param message object of the message for the moving
						 * @return        current message
						 *
						 * \~
						 */
						Message & operator = (Message && message) noexcept;
						/**
						 * \~russian
						 * @brief Оператор присваивания параметров сообщения
						 *
						 * @param message объект сообщения для копирования
						 * @return        текущее сообщение
						 *
						 * \~english
						 * @brief Operator of the assignment of the parameters of a message
						 * @param message object of the message for the copying
						 * @return        current message
						 *
						 * \~
						 */
						Message & operator = (const Message & message) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор сравнения
						 *
						 * @note Оператор объявлен константным: разобранное сообщение выдаётся
						 *       методом message константной ссылкой, и неконстантный оператор
						 *       сравнения оказался бы неприменим ровно к тому значению, ради
						 *       которого он и существует
						 *
						 * @param message объект сообщения для сравнения
						 * @return        результат сравнения
						 *
						 * \~english
						 * @brief Operator of a comparison
						 * @note The operator is declared constant: a parsed message is issued
						 *       by the method message by a constant reference, and a non-constant operator
						 *       of the comparison would turn out to be inapplicable exactly to that value for the sake of
						 *       which it exists
						 * @param message object of the message for the comparison
						 * @return        result of the comparison
						 *
						 * \~
						 */
						bool operator == (const Message & message) const noexcept;
						/**
						 * \~russian
						 * @brief Оператор сравнения
						 *
						 * @param message объект сообщения для сравнения
						 * @return        результат сравнения
						 *
						 * \~english
						 * @brief Operator of a comparison
						 * @param message object of the message for the comparison
						 * @return        result of the comparison
						 *
						 * \~
						 */
						bool operator != (const Message & message) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор перемещения
						 *
						 * @param message объект сообщения для перемещения
						 *
						 * \~english
						 * @brief Constructor of the moving
						 * @param message object of the message for the moving
						 *
						 * \~
						 */
						Message(Message && message) noexcept;
						/**
						 * \~russian
						 * @brief Конструктор копирования
						 *
						 * @param message объект сообщения для копирования
						 *
						 * \~english
						 * @brief Constructor of the copying
						 * @param message object of the message for the copying
						 *
						 * \~
						 */
						Message(const Message & message) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Message() noexcept;
				} message_t;
			public:
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова о готовности принимать данные тела
				 *
				 * @details Вызывается когда выходной буфер опустился ниже low-water
				 *          (после частичного приёма в sendData можно отправлять дальше).
				 *          Идентификатор потока для HTTP/1.x всегда равен STREAM_ID -
				 *          сигнатура универсальна с HTTP/2.
				 *
				 * @param sid идентификатор потока (всегда STREAM_ID)
				 *
				 * \~english
				 * @brief Type of the callback function about the readiness to accept the data of the body
				 * @details It is called when the output buffer has descended below the low-water
				 *          (after a partial acceptance in sendData it is possible to send onward).
				 *          The identifier of the stream for HTTP/1.x is always equal to STREAM_ID -
				 *          the signature is universal with HTTP/2.
				 * @param sid identifier of the stream (always STREAM_ID)
				 *
				 * \~
				 */
				using writable_callback_t = function <void (const uint32_t)>;
				/**
				 * \~russian
				 * @brief Тип функции обратного вызова записи исходящих байтов в сеть
				 *
				 * @details Если установлена - парсер сам отдаёт исходящие байты сетевому слою
				 *          сразу по мере формирования. Если не установлена - исходящие байты
				 *          накапливаются во внутреннем буфере (pull-модель: pending() +
				 *          consumePending()).
				 *
				 * @param buffer буфер исходящих данных
				 * @param size   размер исходящих данных
				 *
				 * \~english
				 * @brief Type of the callback function of the writing of the outgoing octets into the network
				 * @details If it is set - the parser itself issues the outgoing octets to the network layer
				 *          at once as they are formed. If it is not set - the outgoing octets
				 *          accumulate in an internal buffer (the pull model: pending() +
				 *          consumePending()).
				 * @param buffer buffer of the outgoing data
				 * @param size   size of the outgoing data
				 *
				 * \~
				 */
				using write_callback_t = function <void (const void *, const size_t)>;
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the phase of the parsing of an HTTP message
				 * @details The sequence of the events at the parsing of a single message:
				 *          1. (BEGIN, NONE)    - the beginning of the parsing of a new message
				 *          2. (END, HEADERS)   - all the headers are parsed and interpreted
				 *          3. (BEGIN, BODY)    - the beginning of the acceptance of the body (only if the body is present)
				 *          4. (END, BODY)      - the body is fully accepted (only if the body is present)
				 *          5. (BEGIN, TRAILER) - the beginning of the parsing of the trailers (only for a chunked one)
				 *          6. (END, TRAILER)   - the trailers are parsed (only for a chunked one)
				 *          7. (END, NONE)      - the message is fully parsed
				 *          The identifier of the stream for HTTP/1.x is always equal to STREAM_ID -
				 *          the signature is universal with HTTP/2.
				 * @param sid   identifier of the stream (always STREAM_ID)
				 * @param phase phase of the parsing of the HTTP message
				 * @param part  part of the message (the headers, the trailers, the body), NONE - the message as a whole
				 * @return      result of the processing (true - to continue the parsing, false - to interrupt with the error ABORTED)
				 *
				 * \~
				 */
				using phase_callback_t = function <bool (const uint32_t, const phase_t, const part_t)>;
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the boundaries of the chunks (Transfer-Encoding: chunked)
				 * @details It is needed by the consumers to whom the framing of the body «a chunk-into-a-chunk» matters:
				 *          by the transparent proxies (a retranslation with the preservation of the source framing)
				 *          and by the protocols with the semantics of the extensions of the chunks (for example, the signatures of the chunks
				 *          in AWS S3 aws-chunked). The sequence of the events for every chunk:
				 *          1. (BEGIN, size, extension) - the line of the size of the chunk is parsed;
				 *          2. the fragments of the data of the chunk are issued through data_callback_t;
				 *          3. (END, size, "") - the data of the chunk is read to the end (the concluding CRLF is accepted).
				 *          For the last chunk (size == 0) only BEGIN is called - further the
				 *          events of the trailers follow. If the callback function is not set,
				 *          the extensions of the chunks are not accumulated (a zero overhead).
				 * @param phase     phase of the parsing of the chunk (BEGIN - the header is parsed, END - the chunk is read to the end)
				 * @param size      size of the data of the chunk
				 * @param extension raw extensions of the chunk (the content after the ';' without a CRLF), are valid ONLY for the time of the call
				 * @return          result of the processing (true - to continue the parsing, false - to interrupt with the error ABORTED)
				 *
				 * \~
				 */
				using chunk_callback_t = function <bool (const phase_t, const uint64_t, const string_view)>;
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the provider of the headers of a message
				 * @details It is called at the completion of the block of the headers, when the headers are parsed
				 *          and interpreted (the way of the framing of the body is chosen) - the same moment
				 *          as the END_HEADERS at HTTP/2. For the trailers the provider is transmitted as a nullptr.
				 *          The identifier of the stream for HTTP/1.x is always equal to STREAM_ID -
				 *          the signature is universal with HTTP/2.
				 * @param sid       identifier of the stream (always STREAM_ID)
				 * @param provider  object of the provider of the headers of the message (a nullptr for the trailers)
				 * @param endStream flag of the completion of the message (there will be no body)
				 * @return          result of the processing (true - to continue the parsing, false - to interrupt with the error ABORTED)
				 *
				 * \~
				 */
				using provider_callback_t = function <bool (const uint32_t, const provider_t *, const bool)>;
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of a fragment of the body of a message
				 * @details The buffer points into the input data (zero-copy) and is valid ONLY for the time
				 *          of the call. The fragments are issued as the data arrives from the network and do not coincide
				 *          with the boundaries of the chunks. The flag endStream is set on the fragment concluding
				 *          a body of a fixed size (Content-Length); for the bodies chunked and «up to the closing
				 *          of the connection» the end of the body at the moment of a fragment is unknown - the completion is signalled
				 *          by the provider of the trailers or by the phase (END, NONE). The identifier of the stream for HTTP/1.x
				 *          is always equal to STREAM_ID - the signature is universal with HTTP/2.
				 * @param sid       identifier of the stream (always STREAM_ID)
				 * @param buffer    buffer of the data of the body of the message
				 * @param size      size of the data of the body of the message
				 * @param endStream flag of the completion of the message (the fragment concludes the body)
				 * @return          result of the processing (true - to continue the parsing, false - to interrupt with the error ABORTED)
				 *
				 * \~
				 */
				using data_callback_t = function <bool (const uint32_t, const void *, const size_t, const bool)>;
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Type of the pull source of the data of the body of a message (for the big bodies without a superfluous copy)
				 * @details An alternative to sendData: the parser itself requests the data from the source
				 *          exactly then when there is a place in the output buffer. The source fills
				 *          the buffer (not more than cap octets), sets eof = true at the reaching of the end
				 *          of the body and returns the number of the written octets, or -1 at an error.
				 *          The identifier of the stream for HTTP/1.x is always equal to STREAM_ID -
				 *          the signature is universal with HTTP/2.
				 * @param sid    identifier of the stream (always STREAM_ID)
				 * @param buffer buffer for the filling
				 * @param cap    capacity of the buffer
				 * @param eof    flag of the reaching of the end of the body
				 * @return       number of the written octets or -1 at an error
				 *
				 * \~
				 */
				using data_source_callback_t = function <int64_t (const uint32_t, uint8_t *, const size_t, bool &)>;
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Type of the callback function for the processing of the headers or of the trailers of a message
				 * @note The headers and the trailers of a message are processed identically, therefore
				 *       one and the same type of the callback function is used. The name and the value of a header
				 *       are valid ONLY for the time of the call. The identifier of the stream for HTTP/1.x
				 *       is always equal to STREAM_ID - the signature is universal with HTTP/2.
				 * @param sid   identifier of the stream (always STREAM_ID)
				 * @param name  name of the header
				 * @param value value of the header (without the external OWS)
				 * @param part  part of the message (the headers or the trailers)
				 * @return      result of the processing (true - to continue the parsing, false - to interrupt with the error ABORTED)
				 *
				 * \~
				 */
				using header_callback_t = function <bool (const uint32_t, const string_view, const string_view, const part_t)>;
			private:
				/**
				 * \~russian
				 * @brief Структура промежуточных параметров заголовка HTTP
				 *
				 * \~english
				 * @brief Structure of the intermediate parameters of an HTTP header
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Header {
					// Накопитель имени текущего заголовка (также используется для имени метода запроса)
					string name;
					// Накопитель значения текущего заголовка
					string value;
					// Накопитель расширений текущего чанка (заполняется только при установленном chunk-callback'е)
					string chunkExt;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Header() noexcept;
				} header_t;
				/**
				 * \~russian
				 * @brief Структура для хранения статистики тела HTTP-сообщения
				 *
				 * \~english
				 * @brief Structure for the storing of the statistics of the body of an HTTP message
				 *
				 * \~
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
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Statistics_Body() noexcept;
				} statistics_body_t;
				/**
				 * \~russian
				 * @brief Структура для хранения статистики заголовков HTTP
				 *
				 * \~english
				 * @brief Structure for the storing of the statistics of the headers of HTTP
				 *
				 * \~
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
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Statistics_Headers() noexcept;
				} statistics_headers_t;
				/**
				 * \~russian
				 * @brief Структура для хранения флагов состояния парсера
				 *
				 * \~english
				 * @brief Structure for the storing of the flags of the state of the parser
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Flags {
					// Выполняется разбор трейлеров
					bool inTrailers;
					// Заголовок Upgrade получен
					bool upgradeSeen;
					// Количество полученных заголовков Host (RFC 9112 требует ровно один у HTTP/1.1)
					uint8_t hostCount;
					// Количество пропущенных октетов пустых строк перед стартовой строкой запроса
					uint8_t leadingBlanks;
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
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Flags() noexcept;
				} flags_t;
				/**
				 * \~russian
				 * @brief Структура состояния отправки исходящего сообщения
				 *
				 * \~english
				 * @brief Structure of the state of the sending of an outgoing message
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Sender {
					/**
					 * \~russian
					 * @brief Способ кадрирования тела исходящего сообщения
					 *
					 * \~english
					 * @brief Way of the framing of the body of an outgoing message
					 *
					 * \~
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
					/**
					 * \~russian
					 * Признак того, что pull-источник данных исполняется прямо сейчас
					 *
					 * @details Пока источник исполняется, объект функции принадлежит своему
					 *          же вызову: его уничтожение освободило бы память под ногами
					 *          исполняющегося кода. Признак закрывает все пути к такому
					 *          уничтожению - назначение нового источника, подготовку
					 *          отправителя и полную очистку объекта
					 *
					 * \~english
					 * Flag of the pull source of the data being executed right now
					 * @details While the source is being executed, the object of the function belongs to its
					 *          own call: its destruction would free the memory under the feet of the
					 *          executing code. The flag closes all the ways to such a
					 *          destruction - the assignment of a new source, the preparation
					 *          of the sender and the full clearing of the object
					 *
					 * \~
					 */
					bool sourceRunning;
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
					// Остаток тела до полного Content-Length (для кадрирования IDENTITY)
					uint64_t remaining;
					// Максимальный объём тела, выкачиваемый из pull-источника за одну прокачку
					uint64_t pumpLimit;
					// Буфер исходящих байтов (заголовки + кадрированное тело)
					buffer_t output;
					/**
					 * \~russian
					 * @brief Буфер байтов, отдаваемых сетевому слою в текущий момент
					 *
					 * @details Существует, чтобы отдаваемая область не могла быть перемещена
					 *          реентрантной дозаписью из функции обратного вызова записи.
					 *          Между передачами буферы обмениваются местами - выделенная
					 *          память переиспользуется и не освобождается
					 *
					 * \~english
					 * @brief Buffer of the octets issued to the network layer at the current moment
					 * @details It exists so that the issued area could not be moved
					 *          by a reentrant appending from the callback function of the writing.
					 *          Between the transmissions the buffers exchange places - the allotted
					 *          memory is reused and is not freed
					 *
					 * \~
					 */
					buffer_t flushing;
					// Pull-источник данных тела (если задан вместо sendData)
					data_source_callback_t source;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Sender() noexcept;
				} sender_t;
				/**
				 * \~russian
				 * @brief Структура для хранения функций обратного вызова
				 *
				 * \~english
				 * @brief Structure for the storing of the callback functions
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Callbacks {
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки фрагмента тела сообщения
					 *
					 * \~english
					 * @brief Callback function for the processing of a fragment of the body of a message
					 *
					 * \~
					 */
					data_callback_t data;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки фазы разбора HTTP-сообщения
					 *
					 * \~english
					 * @brief Callback function for the processing of the phase of the parsing of an HTTP message
					 *
					 * \~
					 */
					phase_callback_t phase;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки границ чанков
					 *
					 * \~english
					 * @brief Callback function for the processing of the boundaries of the chunks
					 *
					 * \~
					 */
					chunk_callback_t chunk;
					/**
					 * \~russian
					 * @brief Функция обратного вызова записи исходящих байтов в сеть
					 *
					 * \~english
					 * @brief Callback function of the writing of the outgoing octets into the network
					 *
					 * \~
					 */
					write_callback_t write;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки заголовков или трейлеров сообщения
					 *
					 * \~english
					 * @brief Callback function for the processing of the headers or of the trailers of a message
					 *
					 * \~
					 */
					header_callback_t header;
					/**
					 * \~russian
					 * @brief Функция обратного вызова для обработки провайдера заголовков сообщения
					 *
					 * \~english
					 * @brief Callback function for the processing of the provider of the headers of a message
					 *
					 * \~
					 */
					provider_callback_t provider;
					/**
					 * \~russian
					 * @brief Функция обратного вызова о готовности принимать данные тела
					 *
					 * \~english
					 * @brief Callback function about the readiness to accept the data of the body
					 *
					 * \~
					 */
					writable_callback_t writable;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Callbacks() noexcept;
				} callbacks_t;
			private:
				// Код ошибки разбора
				error_t _error;
			private:
				/**
				 * \~russian
				 * @brief Признак сброса парсера во время активного разбора
				 *
				 * @details Взводится методами reset() и clear(), вызванными из функции
				 *          обратного вызова, и заставляет parse() немедленно вернуть
				 *          управление. Без него цикл разбора продолжил бы читать
				 *          оставшиеся байты уже обнулённым конечным автоматом и принял
				 *          бы середину текущего сообщения за начало следующего
				 *
				 * \~english
				 * @brief Flag of a reset of the parser during an active parsing
				 * @details It is raised by the methods reset() and clear() called from a callback
				 *          function and forces parse() to return the control immediately.
				 *          Without it the cycle of the parsing would continue to read
				 *          the remaining octets already by a zeroed finite automaton and would take
				 *          the middle of the current message for the beginning of the next one
				 *
				 * \~
				 */
				bool _recycled;
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
				// Протокол, с которым работает парсер (HTTP/1.x, прокси либо WebSocket поверх него)
				proto_t _proto;
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
				 * \~russian
				 * @brief Метод выбора способа кадрирования тела после завершения заголовков
				 *
				 * \~english
				 * @brief Method of the choice of the way of the framing of the body after the completion of the headers
				 *
				 * \~
				 */
				void beginBody() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод завершения разбора текущего заголовка/трейлера
				 *
				 * @details Имя и значение передаются представлениями: посимвольный разбор
				 *          подаёт их из своих накопителей, быстрый путь - прямо из входного
				 *          буфера. Хвостовые OWS значения отсекаются методом самостоятельно
				 *
				 * @param name  название заголовка
				 * @param value значение заголовка
				 * @return      результат обработки (false - разбор прерван)
				 *
				 * \~english
				 * @brief Method of the completion of the parsing of the current header/trailer
				 * @details The name and the value are transmitted by the representations: the character-by-character parsing
				 *          supplies them from its accumulators, the fast path - right from the input
				 *          buffer. The trailing OWS of the value are cut off by the method itself
				 * @param name  name of the header
				 * @param value value of the header
				 * @return      result of the processing (false - the parsing is interrupted)
				 *
				 * \~
				 */
				bool commitHeader(const string_view name, string_view value) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора целой строки заголовка из входного буфера
				 *
				 * @details Быстрый путь разбора блока заголовков: когда строка заголовка
				 *          целиком присутствует во входном буфере, её имя и значение
				 *          отдаются представлениями прямо во входные данные - без
				 *          накопления в промежуточные строки. Путь берёт на себя только
				 *          безусловно корректную строку: любое отклонение (неполная
				 *          строка, недопустимый символ, превышение лимита, obs-fold)
				 *          возвращает управление посимвольному разбору, который и
				 *          зафиксирует ошибку в положенном месте
				 *
				 * @param data указатель на входные данные
				 * @param size размер входных данных
				 * @return     число потреблённых байт либо 0, если быстрый путь неприменим
				 *
				 * \~english
				 * @brief Method of the parsing of a whole line of a header out of the input buffer
				 * @details The fast path of the parsing of a block of the headers: when a line of a header
				 *          is entirely present in the input buffer, its name and value
				 *          are issued by the representations right into the input data - without
				 *          an accumulation in the intermediate strings. The path takes upon itself only
				 *          an unconditionally correct line: any deviation (an incomplete
				 *          line, an inadmissible character, an exceeding of a limit, an obs-fold)
				 *          returns the control to the character-by-character parsing which
				 *          will fix the error in the due place
				 * @param data pointer to the input data
				 * @param size size of the input data
				 * @return     number of the consumed octets or 0, if the fast path is inapplicable
				 *
				 * \~
				 */
				size_t parseHeaderLine(const char * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод разбора целой стартовой строки ответа из входного буфера
				 *
				 * @details Быстрый путь разбора стартовой строки ответа: когда строка
				 *          целиком присутствует во входном буфере, литерал версии
				 *          сверяется одним сравнением, а код состояния и пояснение
				 *          читаются прямо из входных данных - без цепочки состояний
				 *          на каждый октет. Путь берёт на себя только безусловно
				 *          корректную строку: любое отклонение (неполная строка,
				 *          одиночный LF, иное написание версии, лишние пробелы,
				 *          недопустимый символ, превышение лимита) возвращает
				 *          управление посимвольному разбору, который и зафиксирует
				 *          ошибку в положенном месте
				 *
				 * @param data указатель на входные данные
				 * @param size размер входных данных
				 * @return     число потреблённых байт либо 0, если быстрый путь неприменим
				 *
				 * \~english
				 * @brief Method of the parsing of a whole starting line of an answer out of the input buffer
				 * @details The fast path of the parsing of the starting line of an answer: when the line
				 *          is entirely present in the input buffer, the literal of the version
				 *          is compared by a single comparison, while the status code and the explanation
				 *          are read right from the input data - without a chain of the states
				 *          per octet. The path takes upon itself only an unconditionally
				 *          correct line: any deviation (an incomplete line,
				 *          a single LF, another spelling of the version, superfluous spaces,
				 *          an inadmissible character, an exceeding of a limit) returns
				 *          the control to the character-by-character parsing which will fix
				 *          the error in the due place
				 * @param data pointer to the input data
				 * @param size size of the input data
				 * @return     number of the consumed octets or 0, if the fast path is inapplicable
				 *
				 * \~
				 */
				size_t parseStatusLine(const char * data, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод завершения разбора стартовой строки (request-line/status-line)
				 *
				 * @return результат обработки (false - разбор прерван)
				 *
				 * \~english
				 * @brief Method of the completion of the parsing of the starting line (a request-line/status-line)
				 * @return result of the processing (false - the parsing is interrupted)
				 *
				 * \~
				 */
				bool commitStartLine() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод завершения разбора всего сообщения
				 *
				 * \~english
				 * @brief Method of the completion of the parsing of the whole message
				 *
				 * \~
				 */
				void completeMessage() noexcept;
				/**
				 * \~russian
				 * @brief Метод завершения разбора строки размера чанка
				 *
				 * \~english
				 * @brief Method of the completion of the parsing of the line of the size of a chunk
				 *
				 * \~
				 */
				void chunkSizeComplete() noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки отсутствия тела у ответа сервера (по статус-коду и методу запроса)
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking the absence of a body at an answer of a server (by the status code and the method of the request)
				 * @return result of the checking
				 *
				 * \~
				 */
				bool responseHasNoBody() const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод фиксации ошибки разбора (код ошибки, итоговый статус и запись в лог)
				 *
				 * @param error код ошибки разбора
				 *
				 * \~english
				 * @brief Method of the fixation of an error of the parsing (the error code, the resulting status and a record into the log)
				 * @param error error code of the parsing
				 *
				 * \~
				 */
				void fail(const error_t error) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод передачи исходящих байтов сетевому слою через функцию обратного вызова записи
				 *
				 * @details Если функция записи не установлена - байты остаются во внутреннем
				 *          буфере до выборки через pending()/consumePending().
				 *
				 * \~english
				 * @brief Method of the transmission of the outgoing octets to the network layer through the callback function of the writing
				 * @details If the function of the writing is not set - the octets remain in the internal
				 *          buffer until the selection through pending()/consumePending().
				 *
				 * \~
				 */
				void flush() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения логического объёма ещё не отправленных исходящих байтов
				 *
				 * @return объём не отправленных исходящих байтов
				 *
				 * \~english
				 * @brief Method of getting the logical volume of the not yet sent outgoing octets
				 * @return volume of the not sent outgoing octets
				 *
				 * \~
				 */
				size_t outputPending() const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод дозагрузки выходного буфера из pull-источника данных (если он задан)
				 *
				 * @details Источник пишет напрямую в выходной буфер (без промежуточной копии),
				 *          кадрирование тела применяется к каждой полученной порции.
				 *
				 * @return число полученных от источника байт тела
				 *
				 * \~english
				 * @brief Method of the loading of the output buffer from the pull source of the data (if it is given)
				 * @details The source writes directly into the output buffer (without an intermediate copy),
				 *          the framing of the body is applied to every obtained portion.
				 * @return number of the octets of the body obtained from the source
				 *
				 * \~
				 */
				size_t refillFromSource() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод прокачки pull-источника данных в сеть
				 *
				 * @details В push-режиме (установлена функция обратного вызова записи) качает
				 *          источник до конца тела либо до временного отсутствия данных.
				 *          В pull-режиме наполняет выходной буфер до high-water однократно -
				 *          досылка происходит по мере выборки consumePending().
				 *
				 * \~english
				 * @brief Method of the pumping of the pull source of the data into the network
				 * @details In the push mode (the callback function of the writing is set) it pumps
				 *          the source up to the end of the body or up to a temporary absence of the data.
				 *          In the pull mode it fills the output buffer up to the high-water once -
				 *          the further sending happens as consumePending() selects.
				 *
				 * \~
				 */
				void pumpSource() noexcept;
				/**
				 * \~russian
				 * @brief Метод сигнализации о готовности принимать данные (один раз на провал буфера)
				 *
				 * \~english
				 * @brief Method of the signalling about the readiness to accept the data (once per descent of the buffer)
				 *
				 * \~
				 */
				void maybeNotifyWritable() noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод завершения тела исходящего сообщения (финализация кадрирования)
				 *
				 * \~english
				 * @brief Method of the completion of the body of an outgoing message (the finalization of the framing)
				 *
				 * \~
				 */
				void finishBody() noexcept;
				/**
				 * \~russian
				 * @brief Метод кадрирования и записи порции тела в выходной буфер
				 *
				 * @param buffer буфер данных тела
				 * @param size   размер данных тела
				 *
				 * \~english
				 * @brief Method of the framing and of the writing of a portion of the body into the output buffer
				 * @param buffer buffer of the data of the body
				 * @param size   size of the data of the body
				 *
				 * \~
				 */
				void frameBody(const void * buffer, const size_t size) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова обработки фазы разбора
				 *
				 * @param phase фаза разбора HTTP-сообщения
				 * @param part  часть сообщения
				 * @return      результат обработки (false - разбор прерван с ошибкой ABORTED)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the processing of the phase of the parsing
				 * @param phase phase of the parsing of the HTTP message
				 * @param part  part of the message
				 * @return      result of the processing (false - the parsing is interrupted with the error ABORTED)
				 *
				 * \~
				 */
				bool firePhase(const phase_t phase, const part_t part) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова обработки границ чанков
				 *
				 * @param phase фаза разбора чанка
				 * @param size  размер данных чанка
				 * @return      результат обработки (false - разбор прерван с ошибкой ABORTED)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the processing of the boundaries of the chunks
				 * @param phase phase of the parsing of the chunk
				 * @param size  size of the data of the chunk
				 * @return      result of the processing (false - the parsing is interrupted with the error ABORTED)
				 *
				 * \~
				 */
				bool fireChunk(const phase_t phase, const uint64_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод вызова функции обратного вызова обработки провайдера заголовков сообщения
				 *
				 * @param provider  объект провайдера заголовков сообщения (nullptr для трейлеров)
				 * @param endStream флаг завершения сообщения (тела не будет)
				 * @return          результат обработки (false - разбор прерван с ошибкой ABORTED)
				 *
				 * \~english
				 * @brief Method of the call of the callback function of the processing of the provider of the headers of a message
				 * @param provider  object of the provider of the headers of the message (a nullptr for the trailers)
				 * @param endStream flag of the completion of the message (there will be no body)
				 * @return          result of the processing (false - the parsing is interrupted with the error ABORTED)
				 *
				 * \~
				 */
				bool fireProvider(const provider_t * provider, const bool endStream) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод интерпретации заголовка Connection
				 *
				 * @param begin начало значения заголовка
				 * @param end   конец значения заголовка
				 *
				 * \~english
				 * @brief Method of the interpretation of the header Connection
				 * @param begin beginning of the value of the header
				 * @param end   end of the value of the header
				 *
				 * \~
				 */
				void applyConnection(const char * begin, const char * end) noexcept;
				/**
				 * \~russian
				 * @brief Метод интерпретации заголовка Host
				 *
				 * @details Проверяется отсутствие пробельных символов внутри значения.
				 *          Полная грамматика uri-host не разбирается намеренно: она
				 *          допускает и IP-литералы, и интернационализированные имена,
				 *          и отвергать по ней означало бы ломать законный трафик
				 *
				 * @note Исход возвращается наружу, как и у Content-Length: негодное
				 *       значение прерывает обработку поля до вызова функции обратного
				 *       вызова заголовков. Иначе вызывающая сторона получала бы
				 *       заведомо битый заголовок и узнавала о его негодности лишь по
				 *       итогу разбора - прокси успел бы передать его дальше
				 *
				 * @param begin начало значения заголовка
				 * @param end   конец значения заголовка
				 * @return      результат интерпретации
				 *
				 * \~english
				 * @brief Method of the interpretation of the header Host
				 * @details The absence of the space characters inside the value is checked.
				 *          The full grammar of a uri-host is not parsed deliberately: it
				 *          admits both the IP literals and the internationalized names,
				 *          and to reject by it would mean to break the lawful traffic
				 * @note The outcome is returned outside, as at the Content-Length: an unsuitable
				 *       value interrupts the processing of the field before the call of the callback
				 *       function of the headers. Otherwise the calling side would get
				 *       a knowingly broken header and would learn about its unsuitability only by
				 *       the result of the parsing - a proxy would have managed to pass it onward
				 * @param begin beginning of the value of the header
				 * @param end   end of the value of the header
				 * @return      result of the interpretation
				 *
				 * \~
				 */
				bool applyHost(const char * begin, const char * end) noexcept;
				/**
				 * \~russian
				 * @brief Метод сверки адресата заголовка Host с адресатом цели запроса
				 *
				 * @details Цель в absolute-form несёт адресата сама, и тогда в запросе их
				 *          оказывается два. Получателю предписано брать адресата из цели и
				 *          заголовок игнорировать (RFC 9112 §3.2.2), а клиенту - присылать
				 *          заголовок, совпадающий с адресатом цели (RFC 9110 §7.2).
				 *          Соблюдают это не все звенья цепочки, и расхождение означает, что
				 *          соседние узлы считают адресатом одного запроса разные узлы.
				 *          Сверяются имя узла без учёта регистра и порт с подстановкой
				 *          стандартного для схемы: "anyks.com" и "anyks.com:80" при схеме
				 *          http обозначают один узел. Полная грамматика URI не разбирается -
				 *          она принадлежит слою URI, а не сборщику сообщений
				 *
				 * @param begin начало значения заголовка Host
				 * @param end   конец значения заголовка Host
				 * @return      результат сверки
				 *
				 * \~english
				 * @brief Method of the comparison of the addressee of the header Host with the addressee of the target of the request
				 * @details A target in the absolute-form carries the addressee itself, and then in a request there
				 *          turn out to be two of them. A receiver is prescribed to take the addressee from the target and
				 *          to ignore the header (RFC 9112 §3.2.2), while a client - to send
				 *          a header coinciding with the addressee of the target (RFC 9110 §7.2).
				 *          Not all the links of a chain observe this, and a divergence means that
				 *          the neighbouring nodes consider different nodes to be the addressee of one request.
				 *          The name of the node is compared without the account of the case and the port with a substitution
				 *          of the standard one for the scheme: "anyks.com" and "anyks.com:80" at the scheme
				 *          http designate one node. The full grammar of a URI is not parsed -
				 *          it belongs to the layer of the URI rather than to the assembler of the messages
				 * @param begin beginning of the value of the header Host
				 * @param end   end of the value of the header Host
				 * @return      result of the comparison
				 *
				 * \~
				 */
				bool checkTargetHost(const char * begin, const char * end) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод проверки пригодности полей исходящего сообщения к отправке
				 *
				 * @details Названия полей обязаны быть токенами, значения - состоять только
				 *          из HTAB / SP / VCHAR / obs-text (RFC 9110 §5.1, §5.5). Проверка
				 *          выполняется по тем же таблицам, по которым поля разбираются на
				 *          приёме. Непригодное поле отвергает сообщение целиком, а не
				 *          вычищается: CR или LF внутри значения расщепляет собираемое
				 *          сообщение на два, а молчаливое удаление поля меняет смысл того,
				 *          что вызывающая сторона просила отправить - и о заголовке
				 *          авторизации, и о заголовке кадрирования она узнала бы только
				 *          по последствиям
				 *
				 * @param headers контейнер заголовков исходящего сообщения
				 * @return        результат проверки
				 *
				 * \~english
				 * @brief Method of checking the suitability of the fields of an outgoing message for the sending
				 * @details The names of the fields are obliged to be tokens, the values - to consist only
				 *          of HTAB / SP / VCHAR / obs-text (RFC 9110 §5.1, §5.5). The checking
				 *          is performed by the same tables by which the fields are parsed at the
				 *          acceptance. An unsuitable field rejects the message as a whole rather than
				 *          being cleaned out: a CR or an LF inside a value splits the message being assembled
				 *          into two, while a silent removal of a field changes the sense of that
				 *          which the calling side has asked to send - both about a header
				 *          of the authorization and about a header of the framing it would learn only
				 *          by the consequences
				 * @param headers container of the headers of the outgoing message
				 * @return        result of the checking
				 *
				 * \~
				 */
				bool checkOutgoingFields(const headers_t & headers) const noexcept;
				/**
				 * \~russian
				 * @brief Метод проверки пригодности стартовой строки исходящего сообщения к отправке
				 *
				 * @details У запроса проверяются метод и цель (RFC 9112 §3), у ответа - код
				 *          состояния и пояснение к нему (RFC 9112 §4). Дополнительно
				 *          проверяется наличие заголовка Host у запроса HTTP/1.1
				 *          (RFC 9110 §7.2). Непригодная стартовая строка отвергает сообщение:
				 *          заменить её нечем, а отправленное сообщение получатель обязан
				 *          отвергнуть либо прочитает как два разных
				 *
				 * @param headers контейнер заголовков исходящего сообщения
				 * @return        результат проверки
				 *
				 * \~english
				 * @brief Method of checking the suitability of the starting line of an outgoing message for the sending
				 * @details At a request the method and the target are checked (RFC 9112 §3), at an answer - the status
				 *          code and the explanation to it (RFC 9112 §4). Additionally
				 *          the presence of the header Host at a request of HTTP/1.1 is checked
				 *          (RFC 9110 §7.2). An unsuitable starting line rejects the message:
				 *          there is nothing to replace it with, while a sent message the receiver is obliged
				 *          to reject or will read as two different ones
				 * @param headers container of the headers of the outgoing message
				 * @return        result of the checking
				 *
				 * \~
				 */
				bool checkOutgoingStartLine(const headers_t & headers) const noexcept;
				/**
				 * \~russian
				 * @brief Метод интерпретации заголовка Expect
				 *
				 * @details Значение разбирается как список: клиент вправе прислать
				 *          "100-continue" в любом регистре и в сопровождении других
				 *          ожиданий, а также с параметрами после ";"
				 *
				 * @param begin начало значения заголовка
				 * @param end   конец значения заголовка
				 *
				 * \~english
				 * @brief Method of the interpretation of the header Expect
				 * @details The value is parsed as a list: a client is entitled to send
				 *          a "100-continue" in any case and in an accompaniment of other
				 *          expectations, and also with the parameters after a ";"
				 * @param begin beginning of the value of the header
				 * @param end   end of the value of the header
				 *
				 * \~
				 */
				void applyExpect(const char * begin, const char * end) noexcept;
				/**
				 * \~russian
				 * @brief Метод интерпретации заголовка Content-Length
				 *
				 * @param begin начало значения заголовка
				 * @param end   конец значения заголовка
				 * @return      результат интерпретации
				 *
				 * \~english
				 * @brief Method of the interpretation of the header Content-Length
				 * @param begin beginning of the value of the header
				 * @param end   end of the value of the header
				 * @return      result of the interpretation
				 *
				 * \~
				 */
				bool applyContentLength(const char * begin, const char * end) noexcept;
				/**
				 * \~russian
				 * @brief Метод интерпретации заголовка Transfer-Encoding (накопительно по нескольким заголовкам)
				 *
				 * @param begin начало значения заголовка
				 * @param end   конец значения заголовка
				 *
				 * \~english
				 * @brief Method of the interpretation of the header Transfer-Encoding (accumulatively over several headers)
				 * @param begin beginning of the value of the header
				 * @param end   end of the value of the header
				 *
				 * \~
				 */
				void applyTransferEncoding(const char * begin, const char * end) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Правило принадлежности поля функциям жизненного цикла
				 *
				 * @details Каждое поле объекта принадлежит ровно одной из трёх категорий, и
				 *          категория определяет, какие функции обязаны его трогать. Правило
				 *          записано здесь потому, что его нарушение не видно ни в одном
				 *          отдельно взятом методе: поле добавляют в объект, а внести его в
				 *          нужную функцию забывают, и объект работает с настройками, которых
				 *          ему не давали, либо теряет данные ими же:
				 *
				 *          - настройка соединения (лимиты, протокол работы, пороги выходного
				 *            буфера, объём прокачки источника, функции обратного вызова):
				 *            переносится clone(), возвращается к умолчанию clear(), reset()
				 *            не трогает. Соединения обслуживают клоны, а настраивают фабрику,
				 *            поэтому забытое в clone() поле выключает у клона всё, что оно
				 *            включает - молча и на всех соединениях сразу;
				 *          - состояние соединения (состояние отправки, выходной буфер,
				 *            источник данных тела): clone() не переносит - клон получает
				 *            чистый объект, - clear() сбрасывает, reset() не трогает;
				 *          - состояние сообщения (состояние конечного автомата, флаги,
				 *            накопители, статистика, провайдер, метод ожидаемого ответа):
				 *            сбрасывается reset(), а значит и clear() - он вызывает reset()
				 *            первым делом.
				 *
				 *          Проверять перенос настройки полагается по поведению, а не по
				 *          геттеру: совпадение значений не означает, что настройка применена.
				 *
				 * \~english
				 * @brief Rule of the belonging of a field to the functions of the life cycle
				 * @details Every field of the object belongs to exactly one of the three categories, and
				 *          the category determines which functions are obliged to touch it. The rule
				 *          is written here because its violation is visible in not a single
				 *          separately taken method: a field is added into the object, while to enter it into
				 *          the needed function is forgotten, and the object works with the settings which
				 *          have not been given to it, or loses the data by the same ones:
				 *          - the setting of the connection (the limits, the protocol of the work, the thresholds of the output
				 *            buffer, the volume of the pumping of the source, the callback functions):
				 *            is carried over by clone(), is returned to the default by clear(), reset()
				 *            does not touch it. The connections are served by the clones, while the factory is configured,
				 *            therefore a field forgotten in clone() disables at the clone everything that it
				 *            enables - silently and on all the connections at once;
				 *          - the state of the connection (the state of the sending, the output buffer,
				 *            the source of the data of the body): clone() does not carry it over - the clone gets
				 *            a clean object, - clear() resets it, reset() does not touch it;
				 *          - the state of the message (the state of the finite automaton, the flags,
				 *            the accumulators, the statistics, the provider, the method of the expected answer):
				 *            is reset by reset(), and therefore by clear() as well - it calls reset()
				 *            first of all.
				 *          To check the carrying over of a setting is due by the behaviour rather than by a
				 *          getter: a coincidence of the values does not mean that the setting is applied.
				 *
				 * \~
				 */
				/**
				 * \~russian
				 * @brief Метод полной очистки всех данных парсера
				 *
				 * @details Помимо сброса состояния разбора возвращает лимиты безопасности и
				 *          протокол работы к значениям по умолчанию, полностью сбрасывает
				 *          состояние отправки вместе с порогами выходного буфера и удаляет
				 *          установленные функции обратного вызова. Объект после него
				 *          неотличим от только что построенного.
				 *
				 * \~english
				 * @brief Method of a full clearing of all the data of the parser
				 * @details Besides the reset of the state of the parsing it returns the limits of the safety and
				 *          the protocol of the work to the values by default, fully resets
				 *          the state of the sending together with the thresholds of the output buffer and removes
				 *          the set callback functions. The object after it is
				 *          indistinguishable from a just built one.
				 *
				 * \~
				 */
				void clear() noexcept override;
				/**
				 * \~russian
				 * @brief Метод сброса парсера для разбора следующего сообщения в том же соединении
				 *
				 * @details Дешёвый сброс между сообщениями (keep-alive/pipelining): сохраняет лимиты
				 *          безопасности и установленные функции обратного вызова, провайдер заголовков
				 *          не пересоздаётся, а очищается (переиспользуется выделенная память).
				 *          Метод запроса, установленный через method(), сбрасывается в NONE -
				 *          для направления RESPONSE выставляйте его заново перед каждым ответом.
				 *
				 * \~english
				 * @brief Method of the reset of the parser for the parsing of the next message in the same connection
				 * @details A cheap reset between the messages (keep-alive/pipelining): it preserves the limits
				 *          of the safety and the set callback functions, the provider of the headers
				 *          is not recreated but cleared (the allotted memory is reused).
				 *          The method of the request set through method() is reset to NONE -
				 *          for the direction RESPONSE set it anew before every answer.
				 *
				 * \~
				 */
				void reset() noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод установки метода запроса, которому соответствует ожидаемый ответ
				 *
				 * @details Используется ТОЛЬКО для направления RESPONSE: парсер ответа сам не может
				 *          узнать, на какой запрос пришёл ответ, а метод запроса влияет на кадрирование
				 *          тела (ответ на HEAD содержит Content-Length, но тела не имеет; успешный 2xx
				 *          ответ на CONNECT открывает туннель и тела не имеет). На сборку исходящего
				 *          ответа метод влияет так же, как и на разбор входящего: ответ на HEAD тела
				 *          не несёт, а успешный ответ на CONNECT открывает туннель - см. sendHeaders.
				 *          Сбрасывается в NONE при reset() - выставляйте заново перед каждым ответом
				 *          в keep-alive/конвейере. Клонированием переносится наравне с протоколом
				 *          работы и лимитами.
				 *
				 * @note Подготовка отправителя к следующему сообщению (resetSender) метод НЕ
				 *       сбрасывает: она не трогает состояние разбора, а метод принадлежит
				 *       обоим путям сразу. В конвейере ответов это значит, что установленный
				 *       однажды HEAD либо CONNECT продолжит действовать и на следующие ответы -
				 *       выставляйте метод заново перед каждым из них, иначе обычный ответ
				 *       окажется собран без тела либо туннелем.
				 *
				 * @param method метод запроса клиента
				 *
				 * \~english
				 * @brief Method of setting the method of the request to which the expected answer corresponds
				 * @details It is used ONLY for the direction RESPONSE: the parser of an answer cannot itself
				 *          learn to which request the answer has come, while the method of the request influences the framing
				 *          of the body (an answer to a HEAD contains a Content-Length but has no body; a successful 2xx
				 *          answer to a CONNECT opens a tunnel and has no body). The method influences the assembly of an outgoing
				 *          answer the same as the parsing of an incoming one: an answer to a HEAD carries no body,
				 *          while a successful answer to a CONNECT opens a tunnel - see sendHeaders.
				 *          It is reset to NONE at reset() - set it anew before every answer
				 *          in a keep-alive/pipeline. By a cloning it is carried over on a par with the protocol
				 *          of the work and the limits.
				 * @note The preparation of the sender for the next message (resetSender) does NOT reset
				 *       the method: it does not touch the state of the parsing, while the method belongs to
				 *       both paths at once. In a pipeline of the answers this means that a once set
				 *       HEAD or CONNECT will continue to be in force on the following answers as well -
				 *       set the method anew before every one of them, otherwise an ordinary answer
				 *       will turn out to be assembled without a body or as a tunnel.
				 * @param method method of the request of the client
				 *
				 * \~
				 */
				void method(const method_t method) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения протокола, с которым работает парсер
				 *
				 * @return протокол работы парсера
				 *
				 * \~english
				 * @brief Method of getting the protocol with which the parser works
				 * @return protocol of the work of the parser
				 *
				 * \~
				 */
				proto_t proto() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки протокола, с которым работает парсер
				 *
				 * @details Парсер говорит только на HTTP/1.0 и HTTP/1.1, поэтому
				 *          допустимы лишь значения этого семейства: HTTP1 - прямое
				 *          соединение с узлом, PROXY1 - соединение с прокси-сервером,
				 *          WEBSOCKET1 - соединение, которое предполагается переключить
				 *          на WebSocket. Значение любого другого семейства отвергается
				 *          с записью в лог: разбирать HTTP/2 этот парсер не умеет, и
				 *          молчаливое принятие такого указания создало бы у вызывающей
				 *          стороны ложное представление о происходящем.
				 *          По умолчанию установлен HTTP1.
				 *
				 * @note Режимы PROXY1 и WEBSOCKET1 ужесточают кадрирование: узел,
				 *       передающий сообщение дальше по цепочке, и соединение, которому
				 *       предстоит переключение протокола, обязаны отвергать то, границу
				 *       чего соседнее звено определит иначе.
				 *       PROXY1 отвергает объявленное кадрирование у ответов 1xx и 204,
				 *       кодирование без завершающего chunked и расхождение адресата
				 *       цели запроса с заголовком Host.
				 *       WEBSOCKET1 отвергает объявленное кадрирование у ответа
				 *       [101 Switching Protocols].
				 *       Оба отвергают запрос с заголовком Upgrade, объявляющий тело:
				 *       за пустой строкой ответа 101 начинается поток нового протокола,
				 *       и объявленное тело делает точку переключения предметом догадки.
				 *       Подробности - в описании limits_t
				 *
				 * @param proto протокол работы парсера
				 *
				 * \~english
				 * @brief Method of setting the protocol with which the parser works
				 * @details The parser speaks only HTTP/1.0 and HTTP/1.1, therefore
				 *          only the values of this family are admissible: HTTP1 - a direct
				 *          connection with a node, PROXY1 - a connection with a proxy server,
				 *          WEBSOCKET1 - a connection which is supposed to be switched
				 *          to a WebSocket. A value of any other family is rejected
				 *          with a record into the log: this parser is not able to parse HTTP/2, and
				 *          a silent acceptance of such an indication would create at the calling
				 *          side a false notion of what is happening.
				 *          By default HTTP1 is set.
				 * @note The modes PROXY1 and WEBSOCKET1 toughen the framing: a node
				 *       passing a message onward along the chain, and a connection which
				 *       is to have a switching of the protocol, are obliged to reject that the boundary
				 *       of which the neighbouring link will determine differently.
				 *       PROXY1 rejects an announced framing at the answers 1xx and 204,
				 *       an encoding without a concluding chunked and a divergence of the addressee
				 *       of the target of a request with the header Host.
				 *       WEBSOCKET1 rejects an announced framing at the answer
				 *       [101 Switching Protocols].
				 *       Both reject a request with the header Upgrade announcing a body:
				 *       after the empty line of the answer 101 the stream of a new protocol begins,
				 *       and an announced body makes the point of the switching a subject of a guess.
				 *       The details are in the description of limits_t
				 * @param proto protocol of the work of the parser
				 *
				 * \~
				 */
				void proto(const proto_t proto) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод клонирования объекта парсера
				 *
				 * @details Клон получает те же направление трафика, протокол работы, лимиты
				 *          безопасности, метод ожидаемого ответа, функции обратного вызова и
				 *          пороги выходного буфера, но чистое состояние разбора
				 *          ("фабрика с теми же настройками").
				 *
				 * @return копия объекта парсера
				 *
				 * \~english
				 * @brief Method of cloning the object of the parser
				 * @details The clone gets the same direction of the traffic, protocol of the work, limits
				 *          of the safety, method of the expected answer, callback functions and
				 *          thresholds of the output buffer, but a clean state of the parsing
				 *          («a factory with the same settings»).
				 * @return copy of the object of the parser
				 *
				 * \~
				 */
				unique_ptr <parser_t> clone() const noexcept override;
			public:
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Method of notifying the parser about the completion of the stream of the data (the closing of the connection)
				 * @details It is required for the messages the body of which is framed by the closing of the connection:
				 *          the answers of HTTP/1.0 and the answers without a Content-Length and without a Transfer-Encoding: chunked.
				 *          Such messages have no marker of the end of the body in the protocol - the end is determined
				 *          only by the closing of the connection by the remote side. The network layer is obliged to call
				 *          this method when the connection is closed (a FIN/EOF of the socket is obtained):
				 *          - if the parser reads the body «up to the closing of the connection» - the message is marked
				 *            completed (status() == COMPLETE);
				 *          - if the parser is between the messages - nothing happens
				 *            (a normal closing of a keep-alive connection);
				 *          - if the message is parsed partially (the headers or an under-read body
				 *            with a Content-Length) - the error PREMATURE_EOF is fixed (a break of the connection).
				 *
				 * \~
				 */
				void eof() noexcept override;
				/**
				 * \~russian
				 * @brief Метод разбора данных
				 *
				 * @details Потребляет столько байтов, сколько смог, и возвращает их число.
				 *          Итоговый статус необходимо контролировать методом status():
				 *          - PARTIAL:  данные приняты, сообщение не завершено - нужно ещё байтов;
				 *          - COMPLETE: сообщение полностью разобрано, разбор остановлен ровно на границе
				 *                      сообщения - для конвейерных (pipelined) сообщений вызовите reset()
				 *                      и затем parse() на оставшемся хвосте буфера;
				 *          - ERROR:    ошибка разбора/безопасности - причина в методе error().
				 *          На статусе ERROR возвращаемое значение - позиция, на которой разбор
				 *          остановлен: она годится для диагностики, но побайтово одинаковой при
				 *          разной нарезке входа не гарантируется. Крупноблочные пути указывают
				 *          на начало отвергнутого участка, посимвольный - на сам отвергнутый
				 *          октет, и приводить их к общему знаменателю незачем: продолжать разбор
				 *          после ошибки нельзя, соединение подлежит закрытию. Одинаковыми при
				 *          любой нарезке остаются статус и код ошибки.
				 *          Пустые строки перед стартовой строкой запроса пропускаются
				 *          (RFC 9112 §2.2): устаревшие клиенты дописывают лишний CRLF после
				 *          тела, и без пропуска соединение keep-alive обрывалось бы на ровном
				 *          месте. Правило адресовано серверу, поэтому к ответам не относится,
				 *          выключается переключателем strictEOL и ограничено по числу октетов.
				 *          Код состояния ответа отдаётся вызывающей стороне любым трёхзначным
				 *          (RFC 9112 §4 определяет его как 3DIGIT), включая недопустимые вне
				 *          диапазона 100..599. Отвергать такой ответ парсер не вправе:
				 *          RFC 9110 §15 предписывает обрабатывать его так, как если бы код
				 *          принадлежал классу 5xx, - то есть принять сообщение и кадрировать
				 *          тело по обычным правилам. Подмена класса остаётся за вызывающей
				 *          стороной: парсер не знает, чем для неё является ответ с таким кодом.
				 *
				 * @note Метод не является реентрантным: вызывать его повторно на том же объекте
				 *       из функции обратного вызова недопустимо - вложенный разбор двигал бы
				 *       тот же конечный автомат, и внешний вызов продолжился бы по состоянию,
				 *       которого не ожидает. Чтобы подать следующие данные, дождитесь возврата
				 *       из parse(). Методы стороны отправки из функций обратного вызова
				 *       разрешены: выходной буфер реентрантную дозапись выдерживает
				 *
				 * @note Сброс парсера методами reset() и clear() из функции обратного вызова
				 *       разрешён и немедленно прерывает разбор: метод возвращает число байт,
				 *       потреблённых до сброса, со статусом PARTIAL и без кода ошибки.
				 *       Оставшийся хвост прерванного сообщения парсеру более не принадлежит -
				 *       границу следующего сообщения после сброса определить нечем, поэтому
				 *       вызывающая сторона обязана либо отбросить хвост, либо закрыть
				 *       соединение. Чтобы прервать разбор без сброса состояния, верните
				 *       из функции обратного вызова false - это даёт ошибку ABORTED
				 *
				 * @param buffer буфер данных для разбора
				 * @param size   размер данных для разбора
				 * @return       количество обработанных байт данных
				 *
				 * \~english
				 * @brief Method of parsing the data
				 * @details It consumes as many octets as it has been able to and returns their number.
				 *          The resulting status is necessary to control by the method status():
				 *          - PARTIAL:  the data is accepted, the message is not completed - more octets are needed;
				 *          - COMPLETE: the message is fully parsed, the parsing is stopped exactly at the boundary
				 *                      of the message - for the pipelined messages call reset()
				 *                      and then parse() on the remaining tail of the buffer;
				 *          - ERROR:    an error of the parsing/of the safety - the reason is in the method error().
				 *          At the status ERROR the returned value is the position at which the parsing
				 *          is stopped: it is suitable for a diagnostics, but is not guaranteed to be octet-by-octet identical at
				 *          a different cutting of the input. The large-block paths point
				 *          to the beginning of the rejected section, the character-by-character one - to the rejected
				 *          octet itself, and there is no point in bringing them to a common denominator: to continue the parsing
				 *          after an error is impossible, the connection is subject to a closing. Identical at
				 *          any cutting remain the status and the error code.
				 *          The empty lines before the starting line of a request are skipped
				 *          (RFC 9112 §2.2): the outdated clients append a superfluous CRLF after
				 *          the body, and without the skipping a keep-alive connection would break out of the
				 *          blue. The rule is addressed to a server, therefore it does not relate to the answers,
				 *          is disabled by the switch strictEOL and is limited in the number of the octets.
				 *          The status code of an answer is issued to the calling side as any three-digit one
				 *          (RFC 9112 §4 determines it as a 3DIGIT), including the inadmissible ones outside
				 *          the range 100..599. The parser is not entitled to reject such an answer:
				 *          RFC 9110 §15 prescribes to process it as if the code
				 *          belonged to the class 5xx, - that is to accept the message and to frame
				 *          the body by the usual rules. The substitution of the class remains at the calling
				 *          side: the parser does not know what an answer with such a code is for it.
				 * @note The method is not a reentrant one: to call it repeatedly on the same object
				 *       from a callback function is inadmissible - a nested parsing would move
				 *       the same finite automaton, and the external call would continue by a state
				 *       which it does not expect. To supply the following data, wait for the return
				 *       from parse(). The methods of the side of the sending from the callback functions
				 *       are allowed: the output buffer withstands a reentrant appending
				 * @note The reset of the parser by the methods reset() and clear() from a callback function
				 *       is allowed and immediately interrupts the parsing: the method returns the number of the octets
				 *       consumed before the reset, with the status PARTIAL and without an error code.
				 *       The remaining tail of the interrupted message no longer belongs to the parser -
				 *       after the reset there is nothing to determine the boundary of the next message with, therefore
				 *       the calling side is obliged either to discard the tail or to close
				 *       the connection. To interrupt the parsing without a reset of the state, return
				 *       false from the callback function - this gives the error ABORTED
				 * @param buffer buffer of the data for the parsing
				 * @param size   size of the data for the parsing
				 * @return       number of the processed octets of the data
				 *
				 * \~
				 */
				size_t parse(const void * buffer, const size_t size) noexcept override;
			public:
				/**
				 * \~russian
				 * @brief Метод получения кода ошибки разбора
				 *
				 * @return код ошибки
				 *
				 * \~english
				 * @brief Method of getting the error code of the parsing
				 * @return error code
				 *
				 * \~
				 */
				error_t error() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения человекочитаемого названия текущей ошибки разбора
				 *
				 * @return название текущей ошибки разбора
				 *
				 * \~english
				 * @brief Method of getting the human-readable name of the current error of the parsing
				 * @return name of the current error of the parsing
				 *
				 * \~
				 */
				string_view errorName() const noexcept override;
				/**
				 * \~russian
				 * @brief Метод получения человекочитаемого названия кода ошибки
				 *
				 * @param error код ошибки разбора
				 * @return      название кода ошибки
				 *
				 * \~english
				 * @brief Method of getting the human-readable name of an error code
				 * @param error error code of the parsing
				 * @return      name of the error code
				 *
				 * \~
				 */
				static string_view errorName(const error_t error) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения лимитов безопасности
				 *
				 * @return лимиты безопасности
				 *
				 * \~english
				 * @brief Method of getting the limits of the safety
				 * @return limits of the safety
				 *
				 * \~
				 */
				const limits_t & limits() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки лимитов безопасности
				 *
				 * @param limits лимиты безопасности
				 *
				 * \~english
				 * @brief Method of setting the limits of the safety
				 * @param limits limits of the safety
				 *
				 * \~
				 */
				void limits(const limits_t & limits) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения разобранного сообщения
				 *
				 * @return разобранное сообщение
				 *
				 * \~english
				 * @brief Method of getting the parsed message
				 * @return parsed message
				 *
				 * \~
				 */
				const message_t & message() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод сброса состояния отправки для следующего сообщения в том же соединении
				 *
				 * @details Готовит отправитель к следующему сообщению (keep-alive): сбрасывает
				 *          кадрирование, источник данных и флаги, но НЕ трогает неотправленный
				 *          остаток выходного буфера. Состояние разбора не затрагивается -
				 *          для него используется reset().
				 *
				 * @note Незавершённое исходящее сообщение подготовку отменяет: сброс снял бы
				 *       признак завершённости, и следующий sendHeaders прошёл бы мимо проверки
				 *       незавершённого сообщения - его блок заголовков лёг бы прямо в чужое
				 *       тело, строкой размера чанка у кадрирования chunked. Незавершённое
				 *       сообщение не оставляет соединение пригодным для следующего: границу
				 *       предыдущего получателю определить нечем, и соединение подлежит
				 *       закрытию. Полный сброс объекта для повторного использования
				 *       выполняется методом clear.
				 *
				 * @note Вызов из pull-источника данных отменяется: подготовка уничтожила бы
				 *       объект функции, из которого пришёл вызов.
				 *
				 * \~english
				 * @brief Method of the reset of the state of the sending for the next message in the same connection
				 * @details It prepares the sender for the next message (keep-alive): it resets
				 *          the framing, the source of the data and the flags, but does NOT touch the not sent
				 *          remainder of the output buffer. The state of the parsing is not affected -
				 *          for it reset() is used.
				 * @note An uncompleted outgoing message cancels the preparation: the reset would remove
				 *       the flag of the completeness, and the next sendHeaders would pass by the check
				 *       of an uncompleted message - its block of the headers would lie down right into a foreign
				 *       body, as a line of the size of a chunk at the framing chunked. An uncompleted
				 *       message does not leave the connection suitable for the next one: the boundary
				 *       of the previous one the receiver has nothing to determine with, and the connection is subject to
				 *       a closing. A full reset of the object for a repeated use
				 *       is performed by the method clear.
				 * @note A call from the pull source of the data is cancelled: the preparation would destroy
				 *       the object of the function whence the call has come.
				 *
				 * \~
				 */
				void resetSender() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод назначения pull-источника данных тела сообщения
				 *
				 * @details Источник может назначаться как до вызова sendHeaders, так и после -
				 *          в обоих случаях он относится к текущему (ещё не отправленному до конца)
				 *          исходящему сообщению. Если предыдущее сообщение уже завершено,
				 *          метод сам готовит отправитель к следующему сообщению.
				 *
				 * @note Трейлеры с pull-источником несовместимы: источник завершает тело сам
				 *       по достижении конца данных, и дописывать к завершённому сообщению
				 *       уже нечего. Блок трейлеров отбрасывается с записью в лог и после
				 *       исчерпания источника, и пока источник ещё не исчерпан - во втором
				 *       случае он завершил бы тело нулевым чанком поверх недочитанного
				 *       остатка, и получатель принял бы усечённое сообщение за полное.
				 *       Если трейлеры нужны - выдавайте тело методом sendData, не закрывая
				 *       сообщение флагом endStream, и завершайте его блоком трейлеров.
				 *
				 * @note Из собственного вызова источник уничтожить нельзя: назначение нового
				 *       источника, подготовка отправителя методом resetSender и полная очистка
				 *       объекта методом clear отменяются с записью в лог, пока источник
				 *       исполняется - иначе объект функции был бы уничтожен под ногами
				 *       исполняющегося кода. Источнику, которому больше нечего выдавать,
				 *       достаточно объявить конец тела.
				 *
				 * @note Источник и метод sendData - взаимоисключающие способы подачи одного
				 *       и того же тела. Пока источник не исчерпан, порции, переданные в
				 *       sendData, отбрасываются с записью в лог: они встали бы чужими
				 *       байтами посреди тела, а вызов из самого источника пишет ещё и
				 *       внутрь участка выходного буфера, зарезервированного под текущую
				 *       порцию, разрывая кадрирование. После того как источник объявил
				 *       конец данных, sendData снова принимает тело - этим дошлётся
				 *       остаток, если источник завершился раньше анонсированного
				 *       Content-Length.
				 *
				 * @note При кадрировании фиксированным размером источник завершает тело только
				 *       по исчерпании анонсированного Content-Length. Признак конца данных,
				 *       выставленный раньше, сообщение не завершает: на провод ушло бы усечённое
				 *       тело, а получатель дочитывал бы недостающие байты до таймаута и в
				 *       конвейере принял бы за них начало следующего сообщения. Такой источник
				 *       удаляется с записью в лог, а сообщение остаётся незавершённым - остаток
				 *       анонсированного объёма можно дослать методом sendData либо закрыть
				 *       соединение. Та же политика действует и на пути sendData.
				 *
				 * @param source pull-источник данных тела
				 *
				 * \~english
				 * @brief Method of the assignment of the pull source of the data of the body of a message
				 * @details The source may be assigned both before the call of sendHeaders and after it -
				 *          in both cases it relates to the current (not yet sent to the end)
				 *          outgoing message. If the previous message is already completed,
				 *          the method itself prepares the sender for the next message.
				 * @note The trailers are incompatible with a pull source: the source completes the body itself
				 *       at the reaching of the end of the data, and there is nothing to append to a completed message
				 *       any more. A block of the trailers is discarded with a record into the log both after
				 *       the exhaustion of the source and while the source is not yet exhausted - in the second
				 *       case it would complete the body by a zero chunk over an under-read
				 *       remainder, and the receiver would take a truncated message for a full one.
				 *       If the trailers are needed - issue the body by the method sendData without closing
				 *       the message by the flag endStream, and complete it by a block of the trailers.
				 * @note From its own call the source cannot be destroyed: the assignment of a new
				 *       source, the preparation of the sender by the method resetSender and the full clearing
				 *       of the object by the method clear are cancelled with a record into the log while the source
				 *       is being executed - otherwise the object of the function would be destroyed under the feet of the
				 *       executing code. For a source which has nothing more to issue,
				 *       it suffices to declare the end of the body.
				 * @note The source and the method sendData are the mutually exclusive ways of the supply of one
				 *       and the same body. While the source is not exhausted, the portions transmitted into
				 *       sendData are discarded with a record into the log: they would stand as foreign
				 *       octets in the middle of the body, while a call from the source itself writes also
				 *       inside the section of the output buffer reserved for the current
				 *       portion, tearing the framing. After the source has declared
				 *       the end of the data, sendData accepts the body again - by this the
				 *       remainder is sent, if the source has completed earlier than the announced
				 *       Content-Length.
				 * @note At the framing by a fixed size the source completes the body only
				 *       at the exhaustion of the announced Content-Length. The flag of the end of the data
				 *       set earlier does not complete the message: onto the wire a truncated
				 *       body would go away, while the receiver would read the lacking octets to the timeout and in
				 *       a pipeline would take the beginning of the next message for them. Such a source
				 *       is removed with a record into the log, while the message remains uncompleted - the remainder
				 *       of the announced volume may be sent by the method sendData or the connection may be closed.
				 *       The same policy is in force on the path sendData as well.
				 * @param source pull source of the data of the body
				 *
				 * \~
				 */
				void dataSource(data_source_callback_t source) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения признака незавершённой отправки тела из pull-источника
				 *
				 * @details Истинно, пока источник данных назначен, его тело не исчерпано и
				 *          очередная прокачка способна продвинуться: заголовки отправлены,
				 *          сообщение не завершено и его тело есть чем кадрировать. Ровно этот
				 *          признак управляет циклом дозагрузки: пока он истинен, сетевому слою
				 *          следует вызывать resumeSource() по готовности сокета к записи.
				 *          Учёт способности продвинуться обязателен - иначе источник,
				 *          назначенный до отправки заголовков, либо источник сообщения, тело
				 *          которого кадрировать нечем, удерживали бы признак истинным навсегда
				 *          и цикл дозагрузки крутился бы вхолостую
				 *
				 * @return признак незавершённой отправки тела
				 *
				 * \~english
				 * @brief Method of getting the flag of an uncompleted sending of the body from the pull source
				 * @details It is true while the source of the data is assigned, its body is not exhausted and
				 *          the next pumping is capable of advancing: the headers are sent,
				 *          the message is not completed and there is something to frame its body with. Exactly this
				 *          flag controls the cycle of the loading: while it is true, the network layer
				 *          should call resumeSource() at the readiness of the socket for the writing.
				 *          The account of the capability of advancing is obligatory - otherwise a source
				 *          assigned before the sending of the headers, or a source of a message the body
				 *          of which there is nothing to frame with, would hold the flag true forever
				 *          and the cycle of the loading would spin idly
				 * @return flag of an uncompleted sending of the body
				 *
				 * \~
				 */
				bool sourcePending() const noexcept;
				/**
				 * \~russian
				 * @brief Метод продолжения отправки тела из pull-источника данных
				 *
				 * @details За одну прокачку из источника выкачивается не более лимита,
				 *          заданного методом pumpLimit - тело произвольного размера не
				 *          удерживает управление внутри одного вызова. Сетевой слой обязан
				 *          вызывать метод по готовности сокета к записи, пока он возвращает
				 *          истину. В pull-модели дозагрузка выполняется автоматически при
				 *          выборке consumePending, и вызывать метод не требуется.
				 *
				 * @return признак того, что тело источника ещё не исчерпано
				 *
				 * \~english
				 * @brief Method of the continuation of the sending of the body from the pull source of the data
				 * @details In a single pumping out of the source not more than the limit is pumped,
				 *          given by the method pumpLimit - a body of an arbitrary size does not
				 *          hold the control inside a single call. The network layer is obliged
				 *          to call the method at the readiness of the socket for the writing while it returns
				 *          a truth. In the pull model the loading is performed automatically at the
				 *          selection consumePending, and to call the method is not required.
				 * @return flag of the body of the source not being exhausted yet
				 *
				 * \~
				 */
				bool resumeSource() noexcept;
				/**
				 * \~russian
				 * @brief Метод настройки объёма одной прокачки pull-источника данных
				 *
				 * @param size максимальный объём тела, выкачиваемый за одну прокачку
				 *
				 * \~english
				 * @brief Method of the configuration of the volume of a single pumping of the pull source of the data
				 * @param size largest volume of the body pumped out in a single pumping
				 *
				 * \~
				 */
				void pumpLimit(const uint64_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод настройки порогов выходного буфера отправки
				 *
				 * @details Верхний порог управляет обратным давлением, а не является жёсткой
				 *          ёмкостью буфера: до порога отмеряются байты тела, а разметка
				 *          кадрирования chunked дописывается поверх отмеренного. Поэтому
				 *          заполненный буфер превышает верхний порог на размер разметки
				 *          последней порции - размер шестнадцатеричного заголовка чанка и два
				 *          CRLF, не более десятка байт независимо от размера тела. Превышение
				 *          не накапливается: следующая порция отмеряется от фактического
				 *          заполнения и при исчерпанном пороге равна нулю
				 *
				 * @param high ёмкость выходного буфера отправки (high-water)
				 * @param low  порог сигнала writable (low-water)
				 *
				 * \~english
				 * @brief Method of the configuration of the thresholds of the output buffer of the sending
				 * @details The upper threshold controls the back pressure and is not a rigid
				 *          capacity of the buffer: up to the threshold the octets of the body are measured out, while the marking
				 *          of the framing chunked is appended over the measured out. Therefore a
				 *          filled buffer exceeds the upper threshold by the size of the marking
				 *          of the last portion - the size of the hexadecimal header of a chunk and two
				 *          CRLFs, not more than a dozen octets independently of the size of the body. The exceeding
				 *          does not accumulate: the next portion is measured out from the actual
				 *          filling and at an exhausted threshold is equal to a zero
				 * @param high capacity of the output buffer of the sending (high-water)
				 * @param low  threshold of the signal writable (low-water)
				 *
				 * \~
				 */
				void sendWaterMarks(const size_t high, const size_t low) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод отправки блока заголовков (запрос/ответ/трейлеры) исходящего сообщения
				 *
				 * @details Способ кадрирования тела выбирается по заголовкам контейнера:
				 *          - установлен Content-Length - тело фиксированного размера (IDENTITY);
				 *            объявленный нулевой размер завершает сообщение независимо от
				 *            endStream: на проводе такой блок заголовков уже является
				 *            законченным сообщением без тела;
				 *          - Content-Length отсутствует и endStream == false - добавляется
				 *            Transfer-Encoding: chunked. В HTTP/1.0 кодирования chunked
				 *            не существует: тело ответа кадрируется закрытием соединения,
				 *            а тело запроса кадрировать нечем и оно не принимается вовсе -
				 *            получатель обязан считать запрос без Content-Length и без
				 *            Transfer-Encoding запросом без тела (RFC 9112 §6.3). Телу
				 *            запроса HTTP/1.0 требуется Content-Length;
				 *          - endStream == true - тела нет, заголовки отправляются как есть;
				 *            если контейнер при этом объявляет Transfer-Encoding, оканчивающийся
				 *            токеном chunked, пустое тело завершается нулевым чанком - блок
				 *            заголовков сам по себе конца сообщения не обозначает, и получатель
				 *            иначе ждал бы тело до закрытия соединения.
				 *          Блок отбрасывается с записью в лог, если предыдущее исходящее
				 *          сообщение не завершено: его заголовки ушли бы на провод посреди
				 *          чужого тела. Так же отбрасывается сообщение, объявление
				 *          транспортного кодирования которого содержит chunked не последним:
				 *          дописать chunked нельзя - к телу он применялся бы дважды
				 *          (RFC 9112 §6.1), а изменить порядок кодирований библиотека
				 *          не вправе, поскольку не знает, какие из них применены к телу.
				 *          Контейнер без провайдера в режиме chunked интерпретируется как
				 *          трейлеры (завершает тело последним чанком) - та же семантика,
				 *          что у HTTP/2. Во всех остальных случаях контейнер без провайдера
				 *          отбрасывается с записью в лог: стартовую строку формировать не из
				 *          чего, и блок ушёл бы на провод голыми полями.
				 *          Кадрирование исходящего сообщения приводится в соответствие с RFC 9112:
				 *          одновременная отправка Content-Length и Transfer-Encoding невозможна
				 *          (лишний заголовок вычищается с записью в лог), при кадрировании
				 *          chunked заголовок Transfer-Encoding гарантированно заканчивается
				 *          токеном chunked, а из сообщения версии HTTP/1.0 заголовок
				 *          Transfer-Encoding вычищается целиком - такое сообщение получатель
				 *          обязан считать сообщением с неисправным кадрированием (RFC 9112 §6.1).
				 *          Это исключает генерацию кадров, которые принимающая сторона обязана
				 *          отвергнуть как request smuggling.
				 *          Сообщение не собирается вовсе, если хотя бы один его элемент
				 *          непредставим на проводе: название поля не является токеном,
				 *          значение поля либо пояснение к коду состояния содержит управляющий
				 *          символ (RFC 9110 §5.1, §5.5, RFC 9112 §4), метод или цель запроса
				 *          пусты либо содержат пробел или управляющий символ (RFC 9112 §3),
				 *          код состояния не укладывается в три цифры (RFC 9112 §4), а также
				 *          если запрос HTTP/1.1 не несёт заголовка Host либо несёт его со
				 *          внутренними пробелами (RFC 9110 §7.2). Возврат каретки или перевод
				 *          строки внутри любого из этих элементов расщепляет собираемое
				 *          сообщение на два, и получатель прочитал бы дописанное вызывающей
				 *          стороной как отдельное поле либо отдельное сообщение. Непригодное
				 *          поле отвергает сообщение целиком, а не вычищается из него:
				 *          молчаливое удаление изменило бы смысл того, что просили отправить.
				 *          Исключение составляет блок трейлеров - он отбрасывается один,
				 *          а тело всё равно завершается нулевым чанком: тело к этому моменту
				 *          уже на проводе, и без завершения получатель ждал бы его продолжения
				 *          до закрытия соединения, тогда как трейлеры необязательны
				 *          (RFC 9110 §6.5).
				 *          Кадрирование ответа сервера дополнительно зависит от метода запроса,
				 *          которому ответ соответствует (задаётся методом method):
				 *          - ответ на HEAD и ответ с кодом 1xx, [204 No Content] либо
				 *            [304 Not Modified] заканчивается первой пустой строкой после блока
				 *            заголовков независимо от присутствующих в нём полей
				 *            (RFC 9112 §6.3 п.1), и тело в нём не принимается вовсе: выданные
				 *            следом байты получатель прочитал бы как начало следующего ответа.
				 *            У ответов 1xx и 204 с провода снимаются и объявления кадрирования -
				 *            отправлять их запрещено (§6.1, §6.2); ответу 304 и ответу на HEAD
				 *            они, напротив, разрешены и сохраняются: заголовки описывают тело,
				 *            которое было бы отправлено в ответ на такой же запрос GET;
				 *          - успешный ответ на CONNECT превращает соединение в туннель сразу
				 *            за пустой строкой (§6.3 п.2): объявления кадрирования снимаются
				 *            с провода (§6.2), а переданные далее байты уходят как есть,
				 *            без кадрирования.
				 *
				 * @param headers   контейнер заголовков (провайдер контейнера задаёт стартовую строку)
				 * @param endStream флаг завершения сообщения (тела не будет)
				 *
				 * \~english
				 * @brief Method of the sending of a block of the headers (a request/an answer/the trailers) of an outgoing message
				 * @details The way of the framing of the body is chosen by the headers of the container:
				 *          - a Content-Length is set - a body of a fixed size (IDENTITY);
				 *            an announced zero size completes the message independently of
				 *            endStream: on the wire such a block of the headers is already
				 *            a finished message without a body;
				 *          - a Content-Length is absent and endStream == false - a
				 *            Transfer-Encoding: chunked is added. In HTTP/1.0 the encoding chunked
				 *            does not exist: the body of an answer is framed by the closing of the connection,
				 *            while the body of a request there is nothing to frame with and it is not accepted at all -
				 *            the receiver is obliged to consider a request without a Content-Length and without a
				 *            Transfer-Encoding a request without a body (RFC 9112 §6.3). The body of a
				 *            request of HTTP/1.0 requires a Content-Length;
				 *          - endStream == true - there is no body, the headers are sent as they are;
				 *            if the container thereby announces a Transfer-Encoding ending
				 *            with the token chunked, an empty body is completed by a zero chunk - a block
				 *            of the headers by itself does not designate the end of the message, and the receiver
				 *            would otherwise wait for the body up to the closing of the connection.
				 *          The block is discarded with a record into the log, if the previous outgoing
				 *          message is not completed: its headers would go away onto the wire in the middle of
				 *          a foreign body. The same way a message is discarded the announcement
				 *          of the transport encoding of which contains a chunked not last:
				 *          to append a chunked is impossible - it would be applied to the body twice
				 *          (RFC 9112 §6.1), while to change the order of the encodings the library
				 *          is not entitled, since it does not know which of them are applied to the body.
				 *          A container without a provider in the mode chunked is interpreted as
				 *          the trailers (it completes the body by the last chunk) - the same semantics
				 *          as at HTTP/2. In all the other cases a container without a provider
				 *          is discarded with a record into the log: the starting line there is nothing
				 *          to form from, and the block would go away onto the wire as bare fields.
				 *          The framing of an outgoing message is brought into a correspondence with RFC 9112:
				 *          a simultaneous sending of a Content-Length and of a Transfer-Encoding is impossible
				 *          (the superfluous header is cleaned out with a record into the log), at the framing
				 *          chunked the header Transfer-Encoding is guaranteed to end
				 *          with the token chunked, while from a message of the version HTTP/1.0 the header
				 *          Transfer-Encoding is cleaned out entirely - such a message the receiver
				 *          is obliged to consider a message with a faulty framing (RFC 9112 §6.1).
				 *          This excludes the generation of the frames which the accepting side is obliged
				 *          to reject as a request smuggling.
				 *          The message is not assembled at all, if at least one of its elements
				 *          is unrepresentable on the wire: the name of a field is not a token,
				 *          the value of a field or the explanation to a status code contains a control
				 *          character (RFC 9110 §5.1, §5.5, RFC 9112 §4), the method or the target of a request
				 *          are empty or contain a space or a control character (RFC 9112 §3),
				 *          the status code does not fit into three digits (RFC 9112 §4), and also
				 *          if a request of HTTP/1.1 does not carry a header Host or carries it with the
				 *          internal spaces (RFC 9110 §7.2). A carriage return or a line
				 *          feed inside any of these elements splits the message being assembled
				 *          into two, and the receiver would read that appended by the calling
				 *          side as a separate field or a separate message. An unsuitable
				 *          field rejects the message as a whole rather than being cleaned out of it:
				 *          a silent removal would change the sense of that which has been asked to be sent.
				 *          An exception is a block of the trailers - it is discarded alone,
				 *          while the body is completed by a zero chunk all the same: the body by this moment
				 *          is already on the wire, and without a completion the receiver would wait for its continuation
				 *          up to the closing of the connection, whereas the trailers are optional
				 *          (RFC 9110 §6.5).
				 *          The framing of an answer of a server additionally depends on the method of the request
				 *          to which the answer corresponds (is given by the method method):
				 *          - an answer to a HEAD and an answer with a code 1xx, [204 No Content] or
				 *            [304 Not Modified] ends by the first empty line after the block
				 *            of the headers independently of the fields present in it
				 *            (RFC 9112 §6.3 p.1), and a body in it is not accepted at all: the octets issued
				 *            after it the receiver would read as the beginning of the next answer.
				 *            At the answers 1xx and 204 the announcements of the framing are also removed from the wire -
				 *            to send them is prohibited (§6.1, §6.2); to the answer 304 and to an answer to a HEAD
				 *            they, on the contrary, are allowed and are preserved: the headers describe the body
				 *            which would be sent in an answer to the same request GET;
				 *          - a successful answer to a CONNECT turns the connection into a tunnel at once
				 *            after the empty line (§6.3 p.2): the announcements of the framing are removed
				 *            from the wire (§6.2), while the octets transmitted further go away as they are,
				 *            without a framing.
				 * @param headers   container of the headers (the provider of the container sets the starting line)
				 * @param endStream flag of the completion of the message (there will be no body)
				 *
				 * \~
				 */
				void sendHeaders(const headers_t & headers, const bool endStream) noexcept;
				/**
				 * \~russian
				 * @brief Метод передачи части тела сообщения для отправки (push-модель, bounded buffer)
				 *
				 * @details Копирует в выходной буфер столько байт, сколько влезает до high-water,
				 *          и возвращает это число (0..size). Если вернулось меньше size - буфер
				 *          заполнен: приостановите выдачу и дождитесь функции обратного вызова
				 *          writable. Кадрирование тела (chunked/identity) парсер применяет сам.
				 *          Для кадрирования фиксированного размера (Content-Length) тело
				 *          завершается строго по исчерпании анонсированного размера: флаг
				 *          endStream, выставленный раньше времени, игнорируется с записью
				 *          в лог - иначе на проводе оказалось бы усечённое тело, а получатель
				 *          завис бы в ожидании недостающих байт.
				 *
				 * @param buffer    буфер данных тела
				 * @param size      размер данных тела
				 * @param endStream флаг завершения сообщения
				 * @return          число принятых байт (0..size)
				 *
				 * \~english
				 * @brief Method of the transmission of a part of the body of a message for the sending (the push model, a bounded buffer)
				 * @details It copies into the output buffer as many octets as fit up to the high-water,
				 *          and returns this number (0..size). If less than size has been returned - the buffer
				 *          is filled: suspend the issue and wait for the callback function
				 *          writable. The framing of the body (chunked/identity) the parser applies itself.
				 *          For the framing by a fixed size (Content-Length) the body
				 *          is completed strictly at the exhaustion of the announced size: the flag
				 *          endStream set ahead of the time is ignored with a record
				 *          into the log - otherwise on the wire a truncated body would turn out to be, while the receiver
				 *          would hang in the waiting for the lacking octets.
				 * @param buffer    buffer of the data of the body
				 * @param size      size of the data of the body
				 * @param endStream flag of the completion of the message
				 * @return          number of the accepted octets (0..size)
				 *
				 * \~
				 */
				size_t sendData(const void * buffer, const size_t size, const bool endStream) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения ещё не отправленных исходящих байтов (pull-модель)
				 *
				 * @details View действителен до следующего вызова любого метода парсера.
				 *          После записи в сокет освободите отправленную часть методом
				 *          consumePending(). При установленной функции обратного вызова
				 *          записи буфер опустошается автоматически.
				 *
				 * @return ещё не отправленные исходящие байты (zero-copy view во внутренний буфер)
				 *
				 * \~english
				 * @brief Method of getting the not yet sent outgoing octets (the pull model)
				 * @details The view is valid until the next call of any method of the parser.
				 *          After the writing into the socket free the sent part by the method
				 *          consumePending(). At a set callback function
				 *          of the writing the buffer is emptied automatically.
				 * @return not yet sent outgoing octets (a zero-copy view into the internal buffer)
				 *
				 * \~
				 */
				string_view pending() const noexcept;
				/**
				 * \~russian
				 * @brief Метод освобождения отправленных байтов из исходящего буфера (амортизированно O(1))
				 *
				 * @param size число отправленных байт
				 *
				 * \~english
				 * @brief Method of the release of the sent octets from the outgoing buffer (amortized O(1))
				 * @param size number of the sent octets
				 *
				 * \~
				 */
				void consumePending(const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки фрагмента тела сообщения
				 *
				 * @param callback функция обратного вызова для обработки фрагмента тела сообщения
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of a fragment of the body of a message
				 * @param callback callback function for the processing of a fragment of the body of a message
				 *
				 * \~
				 */
				void on(data_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки фазы разбора HTTP-сообщения
				 *
				 * @param callback функция обратного вызова для обработки фазы разбора HTTP-сообщения
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of the phase of the parsing of an HTTP message
				 * @param callback callback function for the processing of the phase of the parsing of an HTTP message
				 *
				 * \~
				 */
				void on(phase_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки границ чанков
				 *
				 * @param callback функция обратного вызова для обработки границ чанков
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of the boundaries of the chunks
				 * @param callback callback function for the processing of the boundaries of the chunks
				 *
				 * \~
				 */
				void on(chunk_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова записи исходящих байтов в сеть
				 *
				 * @param callback функция обратного вызова записи исходящих байтов в сеть
				 *
				 * \~english
				 * @brief Method of setting the callback function of the writing of the outgoing octets into the network
				 * @param callback callback function of the writing of the outgoing octets into the network
				 *
				 * \~
				 */
				void on(write_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки заголовков или трейлеров сообщения
				 *
				 * @param callback функция обратного вызова для обработки заголовков или трейлеров сообщения
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of the headers or of the trailers of a message
				 * @param callback callback function for the processing of the headers or of the trailers of a message
				 *
				 * \~
				 */
				void on(header_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова для обработки провайдера заголовков сообщения
				 *
				 * @param callback функция обратного вызова для обработки провайдера заголовков сообщения
				 *
				 * \~english
				 * @brief Method of setting the callback function for the processing of the provider of the headers of a message
				 * @param callback callback function for the processing of the provider of the headers of a message
				 *
				 * \~
				 */
				void on(provider_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова о готовности принимать данные тела
				 *
				 * @param callback функция обратного вызова о готовности принимать данные тела
				 *
				 * \~english
				 * @brief Method of setting the callback function about the readiness to accept the data of the body
				 * @param callback callback function about the readiness to accept the data of the body
				 *
				 * \~
				 */
				void on(writable_callback_t callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @note Объект логирования обязателен: разбор записывает в него причину
				 *       каждого отказа, и пустой указатель здесь означает разыменование
				 *       нуля на первом же непригодном сообщении. Проверка перед записью
				 *       не выполняется намеренно - она встала бы на путь, который наполняет
				 *       принимаемый извне трафик, а конструктора без объекта логирования
				 *       у парсера нет вовсе: передать пустой указатель можно только явно.
				 *       Этим парсер отличается от контейнера заголовков, который строится
				 *       и без объектов и потому проверяет их сам
				 *
				 * @param direct направление трафика (запрос/ответ)
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @note The object of the logging is obligatory: the parsing writes into it the reason
				 *       of every refusal, and an empty pointer here means a dereferencing
				 *       of a zero on the very first unsuitable message. A check before the writing
				 *       is not performed deliberately - it would stand on the path which is filled by
				 *       the traffic accepted from the outside, while a constructor without an object of the logging
				 *       the parser has none at all: to pass an empty pointer is possible only explicitly.
				 *       By this the parser differs from the container of the headers which is built
				 *       even without the objects and therefore checks them itself
				 * @param direct direction of the traffic (a request/an answer)
				 * @param fmk    object of the framework
				 * @param log    object for the work with the logs
				 *
				 * \~
				 */
				explicit Parser_HTTP(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Parser_HTTP() noexcept override;
		} parser_http_t;
	};
};

#endif // __AWH_HTTP_PARSER_HTTP1__
