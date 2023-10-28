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
 * Стандартная библиотека
 */
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
				SEND_SHUTDOWN = 0x0B, // Событие отправки сообщения о завершении работы
				WINDOW_UPDATE = 0x0C  // Событие установки нового размера окна фрейма
			};
		private:
			// Флаг требования закрыть подключение
			bool _close;
		private:
			// Флаг идентификации сервиса
			mode_t _mode;
			// Флаг активного последнего события
			event_t _event;
		private:
			// Объект функций обратного вызова
			fn_t _callback;
		private:
			// Список доступных источников для подключения
			vector <string> _origins;
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
			static int begin(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
			/**
			 * create Функция обратного вызова при создании фрейма
			 * @param session объект сессии
			 * @param hd      параметры фрейма
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int create(nghttp2_session * session, const nghttp2_frame_hd * hd, void * ctx) noexcept;
			/**
			 * frameRecv Функция обратного вызова при получении фрейма
			 * @param session объект сессии
			 * @param frame   объект фрейма заголовков
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int frameRecv(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
			/**
			 * frameSend Функция обратного вызова при отправки фрейма
			 * @param session объект сессии
			 * @param frame   объект фрейма заголовков
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int frameSend(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
			/**
			 * error Функция обратного вызова при получении ошибок
			 * @param session объект сессии
			 * @param msg     сообщение ошибки
			 * @param size    размер текста ошибки
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int error(nghttp2_session * session, const char * msg, const size_t size, void * ctx) noexcept;
			/**
			 * close Функция закрытия подключения
			 * @param session объект сессии
			 * @param sid     идентификатор потока
			 * @param error   флаг ошибки если присутствует
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус обработки полученных данных
			 */
			static int close(nghttp2_session * session, const int32_t sid, const uint32_t error, void * ctx) noexcept;
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
			static int chunk(nghttp2_session * session, const uint8_t flags, const int32_t sid, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
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
			static int header(nghttp2_session * session, const nghttp2_frame * frame, const uint8_t * key, const size_t keySize, const uint8_t * val, const size_t valSize, const uint8_t flags, void * ctx) noexcept;
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
			static ssize_t send(nghttp2_session * session, const uint8_t * buffer, const size_t size, const int flags, void * ctx) noexcept;
		public:
			/**
			 * read Функция чтения подготовленных данных для формирования буфера данных который необходимо отправить
			 * @param session объект сессии
			 * @param sid     идентификатор потока
			 * @param buffer  буфер данных которые следует отправить
			 * @param size    размер буфера данных для отправки
			 * @param flags   флаги события для сессии
			 * @param source  объект промежуточных данных локального подключения
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        количество отправленных байт
			 */
			static ssize_t read(nghttp2_session * session, const int32_t sid, uint8_t * buffer, const size_t size, uint32_t * flags, nghttp2_data_source * source, void * ctx) noexcept;
		private:
			/**
			 * completed Метод завершения выполнения операции
			 * @param event событие выполненной операции
			 */
			void completed(const event_t event) noexcept;
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
		public:
			/**
			 * windowUpdate Метод обновления размера окна фрейма
			 * @param sid  идентификатор потока
			 * @param size размер нового окна
			 * @return     результат установки размера офна фрейма
			 */
			bool windowUpdate(const int32_t sid, const int32_t size) noexcept;
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
			bool init(const mode_t mode, const vector <nghttp2_settings_entry> & settings) noexcept;
		public:
			/**
			 * on Метод установки функции обратного вызова триггера выполнения операции
			 * @param callback функция обратного вызова
			 */
			void on(function <void (void)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова начала открытии потока
			 * @param callback функция обратного вызова
			 */
			void on(function <int (const int32_t)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова при создании фрейма
			 * @param callback функция обратного вызова
			 */
			void on(function <int (const int32_t, const frame_t)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова при закрытии потока
			 * @param callback функция обратного вызова
			 */
			void on(function <int (const int32_t, const error_t)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова при отправки сообщения
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint8_t *, const size_t)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова при получении чанка с сервера
			 * @param callback функция обратного вызова
			 */
			void on(function <int (const int32_t, const uint8_t *, const size_t)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова при получении данных заголовка
			 * @param callback функция обратного вызова
			 */
			void on(function <int (const int32_t, const string &, const string &)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова на событие получения ошибки
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const log_t::flag_t, const http::error_t, const string &)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова при обмене фреймами
			 * @param callback функция обратного вызова
			 */
			void on(function <int (const int32_t, const direct_t, const frame_t, const set <flag_t> &)> callback) noexcept;
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
			 _callback(log), _session(nullptr), _fmk(fmk), _log(log) {}
			/**
			 * ~Http2 Деструктор
			 */
			~Http2() noexcept;
	} http2_t;
};

#endif // __AWH_HTTP2__
