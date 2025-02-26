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
 * Стандартные модули
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
#include <http/errors.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Web Класс для работы с парсером HTTP
	 */
	typedef class AWHSHARED_EXPORT Web {
		public:
			/**
			 * Тип используемого HTTP-модуля
			 */
			enum class hid_t : uint8_t {
				NONE   = 0x00, // HTTP-модуль не инициализирован
				CLIENT = 0x01, // HTTP-модуль является клиентом
				SERVER = 0x02  // HTTP-модуль является сервером
			};
			/**
			 * Версии протоколов соответствия
			 */
			enum class proto_t : uint8_t {
				NONE      = 0x00, // Протокол не установлен
				HTTP1     = 0x01, // Протокол принадлежит HTTP/1.0
				HTTP2     = 0x02, // Протокол принадлежит HTTP/2
				HTTP1_1   = 0x03, // Протокол принадлежит HTTP/1.1
				PROXY     = 0x04, // Протокол принадлежит Proxy
				WEBSOCKET = 0x05  // Протокол принадлежит Websocket
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
				double version;
				/**
				 * Provider Конструктор
				 */
				Provider() noexcept : version(HTTP_VERSION) {}
				/**
				 * Provider Конструктор
				 * @param version версия протокола
				 */
				Provider(const double version) noexcept : version(version) {}
			} provider_t;
			/**
			 * Request Класс HTTP-запроса клиента
			 */
			typedef class AWHSHARED_EXPORT Request : public provider_t {
				public:
					// Метод запроса клиента
					method_t method;
					// Адрес URL-запроса
					uri_t::url_t url;
				public:
					/**
					 * Оператор [=] перемещения параметров запроса клиента
					 * @param request объект параметров запроса клиента
					 * @return        текущие параметры запроса клиента
					 */
					Request & operator = (Request && request) noexcept;
					/**
					 * Оператор [=] присванивания параметров запроса клиента
					 * @param request объект параметров запроса клиента
					 * @return        текущие параметры запроса клиента
					 */
					Request & operator = (const Request & request) noexcept;
				public:
					/**
					 * Оператор сравнения
					 * @param request объект параметров запроса клиента
					 * @return        результат сравнения
					 */
					bool operator == (const Request & request) noexcept;
				public:
					/**
					 * Request Конструктор перемещения
					 * @param request объект параметров запроса клиента
					 */
					Request(Request && request) noexcept;
					/**
					 * Request Конструктор копирования
					 * @param request объект параметров запроса клиента
					 */
					Request(const Request & request) noexcept;
				public:
					/**
					 * Request Конструктор
					 */
					Request() noexcept;
					/**
					 * Request Конструктор
					 * @param method метод запроса клиента
					 */
					Request(const method_t method) noexcept;
					/**
					 * Request Конструктор
					 * @param version версия протокола
					 */
					Request(const double version) noexcept;
					/**
					 * Request Конструктор
					 * @param url адрес URL-запроса
					 */
					Request(const uri_t::url_t & url) noexcept;
					/**
					 * Request Конструктор
					 * @param version версия протокола
					 * @param method  метод запроса клиента
					 */
					Request(const double version, const method_t method) noexcept;
					/**
					 * Request Конструктор
					 * @param method метод запроса клиента
					 * @param url    адрес URL-запроса
					 */
					Request(const method_t method, const uri_t::url_t & url) noexcept;
					/**
					 * Request Конструктор
					 * @param version версия протокола
					 * @param url     адрес URL-запроса
					 */
					Request(const double version, const uri_t::url_t & url) noexcept;
					/**
					 * Request Конструктор
					 * @param version версия протокола
					 * @param method  метод запроса клиента
					 * @param url     адрес URL-запроса
					 */
					Request(const double version, const method_t method, const uri_t::url_t & url) noexcept;
				public:
					/**
					 * Request Деструктор
					 */
					~Request() noexcept {}
			} req_t;
			/**
			 * Response Класс HTTP-ответа сервера
			 */
			typedef class AWHSHARED_EXPORT Response : public provider_t {
				public:
					// Код ответа сервера
					uint32_t code;
					// Сообщение сервера
					string message;
				public:
					/**
					 * Оператор [=] перемещения параметров ответа сервера
					 * @param response объект параметров ответа сервера
					 * @return         текущие параметры ответа сервера
					 */
					Response & operator = (Response && response) noexcept;
					/**
					 * Оператор [=] присванивания параметров ответа сервера
					 * @param response объект параметров ответа сервера
					 * @return         текущие параметры ответа сервера
					 */
					Response & operator = (const Response & response) noexcept;
				public:
					/**
					 * Оператор сравнения
					 * @param response объект параметров ответа сервера
					 * @return         результат сравнения
					 */
					bool operator == (const Response & response) noexcept;
				public:
					/**
					 * Response Конструктор перемещения
					 * @param response объект параметров ответа сервера
					 */
					Response(Response && response) noexcept;
					/**
					 * Response Конструктор копирования
					 * @param response объект параметров ответа сервера
					 */
					Response(const Response & response) noexcept;
				public:
					/**
					 * Response Конструктор
					 */
					Response() noexcept;
					/**
					 * Response Конструктор
					 * @param code код ответа сервера
					 */
					Response(const uint32_t code) noexcept;
					/**
					 * Response Конструктор
					 * @param version версия протокола
					 */
					Response(const double version) noexcept;
					/**
					 * Response Конструктор
					 * @param message сообщение сервера
					 */
					Response(const string & message) noexcept;
					/**
					 * Response Конструктор
					 * @param version версия протокола
					 * @param code    код ответа сервера
					 */
					Response(const double version, const uint32_t code) noexcept;
					/**
					 * Response Конструктор
					 * @param code    код ответа сервера
					 * @param message сообщение сервера
					 */
					Response(const uint32_t code, const string & message) noexcept;
					/**
					 * Response Конструктор
					 * @param version версия протокола
					 * @param message сообщение сервера
					 */
					Response(const double version, const string & message) noexcept;
					/**
					 * Response Конструктор
					 * @param version версия протокола
					 * @param code    код ответа сервера
					 * @param message сообщение сервера
					 */
					Response(const double version, const uint32_t code, const string & message) noexcept;
				public:
					/**
					 * Response Деструктор
					 */
					~Response() noexcept {}
			} res_t;
		private:
			/**
			 * Идентификаторы актуального процесса парсинга
			 */
			enum class process_t : uint8_t {
				SIZE      = 0x01, // Ожидание получения размера
				BODY      = 0x02, // Ожидание сбора тела данных
				TRAILERS  = 0x03, // Ожидание получения трейлера
				END_SIZE  = 0x04, // Ожидание получения перевода строки после получения размера чанка
				END_BODY  = 0x05, // Ожидание получения перевода строки после получения тела чанка
				STOP_BODY = 0x06  // Ожидание получения возврата каретки после получения тела чанка
			};
			/**
			 * Chunk Структура собираемого чанка
			 */
			typedef class AWHSHARED_EXPORT Chunk {
				public:
					// Размер чанка
					size_t size;
					// Стейт чанка
					process_t state;
					// Данные чанка
					vector <char> data;
				public:
					/**
					 * clear Метод очистки данных чанка
					 */
					void clear() noexcept;
				public:
					/**
					 * Chunk Конструктор
					 */
					Chunk() noexcept : size(0), state(process_t::SIZE) {}
			} chunk_t;
		private:
			// Идентификатор объекта
			uint64_t _id;
		private:
			// Объект запроса
			req_t _request;
			// Объект ответа
			res_t _response;
		private:
			// Сепаратор для детекции в буфере
			char _separator;
		private:
			// Массив позиций в буфере сепаратора
			int64_t _pos[2];
			// Размер тела сообщения
			int64_t _bodySize;
		private:
			// Объект работы с URI
			uri_t _uri;
			// Объект собираемого чанка
			chunk_t _chunk;
			// Хранилище функций обратного вызова
			fn_t _callbacks;
		private:
			// Тип используемого HTTP-модуля
			hid_t _hid;
			// Стейт текущего запроса
			state_t _state;
		private:
			// Протокол на который запрошено переключение
			string _upgrade;
		private:
			// Полученное тело HTTP-запроса
			vector <char> _body;
		private:
			// Загруженные трейлеры
			unordered_set <string> _trailers;
			// Полученные HTTP заголовки
			unordered_multimap <string, string> _headers;
			// Список стандартных заголовков
			unordered_map <string, set <proto_t>> _standardHeaders;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
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
			 * request Метод установки объекта запроса на сервер
			 * @param request объект запроса на сервер
			 */
			void request(req_t && request) noexcept;
			/**
			 * request Метод установки объекта запроса на сервер
			 * @param request объект запроса на сервер
			 */
			void request(const req_t & request) noexcept;
		public:
			/**
			 * response Метод получения объекта ответа сервера
			 * @return объект ответа сервера
			 */
			const res_t & response() const noexcept;
			/**
			 * response Метод установки объекта ответа сервера
			 * @param response объект ответа сервера
			 */
			void response(res_t && response) noexcept;
			/**
			 * response Метод установки объекта ответа сервера
			 * @param response объект ответа сервера
			 */
			void response(const res_t & response) noexcept;
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
			/**
			 * isStandard Проверка заголовка является ли он стандартным
			 * @param key ключ заголовка для проверки
			 * @return    результат проверки
			 */
			bool isStandard(const string & key) const noexcept;
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
			 * body Метод добавления данных тела
			 * @param body буфер тела для добавления
			 */
			void body(const vector <char> & body) noexcept;
			/**
			 * body Метод добавления данных тела
			 * @param buffer буфер тела для добавления
			 * @param size   размер буфера теля для добавления
			 */
			void body(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * upgrade Метод получение названия протокола для переключения
			 * @return название протокола для переключения
			 */
			const string & upgrade() const noexcept;
			/**
			 * upgrade Метод установки название протокола для переключения
			 * @param upgrade название протокола для переключения
			 */
			void upgrade(const string & upgrade) noexcept;
		public:
			/**
			 * proto Метод извлечения список протоколов к которому принадлежит заголовок
			 * @param key ключ заголовка
			 * @return    список протоколов
			 */
			set <proto_t> proto(const string & key) const noexcept;
		public:
			/**
			 * delHeader Метод удаления заголовка
			 * @param key ключ заголовка
			 */
			void delHeader(const string & key) noexcept;
			/**
			 * header Метод получения данных заголовка
			 * @param key ключ заголовка
			 * @return    значение заголовка
			 */
			string header(const string & key) const noexcept;
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
				if((idw > 0) && (fn != nullptr))
					// Выполняем установку функции обратного вызова
					this->_callbacks.set <A> (idw, fn);
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
			 * Web Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Web(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Web Деструктор
			 */
			~Web() noexcept {}
	} web_t;
};

#endif // __AWH_HTTP_PARSER__
