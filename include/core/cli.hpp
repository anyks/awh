/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_CORE_REST_CLIENT__
#define __AWH_CORE_REST_CLIENT__

/**
 * Стандартная библиотека
 */
#include <thread>
#include <nlohmann/json.hpp>

/**
 * Наши модули
 */
#include <core/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

// Активируем json в качестве объекта пространства имён
using json = nlohmann::json;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Client Класс ядра для работы с клиентом REST
	 */
	typedef class Client : public core_t {
		public:
			/**
			 * Response Структура ответа сервера
			 */
			typedef struct Response {
				bool ok;                                     // Флаг удачного ответа
				u_short code;                                // Код ответа сервера
				string mess;                                 // Сообщение ответа сервера
				string entity;                               // Тело ответа сервера
				unordered_multimap <string, string> headers; // Заголовки сервера
				/**
				 * Response Конструктор
				 */
				Response() : ok(false), code(500), mess(""), entity(""), headers({}) {}
			} res_t;
		private:
			/**
			 * Request Структура запроса к серверу
			 */
			typedef struct Request {
				http_t::method_t method;                             // Метод выполняемого запроса
				const string * entity;                               // Тело запроса (если требуется)
				const uri_t::url_t * uri;                            // Параметры запроса
				const unordered_multimap <string, string> * headers; // Список заголовков запроса
			public:
				/**
				 * operator= Оператор установки параметров запроса
				 * @param uri объект параметра запроса
				 * @return    ссылка на контекст объекта
				 */
				Request & operator=(const uri_t::url_t * uri) noexcept {
					// Устанавливаем текст сообщения
					if(uri != nullptr){
						// Устанавливаем параметры запроса
						this->uri = uri;
						// Сбрасываем данные тела запроса
						this->entity = nullptr;
						// Сбрасываем данные заголовков запроса
						this->headers = nullptr;
						// Сбрасываем метод выполняемого запроса
						this->method = http_t::method_t::GET;
					}
					// Выводим контекст текущего объекта
					return (* this);
				}
			public:
				/**
				 * Request Конструктор
				 */
				Request() : method(http_t::method_t::GET), entity(nullptr), uri(nullptr), headers(nullptr) {}
			} req_t;
		private:
			// Параметры запроса
			req_t req;
			// Параметры ответа
			res_t res;
		private:
			/**
			 * request Метод выполнения HTTP запроса
			 */
			void request() noexcept;
			/**
			 * processing Метод обработки поступающих данных
			 * @param size размер полученных данных
			 */
			void processing(const size_t size) noexcept;
		public:
			/**
			 * GET Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
			/**
			 * DEL Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string DEL(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
		public:
			/**
			 * PUT Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PUT(const uri_t::url_t & url, const json & body, const unordered_multimap <string, string> & headers = {}) noexcept;
			/**
			 * PUT Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PUT(const uri_t::url_t & url, const string & body, const unordered_multimap <string, string> & headers = {}) noexcept;
			/**
			 * PUT Метод REST запроса
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_multimap <string, string> & headers = {}) noexcept;
		public:
			/**
			 * POST Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string POST(const uri_t::url_t & url, const json & body, const unordered_multimap <string, string> & headers = {}) noexcept;
			/**
			 * POST Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string POST(const uri_t::url_t & url, const string & body, const unordered_multimap <string, string> & headers = {}) noexcept;
			/**
			 * POST Метод REST запроса
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string POST(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_multimap <string, string> & headers = {}) noexcept;
		public:
			/**
			 * PATCH Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PATCH(const uri_t::url_t & url, const json & body, const unordered_multimap <string, string> & headers = {}) noexcept;
			/**
			 * PATCH Метод REST запроса в формате JSON
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PATCH(const uri_t::url_t & url, const string & body, const unordered_multimap <string, string> & headers = {}) noexcept;
			/**
			 * PATCH Метод REST запроса
			 * @param url     адрес запроса
			 * @param body    тело запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const string PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & body, const unordered_multimap <string, string> & headers = {}) noexcept;
		public:
			/**
			 * HEAD Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const unordered_multimap <string, string> HEAD(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
			/**
			 * TRACE Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const unordered_multimap <string, string> TRACE(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
			/**
			 * OPTIONS Метод REST запроса
			 * @param url     адрес запроса
			 * @param headers список http заголовков
			 * @return        результат запроса
			 */
			const unordered_multimap <string, string> OPTIONS(const uri_t::url_t & url, const unordered_multimap <string, string> & headers = {}) noexcept;
		public:
			/**
			 * REST Метод выполнения REST запроса на сервер
			 * @param url     адрес запроса
			 * @param method  метод запроса
			 * @param body    тело запроса
			 * @param headers список заголовков
			 */
			void REST(const uri_t::url_t & url, const http_t::method_t method, const string & body = {}, const unordered_multimap <string, string> & headers = {}) noexcept;
		public:
			/**
			 * Client Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Client(const fmk_t * fmk, const log_t * log) noexcept : core_t(fmk, log) {}
			/**
			 * ~Client Деструктор
			 */
			~Client() noexcept {}
	} cli_t;
};

#endif // __AWH_CORE_REST_CLIENT__
