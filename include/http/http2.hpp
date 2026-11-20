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
 * @brief Класс работы с фреймами HTTP/2 — адаптер над парсером HTTP/2 AWH v5,
 *        сохраняющий прежний интерфейс модуля
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
#include "../proto/http/parser/http2/http.hpp"

/**
 * @brief пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс работы с фреймами Http2
	 *
	 */
	typedef class AWH_SHARED_EXPORT Http2 {
		public:
			/**
			 * @brief Идентификации сервиса
			 *
			 */
			enum class mode_t : uint8_t {
				NONE   = 0x00, // Сервис не идентифицирован
				CLIENT = 0x01, // Сервис идентифицирован как клиент
				SERVER = 0x02  // Сервис идентифицирован как сервер
			};
			/**
			 * @brief Направления передачи фреймов
			 *
			 */
			enum class direct_t : uint8_t {
				NONE = 0x00, // Направление не установлено
				SEND = 0x01, // Направление отправки
				RECV = 0x02  // Направление получения
			};
			/**
			 * @brief Флаги передачи данных
			 *
			 */
			enum class flag_t : uint8_t {
				NONE        = 0x00, // Флаг не установлен
				PADDED      = 0x01, // Флаг PADDED
				PRIORITY    = 0x02, // Флаг установки приоритетов
				END_STREAM  = 0x03, // Флаг завершения передачи потока
				END_HEADERS = 0x04  // Флаг завершения передачи заголовков
			};
			/**
			 * @brief Флаги типов фреймов
			 *
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
			 * @brief Параметры настроек HTTP/2
			 *
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
			 * @brief Флаги ошибок протокола HTTP/2
			 *
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
			 * @brief Количество потоков по умолчанию
			 *
			 */
			static constexpr uint32_t CONCURRENT_STREAMS = 0x80;
			/**
			 * @brief Максимальный размер таблицы заголовков по умолчанию
			 *
			 */
			static constexpr uint32_t HEADER_TABLE_SIZE = 0x1000;
			/**
			 * @brief Максимальный размер буфера полезной нагрузки
			 *
			 */
			static constexpr uint32_t MAX_PAYLOAD_SIZE = 0xFFFF;
			/**
			 * @brief Максимальный размер окна по умолчанию
			 *
			 */
			static constexpr uint32_t MAX_WINDOW_SIZE = 0xFFFFFFF;
			/**
			 * @brief Минимальный размер фрейма по умолчанию
			 *
			 */
			static constexpr uint32_t MAX_FRAME_SIZE_MIN = 0x4000;
			/**
			 * @brief Максимальный размер фрейма по умолчанию
			 *
			 */
			static constexpr uint32_t MAX_FRAME_SIZE_MAX = 0xFFFFFF;
			/**
			 * @brief Максимальный размер списка заголовков по умолчанию (SETTINGS_MAX_HEADER_LIST_SIZE)
			 *
			 */
			static constexpr uint32_t MAX_HEADER_LIST_SIZE = 0x10000;
			/**
			 * @brief Код возврата временного отказа функции обратного вызова
			 *
			 * @details Значение сохранено от прежней библиотеки HTTP/2: на него
			 *          опираются вызывающие модули, возвращая код из обработчиков
			 *
			 */
			static constexpr int32_t ERR_TEMPORAL_CALLBACK_FAILURE = -521;
			/**
			 * @brief Код возврата отказа функции обратного вызова
			 *
			 * @details Значение сохранено от прежней библиотеки HTTP/2: на него
			 *          опираются вызывающие модули, возвращая код из обработчиков
			 *
			 */
			static constexpr int32_t ERR_CALLBACK_FAILURE = -902;
		private:
			/**
			 * @brief Событие обмена данными
			 *
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
			// Признак выполнения прокачки: защита от реентерабельного вызова
			bool _pumping;
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
			 * @brief Метод пометки буферов потока на удаление
			 *
			 * @param sid идентификатор потока
			 *
			 */
			void release(const int32_t sid) noexcept;
			/**
			 * @brief Метод отправки накопленной полезной нагрузки потока
			 *
			 * @param sid идентификатор потока
			 *
			 */
			void pump(const int32_t sid) noexcept;
			/**
			 * @brief Метод отправки исходящих байтов в сеть
			 *
			 * @return результат отправки
			 *
			 */
			bool flush() noexcept;
			/**
			 * @brief Метод вызова функции обратного вызова фрейма
			 *
			 * @param sid    идентификатор потока
			 * @param direct направление передачи фрейма
			 * @param type   тип фрейма
			 * @param flags  флаги фрейма
			 * @return       статус обработки фрейма
			 *
			 */
			int32_t fire(const int32_t sid, const direct_t direct, const frame_t type, const std::set <flag_t> & flags) noexcept;
			/**
			 * @brief Метод преобразования кода ошибки протокола HTTP/2
			 *
			 * @param code код ошибки протокола
			 * @return     код ошибки модуля
			 *
			 */
			static error_t error(const uint32_t code) noexcept;
			/**
			 * @brief Метод преобразования списка заголовков в поля парсера
			 *
			 * @param headers список заголовков
			 * @param result  список полей парсера
			 *
			 */
			static void fields(const vector <std::pair <string, string>> & headers, vector <http::h2::hpack::field_t> & result) noexcept;
		private:
			/**
			 * @brief Метод начала выполнения операции
			 *
			 * @param event событие выполняемой операции
			 *
			 */
			void activate(const event_t event) noexcept;
			/**
			 * @brief Метод завершения выполнения операции
			 *
			 * @param event событие выполненной операции
			 *
			 */
			void completed(const event_t event) noexcept;
		private:
			/**
			 * @brief Метод обновления размера окна фрейма
			 *
			 * @param sid  идентификатор потока
			 * @param size размер нового окна
			 * @return     результат установки размера окна фрейма
			 *
			 */
			bool windowUpdate(const int32_t sid, const int32_t size) noexcept;
		public:
			/**
			 * @brief Метод выполнения пинга
			 *
			 * @return результат работы пинга
			 *
			 */
			bool ping() noexcept;
		public:
			/**
			 * @brief Метод запрещения получения данных с клиента
			 *
			 * @return результат выполнения операции
			 *
			 */
			bool shutdown() noexcept;
		public:
			/**
			 * @brief Метод чтения данных фрейма из бинарного буфера
			 *
			 * @param buffer буфер бинарных данных для чтения фрейма
			 * @param size   размер буфера бинарных данных
			 * @return       результат чтения данных фрейма
			 *
			 */
			bool frame(const uint8_t * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод продолжения отправки фреймов, отложенных из-за заполненной очереди
			 *
			 * @note Если функция обратного вызова «writable» сообщает, что очередь отправки
			 *       заполнена, байты не отдаются, а накапливаются до этого вызова
			 *
			 * @return результат отправки
			 *
			 */
			bool resume() noexcept;
			/**
			 * @brief Метод получения размера данных потока, ещё не отданных в сеть
			 *
			 * @param id идентификатор потока
			 * @return   размер данных в байтах
			 *
			 */
			size_t pending(const int32_t id) const noexcept;
		public:
			/**
			 * @brief Метод выполнения сброса подключения
			 *
			 * @param sid   идентификатор потока
			 * @param error код отправляемой ошибки
			 * @return      результат отправки сообщения
			 *
			 */
			bool reject(const int32_t sid, const error_t error) noexcept;
		private:
			/**
			 * @brief Метод отправки списка разрешённых источников
			 *
			 */
			void sendOrigin() noexcept;
			/**
			 * @brief Метод отправки расширения альтернативного сервиса RFC7383
			 *
			 * @param sid идентификатор потока
			 *
			 */
			void sendAltSvc(const int32_t sid) noexcept;
		public:
			/**
			 * @brief Метод отправки трейлеров
			 *
			 * @param id      идентификатор потока
			 * @param headers заголовки отправляемые
			 * @return        результат отправки данных фрейма
			 *
			 */
			bool sendTrailers(const int32_t id, const vector <std::pair <string, string>> & headers) noexcept;
			/**
			 * @brief Метод отправки бинарных данных
			 *
			 * @param id     идентификатор потока
			 * @param buffer буфер бинарных данных передаваемых
			 * @param size   размер передаваемых данных в байтах
			 * @param flag   флаг передаваемого потока по сети
			 * @return       результат отправки данных фрейма
			 *
			 */
			bool sendData(const int32_t id, const uint8_t * buffer, const size_t size, const flag_t flag) noexcept;
		public:
			/**
			 * @brief Метод отправки push-уведомлений
			 *
			 * @param id      идентификатор потока
			 * @param headers заголовки отправляемые
			 * @param flag    флаг передаваемого потока по сети
			 * @return        идентификатор зарезервированного потока либо -1 при ошибке
			 *
			 */
			int32_t sendPush(const int32_t id, const vector <std::pair <string, string>> & headers, const flag_t flag) noexcept;
			/**
			 * @brief Метод отправки заголовков
			 *
			 * @param id      идентификатор потока
			 * @param headers заголовки отправляемые
			 * @param flag    флаг передаваемого потока по сети
			 * @return        идентификатор выделенного потока либо результат отправки
			 *
			 */
			int32_t sendHeaders(const int32_t id, const vector <std::pair <string, string>> & headers, const flag_t flag) noexcept;
		public:
			/**
			 * @brief Метод отправки сообщения закрытия всех потоков
			 *
			 * @param last   идентификатор последнего потока
			 * @param error  код отправляемой ошибки
			 * @param buffer буфер отправляемых данных если требуется
			 * @param size   размер отправляемого буфера данных
			 * @return       результат отправки данных фрейма
			 *
			 */
			bool goaway(const int32_t last, const error_t error, const uint8_t * buffer = nullptr, const size_t size = 0) noexcept;
		public:
			/**
			 * @brief Метод очистки активной сессии
			 *
			 */
			void free() noexcept;
			/**
			 * @brief Метод закрытия подключения
			 *
			 */
			void close() noexcept;
		public:
			/**
			 * @brief Метод проверки инициализации модуля
			 *
			 * @return результат проверки инициализации
			 *
			 */
			bool initialized() const noexcept;
		public:
			/**
			 * @brief Метод установки функций обратного вызова
			 *
			 * @param callback функции обратного вызова
			 *
			 */
			void callback(const callback_t & callback) noexcept;
		public:
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A    тип идентификатора функции
			 * @tparam B    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 *
			 */
			template <typename A, typename B, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 *
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
			 * @brief Метод установки списка разрешённых источников
			 *
			 * @param origins список разрешённых источников
			 *
			 */
			void origin(const vector <string> & origins) noexcept;
		public:
			/**
			 * @brief Метод установки списка альтернативных сервисов
			 *
			 * @param origins список альтернативных сервисов
			 *
			 */
			void altsvc(const std::unordered_multimap <string, string> & origins) noexcept;
		public:
			/**
			 * @brief Метод инициализации
			 *
			 * @param mode     идентификатор сервиса
			 * @param settings параметры настроек сессии
			 * @return         результат выполнения инициализации
			 *
			 */
			bool init(const mode_t mode, const std::map <settings_t, uint32_t> & settings) noexcept;
		public:
			/**
			 * @brief Оператор [=] зануления фрейма Http2
			 *
			 * @return сформированный объект Http2
			 *
			 */
			Http2 & operator = (nullptr_t) noexcept;
			/**
			 * @brief Оператор [=] копирования объекта фрейма Http2
			 *
			 * @param ctx объект фрейма Http2
			 * @return    сформированный объект Http2
			 *
			 */
			Http2 & operator = (const Http2 & ctx) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 */
			explicit Http2(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Http2() noexcept;
	} http2_t;
};
