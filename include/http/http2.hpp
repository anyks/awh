/**
 * @file: htt2.hpp
 * @date: 2023-09-27
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

#ifndef __AWH_HTTP2__
#define __AWH_HTTP2__

/**
 * Стандартные модули
 */
#include <map>
#include <cmath>
#include <queue>
#include <string>
#include <vector>
#include <unordered_map>

/**
 * Методы только для OS Windows
 */
#if defined(_WIN32) || defined(_WIN64)
	// Подключаем библиотеку асинхронного ввода-вывода
	#include <io.h>
	// Подключаем вспомогательную библиотеку
	#include <fcntl.h>
	// Разрешаем статическую сборку для NGTCP2
	#define NGTCP2_STATICLIB
	// Разрешаем статическую сборку для NGHTTP2
    #define NGHTTP2_STATICLIB
	// Разрешаем статическую сборку для NGHTTP3
	#define NGHTTP3_STATICLIB
#endif

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/socket.hpp>
#include <http/errors.hpp>

/**
 * Подключаем NgHttp2
 */
#include <nghttp2/nghttp2.h>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Http2 Класс работы с фреймами Http2
	 */
	typedef class Http2 {
		public:
			/**
			 * Идентификации сервиса
			 */
			enum class mode_t : uint8_t {
				NONE   = 0x00, // Сервис не идентифицирован
				CLIENT = 0x01, // Сервис идентифицирован как клиент
				SERVER = 0x02  // Сервис идентифицирован как сервер
			};
			/**
			 * Направления передачи фреймов
			 */
			enum class direct_t : uint8_t {
				NONE = 0x00, // Направление не установлено
				SEND = 0x01, // Направление отправки
				RECV = 0x02  // Направление получения
			};
			/**
			 * Флаги передачи данных
			 */
			enum class flag_t : uint8_t {
				NONE        = 0x00, // Флаг не установлен
				PADDED      = 0x01, // Флаг PADDED
				PRIORITY    = 0x02, // Флаг установки приоритетов
				END_STREAM  = 0x03, // Флаг завершения передачи потока
				END_HEADERS = 0x04  // Флаг завершения передачи заголовков
			};
			/**
			 * Флаги типов фреймов
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
			 * Параметры настроек HTTP/2
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
				HEADER_TABLE_SIZE = 0x09  // Максимальный размер таблицы заголовков
			};
			/**
			 * Флаги ошибок протокола HTTP/2
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
			// Количество потоков по умолчанию
			static constexpr uint32_t CONCURRENT_STREAMS = 0x80;
			// Максимальный размер таблицы заголовков по умолчанию
			static constexpr uint32_t HEADER_TABLE_SIZE = 0x1000;
			// Максимальный размер буфера полезной нагрузки
			static constexpr uint32_t MAX_PAYLOAD_SIZE = 0xFFFF;
			// Максимальный размер окна по умолчанию
			static constexpr uint32_t MAX_WINDOW_SIZE = 0xFFFFFFF;
			// Минимальный размер фрейма по умолчанию
			static constexpr uint32_t MAX_FRAME_SIZE_MIN = 0x4000;
			// Максимальный размер фрейма по умолчанию
			static constexpr uint32_t MAX_FRAME_SIZE_MAX = 0xFFFFFF;
		private:
			/**
			 * Событие обмена данными
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
			/**
			 * Payload Структура полезной нагрузки
			 */
			typedef struct Payload {
				int32_t sid;                  // Идентификатор потока
				flag_t flag;                  // Флаг передаваемого потока по сети
				size_t size;                  // Размер буфера
				size_t offset;                // Смещение в бинарном буфере
				unique_ptr <uint8_t []> data; // Данные буфера
				/**
				 * Payload Конструктор
				 */
				Payload() noexcept : sid(0), flag(flag_t::NONE), size(0), offset(0), data(nullptr) {}
			} payload_t;
		private:
			// Флаг требования закрыть подключение
			bool _close;
		private:
			// Флаг идентификации сервиса
			mode_t _mode;
			// Флаг активного последнего события
			event_t _event;
		private:
			// Хранилище функций обратного вызова
			fn_t _callbacks;
		private:
			// Объект работы с сокетами
			socket_t _socket;
		private:
			// Список доступных источников для подключения
			vector <string> _origins;
		private:
			// Буферы отправляемой полезной нагрузки
			map <int32_t, queue <payload_t>> _payloads;
		private:
			// Список отправляемых альтернативных сервисов
			unordered_multimap <string, string> _altsvc;
		private:
			// Ессия HTTP/2 подключения
			nghttp2_session * _session;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			/**
			 * debug Функция обратного вызова при получении отладочной информации
			 * @param format формат вывода отладочной информации
			 * @param args   список аргументов отладочной информации
			 */
			static void debug(const char * format, va_list args) noexcept;
			/**
			 * begin Функция обратного вызова активации получения фрейма заголовков
			 * @param session объект сессии
			 * @param frame   объект фрейма заголовков
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int32_t begin(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
			/**
			 * create Функция обратного вызова при создании фрейма
			 * @param session объект сессии
			 * @param hd      параметры фрейма
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int32_t create(nghttp2_session * session, const nghttp2_frame_hd * hd, void * ctx) noexcept;
			/**
			 * frameRecv Функция обратного вызова при получении фрейма
			 * @param session объект сессии
			 * @param frame   объект фрейма заголовков
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int32_t frameRecv(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
			/**
			 * frameSend Функция обратного вызова при отправки фрейма
			 * @param session объект сессии
			 * @param frame   объект фрейма заголовков
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int32_t frameSend(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
			/**
			 * close Функция закрытия подключения
			 * @param session объект сессии
			 * @param sid     идентификатор потока
			 * @param error   флаг ошибки если присутствует
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int32_t close(nghttp2_session * session, const int32_t sid, const uint32_t error, void * ctx) noexcept;
			/**
			 * error Функция обратного вызова при получении ошибок
			 * @param session объект сессии
			 * @param code    код полученной ошибки
			 * @param msg     сообщение ошибки
			 * @param size    размер текста ошибки
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int32_t error(nghttp2_session * session, const int32_t code, const char * msg, const size_t size, void * ctx) noexcept;
			/**
			 * chunk Функция обратного вызова при получении чанка
			 * @param session объект сессии
			 * @param flags   флаги события для сессии
			 * @param sid     идентификатор потока
			 * @param buffer  буфер данных который содержит полученный чанк
			 * @param size    размер полученного буфера данных чанка
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int32_t chunk(nghttp2_session * session, const uint8_t flags, const int32_t sid, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
			/**
			 * header Функция обратного вызова при получении заголовка
			 * @param session объект сессии
			 * @param frame   объект фрейма заголовков
			 * @param key     данные ключа заголовка
			 * @param keySize размер ключа заголовка
			 * @param val     данные значения заголовка
			 * @param valSize размер значения заголовка
			 * @param flags   флаги события для сессии
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int32_t header(nghttp2_session * session, const nghttp2_frame * frame, nghttp2_rcbuf * name, nghttp2_rcbuf * value, const uint8_t flags, void * ctx) noexcept;
		private:
			/**
			 * send Функция обратного вызова при подготовки данных для отправки
			 * @param session объект сессии
			 * @param buffer  буфер данных которые следует отправить
			 * @param size    размер буфера данных для отправки
			 * @param flags   флаги события для сессии
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        количество отправленных байт
			 */
			static ssize_t send(nghttp2_session * session, const uint8_t * buffer, const size_t size, const int32_t flags, void * ctx) noexcept;
			/**
			 * send Функция отправки подготовленного буфера данных по сети
			 * @param session объект сессии
			 * @param sid     идентификатор потока
			 * @param buffer  буфер данных которые следует отправить
			 * @param size    размер буфера данных для отправки
			 * @param flags   флаги события для сессии
			 * @param source  объект промежуточных данных локального подключения
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        количество отправленных байт
			 */
			static ssize_t send(nghttp2_session * session, const int32_t sid, uint8_t * buffer, const size_t size, uint32_t * flags, nghttp2_data_source * source, void * ctx) noexcept;
		private:
			/**
			 * available Метод проверки сколько байт доступно для отправки
			 * @param sid идентификатор потока
			 * @return    количество байт доступных для отправки
			 */
			size_t available(const int32_t sid) const noexcept;
		private:
			/**
			 * commit Метод применения изменений
			 * @param event событие которому соответствует фиксация
			 * @return      результат отправки
			 */
			bool commit(const event_t event) noexcept;
		private:
			/**
			 * completed Метод завершения выполнения операции
			 * @param event событие выполненной операции
			 */
			void completed(const event_t event) noexcept;
		private:
			/**
			 * submit Метод подготовки отправки данных полезной нагрузки
			 * @param sid  идентификатор потока 
			 * @param flag флаг передаваемого потока по сети
			 * @return     результат работы функции
			 */
			bool submit(const int32_t sid, const flag_t flag) noexcept;
		private:
			/**
			 * windowUpdate Метод обновления размера окна фрейма
			 * @param sid  идентификатор потока
			 * @param size размер нового окна
			 * @return     результат установки размера офна фрейма
			 */
			bool windowUpdate(const int32_t sid, const int32_t size) noexcept;
		public:
			/**
			 * ping Метод выполнения пинга
			 * @return результат работы пинга
			 */
			bool ping() noexcept;
		public:
			/**
			 * shutdown Метод запрещения получения данных с клиента
			 * @return результат выполнения операции
			 */
			bool shutdown() noexcept;
		public:
			/**
			 * frame Метод чтения данных фрейма из бинарного буфера
			 * @param buffer буфер бинарных данных для чтения фрейма
			 * @param size   размер буфера бинарных данных
			 * @return       результат чтения данных фрейма
			 */
			bool frame(const uint8_t * buffer, const size_t size) noexcept;
		public:
			/**
			 * reject Метод выполнения сброса подключения
			 * @param sid   идентификатор потока
			 * @param error код отправляемой ошибки
			 * @return      результат отправки сообщения
			 */
			bool reject(const int32_t sid, const error_t error) noexcept;
		private:
			/**
			 * sendOrigin Метод отправки списка разрешённых источников
			 */
			void sendOrigin() noexcept;
			/**
			 * sendAltSvc Метод отправки расширения альтернативного сервиса RFC7383
			 * @param sid идентификатор потока
			 */
			void sendAltSvc(const int32_t sid) noexcept;
		public:
			/**
			 * sendTrailers Метод отправки трейлеров
			 * @param id      идентификатор потока
			 * @param headers заголовки отправляемые
			 * @return        результат отправки данных фрейма
			 */
			bool sendTrailers(const int32_t id, const vector <pair <string, string>> & headers) noexcept;
			/**
			 * sendData Метод отправки бинарных данных
			 * @param id     идентификатор потока
			 * @param buffer буфер бинарных данных передаваемых
			 * @param size   размер передаваемых данных в байтах
			 * @param flag   флаг передаваемого потока по сети
			 * @return       результат отправки данных фрейма
			 */
			bool sendData(const int32_t id, const uint8_t * buffer, const size_t size, const flag_t flag) noexcept;
		public:
			/**
			 * sendPush Метод отправки push-уведомлений
			 * @param id      идентификатор потока
			 * @param headers заголовки отправляемые
			 * @param flag    флаг передаваемого потока по сети
			 * @return        флаг завершения потока передачи данных
			 */
			int32_t sendPush(const int32_t id, const vector <pair <string, string>> & headers, const flag_t flag) noexcept;
			/**
			 * sendHeaders Метод отправки заголовков
			 * @param id      идентификатор потока
			 * @param headers заголовки отправляемые
			 * @param flag    флаг передаваемого потока по сети
			 * @return        флаг завершения потока передачи данных
			 */
			int32_t sendHeaders(const int32_t id, const vector <pair <string, string>> & headers, const flag_t flag) noexcept;
		public:
			/**
			 * goaway Метод отправки сообщения закрытия всех потоков
			 * @param last   идентификатор последнего потока
			 * @param error  код отправляемой ошибки
			 * @param buffer буфер отправляемых данных если требуется
			 * @param size   размер отправляемого буфера данных
			 * @return       результат отправки данных фрейма
			 */
			bool goaway(const int32_t last, const error_t error, const uint8_t * buffer = nullptr, const size_t size = 0) noexcept;
		public:
			/**
			 * free Метод очистки активной сессии
			 */
			void free() noexcept;
			/**
			 * close Метод закрытия подключения
			 */
			void close() noexcept;
		public:
			/**
			 * is Метод проверки инициализации модуля
			 * @return результат проверки инициализации
			 */
			bool is() const noexcept;
		public:
			/**
			 * callbacks Метод установки функций обратного вызова
			 * @param callbacks функции обратного вызова
			 */
			void callbacks(const fn_t & callbacks) noexcept;
		public:
			/**
			 * callback Шаблон метода установки финкции обратного вызова
			 * @tparam A тип функции обратного вызова
			 */
			template <typename A>
			/**
			 * callback Метод установки функции обратного вызова
			 * @param idw идентификатор функции обратного вызова
			 * @param fn  функция обратного вызова для установки
			 */
			void callback(const uint64_t idw, function <A> fn) noexcept {
				// Если функция обратного вызова передана
				if((idw > 0) && (fn != nullptr)){
					// Если установлена триггерная функция
					if(idw == 1){
						// Если активное событие не установлено
						if(this->_event == event_t::NONE){
							// Выполняем функцию обратного вызова
							fn();
							// Выходим из функции
							return;
						}
					}
					// Выполняем установку функции обратного вызова
					this->_callbacks.set <A> (idw, fn);
				}
			}
			/**
			 * callback Шаблон метода установки финкции обратного вызова
			 * @tparam A тип функции обратного вызова
			 */
			template <typename A>
			/**
			 * callback Метод установки функции обратного вызова
			 * @param name название функции обратного вызова
			 * @param fn   функция обратного вызова для установки
			 */
			void callback(const string & name, function <A> fn) noexcept {
				// Если функция обратного вызова передана
				if(!name.empty() && (fn != nullptr))
					// Выполняем установку функции обратного вызова
					this->_callbacks.set <A> (name, fn);
			}
		public:
			/**
			 * origin Метод установки списка разрешённых источников
			 * @param origins список разрешённых источников
			 */
			void origin(const vector <string> & origins) noexcept;
		public:
			/**
			 * altsvc Метод установки списка альтернативных сервисов
			 * @param origins список альтернативных сервисов
			 */
			void altsvc(const unordered_multimap <string, string> & origins) noexcept;
		public:
			/**
			 * init Метод инициализации
			 * @param mode     идентификатор сервиса
			 * @param settings параметры настроек сессии
			 * @return         результат выполнения инициализации
			 */
			bool init(const mode_t mode, const map <settings_t, uint32_t> & settings) noexcept;
		public:
			/**
			 * Оператор [=] зануления фрейма Http2
			 * @return сформированный объект Http2
			 */
			Http2 & operator = (std::nullptr_t) noexcept;
			/**
			 * Оператор [=] копирования объекта фрейма Http2
			 * @param ctx объект фрейма Http2
			 * @return    сформированный объект Http2
			 */
			Http2 & operator = (const Http2 & ctx) noexcept;
		public:
			/**
			 * Http2 Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Http2(const fmk_t * fmk, const log_t * log) noexcept :
			 _close(false), _mode(mode_t::NONE), _event(event_t::NONE),
			 _callbacks(log), _socket(fmk, log), _session(nullptr), _fmk(fmk), _log(log) {}
			/**
			 * ~Http2 Деструктор
			 */
			~Http2() noexcept;
	} http2_t;
};

#endif // __AWH_HTTP2__
