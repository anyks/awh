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
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <client/web/errors.hpp>

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
		private:
			// Объект функций обратного вызова
			fn_t _callback;
		public:
			// Ессия HTTP/2 подключения
			nghttp2_session * session;
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
			 * frame Функция обратного вызова при получении фрейма заголовков HTTP/2 с сервера
			 * @param session объект сессии HTTP/2
			 * @param frame   объект фрейма заголовков HTTP/2
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус полученных данных
			 */
			static int frame(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
			/**
			 * begin Функция начала получения фрейма заголовков HTTP/2
			 * @param session объект сессии HTTP/2
			 * @param frame   объект фрейма заголовков HTTP/2
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус полученных данных
			 */
			static int begin(nghttp2_session * session, const nghttp2_frame * frame, void * ctx) noexcept;
			/**
			 * close Функция закрытия подключения с сервером HTTP/2
			 * @param session объект сессии HTTP/2
			 * @param sid     идентификатор потока
			 * @param error   флаг ошибки HTTP/2 если присутствует
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        статус полученного события
			 */
			static int close(nghttp2_session * session, const int32_t sid, const uint32_t error, void * ctx) noexcept;
			/**
			 * chunk Функция обратного вызова при получении чанка с сервера HTTP/2
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
			 * send Функция обратного вызова при подготовки данных для отправки на сервер
			 * @param session объект сессии HTTP/2
			 * @param buffer  буфер данных которые следует отправить
			 * @param size    размер буфера данных для отправки
			 * @param flags   флаги события для сессии HTTP/2
			 * @param ctx     передаваемый промежуточный контекст
			 * @return        количество отправленных байт
			 */
			static ssize_t send(nghttp2_session * session, const uint8_t * buffer, const size_t size, const int flags, void * ctx) noexcept;
			/**
			 * read Функция чтения подготовленных данных для формирования буфера данных который необходимо отправить на HTTP/2 сервер
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
		public:
			/**
			 * free Метод очистки активной сессии
			 */
			void free() noexcept;
			/**
			 * close Метод закрытия подключения
			 * @return результат закрытия подключения
			 */
			bool close() noexcept;
		public:
			/**
			 * init Метод инициализации
			 * @param settings параметры настроек сессии
			 * @return         результат выполнения инициализации
			 */
			bool init(const vector <nghttp2_settings_entry> & settings) noexcept;
		public:
			/**
			 * on Метод установки функции обратного вызова начала открытии потока
			 * @param callback функция обратного вызова
			 */
			void on(function <int (const int32_t)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова при получении фрейма заголовков
			 * @param callback функция обратного вызова
			 */
			void on(function <int (const nghttp2_frame *)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова при закрытии потока
			 * @param callback функция обратного вызова
			 */
			void on(function <int (const int32_t, const uint32_t)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова при отправки сообщения на сервер
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint8_t *, const size_t, const int)> callback) noexcept;
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
			void on(function <void (const log_t::flag_t, const web::error_t, const string &)> callback) noexcept;
		public:
			/**
			 * NgHttp2 Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			NgHttp2(const fmk_t * fmk, const log_t * log) noexcept :
			 _callback(log), session(nullptr), _fmk(fmk), _log(log) {}
			/**
			 * ~NgHttp2 Деструктор
			 */
			~NgHttp2() noexcept {}
	} nghttp2_t;
};

#endif // __AWH_NGHTTP2__
