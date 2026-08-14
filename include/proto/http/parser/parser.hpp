/**
 * @file parser.hpp
 * @date 2026-07-18
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
 * @brief Заголовочный файл базового класса HTTP-парсера — общий контракт разбора сообщений: фазы обработки,
 *        части сообщения, статусы и базовые лимиты, конкретизируемые наследниками для HTTP/1.x и HTTP/2
 *
 * \~english
 * @brief Header file of the base class of the HTTP parser — the common contract of the parsing of the messages: the phases of the processing,
 *        the parts of a message, the statuses and the base limits made concrete by the heirs for HTTP/1.x and HTTP/2
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER__
#define __AWH_HTTP_PARSER__

/**
 * Стандартные заголовочные файлы
 */
#include <memory>
#include <cstddef>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../http.hpp"
#include "../../../sys/fmk.hpp"
#include "../../../sys/log.hpp"
#include "../../../sys/global.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../../sys/macro_push.hpp"

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
		 * @brief Базовый класс HTTP-парсера
		 *
		 * @details Содержит только контракт, общий для парсеров всех версий протокола
		 *          (HTTP/1.x, HTTP/2, HTTP/3): жизненный цикл разбора, семантику частей
		 *          сообщения (заголовки/тело/трейлеры), общее ядро лимитов безопасности
		 *          и единую точку подачи байтов из сети. Всё специфичное для конкретной
		 *          версии протокола (коды ошибок, флаги, структура сообщения, расширенные
		 *          лимиты) определяется в классах-наследниках.
		 *
		 * \~english
		 * @brief Base class of the HTTP parser
		 * @details Contains only the contract common for the parsers of all the versions of the protocol
		 *          (HTTP/1.x, HTTP/2, HTTP/3): the life cycle of the parsing, the semantics of the parts
		 *          of a message (the headers/the body/the trailers), the common core of the limits of safety
		 *          and a single point of the supply of the octets from the network. Everything specific to a particular
		 *          version of the protocol (the error codes, the flags, the structure of a message, the extended
		 *          limits) is determined in the classes-heirs
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Parser {
			public:
				/**
				 * \~russian
				 * @brief Максимальное число заголовков
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Largest number of the headers
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t MAX_HEADER_COUNT = (128);
				/**
				 * \~russian
				 * @brief Максимальная длина имени заголовка
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Largest length of the name of a header
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t MAX_HEADER_NAME = (1 * 1024);
				/**
				 * \~russian
				 * @brief Максимальная длина значения заголовка
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Largest length of the value of a header
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t MAX_HEADER_VALUE = (16 * 1024);
				/**
				 * \~russian
				 * @brief Суммарный размер всех заголовков
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Total size of all the headers
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr size_t MAX_HEADERS_TOTAL = (64 * 1024);
				/**
				 * \~russian
				 * @brief Максимальный размер тела
				 *
				 * @note Значения по умолчанию подобраны консервативно
				 *
				 * \~english
				 * @brief Largest size of the body
				 * @note The values by default are selected conservatively
				 *
				 * \~
				 */
				static constexpr uint64_t MAX_BODY_SIZE = (64ull * 1024 * 1024);
			public:
				/**
				 * \~russian
				 * @brief Фаза разбора части HTTP-сообщения
				 *
				 * @details Фаза относится не к сообщению, а к части, названной рядом
				 *          идущим значением part_t: END вместе с HEADERS означает конец
				 *          блока заголовков, END вместе с BODY - конец тела, END вместе
				 *          с TRAILER - конец блока трейлеров. Концом самого сообщения
				 *          является только END вместе с NONE.
				 *
				 * @warning Фазу нельзя читать в отрыве от части. Усечённый ответ, у которого
				 *          принят лишь блок заголовков, выдаёт END точно так же, как
				 *          дочитанный до конца, - разница целиком в части. Признак,
				 *          собранный по одной лишь фазе, объявит принятым сообщение,
				 *          у которого не хватает тела. У HTTP/1.x итоговый признак
				 *          завершённости сообщения отдельный - status() == COMPLETE,
				 *          и на усечённом сообщении он даёт PARTIAL. У HTTP/2 и HTTP/3
				 *          status() относится к соединению целиком, а не к сообщению,
				 *          и признаком завершённости сообщения там служит только END
				 *          вместе с NONE.
				 *
				 *          Расхождение принятого тела с объявленной длиной сверяют все три
				 *          парсера до того, как объявить конец сообщения: HTTP/1.x оставляет
				 *          сообщение незавершённым, HTTP/2 и HTTP/3 сбрасывают поток. Ни один
				 *          из них не объявит END вместе с NONE на недобранном теле
				 *
				 * \~english
				 * @brief Phase of the parsing of a part of an HTTP message
				 * @details The phase relates not to the message but to the part named by the alongside
				 *          going value part_t: END together with HEADERS means the end of the
				 *          block of the headers, END together with BODY - the end of the body, END together
				 *          with TRAILER - the end of the block of the trailers. The end of the message itself
				 *          is only END together with NONE.
				 * @warning The phase cannot be read separately from the part. A truncated answer of which
				 *          only the block of the headers has been accepted issues END exactly the same as
				 *          one read to the end - the difference is entirely in the part. A flag
				 *          assembled by the phase alone will declare accepted a message
				 *          of which the body is lacking. At HTTP/1.x the resulting flag
				 *          of the completeness of a message is a separate one - status() == COMPLETE,
				 *          and on a truncated message it gives PARTIAL. At HTTP/2 and HTTP/3
				 *          status() relates to the connection as a whole rather than to a message,
				 *          and the flag of the completeness of a message serves there only as END
				 *          together with NONE.
				 *          A divergence of the accepted body from the announced length is compared by all the three
				 *          parsers before declaring the end of the message: HTTP/1.x leaves
				 *          the message uncompleted, HTTP/2 and HTTP/3 reset the stream. Not one
				 *          of them will declare END together with NONE on an under-collected body
				 *
				 * \~
				 */
				enum class phase_t : uint8_t {
					NONE  = 0x00, // Фаза не определена
					BEGIN = 0x01, // Начало части сообщения
					END   = 0x02  // Конец части сообщения (конец самого сообщения - вместе с part_t::NONE)
				};
				/**
				 * \~russian
				 * @brief Часть HTTP-сообщения, к которой относится фаза разбора
				 *
				 * \~english
				 * @brief Part of an HTTP message to which the phase of the parsing relates
				 *
				 * \~
				 */
				enum class part_t : uint8_t {
					NONE    = 0x00, // Сообщение целиком (фаза относится к самому сообщению, а не к его части)
					BODY    = 0x01, // Тело сообщения
					HEADERS = 0x02, // Заголовки сообщения
					TRAILER = 0x03  // Заголовки трейлера
				};
				/**
				 * \~russian
				 * @brief Итоговый статус разбора HTTP-сообщения
				 *
				 * @details Для мультиплексируемых протоколов (HTTP/2, HTTP/3) статус
				 *          относится к соединению в целом, а не к отдельному сообщению
				 *
				 * \~english
				 * @brief Resulting status of the parsing of an HTTP message
				 * @details For the multiplexed protocols (HTTP/2, HTTP/3) the status
				 *          relates to the connection as a whole rather than to a separate message
				 *
				 * \~
				 */
				enum class status_t : uint8_t {
					NONE     = 0x00, // Статус не определён (разбор ещё не начинался)
					ERROR    = 0x01, // Ошибка разбора/безопасности
					PARTIAL  = 0x02, // Данные приняты, но разбор ещё не завершён — нужно ещё байтов
					COMPLETE = 0x03  // Разбор полностью завершён (в буфере могут идти следующие данные)
				};
			public:
				/**
				 * \~russian
				 * @brief Структура общего ядра ограничений безопасности
				 *
				 * @details Содержит только лимиты, осмысленные для любой версии протокола.
				 *          Парсеры конкретных версий расширяют структуру своими лимитами
				 *          (HTTP/1.x — чанки и стартовая строка, HTTP/2 — блоки заголовков,
				 *          CONTINUATION-фреймы, частотные лимиты и т.д.)
				 *
				 * \~english
				 * @brief Structure of the common core of the limitations of safety
				 * @details Contains only the limits meaningful for any version of the protocol.
				 *          The parsers of the particular versions extend the structure by their own limits
				 *          (HTTP/1.x — the chunks and the starting line, HTTP/2 — the blocks of the headers,
				 *          the CONTINUATION frames, the frequency limits and so on)
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Limits {
					// Максимальная длина имени заголовка
					size_t maxHeaderName;
					// Максимальная длина значения заголовка
					size_t maxHeaderValue;
					// Максимальное число заголовков
					size_t maxHeaderCount;
					// Суммарный размер всех заголовков
					size_t maxHeadersTotal;
					// Максимальный размер тела
					uint64_t maxBodySize;
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
					virtual ~Limits() noexcept = default;
				} limits_t;
			protected:
				// Итоговый статус разбора
				status_t _status;
				// Направление потока данных (запрос/ответ)
				direct_t _direct;
			protected:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод получения итогового статуса разбора
				 *
				 * @return итоговый статус разбора
				 *
				 * \~english
				 * @brief Method of getting the resulting status of the parsing
				 * @return resulting status of the parsing
				 *
				 * \~
				 */
				status_t status() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод полной очистки всех данных парсера
				 *
				 * @details Помимо сброса состояния разбора возвращает настройки парсера
				 *          (лимиты безопасности, функции обратного вызова) к значениям
				 *          по умолчанию — детали определяются классом-наследником
				 *
				 * \~english
				 * @brief Method of a full clearing of all the data of the parser
				 * @details Besides the reset of the state of the parsing it returns the settings of the parser
				 *          (the limits of safety, the callback functions) to the values
				 *          by default — the details are determined by the class-heir
				 *
				 * \~
				 */
				virtual void clear() noexcept;
				/**
				 * \~russian
				 * @brief Метод сброса состояния парсера
				 *
				 * @details Дешёвый сброс с сохранением настроек (лимиты безопасности,
				 *          функции обратного вызова): для HTTP/1.x — подготовка к разбору
				 *          следующего сообщения в том же соединении (keep-alive/pipelining),
				 *          для мультиплексируемых протоколов — полный сброс соединения
				 *
				 * \~english
				 * @brief Method of the reset of the state of the parser
				 * @details A cheap reset with the preservation of the settings (the limits of safety,
				 *          the callback functions): for HTTP/1.x — the preparation for the parsing of the
				 *          next message in the same connection (keep-alive/pipelining),
				 *          for the multiplexed protocols — a full reset of the connection
				 *
				 * \~
				 */
				virtual void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения человекочитаемого названия текущей ошибки разбора
				 *
				 * @details Каждый парсер определяет собственную систему кодов ошибок —
				 *          типизированный доступ к коду предоставляется наследником,
				 *          база гарантирует только текстовое представление для логов
				 *
				 * @return название текущей ошибки разбора
				 *
				 * \~english
				 * @brief Method of getting the human-readable name of the current error of the parsing
				 * @details Every parser determines its own system of the error codes —
				 *          a typed access to the code is provided by the heir,
				 *          the base guarantees only a text representation for the logs
				 * @return name of the current error of the parsing
				 *
				 * \~
				 */
				virtual string_view errorName() const noexcept = 0;
			public:
				/**
				 * \~russian
				 * @brief Метод клонирования объекта парсера
				 *
				 * @details Клон получает те же направление трафика, лимиты безопасности
				 *          и функции обратного вызова, но чистое состояние разбора
				 *          ("фабрика с теми же настройками")
				 *
				 * @return копия объекта парсера
				 *
				 * \~english
				 * @brief Method of cloning the object of the parser
				 * @details The clone gets the same direction of the traffic, the limits of safety
				 *          and the callback functions, but a clean state of the parsing
				 *          («a factory with the same settings»)
				 * @return copy of the object of the parser
				 *
				 * \~
				 */
				virtual unique_ptr <Parser> clone() const noexcept = 0;
			public:
				/**
				 * \~russian
				 * @brief Метод уведомления парсера о завершении потока данных (закрытии соединения)
				 *
				 * @details Сетевой слой обязан вызвать этот метод, когда соединение закрыто
				 *          удалённой стороной (получен FIN/EOF сокета). Реакция определяется
				 *          протоколом: HTTP/1.x завершает тело "до закрытия соединения" либо
				 *          фиксирует обрыв, HTTP/2 проверяет корректность завершения сессии
				 *
				 * \~english
				 * @brief Method of notifying the parser about the completion of the stream of the data (the closing of the connection)
				 * @details The network layer is obliged to call this method when the connection is closed
				 *          by the remote side (a FIN/EOF of the socket has been obtained). The reaction is determined by
				 *          the protocol: HTTP/1.x completes the body «up to the closing of the connection» or
				 *          fixes a break, HTTP/2 checks the correctness of the completion of the session
				 *
				 * \~
				 */
				virtual void eof() noexcept = 0;
				/**
				 * \~russian
				 * @brief Метод разбора данных
				 *
				 * @details Потребляет столько байтов, сколько смог, и возвращает их число.
				 *          Итоговый статус необходимо контролировать методом status().
				 *          Точная семантика границ потребления определяется протоколом:
				 *          HTTP/1.x останавливается на границе завершённого сообщения,
				 *          мультиплексируемые протоколы буферизуют неполные кадры внутри
				 *
				 * @param buffer буфер данных для разбора
				 * @param size   размер данных для разбора
				 * @return       количество обработанных байт данных
				 *
				 * \~english
				 * @brief Method of parsing the data
				 * @details Consumes as many octets as it has been able to and returns their number.
				 *          The resulting status is necessary to control by the method status().
				 *          The exact semantics of the boundaries of the consumption is determined by the protocol:
				 *          HTTP/1.x stops at the boundary of a completed message,
				 *          the multiplexed protocols buffer the incomplete frames inside
				 * @param buffer buffer of the data for the parsing
				 * @param size   size of the data for the parsing
				 * @return       number of the processed octets of the data
				 *
				 * \~
				 */
				virtual size_t parse(const void * buffer, const size_t size) noexcept = 0;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param direct направление потока данных
				 * @param fmk    объект фреймворка
				 * @param log    объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param direct direction of the stream of the data
				 * @param fmk    object of the framework
				 * @param log    object for the work with the logs
				 *
				 * \~
				 */
				explicit Parser(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept;
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
				virtual ~Parser() noexcept;
        } parser_t;
    };
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../../sys/macro_pop.hpp"

#endif // __AWH_HTTP_PARSER__
