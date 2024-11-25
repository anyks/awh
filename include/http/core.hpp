/**
 * @file: core.hpp
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

#ifndef __AWH_HTTP__
#define __AWH_HTTP__

/**
 * Стандартные модули
 */
#include <ctime>
#include <string>
#include <vector>
#include <cstring>
#include <unordered_set>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/hash.hpp>
#include <http/web.hpp>
#include <auth/client.hpp>
#include <auth/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;
using namespace std::placeholders;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Http Класс для работы с REST
	 */
	typedef class AWHSHARED_EXPORT Http {
		public:
			/**
			 * Флаги наборов
			 */
			enum class suite_t : uint8_t {
				NONE     = 0x00, // Набор не установлен
				BODY     = 0x01, // Набор соответствует телу сообщения
				BLACK    = 0x02, // Набор соответствует заголовку чёрного списка
				HEADER   = 0x03, // Набор соответствует заголовку сообщения
				STANDARD = 0x04  // Набор соответствует стандартному заголовку
			};
			/**
			 * Статусы проверки авторизации
			 */
			enum class status_t : uint8_t {
				NONE  = 0x00, // Проверка авторизации не проводилась
				GOOD  = 0x01, // Авторизация прошла успешно
				RETRY = 0x02, // Требуется повторить попытку
				FAULT = 0x03  // Авторизация не удалась
			};
			/**
			 * Флаг выполняемого процесса
			 */
			enum class process_t : uint8_t {
				NONE     = 0x00, // Операция не установлена
				REQUEST  = 0x01, // Операция запроса
				RESPONSE = 0x02  // Операция ответа
			};
			/**
			 * Идентичность протокола
			 */
			enum class identity_t : uint8_t {
				NONE  = 0x00, // Протокол не установлен
				WS    = 0x01, // Протокол Websocket
				HTTP  = 0x02, // Протокол HTTP
				PROXY = 0x03  // Протокол Proxy
			};
			/**
			 * Стейты работы модуля
			 */
			enum class state_t : uint8_t {
				NONE      = 0x00, // Режим стейта не выставлен
				END       = 0x01, // Режим завершения сбора данных
				GOOD      = 0x02, // Режим удачного выполнения запроса
				ALIVE     = 0x03, // Режим уставновки постоянного подключения
				BROKEN    = 0x04, // Режим бракованных данных
				TRAILERS  = 0x05, // Режим запроса получения трейлеров
				HANDSHAKE = 0x06  // Режим выполненного рукопожатия
			};
			/**
			 * Формат сжатия тела запроса
			 */
			enum class compressor_t : uint8_t {
				NONE    = 0x00, // Метод компрессии не установлен
				LZ4     = 0x01, // Метод компрессии Lz4
				LZMA    = 0x02, // Метод компрессии LZma
				ZSTD    = 0x03, // Метод компрессии ZStd
				GZIP    = 0x04, // Метод компрессии GZip
				BZIP2   = 0x05, // Метод компрессии BZip2
				BROTLI  = 0x06, // Метод компрессии Brotli
				DEFLATE = 0x07  // Метод компрессии Deflate
			};
		public:
			/**
			 * Ident Структура идентификации сервиса
			 */
			typedef struct Ident {
				// Идентификатор сервиса
				string id;
				// Название сервиса
				string name;
				// Версия модуля приложения
				string version;
				/**
				 * Ident Конструктор
				 */
				Ident() noexcept :
				 id{AWH_SHORT_NAME},
				 name{AWH_NAME},
				 version{AWH_VERSION} {}
			} ident_t;
		protected:
			/**
			 * TransferEncoding Параметры запроса для Transfer-Encoding
			 */
			typedef struct TransferEncoding {
				bool enabled;  // Флаг активирования передачи ответа Transfer-Encoding
				bool trailers; // Флаг разрешающий передавать трейлеры
				bool chunking; // Флаг разрешающий передавать тело чанками
				/**
				 * TransferEncoding Конструктор
				 */
				TransferEncoding() noexcept :
				 enabled(false), trailers(false), chunking(false) {}
			} __attribute__((packed)) te_t;
			/**
			 * Compressor Структура параметров компрессора
			 */
			typedef struct Compressor {
				compressor_t current;                    // Компрессор которым сжаты данные полезной нагрузки в настоящий момент времени
				compressor_t selected;                   // Выбранный компрессор которым необходимо выполнить сжатие данных полезной нагрузки
				std::map <float, compressor_t> supports; // Список поддерживаемых компрессоров
				/**
				 * Compressor Конструктор
				 */
				Compressor() noexcept :
				 current(compressor_t::NONE),
				 selected(compressor_t::NONE) {}
			} compressors_t;
			/**
			 * Auth Структура объекта авторизации
			 */
			typedef struct Auth {
				client::auth_t client; // Объект для работы с клиентской авторизацией
				server::auth_t server; // Объект для работы с серверной авторизацией
				/**
				 * Auth Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Auth(const fmk_t * fmk, const log_t * log) noexcept : client(fmk, log), server(fmk, log) {}
			} auth_t;
		private:
			// Объект работы с операционной системой
			os_t _os;
		protected:
			// Объект работы с URI
			uri_t _uri;
			// Хранилище функций обратного вызова
			fn_t _callbacks;
		private:
			// Объект Transfer-Encoding
			mutable te_t _te;
		protected:
			// Объект HTTP-парсера
			mutable web_t _web;
			// Объект авторизации
			mutable auth_t _auth;
		protected:
			// Объект хэширования
			mutable hash_t _hash;
		protected:
			// Флаг зашифрованной полезной нагрузки
			bool _crypted;
			// Флаг зашифрованных данных
			bool _encryption;
		private:
			// Флаг точной установки хоста
			bool _precise;
		private:
			// Размер одного чанка
			size_t _chunk;
		private:
			// Идентификация сервиса
			ident_t _ident;
		protected:
			// Стейт текущего запроса
			state_t _state;
			// Стейт проверки авторизации
			status_t _status;
		protected:
			// Идентичность протокола
			identity_t _identity;
		protected:
			// Формат шифрования
			hash_t::cipher_t _cipher;
		protected:
			// Компрессор для жатия данных
			compressors_t _compressors;
		private:
			// User-Agent для HTTP-запроса
			mutable string _userAgent;
		private:
			// Список HTTP-ответов
			std::map <uint16_t, string> _responses;
		protected:
			// Чёрный список заголовков
			mutable std::unordered_set <string> _black;
		protected:
			// Список отправляемых трейлеров
			std::unordered_map <string, string> _trailers;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * chunking Метод вывода полученных чанков полезной нагрузки
			 * @param id     идентификатор объекта
			 * @param buffer буфер данных чанка полезной нагрузки
			 * @param web    объект HTTP-парсера
			 */
			void chunking(const uint64_t id, const vector <char> & buffer, const web_t * web) noexcept;
		protected:
			/**
			 * encrypt Метод выполнения шифрования полезной нагрузки
			 */
			void encrypt() noexcept;
			/**
			 * decrypt Метод выполнения дешифровани полезной нагрузки
			 */
			void decrypt() noexcept;
		protected:
			/**
			 * compress Метод выполнения декомпрессии полезной нагрузки
			 */
			void compress() noexcept;
			/**
			 * decompress Метод выполнения компрессии полезной нагрузки
			 */
			void decompress() noexcept;
		public:
			/**
			 * commit Метод применения полученных результатов
			 */
			virtual void commit() noexcept;
		protected:
			/**
			 * status Метод проверки текущего статуса
			 * @return результат проверки текущего статуса
			 */
			virtual status_t status() noexcept = 0;
		public:
			/**
			 * precise Метод установки флага точной установки хоста
			 * @param mode флаг для установки
			 */
			void precise(const bool mode) noexcept;
		public:
			/**
			 * clear Метод очистки собранных данных
			 */
			virtual void clear() noexcept;
			/**
			 * reset Метод сброса параметров запроса
			 */
			virtual void reset() noexcept;
			/**
			 * clear Метод очистки данных HTTP-протокола
			 * @param suite тип набора к которому соответствует заголовок
			 */
			void clear(const suite_t suite) noexcept;
		public:
			/**
			 * parse Метод парсинга сырых данных
			 * @param buffer буфер данных для обработки
			 * @param size   размер буфера данных
			 * @return       размер обработанных данных
			 */
			size_t parse(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * proto Метод извлечения список протоколов к которому принадлежит заголовок
			 * @param key ключ заголовка
			 * @return    список протоколов
			 */
			std::set <web_t::proto_t> proto(const string & key) const noexcept;
		public:
			/**
			 * payload Метод чтения чанка полезной нагрузки
			 * @return текущий чанк полезной нагрузки
			 */
			const vector <char> payload() const noexcept;
			/**
			 * payload Метод установки чанка полезной нагрузки
			 * @param payload буфер чанка полезной нагрузки
			 */
			void payload(const vector <char> & payload) noexcept;
		public:
			/**
			 * black Метод добавления заголовка в чёрный список
			 * @param key ключ заголовка
			 */
			void black(const string & key) noexcept;
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
			 * trailers Метод получения списка установленных трейлеров
			 * @return количество установленных трейлеров
			 */
			size_t trailers() const noexcept;
			/**
			 * trailer Метод установки трейлера
			 * @param key ключ заголовка
			 * @param val значение заголовка
			 */
			void trailer(const string & key, const string & val) noexcept;
		public:
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
			const std::unordered_multimap <string, string> & headers() const noexcept;
			/**
			 * headers Метод установки списка заголовков
			 * @param headers список заголовков для установки
			 */
			void headers(const std::unordered_multimap <string, string> & headers) noexcept;
		public:
			/**
			 * header2 Метод добавления заголовка в формате HTTP/2
			 * @param key ключ заголовка
			 * @param val значение заголовка
			 */
			void header2(const string & key, const string & val) noexcept;
			/**
			 * headers2 Метод установки списка заголовков в формате HTTP/2
			 * @param headers список заголовков для установки
			 */
			void headers2(const vector <std::pair <string, string>> & headers) noexcept;
		public:
			/**
			 * auth Метод проверки статуса авторизации
			 * @return результат проверки
			 */
			status_t auth() const noexcept;
			/**
			 * auth Метод извлечения строки авторизации
			 * @param flag флаг выполняемого процесса
			 * @param prov параметры провайдера обмена сообщениями
			 * @return     строка авторизации на удалённом сервере
			 */
			string auth(const process_t flag, const web_t::provider_t & prov) const noexcept;
		public:
			/**
			 * url Метод извлечения параметров запроса
			 * @return установленные параметры запроса
			 */
			const uri_t::url_t & url() const noexcept;
		public:
			/**
			 * compression Метод извлечения выбранного метода компрессии
			 * @return метод компрессии
			 */
			compressor_t compression() const noexcept;
			/**
			 * compression Метод установки выбранного метода компрессии
			 * @param compressor метод компрессии
			 */
			void compression(const compressor_t compressor) noexcept;
			/**
			 * compressors Метод установки списка поддерживаемых компрессоров
			 * @param compressors методы компрессии данных полезной нагрузки
			 */
			void compressors(const vector <compressor_t> & compressors) noexcept;
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
			 * empty Метод проверки существования данных
			 * @param suite тип набора к которому соответствует заголовок
			 */
			bool empty(const suite_t suite) const noexcept;
		public:
			/**
			 * is Метод проверки активного состояния
			 * @param state состояние которое необходимо проверить
			 */
			bool is(const state_t state) const noexcept;
			/**
			 * is Метод проверки существования заголовка
			 * @param suite тип набора к которому соответствует заголовок
			 * @param key   ключ заголовка для проверки
			 * @return      результат проверки
			 */
			bool is(const suite_t suite, const string & key) const noexcept;
		public:
			/**
			 * rm Метод удаления установленных заголовков
			 * @param suite тип набора к которому соответствует заголовок
			 * @param key   ключ заголовка для удаления
			 */
			void rm(const suite_t suite, const string & key) const noexcept;
		public:
			/**
			 * request Метод получения объекта запроса на сервер
			 * @return объект запроса на сервер
			 */
			const web_t::req_t & request() const noexcept;
			/**
			 * request Метод добавления объекта запроса на сервер
			 * @param req объект запроса на сервер
			 */
			void request(const web_t::req_t & req) noexcept;
		public:
			/**
			 * response Метод получения объекта ответа сервера
			 * @return объект ответа сервера
			 */
			const web_t::res_t & response() const noexcept;
			/**
			 * response Метод добавления объекта ответа сервера
			 * @param res объект ответа сервера
			 */
			void response(const web_t::res_t & res) noexcept;
		public:
			/**
			 * date Метод получения текущей даты для HTTP-запроса
			 * @param stamp штамп времени в числовом виде
			 * @return      штамп времени в текстовом виде
			 */
			const string date(const time_t stamp = 0) const noexcept;
			/**
			 * message Метод получения HTTP сообщения
			 * @param code код сообщения для получение
			 * @return     соответствующее коду HTTP сообщение
			 */
			const string & message(const uint32_t code) const noexcept;
		public:
			/**
			 * mapping Метод маппинга полученных данных
			 * @param flag флаг выполняемого процесса
			 * @param http объект для маппинга
			 */
			void mapping(const process_t flag, Http & http) noexcept;
		public:
			/**
			 * trailer Метод получения буфера отправляемого трейлера
			 * @return буфер данных ответа в бинарном виде
			 */
			vector <char> trailer() const noexcept;
			/**
			 * trailers2 Метод получения буфера отправляемых трейлеров (для протокола HTTP/2)
			 * @return буфер данных ответа в бинарном виде
			 */
			vector <std::pair <string, string>> trailers2() const noexcept;
		public:
			/**
			 * proxy Метод создания запроса для авторизации на прокси-сервере
			 * @param req объект параметров REST-запроса
			 * @return    буфер данных запроса в бинарном виде
			 */
			virtual vector <char> proxy(const web_t::req_t & req) const noexcept;
			/**
			 * proxy2 Метод создания запроса для авторизации на прокси-сервере (для протокола HTTP/2)
			 * @param req объект параметров REST-запроса
			 * @return    буфер данных запроса в бинарном виде
			 */
			virtual vector <std::pair <string, string>> proxy2(const web_t::req_t & req) const noexcept;
		public:
			/**
			 * reject Метод создания отрицательного ответа
			 * @param req объект параметров REST-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			virtual vector <char> reject(const web_t::res_t & res) const noexcept;
			/**
			 * reject2 Метод создания отрицательного ответа (для протокола HTTP/2)
			 * @param req объект параметров REST-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			virtual vector <std::pair <string, string>> reject2(const web_t::res_t & res) const noexcept;
		public:
			/**
			 * process Метод создания выполняемого процесса в бинарном виде
			 * @param flag флаг выполняемого процесса
			 * @param prov параметры провайдера обмена сообщениями
			 * @return     буфер данных в бинарном виде
			 */
			virtual vector <char> process(const process_t flag, const web_t::provider_t & prov) const noexcept;
			/**
			 * process2 Метод создания выполняемого процесса в бинарном виде (для протокола HTTP/2)
			 * @param flag флаг выполняемого процесса
			 * @param prov параметры провайдера обмена сообщениями
			 * @return     буфер данных в бинарном виде
			 */
			virtual vector <std::pair <string, string>> process2(const process_t flag, const web_t::provider_t & prov) const noexcept;
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
					// Устанавливаем функцию обратного вызова в дочерний модуль
					this->_web.callback <A> (idw, fn);
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
				if(!name.empty() && (fn != nullptr)){
					// Устанавливаем функцию обратного вызова в дочерний модуль
					this->_web.callback <A> (name, fn);
					// Выполняем установку функции обратного вызова
					this->_callbacks.set <A> (name, fn);
				}
			}
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
			 * identity Метод извлечения идентичности протокола модуля
			 * @return флаг идентичности протокола модуля
			 */
			identity_t identity() const noexcept;
			/**
			 * identity Метод установки идентичности протокола модуля
			 * @param identity идентичность протокола модуля
			 */
			void identity(const identity_t identity) noexcept;
		public:
			/**
			 * chunk Метод установки размера чанка
			 * @param size размер чанка для установки
			 */
			void chunk(const size_t size) noexcept;
		public:
			/**
			 * userAgent Метод установки User-Agent для HTTP-запроса
			 * @param userAgent агент пользователя для HTTP-запроса
			 */
			void userAgent(const string & userAgent) noexcept;
		public:
			/**
			 * ident Метод получения идентификации сервера
			 * @param flag флаг выполняемого процесса
			 * @return     сформированный агент
			 */
			string ident(const process_t flag) const noexcept;
			/**
			 * ident Метод установки идентификации сервера
			 * @param id   идентификатор сервиса
			 * @param name название сервиса
			 * @param ver  версия сервиса
			 */
			void ident(const string & id, const string & name, const string & ver) noexcept;
		public:
			/**
			 * crypted Метод проверки на зашифрованные данные
			 * @return флаг проверки на зашифрованные данные
			 */
			virtual bool crypted() const noexcept;
		public:
			/**
			 * encryption Метод активации шифрования
			 * @param mode флаг активации шифрования
			 */
			virtual void encryption(const bool mode) noexcept;
			/**
			 * encryption Метод установки параметров шифрования
			 * @param pass   пароль шифрования передаваемых данных
			 * @param salt   соль шифрования передаваемых данных
			 * @param cipher размер шифрования передаваемых данных
			 */
			virtual void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
		public:
			/**
			 * Http Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Http(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * ~Http Деструктор
			 */
			virtual ~Http() noexcept {}
	} http_t;
};

#endif // __AWH_HTTP__
