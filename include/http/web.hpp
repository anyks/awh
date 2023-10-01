/**
 * @file: web.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_HTTP_PARSER__
#define __AWH_HTTP_PARSER__

/**
 * Стандартная библиотека
 */
#include <set>
#include <string>
#include <vector>
#include <cstring>
#include <unordered_set>
#include <unordered_map>

/**
 * Наши модули
 */
#include <sys/fn.hpp>
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/uri.hpp>
#include <net/net.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Web Класс для работы с парсером HTTP
	 */
	typedef class Web {
		public:
			/**
			 * Тип используемого HTTP-модуля
			 */
			enum class hid_t : uint8_t {
				NONE   = 0x00, // HTTP модуль не инициализирован
				CLIENT = 0x01, // HTTP модуль является клиентом
				SERVER = 0x02  // HTTP модуль является сервером
			};
			/**
			 * Стейты работы модуля
			 */
			enum class state_t : uint8_t {
				END     = 0x01, // Режим завершения сбора данных
				BODY    = 0x02, // Режим чтения тела сообщения
				QUERY   = 0x03, // Режим ожидания получения запроса
				HEADERS = 0x04  // Режим чтения заголовков
			};
			/**
			 * Методы HTTP-запроса
			 */
			enum class method_t : uint8_t {
				NONE    = 0x00, // Метод не установлен
				GET     = 0x01, // Метод GET
				PUT     = 0x02, // Метод PUT
				DEL     = 0x03, // Метод DELETE
				POST    = 0x04, // Метод POST
				HEAD    = 0x05, // Метод HEAD
				PATCH   = 0x06, // Метод PATCH
				TRACE   = 0x07, // Метод TRACE
				OPTIONS = 0x08, // Метод OPTIONS
				CONNECT = 0x09  // Метод CONNECT
			};
		public:
			/**
			 * Provider Структура провайдера
			 */
			typedef struct Provider {
				// Версия протокола
				float version;
				/**
				 * Provider Конструктор
				 */
				Provider() noexcept : version(HTTP_VERSION) {}
				/**
				 * Provider Конструктор
				 * @param version версия протокола
				 */
				Provider(const float version) noexcept : version(version) {}
			} provider_t;
			/**
			 * Request Структура запроса
			 */
			typedef struct Request : public provider_t {
				// Метод запроса
				method_t method;
				// Адрес URL-запроса
				uri_t::url_t url;
				/**
				 * Request Конструктор
				 */
				Request() noexcept : provider_t(), method(method_t::NONE) {}
				/**
				 * Request Конструктор
				 * @param method метод запроса
				 */
				Request(const method_t method) noexcept : provider_t(), method(method) {}
				/**
				 * Request Конструктор
				 * @param version версия протокола
				 */
				Request(const float version) noexcept : provider_t(version), method(method_t::NONE) {}
				/**
				 * Request Конструктор
				 * @param url адрес URL-запроса
				 */
				Request(const uri_t::url_t & url) noexcept : provider_t(), method(method_t::NONE), url(url) {}
				/**
				 * Request Конструктор
				 * @param version версия протокола
				 * @param method  метод запроса
				 */
				Request(const float version, const method_t method) noexcept : provider_t(version), method(method) {}
				/**
				 * Request Конструктор
				 * @param method метод запроса
				 * @param url    адрес URL-запроса
				 */
				Request(const method_t method, const uri_t::url_t & url) noexcept : provider_t(), method(method), url(url) {}
				/**
				 * Request Конструктор
				 * @param version версия протокола
				 * @param url     адрес URL-запроса
				 */
				Request(const float version, const uri_t::url_t & url) noexcept : provider_t(version), method(method_t::NONE), url(url) {}
				/**
				 * Request Конструктор
				 * @param version версия протокола
				 * @param method  метод запроса
				 * @param url     адрес URL-запроса
				 */
				Request(const float version, const method_t method, const uri_t::url_t & url) noexcept : provider_t(version), method(method), url(url) {}
			} req_t;
			/**
			 * Response Структура ответа сервера
			 */
			typedef struct Response : public provider_t {
				// Код ответа сервера
				u_int code;
				// Сообщение сервера
				string message;
				/**
				 * Response Конструктор
				 */
				Response() noexcept : provider_t(), code(0), message{""} {}
				/**
				 * Response Конструктор
				 * @param code код ответа сервера
				 */
				Response(const u_int code) noexcept : provider_t(), code(code), message{""} {}
				/**
				 * Response Конструктор
				 * @param version версия протокола
				 */
				Response(const float version) noexcept : provider_t(version), code(0), message{""} {}
				/**
				 * Response Конструктор
				 * @param message сообщение сервера
				 */
				Response(const string & message) noexcept : provider_t(), code(0), message(message) {}
				/**
				 * Response Конструктор
				 * @param version версия протокола
				 * @param code    код ответа сервера
				 */
				Response(const float version, const u_int code) noexcept : provider_t(version), code(code), message{""} {}
				/**
				 * Response Конструктор
				 * @param code    код ответа сервера
				 * @param message сообщение сервера
				 */
				Response(const u_int code, const string & message) noexcept : provider_t(), code(code), message(message) {}
				/**
				 * Response Конструктор
				 * @param version версия протокола
				 * @param message сообщение сервера
				 */
				Response(const float version, const string & message) noexcept : provider_t(version), code(0), message(message) {}
				/**
				 * Response Конструктор
				 * @param version версия протокола
				 * @param code    код ответа сервера
				 * @param message сообщение сервера
				 */
				Response(const float version, const u_int code, const string & message) noexcept : provider_t(version), code(code), message(message) {}
			} res_t;
		private:
			/**
			 * Стейты работы чанка
			 */
			enum class cstate_t : uint8_t {
				SIZE     = 0x01, // Ожидание получения размера
				BODY     = 0x02, // Ожидание сбора тела данных
				ENDSIZE  = 0x03, // Ожидание получения перевода строки после получения размера чанка
				ENDBODY  = 0x04, // Ожидание получения перевода строки после получения тела чанка
				STOPBODY = 0x05  // Ожидание получения возврата каретки после получения тела чанка
			};
			/**
			 * Chunk Структура собираемого чанка
			 */
			typedef struct Chunk {
				public:
					size_t size;        // Размер чанка
					cstate_t state;     // Стейт чанка
					vector <char> data; // Данные чанка
				public:
					/**
					 * clear Метод очистки данных чанка
					 */
					void clear() noexcept {
						// Обнуляем размер чанка
						this->size = 0;
						// Обнуляем буфер данных
						this->data.clear();
						// Выполняем сброс стейта чанка
						this->state = cstate_t::SIZE;
					}
				public:
					/**
					 * Chunk Конструктор
					 */
					Chunk() noexcept : size(0), state(cstate_t::SIZE) {}
			} chunk_t;
		private:
			// Идентификатор объекта
			uint64_t _id;
		private:
			// Объект запроса
			req_t _req;
			// Объект ответа
			res_t _res;
		private:
			// Сепаратор для детекции в буфере
			char _separator;
		private:
			// Массив позиций в буфере сепаратора
			int64_t _pos[2];
			// Размер тела сообщения
			int64_t _bodySize;
		private:
			// Создаём объект работы с URI
			uri_t _uri;
			// Объявляем функции обратного вызова
			fn_t _callback;
			// Объект собираемого чанка
			chunk_t _chunk;
		private:
			// Тип используемого HTTP модуля
			hid_t _hid;
			// Стейт текущего запроса
			state_t _state;
		private:
			// Полученное тело HTTP запроса
			vector <char> _body;
			// Полученные HTTP заголовки
			unordered_multimap <string, string> _headers;
		private:
			// Создаём объект фреймворка
			const fmk_t * _fmk;
			// Создаём объект работы с логами
			const log_t * _log;
		private:
			/**
			 * parseBody Метод извлечения полезной нагрузки
			 * @param buffer буфер данных для чтения
			 * @param size   размер буфера данных для чтения
			 * @return       размер обработанных данных
			 */
			size_t readPayload(const char * buffer, const size_t size) noexcept;
			/**
			 * readHeaders Метод извлечения заголовков
			 * @param buffer буфер данных для чтения
			 * @param size   размер буфера данных для чтения
			 * @return       размер обработанных данных
			 */
			size_t readHeaders(const char * buffer, const size_t size) noexcept;
		private:
			/**
			 * prepare Метод препарирования HTTP заголовков
			 * @param buffer   буфер данных для парсинга
			 * @param size     размер буфера данных для парсинга
			 * @param callback функция обратного вызова
			 */
			void prepare(const char * buffer, const size_t size, function <void (const char *, const size_t, const size_t, const bool)> callback) noexcept;
		public:
			/**
			 * dump Метод получения бинарного дампа
			 * @return бинарный дамп данных
			 */
			vector <char> dump() const noexcept;
			/**
			 * dump Метод установки бинарного дампа
			 * @param data бинарный дамп данных
			 */
			void dump(const vector <char> & data) noexcept;
		public:
			/**
			 * parse Метод выполнения парсинга HTTP буфера данных
			 * @param buffer буфер данных для парсинга
			 * @param size   размер буфера данных для парсинга
			 * @return       размер обработанных данных
			 */
			size_t parse(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * clear Метод очистки собранных данных
			 */
			void clear() noexcept;
			/**
			 * reset Метод сброса стейтов парсера
			 */
			void reset() noexcept;
		public:
			/**
			 * request Метод получения объекта запроса на сервер
			 * @return объект запроса на сервер
			 */
			const req_t & request() const noexcept;
			/**
			 * request Метод добавления объекта запроса на сервер
			 * @param req объект запроса на сервер
			 */
			void request(const req_t & req) noexcept;
		public:
			/**
			 * response Метод получения объекта ответа сервера
			 * @return объект ответа сервера
			 */
			const res_t & response() const noexcept;
			/**
			 * response Метод добавления объекта ответа сервера
			 * @param res объект ответа сервера
			 */
			void response(const res_t & res) noexcept;
		public:
			/**
			 * isEnd Метод проверки завершения обработки
			 * @return результат проверки
			 */
			bool isEnd() const noexcept;
			/**
			 * isHeader Метод проверки существования заголовка
			 * @param key ключ заголовка для проверки
			 * @return    результат проверки
			 */
			bool isHeader(const string & key) const noexcept;
		public:
			/**
			 * clearBody Метод очистки данных тела
			 */
			void clearBody() noexcept;
			/**
			 * clearHeaders Метод очистки списка заголовков
			 */
			void clearHeaders() noexcept;
		public:
			/**
			 * body Метод получения данных тела запроса
			 * @return буфер данных тела запроса
			 */
			const vector <char> & body() const noexcept;
			/**
			 * body Метод установки данных тела
			 * @param body буфер тела для установки
			 */
			void body(const vector <char> & body) noexcept;
		public:
			/**
			 * rmHeader Метод удаления заголовка
			 * @param key ключ заголовка
			 */
			void rmHeader(const string & key) noexcept;
			/**
			 * header Метод получения данных заголовка
			 * @param key ключ заголовка
			 * @return    значение заголовка
			 */
			const string header(const string & key) const noexcept;
			/**
			 * header Метод добавления заголовка
			 * @param key ключ заголовка
			 * @param val значение заголовка
			 */
			void header(const string & key, const string & val) noexcept;
		public:
			/**
			 * headers Метод получения списка заголовков
			 * @return список существующих заголовков
			 */
			const unordered_multimap <string, string> & headers() const noexcept;
			/**
			 * headers Метод установки списка заголовков
			 * @param headers список заголовков для установки
			 */
			void headers(const unordered_multimap <string, string> & headers) noexcept;
		public:
			/**
			 * id Метод получения идентификатора объекта
			 * @return идентификатор объекта
			 */
			uint64_t id() const noexcept;
			/**
			 * id Метод установки идентификатора объекта
			 * @param id идентификатор объекта
			 */
			void id(const uint64_t id) noexcept;
		public:
			/**
			 * hid Метод вывода идентификатора модуля
			 * @return тип используемого HTTP-модуля
			 */
			const hid_t hid() const noexcept;
			/**
			 * hid Метод установки идентификатора модуля
			 * @param hid тип используемого HTTP-модуля
			 */
			void hid(const hid_t hid) noexcept;
		public:
			/** 
			 * state Метод установки стейта ожидания данных
			 * @param state стейт ожидания данных для установки
			 */
			void state(const state_t state) noexcept;
		public:
			/** 
			 * on Метод установки функции вывода ответа сервера на ранее выполненный запрос
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const u_int, const string &)> callback) noexcept;
			/** 
			 * on Метод установки функции вывода запроса клиента на выполненный запрос к серверу
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const method_t, const uri_t::url_t &)> callback) noexcept;
		public:
			/** 
			 * on Метод установки функции вывода полученного заголовка с сервера
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const string &, const string &)> callback) noexcept;
			/**
			 * on Метод установки функции обратного вызова для получения чанков
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const vector <char> &, const Web *)> callback) noexcept;
		public:
			/** 
			 * on Метод установки функции вывода полученного тела данных с сервера
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const u_int, const string &, const vector <char> &)> callback) noexcept;
			/** 
			 * on Метод установки функции вывода полученного тела данных с сервера
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const method_t, const uri_t::url_t &, const vector <char> &)> callback) noexcept;
		public:
			/** 
			 * on Метод установки функции вывода полученных заголовков с сервера
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const u_int, const string &, const unordered_multimap <string, string> &)> callback) noexcept;
			/** 
			 * on Метод установки функции вывода полученных заголовков с сервера
			 * @param callback функция обратного вызова
			 */
			void on(function <void (const uint64_t, const method_t, const uri_t::url_t &, const unordered_multimap <string, string> &)> callback) noexcept;
		public:
			/**
			 * Web Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Web(const fmk_t * fmk, const log_t * log) noexcept :
			 _separator('\0'), _pos{-1,-1}, _bodySize(-1), _uri(fmk), _callback(log),
			 _hid(hid_t::NONE), _state(state_t::QUERY), _fmk(fmk), _log(log) {}
			/**
			 * ~Web Деструктор
			 */
			~Web() noexcept {}
	} web_t;
};

#endif // __AWH_HTTP_PARSER__
