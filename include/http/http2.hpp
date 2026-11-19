/**
 * @file http2.hpp
 * @date 2023-09-27
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
 * @brief Класс работы с фреймами HTTP/2 — адаптер над парсером HTTP/2 AWH v5,
 *        сохраняющий прежний интерфейс модуля
 *
 * \~english
 * @brief Class of the work with the frames of HTTP/2 — an adapter over the HTTP/2 parser of AWH v5
 *        preserving the former interface of the module
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

#pragma once

/**
 * Стандартные модули
 */
#include <map>
#include <set>
#include <cmath>
#include <queue>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

/**
 * Наши модули
 */
#include "errors.hpp"
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"
#include "../sys/buffer.hpp"
#include "../sys/callback.hpp"
#include "../net/socket.hpp"

/**
 * Подключаем парсер HTTP/2 из AWH v5
 */
#include <proto/http/parser/http2/http.hpp>

/**
 * \~russian
 * @brief пространство имён
 *
 * \~english
 * @brief namespace
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
	 * @brief Класс работы с фреймами Http2
	 *
	 * \~english
	 * @brief Class of the work with the frames of Http2
	 *
	 * \~
	 */
	typedef class AWH_SHARED_EXPORT Http2 {
		public:
			/**
			 * \~russian
			 * @brief Идентификации сервиса
			 *
			 * \~english
			 * @brief Identification of the service
			 *
			 * \~
			 */
			enum class mode_t : uint8_t {
				NONE   = 0x00, // Сервис не идентифицирован
				CLIENT = 0x01, // Сервис идентифицирован как клиент
				SERVER = 0x02  // Сервис идентифицирован как сервер
			};
			/**
			 * \~russian
			 * @brief Направления передачи фреймов
			 *
			 * \~english
			 * @brief Directions of the transmission of the frames
			 *
			 * \~
			 */
			enum class direct_t : uint8_t {
				NONE = 0x00, // Направление не установлено
				SEND = 0x01, // Направление отправки
				RECV = 0x02  // Направление получения
			};
			/**
			 * \~russian
			 * @brief Флаги передачи данных
			 *
			 * \~english
			 * @brief Flags of the transmission of the data
			 *
			 * \~
			 */
			enum class flag_t : uint8_t {
				NONE        = 0x00, // Флаг не установлен
				PADDED      = 0x01, // Флаг PADDED
				PRIORITY    = 0x02, // Флаг установки приоритетов
				END_STREAM  = 0x03, // Флаг завершения передачи потока
				END_HEADERS = 0x04  // Флаг завершения передачи заголовков
			};
			/**
			 * \~russian
			 * @brief Флаги типов фреймов
			 *
			 * \~english
			 * @brief Flags of the types of the frames
			 *
			 * \~
			 */
			enum class frame_t : uint8_t {
				NONE            = 0x00, // Фрейм не установлен
				DATA            = 0x01, // Фрейм данных
				PING            = 0x02, // Фрейм пингов
				GOAWAY          = 0x03, // Фрейм требования отключиться от сервера
				ALTSVC          = 0x04, // Фрейм передачи альтернативных желаемых протоколов
				ORIGIN          = 0x05, // Фрейм списка разрешённых ресурсов для подключения
				HEADERS         = 0x06, // Фрейм заголовков
				PRIORITY        = 0x07, // Фрейм приоритетов
				SETTINGS        = 0x08, // Фрейм полученя настроек
				RST_STREAM      = 0x09, // Фрейм сброса подключения клиента
				CONTINUATION    = 0x0A, // Фрейм продолжения работы
				PUSH_PROMISE    = 0x0B, // Фрейм отправки push-уведомления
				WINDOW_UPDATE   = 0x0C, // Фрейм обновления окна фрейма
				PRIORITY_UPDATE = 0x0D  // Фрейм обновления приоритетов
			};
			/**
			 * \~russian
			 * @brief Параметры настроек HTTP/2
			 *
			 * \~english
			 * @brief Parameters of the settings of HTTP/2
			 *
			 * \~
			 */
			enum class settings_t : uint8_t {
				NONE              = 0x00, // Настройки не установлены
				STREAMS           = 0x01, // Максимальное количество потоков
				CONNECT           = 0x02, // Разрешение использования метода CONNECT
				FRAME_SIZE        = 0x03, // Максимальный размер фрейма
				ENABLE_PUSH       = 0x04, // Разрешение присылать push-уведомления
				WINDOW_SIZE       = 0x05, // Максимальный размер окна полезной нагрузки
				PAYLOAD_SIZE      = 0x06, // Максимальный размер буфера полезной нагурзки
				ENABLE_ALTSVC     = 0x07, // Разрешение передавать расширения ALTSVC
				ENABLE_ORIGIN     = 0x08, // Разрешение передавать расширение ORIGIN
				HEADER_TABLE_SIZE = 0x09, // Максимальный размер таблицы заголовков
				HEADER_LIST_SIZE  = 0x0A  // Максимальный размер списка заголовков (0 - без ограничения)
			};
			/**
			 * \~russian
			 * @brief Флаги ошибок протокола HTTP/2
			 *
			 * \~english
			 * @brief Flags of the errors of the HTTP/2 protocol
			 *
			 * \~
			 */
			enum class error_t : uint8_t {
				NONE                = 0x00, // Ошибка не установлена
				CANCEL              = 0x01, // Требование выполнения отмены запроса
				CONNECT_ERROR       = 0x02, // Ошибка TCP-соединения для метода CONNECT
				STREAM_CLOSED       = 0x03, // Получен кадр для завершения потока
				REFUSED_STREAM      = 0x04, // Поток не обработан
				PROTOCOL_ERROR      = 0x05, // Ошибка протокола HTTP/2
				INTERNAL_ERROR      = 0x06, // Получена ошибка реализации
				FRAME_SIZE_ERROR    = 0x07, // Размер кадра некорректен
				SETTINGS_TIMEOUT    = 0x08, // Установка параметров завершилась по таймауту
				COMPRESSION_ERROR   = 0x09, // Состояние компрессии не обновлено
				ENHANCE_YOUR_CALM   = 0x0A, // Превышена емкость для обработки
				HTTP_1_1_REQUIRED   = 0x0B, // Для запроса требуется протокол HTTP/1.1
				FLOW_CONTROL_ERROR  = 0x0C, // Ошибка превышения предела управления потоком
				INADEQUATE_SECURITY = 0x0D  // Согласованные параметры SSL не приемлемы
			};
		public:
			/**
			 * \~russian
			 * @brief Количество потоков по умолчанию
			 *
			 * \~english
			 * @brief Number of the streams by default
			 *
			 * \~
			 */
			static constexpr uint32_t CONCURRENT_STREAMS = 0x80;
			/**
			 * \~russian
			 * @brief Максимальный размер таблицы заголовков по умолчанию
			 *
			 * \~english
			 * @brief Largest size of the table of the headers by default
			 *
			 * \~
			 */
			static constexpr uint32_t HEADER_TABLE_SIZE = 0x1000;
			/**
			 * \~russian
			 * @brief Максимальный размер буфера полезной нагрузки
			 *
			 * \~english
			 * @brief Largest size of the buffer of the payload
			 *
			 * \~
			 */
			static constexpr uint32_t MAX_PAYLOAD_SIZE = 0xFFFF;
			/**
			 * \~russian
			 * @brief Максимальный размер окна по умолчанию
			 *
			 * \~english
			 * @brief Largest size of the window by default
			 *
			 * \~
			 */
			static constexpr uint32_t MAX_WINDOW_SIZE = 0xFFFFFFF;
			/**
			 * \~russian
			 * @brief Минимальный размер фрейма по умолчанию
			 *
			 * \~english
			 * @brief Smallest size of a frame by default
			 *
			 * \~
			 */
			static constexpr uint32_t MAX_FRAME_SIZE_MIN = 0x4000;
			/**
			 * \~russian
			 * @brief Максимальный размер фрейма по умолчанию
			 *
			 * \~english
			 * @brief Largest size of a frame by default
			 *
			 * \~
			 */
			static constexpr uint32_t MAX_FRAME_SIZE_MAX = 0xFFFFFF;
			/**
			 * \~russian
			 * @brief Максимальный размер списка заголовков по умолчанию (SETTINGS_MAX_HEADER_LIST_SIZE)
			 *
			 * \~english
			 * @brief Largest size of the list of the headers by default (SETTINGS_MAX_HEADER_LIST_SIZE)
			 *
			 * \~
			 */
			static constexpr uint32_t MAX_HEADER_LIST_SIZE = 0x10000;
			/**
			 * \~russian
			 * @brief Код возврата временного отказа функции обратного вызова
			 *
			 * @details Значение сохранено от прежней библиотеки HTTP/2: на него
			 *          опираются вызывающие модули, возвращая код из обработчиков
			 *
			 * \~english
			 * @brief Code of the return of a temporary failure of a callback function
			 *
			 * @details The value is preserved from the former HTTP/2 library: the calling
			 *          modules rely on it returning the code from their handlers
			 *
			 * \~
			 */
			static constexpr int32_t ERR_TEMPORAL_CALLBACK_FAILURE = -521;
			/**
			 * \~russian
			 * @brief Код возврата отказа функции обратного вызова
			 *
			 * @details Значение сохранено от прежней библиотеки HTTP/2: на него
			 *          опираются вызывающие модули, возвращая код из обработчиков
			 *
			 * \~english
			 * @brief Code of the return of a failure of a callback function
			 *
			 * @details The value is preserved from the former HTTP/2 library: the calling
			 *          modules rely on it returning the code from their handlers
			 *
			 * \~
			 */
			static constexpr int32_t ERR_CALLBACK_FAILURE = -902;
		private:
			/**
			 * \~russian
			 * @brief Событие обмена данными
			 *
			 * \~english
			 * @brief Event of the exchange of the data
			 *
			 * \~
			 */
			enum class event_t : uint8_t {
				NONE          = 0x00, // Событие не установлено
				SEND_PUSH     = 0x01, // События отправки промисов
				SEND_PING     = 0x02, // Событие отправки пинга
				SEND_DATA     = 0x03, // Событие отправки данных
				RECV_FRAME    = 0x04, // События получения данных
				SEND_ALTSVC   = 0x05, // Событие отправки альтернативного сервиса
				SEND_GOAWAY   = 0x06, // Событие отправки сообщения закрытия потоков
				SEND_ORIGIN   = 0x07, // События отправки доверенных источников
				SEND_REJECT   = 0x08, // Событие отправки сброса подключения
				SEND_HEADERS  = 0x09, // Событие отправки заголовков
				SEND_TRAILERS = 0x0A, // Событие отправки трейлеров
				SEND_SHUTDOWN = 0x0B  // Событие отправки сообщения о завершении работы
			};
		private:
			// Флаг требования закрыть подключение
			bool _close;
			// Флаг остановки отправки: очередь отправки заполнена, байты ждут вызова resume()
			bool _wouldblock;
			// Флаг требования прервать разбор из функции обратного вызова
			bool _failed;
		private:
			// Флаг идентификации сервиса
			mode_t _mode;
			// Флаг активного последнего события
			event_t _event;
		private:
			/**
			 * Глубина вложенности выполняемых операций. Вызывающие могут работать с модулем
			 * из функций обратного вызова, поэтому одиночного флага события недостаточно:
			 * закрытие сессии и триггер выполняются только после выхода из самой внешней операции
			 */
			uint32_t _depth;
		private:
			// Максимальный размер списка заголовков
			uint32_t _maxHeaderListSize;
		private:
			// Список закрытых потоков, буферы которых следует удалить
			std::set <int32_t> _closed;
		private:
			// Объект работы с сокетами
			socket_t _socket;
		private:
			// Хранилище функций обратного вызова
			callback_t _callback;
		private:
			// Список доступных источников для подключения
			vector <string> _origins;
		private:
			// Список отправляемых альтернативных сервисов
			std::unordered_multimap <string, string> _altsvc;
		private:
			// Буферы отправляемой полезной нагрузки
			std::map <int32_t, std::unique_ptr <buffer_t>> _payloads;
			// Список подготовленных для отправки записей
			std::map <int32_t, std::queue <std::pair <size_t, flag_t>>> _records;
		private:
			// Объект парсера HTTP/2 из AWH v5
			std::unique_ptr <http::parser_http2_t> _engine;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * \~russian
			 * @brief Метод пометки буферов потока на удаление
			 *
			 * @param sid идентификатор потока
			 *
			 * \~english
			 * @brief Method of the marking of the buffers of a stream for a deletion
			 *
			 * @param sid identifier of the stream
			 *
			 * \~
			 */
			void release(const int32_t sid) noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки накопленной полезной нагрузки потока
			 *
			 * @param sid идентификатор потока
			 *
			 * \~english
			 * @brief Method of the sending of the accumulated payload of a stream
			 *
			 * @param sid identifier of the stream
			 *
			 * \~
			 */
			void pump(const int32_t sid) noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки исходящих байтов в сеть
			 *
			 * @return результат отправки
			 *
			 * \~english
			 * @brief Method of the sending of the outgoing octets into the network
			 *
			 * @return result of the sending
			 *
			 * \~
			 */
			bool flush() noexcept;
			/**
			 * \~russian
			 * @brief Метод вызова функции обратного вызова фрейма
			 *
			 * @param sid    идентификатор потока
			 * @param direct направление передачи фрейма
			 * @param type   тип фрейма
			 * @param flags  флаги фрейма
			 * @return       статус обработки фрейма
			 *
			 * \~english
			 * @brief Method of the call of the callback function of a frame
			 *
			 * @param sid    identifier of the stream
			 * @param direct direction of the transmission of the frame
			 * @param type   type of the frame
			 * @param flags  flags of the frame
			 * @return       status of the processing of the frame
			 *
			 * \~
			 */
			int32_t fire(const int32_t sid, const direct_t direct, const frame_t type, const std::set <flag_t> & flags) noexcept;
			/**
			 * \~russian
			 * @brief Метод преобразования кода ошибки протокола HTTP/2
			 *
			 * @param code код ошибки протокола
			 * @return     код ошибки модуля
			 *
			 * \~english
			 * @brief Method of the transformation of the error code of the HTTP/2 protocol
			 *
			 * @param code error code of the protocol
			 * @return     error code of the module
			 *
			 * \~
			 */
			static error_t error(const uint32_t code) noexcept;
			/**
			 * \~russian
			 * @brief Метод преобразования списка заголовков в поля парсера
			 *
			 * @param headers список заголовков
			 * @param result  список полей парсера
			 *
			 * \~english
			 * @brief Method of the transformation of the list of the headers into the fields of the parser
			 *
			 * @param headers list of the headers
			 * @param result  list of the fields of the parser
			 *
			 * \~
			 */
			static void fields(const vector <std::pair <string, string>> & headers, vector <http::h2::hpack::field_t> & result) noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод начала выполнения операции
			 *
			 * @param event событие выполняемой операции
			 *
			 * \~english
			 * @brief Method of the beginning of the execution of an operation
			 *
			 * @param event event of the operation being executed
			 *
			 * \~
			 */
			void activate(const event_t event) noexcept;
			/**
			 * \~russian
			 * @brief Метод завершения выполнения операции
			 *
			 * @param event событие выполненной операции
			 *
			 * \~english
			 * @brief Method of the completion of the execution of an operation
			 *
			 * @param event event of the executed operation
			 *
			 * \~
			 */
			void completed(const event_t event) noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод обновления размера окна фрейма
			 *
			 * @param sid  идентификатор потока
			 * @param size размер нового окна
			 * @return     результат установки размера окна фрейма
			 *
			 * \~english
			 * @brief Method of the updating of the size of the window of a frame
			 *
			 * @param sid  identifier of the stream
			 * @param size size of the new window
			 * @return     result of the installation of the size of the window of a frame
			 *
			 * \~
			 */
			bool windowUpdate(const int32_t sid, const int32_t size) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод выполнения пинга
			 *
			 * @return результат работы пинга
			 *
			 * \~english
			 * @brief Method of the execution of a ping
			 *
			 * @return result of the work of the ping
			 *
			 * \~
			 */
			bool ping() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод запрещения получения данных с клиента
			 *
			 * @return результат выполнения операции
			 *
			 * \~english
			 * @brief Method of the prohibition of the acceptance of the data from the client
			 *
			 * @return result of the execution of the operation
			 *
			 * \~
			 */
			bool shutdown() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод чтения данных фрейма из бинарного буфера
			 *
			 * @param buffer буфер бинарных данных для чтения фрейма
			 * @param size   размер буфера бинарных данных
			 * @return       результат чтения данных фрейма
			 *
			 * \~english
			 * @brief Method of the reading of the data of a frame out of a binary buffer
			 *
			 * @param buffer buffer of the binary data for the reading of the frame
			 * @param size   size of the buffer of the binary data
			 * @return       result of the reading of the data of the frame
			 *
			 * \~
			 */
			bool frame(const uint8_t * buffer, const size_t size) noexcept;
			/**
			 * \~russian
			 * @brief Метод продолжения отправки фреймов, отложенных из-за заполненной очереди
			 *
			 * @note Если функция обратного вызова «writable» сообщает, что очередь отправки
			 *       заполнена, байты не отдаются, а накапливаются до этого вызова
			 *
			 * @return результат отправки
			 *
			 * \~english
			 * @brief Method of the continuation of the sending of the frames postponed because of a filled queue
			 *
			 * @note If the callback function «writable» reports that the queue of the sending
			 *       is filled, the octets are not issued but accumulated until this call
			 *
			 * @return result of the sending
			 *
			 * \~
			 */
			bool resume() noexcept;
			/**
			 * \~russian
			 * @brief Метод получения размера данных потока, ещё не отданных в сеть
			 *
			 * @param id идентификатор потока
			 * @return   размер данных в байтах
			 *
			 * \~english
			 * @brief Method of the getting of the size of the data of a stream not yet issued into the network
			 *
			 * @param id identifier of the stream
			 * @return   size of the data in the octets
			 *
			 * \~
			 */
			size_t pending(const int32_t id) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод выполнения сброса подключения
			 *
			 * @param sid   идентификатор потока
			 * @param error код отправляемой ошибки
			 * @return      результат отправки сообщения
			 *
			 * \~english
			 * @brief Method of the execution of a reset of the connection
			 *
			 * @param sid   identifier of the stream
			 * @param error code of the error being sent
			 * @return      result of the sending of the message
			 *
			 * \~
			 */
			bool reject(const int32_t sid, const error_t error) noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод отправки списка разрешённых источников
			 *
			 * \~english
			 * @brief Method of the sending of the list of the allowed origins
			 *
			 * \~
			 */
			void sendOrigin() noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки расширения альтернативного сервиса RFC7383
			 *
			 * @param sid идентификатор потока
			 *
			 * \~english
			 * @brief Method of the sending of the extension of an alternative service RFC7383
			 *
			 * @param sid identifier of the stream
			 *
			 * \~
			 */
			void sendAltSvc(const int32_t sid) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод отправки трейлеров
			 *
			 * @param id      идентификатор потока
			 * @param headers заголовки отправляемые
			 * @return        результат отправки данных фрейма
			 *
			 * \~english
			 * @brief Method of the sending of the trailers
			 *
			 * @param id      identifier of the stream
			 * @param headers headers being sent
			 * @return        result of the sending of the data of the frame
			 *
			 * \~
			 */
			bool sendTrailers(const int32_t id, const vector <std::pair <string, string>> & headers) noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки бинарных данных
			 *
			 * @param id     идентификатор потока
			 * @param buffer буфер бинарных данных передаваемых
			 * @param size   размер передаваемых данных в байтах
			 * @param flag   флаг передаваемого потока по сети
			 * @return       результат отправки данных фрейма
			 *
			 * \~english
			 * @brief Method of the sending of the binary data
			 *
			 * @param id     identifier of the stream
			 * @param buffer buffer of the binary data being transmitted
			 * @param size   size of the transmitted data in the octets
			 * @param flag   flag of the stream being transmitted over the network
			 * @return       result of the sending of the data of the frame
			 *
			 * \~
			 */
			bool sendData(const int32_t id, const uint8_t * buffer, const size_t size, const flag_t flag) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод отправки push-уведомлений
			 *
			 * @param id      идентификатор потока
			 * @param headers заголовки отправляемые
			 * @param flag    флаг передаваемого потока по сети
			 * @return        идентификатор зарезервированного потока либо -1 при ошибке
			 *
			 * \~english
			 * @brief Method of the sending of the push announcements
			 *
			 * @param id      identifier of the stream
			 * @param headers headers being sent
			 * @param flag    flag of the stream being transmitted over the network
			 * @return        identifier of the reserved stream or -1 at an error
			 *
			 * \~
			 */
			int32_t sendPush(const int32_t id, const vector <std::pair <string, string>> & headers, const flag_t flag) noexcept;
			/**
			 * \~russian
			 * @brief Метод отправки заголовков
			 *
			 * @param id      идентификатор потока
			 * @param headers заголовки отправляемые
			 * @param flag    флаг передаваемого потока по сети
			 * @return        идентификатор выделенного потока либо результат отправки
			 *
			 * \~english
			 * @brief Method of the sending of the headers
			 *
			 * @param id      identifier of the stream
			 * @param headers headers being sent
			 * @param flag    flag of the stream being transmitted over the network
			 * @return        identifier of the allotted stream or result of the sending
			 *
			 * \~
			 */
			int32_t sendHeaders(const int32_t id, const vector <std::pair <string, string>> & headers, const flag_t flag) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод отправки сообщения закрытия всех потоков
			 *
			 * @param last   идентификатор последнего потока
			 * @param error  код отправляемой ошибки
			 * @param buffer буфер отправляемых данных если требуется
			 * @param size   размер отправляемого буфера данных
			 * @return       результат отправки данных фрейма
			 *
			 * \~english
			 * @brief Method of the sending of the message of the closing of all the streams
			 *
			 * @param last   identifier of the last stream
			 * @param error  code of the error being sent
			 * @param buffer buffer of the data being sent if required
			 * @param size   size of the buffer of the data being sent
			 * @return       result of the sending of the data of the frame
			 *
			 * \~
			 */
			bool goaway(const int32_t last, const error_t error, const uint8_t * buffer = nullptr, const size_t size = 0) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки активной сессии
			 *
			 * \~english
			 * @brief Method of the cleaning of the active session
			 *
			 * \~
			 */
			void free() noexcept;
			/**
			 * \~russian
			 * @brief Метод закрытия подключения
			 *
			 * \~english
			 * @brief Method of the closing of the connection
			 *
			 * \~
			 */
			void close() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки инициализации модуля
			 *
			 * @return результат проверки инициализации
			 *
			 * \~english
			 * @brief Method of the checking of the initialization of the module
			 *
			 * @return result of the checking of the initialization
			 *
			 * \~
			 */
			bool initialized() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки функций обратного вызова
			 *
			 * @param callback функции обратного вызова
			 *
			 * \~english
			 * @brief Method of the installation of the callback functions
			 *
			 * @param callback callback functions
			 *
			 * \~
			 */
			void callback(const callback_t & callback) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the method of the connection of a callback function
			 *
			 * @tparam T    type of the callback function
			 * @tparam Args arguments of the callback function
			 *
			 * \~
			 */
			template <typename T, class... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 * \~english
			 * @brief Method of the connection of a callback function
			 *
			 * @param name identifier of the callback function
			 * @param args arguments of the callback function
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			auto on(const char * name, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(name != nullptr)
					// Ставим функцию обратного вызова в контейнер
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the method of the connection of a callback function
			 *
			 * @tparam T    type of the callback function
			 * @tparam Args arguments of the callback function
			 *
			 * \~
			 */
			template <typename T, class... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 * \~english
			 * @brief Method of the connection of a callback function
			 *
			 * @param name identifier of the callback function
			 * @param args arguments of the callback function
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			auto on(const string & name, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Ставим функцию обратного вызова в контейнер
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the method of the connection of a callback function
			 *
			 * @tparam T    type of the callback function
			 * @tparam Args arguments of the callback function
			 *
			 * \~
			 */
			template <typename T, class... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 * \~english
			 * @brief Method of the connection of a callback function
			 *
			 * @param fid  identifier of the callback function
			 * @param args arguments of the callback function
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			auto on(const uint64_t fid, Args... args) noexcept -> uint64_t {
				// Если мы получили название функции обратного вызова
				if(fid > 0){
					// Если установлена триггерная функция
					if(fid == 1){
						// Если активное событие не установлено
						if(this->_event == event_t::NONE){
							// Вызываем функцию обратного вызова
							std::apply(std::bind(args...), std::make_tuple());
							// Выходим из функции
							return 0;
						}
					}
					// Ставим функцию обратного вызова в контейнер
					return this->_callback.on <T> (fid, args...);
				}
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * \~russian
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A    тип идентификатора функции
			 * @tparam B    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 * \~english
			 * @brief Template of the method of the connection of a callback function
			 *
			 * @tparam A    type of the identifier of the function
			 * @tparam B    type of the callback function
			 * @tparam Args arguments of the callback function
			 *
			 * \~
			 */
			template <typename A, typename B, class... Args>
			/**
			 * \~russian
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
			 * \~english
			 * @brief Method of the connection of a callback function
			 *
			 * @param fid  identifier of the callback function
			 * @param args arguments of the callback function
			 * @return     identifier of the added callback function
			 *
			 * \~
			 */
			auto on(const A fid, Args... args) noexcept -> uint64_t {
				// Если мы получили на вход число
				if(is_integral_v <A> || is_enum_v <A> || is_floating_point_v <A>)
					// Ставим функцию обратного вызова в контейнер
					return this->_callback.on <B> (static_cast <uint64_t> (fid), args...);
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * \~russian
			 * @brief Метод установки списка разрешённых источников
			 *
			 * @param origins список разрешённых источников
			 *
			 * \~english
			 * @brief Method of the installation of the list of the allowed origins
			 *
			 * @param origins list of the allowed origins
			 *
			 * \~
			 */
			void origin(const vector <string> & origins) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки списка альтернативных сервисов
			 *
			 * @param origins список альтернативных сервисов
			 *
			 * \~english
			 * @brief Method of the installation of the list of the alternative services
			 *
			 * @param origins list of the alternative services
			 *
			 * \~
			 */
			void altsvc(const std::unordered_multimap <string, string> & origins) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод инициализации
			 *
			 * @param mode     идентификатор сервиса
			 * @param settings параметры настроек сессии
			 * @return         результат выполнения инициализации
			 *
			 * \~english
			 * @brief Method of the initialization
			 *
			 * @param mode     identifier of the service
			 * @param settings parameters of the settings of the session
			 * @return         result of the execution of the initialization
			 *
			 * \~
			 */
			bool init(const mode_t mode, const std::map <settings_t, uint32_t> & settings) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор [=] зануления фрейма Http2
			 *
			 * @return сформированный объект Http2
			 *
			 * \~english
			 * @brief Operator [=] of the zeroing of the frame of Http2
			 *
			 * @return formed object Http2
			 *
			 * \~
			 */
			Http2 & operator = (nullptr_t) noexcept;
			/**
			 * \~russian
			 * @brief Оператор [=] копирования объекта фрейма Http2
			 *
			 * @param ctx объект фрейма Http2
			 * @return    сформированный объект Http2
			 *
			 * \~english
			 * @brief Operator [=] of the copying of the object of the frame of Http2
			 *
			 * @param ctx object of the frame of Http2
			 * @return    formed object Http2
			 *
			 * \~
			 */
			Http2 & operator = (const Http2 & ctx) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 *
			 * @param fmk framework object
			 * @param log log object
			 *
			 * \~
			 */
			explicit Http2(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Http2() noexcept;
	} http2_t;
};
