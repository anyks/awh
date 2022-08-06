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
#include <sys/fmk.hpp>
#include <sys/log.hpp>

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
			 * Тип используемого HTTP модуля
			 */
			enum class hid_t : uint8_t {
				NONE   = 0x00, // HTTP модуль не инициализирован
				CLIENT = 0x01, // HTTP модуль является клиентом
				SERVER = 0x02  // HTTP модуль является сервером
			};
			/**
			 * Методы HTTP запроса
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
			/**
			 * Query Структура запроса
			 */
			typedef struct Query {
				float ver;       // Версия протокола
				u_int code;      // Код ответа сервера
				method_t method; // Метод запроса
				string uri;      // Параметры запроса
				string message;  // Сообщение сервера
				/**
				 * Query Конструктор
				 */
				Query() noexcept : code(0), ver(HTTP_VERSION), method(method_t::NONE), uri(""), message("") {}
			} query_t;
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
			 * Стейты работы модуля
			 */
			enum class state_t : uint8_t {
				END     = 0x01, // Режим завершения сбора данных
				BODY    = 0x02, // Режим чтения тела сообщения
				QUERY   = 0x03, // Режим ожидания получения запроса
				HEADERS = 0x04  // Режим чтения заголовков
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
			// Сепаратор для детекции в буфере
			char _sep;
			// Размер тела сообщения
			int64_t _bodySize;
			// Массив позиций в буфере сепаратора
			int64_t _pos[2] = {-1, -1};
		private:
			// Объект параметров запроса
			query_t _query;
			// Объект собираемого чанка
			chunk_t _chunk;
		private:
			// Стейт текущего запроса
			state_t _state;
			// Тип используемого HTTP модуля
			hid_t _httpType;
		private:
			// Данные пустого заголовка
			string _header;
			// Полученное тело HTTP запроса
			vector <char> _body;
			// Полученные HTTP заголовки
			unordered_multimap <string, string> _headers;
		private:
			// Функция вызова при получении чанка
			function <void (const vector <char> &, const Web *)> _fn;
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
			 * query Метод получения объекта запроса сервера
			 * @return объект запроса сервера
			 */
			const query_t & query() const noexcept;
			/**
			 * query Метод добавления объекта запроса клиента
			 * @param query объект запроса клиента
			 */
			void query(const query_t & query) noexcept;
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
			/**
			 * addBody Метод добавления буфера тела данных запроса
			 * @param buffer буфер данных тела запроса
			 * @param size   размер буфера данных
			 */
			void body(const char * buffer, const size_t size) noexcept;
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
			const string & header(const string & key) const noexcept;
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
			 * init Метод инициализации модуля
			 * @param hid тип используемого HTTP модуля
			 */
			void init(const hid_t hid) noexcept;
		public:
			/**
			 * chunking Метод установки функции обратного вызова для получения чанков
			 * @param callback функция обратного вызова
			 */
			void chunking(function <void (const vector <char> &, const Web *)> callback) noexcept;
		public:
			/**
			 * Web Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Web(const fmk_t * fmk, const log_t * log) noexcept :
			 _sep('\0'), _bodySize(-1), _state(state_t::QUERY),
			 _httpType(hid_t::NONE), _header(""), _fn(nullptr), _fmk(fmk), _log(log) {}
			/**
			 * ~Web Деструктор
			 */
			~Web() noexcept {}
	} web_t;
};

#endif // __AWH_HTTP_PARSER__
