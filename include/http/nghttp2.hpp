/**
 * @file: nghtt2.hpp
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

#ifndef __AWH_NGHTTP2__
#define __AWH_NGHTTP2__

/**
 * Стандартная библиотека
 */
#include <string>
#include <vector>

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
	 * NgHttp2 Класс работы с фреймами NgHttp2
	 */
	typedef class NgHttp2 {
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
		private:
			/**
			 * Событие обмена данными
			 */
			enum class event_t : uint8_t {
				NONE         = 0x00, // Событие не установлено
				SEND_PING    = 0x01, // Событие отправки пинга
				SEND_DATA    = 0x02, // Событие отправки данных
				RECV_FRAME   = 0x03, // События получения данных
				SEND_ORIGIN  = 0x04, // События отправки доверенных источников
				SEND_HEADERS = 0x05  // Событие отправки заголовков
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
			 * begin Функция начала получения фрейма заголовков HTTP/2
			 * @param session объект сессии HTTP/2
			 * @param frame   объект фрейма заголовков HTTP/2
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус полученных данных
			 */
			static int begin(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
			/**
			 * frameRecv Функция обратного вызова при получении фрейма
			 * @param session объект сессии HTTP/2
			 * @param frame   объект фрейма заголовков HTTP/2
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус полученных данных
			 */
			static int frameRecv(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
			/**
			 * frameSend Функция обратного вызова при отправки фрейма
			 * @param session объект сессии HTTP/2
			 * @param frame   объект фрейма заголовков HTTP/2
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус полученных данных
			 */
			static int frameSend(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
			/**
			 * close Функция закрытия подключения
			 * @param session объект сессии HTTP/2
			 * @param sid     идентификатор потока
			 * @param error   флаг ошибки HTTP/2 если присутствует
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус полученного события
			 */
			static int close(nghttp2_session * session, const int32_t sid, const uint32_t error, void * ctx) noexcept;
			/**
			 * chunk Функция обратного вызова при получении чанка
			 * @param session объект сессии HTTP/2
			 * @param flags   флаги события для сессии HTTP/2
			 * @param sid     идентификатор потока
			 * @param buffer  буфер данных который содержит полученный чанк
			 * @param size    размер полученного буфера данных чанка
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус полученных данных
			 */
			static int chunk(nghttp2_session * session, const uint8_t flags, const int32_t sid, const uint8_t * buffer, const size_t size, void * ctx) noexcept;
			/**
			 * header Функция обратного вызова при получении заголовка HTTP/2
			 * @param session объект сессии HTTP/2
			 * @param frame   объект фрейма заголовков HTTP/2
			 * @param key     данные ключа заголовка
			 * @param keySize размер ключа заголовка
			 * @param val     данные значения заголовка
			 * @param valSize размер значения заголовка
			 * @param flags   флаги события для сессии HTTP/2
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус полученных данных
			 */
			static int header(nghttp2_session * session, const nghttp2_frame * frame, const uint8_t * key, const size_t keySize, const uint8_t * val, const size_t valSize, const uint8_t flags, void * ctx) noexcept;
		private:
			/**
			 * send Функция обратного вызова при подготовки данных для отправки
			 * @param session объект сессии HTTP/2
			 * @param buffer  буфер данных которые следует отправить
			 * @param size    размер буфера данных для отправки
			 * @param flags   флаги события для сессии HTTP/2
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        количество отправленных байт
			 */
			static ssize_t send(nghttp2_session * session, const uint8_t * buffer, const size_t size, const int flags, void * ctx) noexcept;
		public:
			/**
			 * read Функция чтения подготовленных данных для формирования буфера данных который необходимо отправить
			 * @param session объект сессии HTTP/2
			 * @param sid     идентификатор потока
			 * @param buffer  буфер данных которые следует отправить
			 * @param size    размер буфера данных для отправки
			 * @param flags   флаги события для сессии HTTP/2
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
			 * frame Метод чтения данных фрейма из бинарного буфера
			 * @param buffer буфер бинарных данных для чтения фрейма
			 * @param size   размер буфера бинарных данных
			 * @return       результат чтения данных фрейма
			 */
			bool frame(const uint8_t * buffer, const size_t size) noexcept;
		public:
			/**
			 * sendOrigin Метод отправки списка разрешенных источников
			 * @param origins список разрешённых источников
			 */
			bool sendOrigin(const vector <string> & origins) noexcept;
			/**
			 * sendData Метод отправки бинарных данных на сервер
			 * @param id     идентификатор потока
			 * @param buffer буфер бинарных данных передаваемых на сервер
			 * @param size   размер передаваемых данных в байтах
			 * @param end    флаг завершения потока передачи данных
			 * @return       результат отправки данных фрейма
			 */
			bool sendData(const int32_t id, const uint8_t * buffer, const size_t size, const bool end) noexcept;
			/**
			 * sendHeaders Метод отправки заголовков на сервер
			 * @param id      идентификатор потока
			 * @param headers заголовки отправляемые на сервер
			 * @param end     размер сообщения в байтах
			 * @return        флаг завершения потока передачи данных
			 */
			int32_t sendHeaders(const int32_t id, const vector <pair <string, string>> & headers, const bool end) noexcept;
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
			 * on Метод установки функции обратного вызова при закрытии потока
			 * @param callback функция обратного вызова
			 */
			void on(function <int (const int32_t, const uint32_t)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова при отправки сообщения на сервер
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
			void on(function <int (const int32_t, const direct_t, const uint8_t, const uint8_t)> callback) noexcept;
		public:
			/**
			 * Оператор [=] зануления фрейма NgHttp2
			 * @return сформированный объект NgHttp2
			 */
			NgHttp2 & operator = (std::nullptr_t) noexcept;
			/**
			 * Оператор [=] копирования объекта фрейма NgHttp2
			 * @param ctx объект фрейма NgHttp2
			 * @return    сформированный объект NgHttp2
			 */
			NgHttp2 & operator = (const NgHttp2 & ctx) noexcept;
		public:
			/**
			 * NgHttp2 Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			NgHttp2(const fmk_t * fmk, const log_t * log) noexcept :
			 _close(false), _mode(mode_t::NONE), _event(event_t::NONE),
			 _callback(log), _session(nullptr), _fmk(fmk), _log(log) {}
			/**
			 * ~NgHttp2 Деструктор
			 */
			~NgHttp2() noexcept;
	} nghttp2_t;
};

#endif // __AWH_NGHTTP2__
