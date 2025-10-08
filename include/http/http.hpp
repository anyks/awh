/**
 * @file: http.hpp
 * @date: 2025-10-06
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_HTTP__
#define __AWH_HTTP__

/**
 * Стандартные модули
 */
#include <string>
#include <vector>
#include <unordered_set>

/**
 * Наши модули
 */
#include "web2.hpp"
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"
#include "../sys/hash.hpp"
#include "../auth/client2.hpp"
#include "../auth/server2.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс для работы HTTP-протоколом
	 *
	 */
	typedef class AWH_SHARED_EXPORT Http {
		public:
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
		protected:
			/**
			 * Результат выполнения рукопожатия
			 */
			enum class handshake_t : uint8_t {
				NONE  = 0x00, // Проверка авторизации не проводилась
				GOOD  = 0x01, // Авторизация прошла успешно
				RETRY = 0x02, // Требуется повторить попытку
				FAULT = 0x03  // Авторизация не удалась
			};
		protected:
			/**
			 * @brief Параметры запроса для Transfer-Encoding
			 *
			 */
			typedef struct Transfer {
				// Флаг активирования передачи ответа Transfer-Encoding
				bool enabled;
				// Флаг разрешающий передавать трейлеры
				bool trailers;
				// Флаг разрешающий передавать тело чанками
				bool chunking;
				/**
				 * @brief Конструктор
				 *
				 */
				Transfer() noexcept :
				 enabled(false),
				 trailers(false),
				 chunking(false) {}
			} __attribute__((packed)) transfer_t;
			/**
			 * @brief Структура работы сессии
			 *
			 */
			typedef struct Session {
				// Флаг точной установки хоста
				bool exactHost;
				// Размер одного чанка
				size_t chunkSize;
				// Стейт текущего запроса
				state_t state;
				// Идентичность протокола
				identity_t identity;
				// Результат выполнения рукопожатия
				handshake_t handshake;
				/**
				 * @brief Конструктор
				 *
				 */
				Session() noexcept :
				 exactHost(false),
				 chunkSize(AWH_CHUNK_SIZE),
				 state(state_t::NONE),
				 identity(identity_t::NONE),
				 handshake(handshake_t::NONE) {}
			} __attribute__((packed)) session_t;
			/**
			 * @brief Структура параметров компрессора
			 *
			 */
			typedef struct Compressor {
				// Компрессор которым сжаты данные полезной нагрузки в настоящий момент времени
				compressor_t current;
				// Выбранный компрессор которым необходимо выполнить сжатие данных полезной нагрузки
				compressor_t selected;
				// Список поддерживаемых компрессоров
				std::map <float, compressor_t> supports;
				/**
				 * @brief Конструктор
				 *
				 */
				Compressor() noexcept :
				 current(compressor_t::NONE),
				 selected(compressor_t::NONE) {}
			} compressors_t;
			/**
			 * @brief Структура трансформера данных
			 *
			 */
			typedef struct Encryption {
				// Флаг зашифрованной полезной нагрузки
				bool crypted;
				// Флаг активации шифрования данных
				bool enabled;
				// Формат шифрования
				hash_t::cipher_t cipher;
				/**
				 * @brief Конструктор
				 *
				 */
				Encryption() noexcept :
				 crypted(false), enabled(false),
				 cipher(hash_t::cipher_t::AES128) {}
			} __attribute__((packed)) encrypt_t;
			/**
			 * @brief Структура авторизации агента
			 *
			 */
			typedef struct Auth {
				// Объект для работы с клиентской авторизацией
				client::auth_t client;
				// Объект для работы с серверной авторизацией
				server::auth_t server;
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Auth(const fmk_t * fmk, const log_t * log) noexcept :
				 client(fmk, log), server(fmk, log) {}
			} auth_t;
			/**
			 * @brief Структура агента подключения
			 *
			 */
			typedef struct Agent {
				// Идентификатор сервиса
				string id;
				// Название сервиса
				string name;
				// User-Agent для HTTP-запроса
				string user;
				// Версия модуля приложения
				string version;
				/**
				 * @brief Конструктор
				 *
				 */
				Agent() noexcept :
				 id{AWH_SHORT_NAME}, name{AWH_NAME},
				 user{HTTP_HEADER_AGENT}, version{AWH_VERSION} {}
			} agent_t;
		private:
			// Объект работы с операционной системой
			os_t _os;
		protected:
			// Объект работы с URI
			uri_t _uri;
			// Объект HTTP-парсера
			web_t _web;
			// Объект авторизации
			auth_t _auth;
			// Объект хэширования
			hash_t _hash;
			// Объект агента подключения
			agent_t _agent;
			// Параметры шифрования
			encrypt_t _encrypt;
			// Объект работы сессии
			session_t _session;
			// Объект Transfer-Encoding
			transfer_t _transfer;
			// Компрессор для жатия данных
			compressors_t _compressors;
		protected:
			// Хранилище функций обратного вызова
			callback_t _callback;
		private:
			// Список HTTP-ответов
			std::map <uint16_t, string> _responses;
		protected:
			// Список отправляемых трейлеров
			std::unordered_map <string, string> _trailers;
		protected:
			// Чёрный список заголовков
			mutable std::unordered_set <string> _blacklist;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		private:
			/**
			 * @brief Метод инициализации модуля
			 *
			 */
			void init() noexcept;
		private:
			/**
			 * @brief Функция выбора типа компрессора
			 *
			 * @param compressor название компрессора в текстовом виде
			 * @return           результат работы функции
			 */
			bool matchingCompressor(const string & compressor) noexcept;
		private:
			/**
			 * @brief Метод вывода полученных чанков полезной нагрузки
			 *
			 * @param id     идентификатор объекта
			 * @param buffer буфер данных чанка полезной нагрузки
			 * @param web    объект HTTP-парсера
			 */
			void chunking(const uint32_t id, const buffer_t & buffer, const web_t * web) noexcept;
		protected:
			/**
			 * @brief Метод выполнения шифрования полезной нагрузки
			 *
			 */
			void encrypt() noexcept;
			/**
			 * @brief Метод выполнения дешифровани полезной нагрузки
			 *
			 */
			void decrypt() noexcept;
		protected:
			/**
			 * @brief Метод выполнения декомпрессии полезной нагрузки
			 *
			 */
			void compress() noexcept;
			/**
			 * @brief Метод выполнения компрессии полезной нагрузки
			 *
			 */
			void decompress() noexcept;
		public:
			/**
			 * @brief Метод применения полученных результатов
			 *
			 */
			virtual void commit() noexcept;
		protected:
			/**
			 * @brief Метод проверки выполнения рукопожатия
			 *
			 * @return результат выполнения рукопожатия
			 */
			virtual handshake_t handshake() noexcept = 0;
		public:
			/**
			 * @brief Метод проверки текущего статуса рукопожатия
			 *
			 * @return текущий статус рукопожатия
			 */
			handshake_t status() const noexcept;
		public:
			/**
			 * @brief Метод получения идентификатора объекта
			 *
			 * @return идентификатор объекта
			 */
			uint32_t id() const noexcept;
			/**
			 * @brief Метод установки идентификатора объекта
			 *
			 * @param id идентификатор объекта
			 */
			void id(const uint32_t id) noexcept;
		public:
			/**
			 * @brief Метод извлечения идентичности протокола модуля
			 *
			 * @return флаг идентичности протокола модуля
			 */
			identity_t identity() const noexcept;
			/**
			 * @brief Метод установки идентичности протокола модуля
			 *
			 * @param identity идентичность протокола модуля
			 */
			void identity(const identity_t identity) noexcept;
		public:
			/**
			 * @brief Метод сброса параметров запроса
			 *
			 */
			virtual void reset() noexcept;
		public:
			/**
			 * @brief Метод очистки собранных данных
			 *
			 */
			virtual void clear() noexcept;
			/**
			 * @brief Метод очистки данных HTTP-юнита
			 *
			 * @param unit HTTP-юнит данные которого очищаются
			 */
			void clear(const web_t::unit_t unit) noexcept;
		public:
			/**
			 * @brief Метод установки флага точной установки хоста
			 *
			 * @param mode флаг для установки
			 */
			void exactHost(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод установки размера чанка
			 *
			 * @param size размер чанка для установки
			 */
			void chunkSize(const size_t size) noexcept;
		public:
			/**
			 * @brief Метод проверки существования данных в чёрном списке
			 *
			 */
			bool emptyBlacklist() const noexcept;
			/**
			 * @brief delInBlacklist Метод удаления заголовка HTTP-протокола из чёрного списка
			 *
			 * @param name название заголовка HTTP-протокола для удаления
			 * @return     результат удаления
			 */
			bool delInBlacklist(const string name) noexcept;
			/**
			 * @brief Метод добавления заголовка в чёрный список
			 *
			 * @param name название заголовка HTTP-протокола
			 */
			bool addToBlacklist(const string name) noexcept;
			/**
			 * @brief Проверка заголовка HTTP-протокола находится ли он в чёрном списке
			 *
			 * @param name название заголовка HTTP-протокола для проверки
			 * @return     результат проверки
			 */
			bool isInBlacklist(const string name) const noexcept;
		public:
			/**
			 * @brief Метод проверки активного состояния
			 *
			 * @param state состояние которое необходимо проверить
			 */
			bool state(const state_t state) const noexcept;
		public:
			/**
			 * @brief Проверка заголовка HTTP-протокола является ли он стандартным
			 *
			 * @param name название заголовка HTTP-протокола для проверки
			 * @return     результат проверки
			 */
			bool standard(const string & name) const noexcept;
		public:
			/**
			 * @brief Метод извлечения параметров запроса
			 *
			 * @return установленные параметры запроса
			 */
			const uri_t::url_t & url() const noexcept;
		public:
			/**
			 * @brief Метод получения объекта запроса на сервер
			 *
			 * @return объект запроса на сервер
			 */
			const web_t::req_t & request() const noexcept;
			/**
			 * @brief Метод добавления объекта запроса на сервер
			 *
			 * @param req объект запроса на сервер
			 */
			void request(const web_t::req_t & req) noexcept;
		public:
			/**
			 * @brief Метод получения объекта ответа сервера
			 *
			 * @return объект ответа сервера
			 */
			const web_t::res_t & response() const noexcept;
			/**
			 * @brief Метод добавления объекта ответа сервера
			 *
			 * @param res объект ответа сервера
			 */
			void response(const web_t::res_t & res) noexcept;
		public:
			/**
			 * @brief Метод получения текущей даты для заголовка HTTP-протокола
			 *
			 * @param date дата в формате UnixTimestamp
			 * @return     штамп времени в текстовом виде
			 */
			string date(const uint64_t date = 0) const noexcept;
		public:
			/**
			 * @brief Метод получения сообщения ответа HTTP-протокола
			 *
			 * @param code код сообщения ответа для получения
			 * @return     соответствующее коду сообщения ответа HTTP-протокола
			 */
			const string & message(const uint32_t code) const noexcept;
		public:
			/**
			 * @brief Метод маппинга полученных данных
			 *
			 * @param flag флаг выполняемого процесса
			 * @param http объект для маппинга
			 */
			void mapping(const process_t flag, Http & http) noexcept;
		public:
			/**
			 * @brief Метод парсинга сырых данных
			 *
			 * @param buffer буфер данных для обработки
			 * @param size   размер буфера данных
			 * @return       размер обработанных данных
			 */
			size_t parse(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения бинарного дампа
			 *
			 * @return бинарный дамп данных
			 */
			buffer_t dump() const noexcept;
			/**
			 * @brief Метод установки бинарного дампа
			 *
			 * @param data бинарный дамп данных
			 */
			void dump(const buffer_t & data) noexcept;
			/**
			 * @brief Метод установки бинарного дампа
			 *
			 * @param buffer буфер бинарных данных
			 * @param size   размер бинарных данных
			 */
			void dump(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения данных тела HTTP-протокола
			 *
			 * @return буфер данных тела HTTP-протокола
			 */
			buffer_t & body() noexcept;
			/**
			 * @brief Метод переноса данных тела HTTP-протокола
			 *
			 * @param body буфер тела HTTP-протокола для переноса
			 */
			void body(buffer_t && body) noexcept;
			/**
			 * @brief Метод установки данных тела HTTP-протокола
			 *
			 * @param body буфер тела HTTP-протокола для установки
			 */
			void body(const buffer_t & body) noexcept;
			/**
			 * @brief Метод перемещения данных тела HTTP-протокола
			 *
			 * @param body буфер тела HTTP-протокола для установки
			 */
			void body(vector <char> && body) noexcept;
			/**
			 * @brief Метод установки данных тела HTTP-протокола
			 *
			 * @param body буфер тела HTTP-протокола для установки
			 */
			void body(const vector <char> & body) noexcept;
			/**
			 * @brief Метод добавления данных тела HTTP-протокола
			 *
			 * @param buffer буфер тела HTTP-протокола для добавления
			 * @param size   размер буфера теля HTTP-протокола для добавления
			 */
			void body(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод чтения чанка полезной нагрузки
			 *
			 * @return актуальный чанк полезной нагрузки
			 */
			buffer_t chunk() noexcept;
			/**
			 * @brief Метод добавления чанка полезной нагрузки
			 *
			 * @param buffer буфер чанка полезной нагрузки
			 */
			void chunk(const buffer_t & buffer) noexcept;
			/**
			 * @brief Метод добавления чанка полезной нагрузки
			 *
			 * @param buffer буфер чанка полезной нагрузки
			 */
			void chunk(const vector <char> & buffer) noexcept;
			/**
			 * @brief Метод добавления чанка полезной нагрузки
			 *
			 * @param buffer буфер чанка полезной нагрузки
			 * @param size   размер буфера теля для добавления
			 */
			void chunk(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод получения списка заголовков HTTP-протокола
			 *
			 * @return список существующих заголовков HTTP-протокола
			 */
			headers_t & headers() noexcept;
			/**
			 * @brief Метод переноса списка заголовков HTTP-протокола
			 *
			 * @param headers список заголовков HTTP-протокола для переноса
			 */
			void headers(headers_t && headers) noexcept;
			/**
			 * @brief Метод установки списка заголовков HTTP-протокола
			 *
			 * @param headers список заголовков HTTP-протокола для установки
			 */
			void headers(const headers_t & headers) noexcept;
		public:
			/**
			 * @brief Метод получения данных заголовка HTTP-протокола
			 *
			 * @param name название заголовка HTTP-протокола
			 * @return     содержимое заголовка HTTP-протокола
			 */
			const string & header(const string & name) const noexcept;
			/**
			 * @brief Метод добавления заголовка HTTP-протокола
			 *
			 * @param name    название заголовка HTTP-протокола
			 * @param content содержимое заголовка HTTP-протокола
			 */
			void header(const string & name, const string & content) noexcept;
		public:
			/**
			 * @brief Метод получение типа протокола для переключения
			 *
			 * @return тип протокола для переключения
			 */
			const web_t::proto_t upgrade() const noexcept;
			/**
			 * @brief Метод установки типа протокола для переключения
			 *
			 * @param upgrade тип протокола для переключения
			 */
			void upgrade(const web_t::proto_t upgrade) noexcept;
		public:
			/**
			 * @brief Метод извлечения выбранного метода компрессии
			 *
			 * @return метод компрессии
			 */
			compressor_t compression() const noexcept;
			/**
			 * @brief Метод установки выбранного метода компрессии
			 *
			 * @param compressor метод компрессии
			 */
			void compression(const compressor_t compressor) noexcept;
			/**
			 * @brief Метод установки списка поддерживаемых компрессоров
			 *
			 * @param compressors методы компрессии данных полезной нагрузки
			 */
			void compressors(const vector <compressor_t> & compressors) noexcept;
		public:
			/**
			 * @brief Метод получения агента сервера для HTTP-протокола
			 *
			 * @param flag флаг выполняемого процесса
			 * @return     сформированный агент
			 */
			string agent(const process_t flag) const noexcept;
			/**
			 * @brief Метод установки пользовательского агента для HTTP-протокола
			 *
			 * @param agent пользовательский агент для HTTP-протокола
			 */
			void agent(const string & agent) noexcept;
			/**
			 * @brief Метод установки агента сервера для HTTP-протокола
			 *
			 * @param id   идентификатор сервиса
			 * @param name название сервиса
			 * @param ver  версия сервиса
			 */
			void agent(const string & id, const string & name, const string & ver) noexcept;
		public:
			/**
			 * @brief Метод получения количества установленных трейлеров
			 *
			 * @return количество установленных трейлеров
			 */
			size_t trailersCount() const noexcept;
			/**
			 * @brief Метод получения буфера отправляемых трейлеров
			 *
			 * @return буфер данных ответа в бинарном виде
			 */
			buffer_t trailers() const noexcept;
			/**
			 * @brief Метод получения буфера отправляемых трейлеров (для протокола HTTP/2)
			 *
			 * @return буфер данных ответа в бинарном виде
			 */
			vector <std::pair <string, string>> trailers2() const noexcept;
			/**
			 * @brief Метод установки трейлера
			 *
			 * @param name    название заголовка HTTP-протокола
			 * @param content содержимое заголовка HTTP-протокола
			 */
			void trailer(const string & name, const string & content) noexcept;
		public:
			/**
			 * @brief Метод извлечения строки авторизации
			 *
			 * @param flag флаг выполняемого процесса
			 * @param prov параметры провайдера обмена сообщениями
			 * @return     строка авторизации на удалённом сервере
			 */
			string auth(const process_t flag, const web_t::provider_t & prov) noexcept;
		public:
			/**
			 * @brief Метод извлечения список протоколов к которому принадлежит заголовок HTTP-протокола
			 *
			 * @param name название заголовка HTTP-протокола
			 * @return     список соответствующих протоколов
			 */
			const std::set <web_t::proto_t> & proto(const string & name) const noexcept;
		public:
			/**
			 * @brief Метод создания запроса для авторизации на прокси-сервере
			 *
			 * @param req объект параметров HTTP-запроса
			 * @return    буфер данных запроса в бинарном виде
			 */
			virtual buffer_t proxy(const web_t::req_t & req) noexcept;
			/**
			 * @brief Метод создания запроса для авторизации на прокси-сервере (для протокола HTTP/2)
			 *
			 * @param req объект параметров HTTP-запроса
			 * @return    буфер данных запроса в бинарном виде
			 */
			virtual vector <std::pair <string, string>> proxy2(const web_t::req_t & req) noexcept;
		public:
			/**
			 * @brief Метод создания отрицательного ответа
			 *
			 * @param res объект параметров HTTP-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			virtual buffer_t reject(const web_t::res_t & res) noexcept;
			/**
			 * @brief Метод создания отрицательного ответа (для протокола HTTP/2)
			 *
			 * @param res объект параметров HTTP-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			virtual vector <std::pair <string, string>> reject2(const web_t::res_t & res) noexcept;
		public:
			/**
			 * @brief Метод создания выполняемого процесса в бинарном виде
			 *
			 * @param flag флаг выполняемого процесса
			 * @param prov параметры провайдера обмена сообщениями
			 * @return     буфер данных в бинарном виде
			 */
			virtual buffer_t process(const process_t flag, const web_t::provider_t & prov) noexcept;
			/**
			 * @brief Метод создания выполняемого процесса в бинарном виде (для протокола HTTP/2)
			 *
			 * @param flag флаг выполняемого процесса
			 * @param prov параметры провайдера обмена сообщениями
			 * @return     буфер данных в бинарном виде
			 */
			virtual vector <std::pair <string, string>> process2(const process_t flag, const web_t::provider_t & prov) noexcept;
		public:
			/**
			 * @brief Метод установки функций обратного вызова
			 *
			 * @param callback функции обратного вызова
			 */
			void callback(const callback_t & callback) noexcept;
		public:
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const char * name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(name != nullptr)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param name  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const string & name, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(!name.empty())
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (name, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam T    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename T, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const uint32_t fid, Args... args) noexcept -> uint32_t {
				// Если мы получили название функции обратного вызова
				if(fid > 0)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <T> (fid, args...);
				// Выводим результат по умолчанию
				return 0;
			}
			/**
			 * @brief Шаблон метода подключения финкции обратного вызова
			 *
			 * @tparam A    тип идентификатора функции
			 * @tparam B    тип функции обратного вызова
			 * @tparam Args аргументы функции обратного вызова
			 */
			template <typename A, typename B, class... Args>
			/**
			 * @brief Метод подключения финкции обратного вызова
			 *
			 * @param fid  идентификатор функкции обратного вызова
			 * @param args аргументы функции обратного вызова
			 * @return     идентификатор добавленной функции обратного вызова
			 */
			auto on(const A fid, Args... args) noexcept -> uint32_t {
				// Если мы получили на вход число
				if constexpr (is_arithmetic_v <A> || is_enum_v <A>)
					// Выполняем установку функции обратного вызова
					return this->_callback.on <B> (static_cast <uint32_t> (fid), args...);
				// Выводим результат по умолчанию
				return 0;
			}
		public:
			/**
			 * @brief Метод проверки на зашифрованные данные
			 *
			 * @return флаг проверки на зашифрованные данные
			 */
			virtual bool crypted() const noexcept;
		public:
			/**
			 * @brief Метод активации шифрования
			 *
			 * @param mode флаг активации шифрования
			 */
			virtual void encryption(const bool mode) noexcept;
			/**
			 * @brief Метод установки параметров шифрования
			 *
			 * @param pass   пароль шифрования передаваемых данных
			 * @param salt   соль шифрования передаваемых данных
			 * @param cipher размер шифрования передаваемых данных
			 */
			virtual void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Http(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @param identity идентичность протокола модуля
			 * @param fmk      объект фреймворка
			 * @param log      объект для работы с логами
			 */
			Http(const identity_t identity, const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Http() noexcept {}
	} http_t;
};

#endif // __AWH_HTTP__
