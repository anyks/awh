/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
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
#include <fmk.hpp>
#include <log.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
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
			enum class hid_t : uint8_t {NONE, CLIENT, SERVER};
			/**
			 * Методы HTTP запроса
			 */
			enum class method_t : uint8_t {NONE, GET, PUT, DEL, POST, HEAD, PATCH, TRACE, OPTIONS, CONNECT};
			/**
			 * Query Структура запроса
			 */
			typedef struct Query {
				float ver;       // Версия протокола
				u_short code;    // Код ответа сервера
				method_t method; // Метод запроса
				string uri;      // Параметры запроса
				string message;  // Сообщение сервера
				/**
				 * Query Конструктор
				 */
				Query() : code(0), ver(HTTP_VERSION), method(method_t::NONE), uri(""), message("") {}
			} query_t;
		private:
			/**
			 * Стейты работы чанка
			 */
			enum class cstate_t : uint8_t {
				SIZE,    // Ожидание получения размера
				BODY,    // Ожидание сбора тела данных
				ENDSIZE, // Ожидание получения перевода строки после получения размера чанка
				ENDBODY, // Ожидание получения перевода строки после получения тела чанка
				STOPBODY // Ожидание получения возврата каретки после получения тела чанка
			};
			/**
			 * Стейты работы модуля
			 */
			enum class state_t : uint8_t {
				END,    // Режим завершения сбора данных
				BODY,   // Режим чтения тела сообщения
				QUERY,  // Режим ожидания получения запроса
				HEADERS // Режим чтения заголовков
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
					Chunk() : size(0), state(cstate_t::SIZE) {}
			} chunk_t;
		private:
			// Сепаратор для детекции в буфере
			char sep = '\0';
			// Размер тела сообщения
			int64_t bodySize = -1;
			// Массив позиций в буфере сепаратора
			int64_t pos[2] = {-1, -1};
		private:
			// Объект параметров запроса
			query_t query;
			// Объект собираемого чанка
			chunk_t chunk;
		private:
			// Тип используемого HTTP модуля
			hid_t httpType = hid_t::NONE;
			// Стейт текущего запроса
			state_t state = state_t::QUERY;
		private:
			// Данные пустого заголовка
			string header = "";
			// Полученное тело HTTP запроса
			vector <char> body;
			// Полученные HTTP заголовки
			unordered_multimap <string, string> headers;
		private:
			// Список контекстов передаваемых объектов
			vector <void *> ctx = {nullptr};
			// Функция вызова при получении чанка
			function <void (const vector <char> &, const Web *, void *)> chunkingFn = nullptr;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Создаём объект работы с логами
			const log_t * log = nullptr;
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
			 * getQuery Метод получения объекта запроса сервера
			 * @return объект запроса сервера
			 */
			const query_t & getQuery() const noexcept;
			/**
			 * setQuery Метод добавления объекта запроса клиента
			 * @param query объект запроса клиента
			 */
			void setQuery(const query_t & query) noexcept;
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
			 * setBody Метод установки данных тела
			 * @param body буфер тела для установки
			 */
			void setBody(const vector <char> & body) noexcept;
			/**
			 * addBody Метод добавления буфера тела данных запроса
			 * @param buffer буфер данных тела запроса
			 * @param size   размер буфера данных
			 */
			void addBody(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * addHeader Метод добавления заголовка
			 * @param key ключ заголовка
			 * @param val значение заголовка
			 */
			void addHeader(const string & key, const string & val) noexcept;
			/**
			 * setHeaders Метод установки списка заголовков
			 * @param headers список заголовков для установки
			 */
			void setHeaders(const unordered_multimap <string, string> & headers) noexcept;
		public:
			/**
			 * getBody Метод получения данных тела запроса
			 * @return буфер данных тела запроса
			 */
			const vector <char> & getBody() const noexcept;
			/**
			 * getHeader Метод получения данных заголовка
			 * @param key ключ заголовка
			 * @return    значение заголовка
			 */
			const string & getHeader(const string & key) const noexcept;
			/**
			 * getHeaders Метод получения списка заголовков
			 * @return список существующих заголовков
			 */
			const unordered_multimap <string, string> & getHeaders() const noexcept;
		public:
			/**
			 * init Метод инициализации модуля
			 * @param hid тип используемого HTTP модуля
			 */
			void init(const hid_t hid) noexcept;
		public:
			/**
			 * setChunkingFn Метод установки функции обратного вызова для получения чанков
			 * @param ctx      контекст для вывода в сообщении
			 * @param callback функция обратного вызова
			 */
			void setChunkingFn(void * ctx, function <void (const vector <char> &, const Web *, void *)> callback) noexcept;
		public:
			/**
			 * Web Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Web(const fmk_t * fmk, const log_t * log) noexcept : fmk(fmk), log(log) {}
			/**
			 * ~Web Деструктор
			 */
			~Web() noexcept {}
	} web_t;
};

#endif // __AWH_HTTP_PARSER__
