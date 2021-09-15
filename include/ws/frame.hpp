/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_FRAME__
#define __AWH_FRAME__

/**
 * Стандартная библиотека
 */
#include <map>
#include <vector>
#include <string>
#include <random>
#include <cstring>
#include <algorithm>

// Если - это Windows
#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
// Если - это Unix
#else
	#include <arpa/inet.h>
#endif

/**
 * Наши модули
 */
#include <fmk.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Frame Класс для работы с фреймом WebSocket
	 */
	typedef class Frame {
		private:
			/**
			 * Коды сообщений
			 */
			map <u_short, pair <string, string>> codes = {
				{1000, {"CLOSE_NORMAL", "Successful operation/regular socket shutdown"}},
				{1001, {"CLOSE_GOING_AWAY", "Client is leaving (browser tab closing)"}},
				{1002, {"CLOSE_PROTOCOL_ERROR", "Endpoint received a malformed frame"}},
				{1003, {"CLOSE_UNSUPPORTED", "Endpoint received an unsupported frame (e.g. binary-only endpoint received text frame)"}},
				{1004, {"CLOSE_RESERVED", "Reserved"}},
				{1005, {"CLOSED_NO_STATUS", "Expected close status, received none"}},
				{1006, {"CLOSE_ABNORMAL", "No close code frame has been receieved"}},
				{1007, {"CLOSE_UNSUPPORTED_PAYLOAD", "Endpoint received inconsistent message (e.g. malformed UTF-8)"}},
				{1008, {"CLOSE_POLICY_VIOLATION", "Generic code used for situations other than 1003 and 1009"}},
				{1009, {"CLOSE_TOO_LARGE", "Endpoint won't process large frame"}},
				{1010, {"CLOSE_MANDATORY_EXTENSION", "Client wanted an extension which server did not negotiate"}},
				{1011, {"CLOSE_SERVER_ERROR", "Internal server error while operating"}},
				{1012, {"CLOSE_SERVICE_RESTART", "Server/service is restarting"}},
				{1013, {"CLOSE_TRY_AGAIN_LATER", "Temporary server condition forced blocking client's request"}},
				{1014, {"CLOSE_BAD_GATEWAY", "Server acting as gateway received an invalid response"}},
				{1015, {"CLOSE_TLS_HANDSHAKE_FAIL", "Transport Layer Security handshake failure"}}
			};
		public:
			/**
			 * Опкоды запроса
			 */
			enum class opcode_t : u_short {
				TEXT          = 0x1, // Текстовый фрейм
				PING          = 0x9, // Проверка подключения от сервера
				PONG          = 0xA, // Ответ серверу на проверку подключения
				CLOSE         = 0x8, // Выполнить закрытие соединения этим фреймом
				BINARY        = 0x2, // Двоичный фрейм
				DATAUNUSED    = 0x3, // Для будущих фреймов с данными
				CONTINUATION  = 0x0, // Работа продолжается в текущем режиме
				CONTROLUNUSED = 0xB  // Зарезервированы для будущих управляющих фреймов
			};
			/**
			 * Message Структура сообщений удалённой стороны
			 */
			typedef class Message {
				public:
					// Код сообщения
					u_short code;
					// Текст и тип сообщения
					string text, type;
				private:
					/**
					 * find Метод поиска типа сообщения
					 */
					void find() noexcept {
						// Если код сообщения передан
						if(code > 0){
							// Выполняем поиск типа сообщений
							auto it = codes.find(this->code);
							// Если тип сообщения найден, устанавливаем
							if(it != codes.end()){
								// Устанавливаем тип сообщения
								this->type = it->second.first;
								// Устанавливаем текст сообщения
								this->text = it->second.second;
							}
						}
					}
				public:
					/**
					 * operator= Оператор установки текстового сообщения
					 * @param text текст сообщения
					 * @return     ссылка на контекст объекта
					 */
					Message & operator=(const string & text) noexcept {
						// Устанавливаем текст сообщения
						if(!text.empty()) this->text = text;
						// Выводим контекст текущего объекта
						return (* this);
					}
					/**
					 * operator= Оператор установки кода сообщения
					 * @param code код сообщения
					 * @return     ссылка на контекст объекта
					 */
					Message & operator=(const u_short code) noexcept {
						// Если код сообщения передан
						if(code > 0){
							// Устанавливаем код сообщения
							this->code = code;
							// Выполняем поиск сообщения
							this->find();
						}
						// Выводим контекст текущего объекта
						return (* this);
					}
				public:
					/**
					 * Message Конструктор
					 * @param code код сообщения
					 * @param text текст сообщения
					 */
					Message(const u_short code = 0, const string & text = "") noexcept : code(code), text(text), type("") {
						// Выполняем поиск сообщения
						this->find();
					}
			} mess_t;
			/**
			 * Head Структура шапки
			 */
			typedef struct Head {
				bool fin;         // Фрейм является финальным
				bool mask;        // Маска протокола
				bool rsv[3];      // Расширения протокола
				uint8_t size;     // Размер блока заголовков
				uint8_t payload;  // Размер полезной нагрузки
				opcode_t optcode; // Опциональные коды
				/**
				 * Head Конструктор
				 */
				Head() : fin(true), mask(true), rsv{false, false, false}, size(0), payload(0), optcode(opcode_t::TEXT) {}
			} head_t;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Адрес файла для сохранения логов
			const char * logfile = nullptr;
		private:
			/**
			 * head Метод извлечения заголовка фрейма
			 * @param head   объект для извлечения заголовка
			 * @param buffer буфер с данными заголовка
			 * @param size   размер передаваемого буфера
			 */
			void head(head_t & head, const char * buffer, const size_t size) const noexcept;
			/**
			 * frame Функция создания бинарного фрейма
			 * @param payload бинарный буфер фрейма
			 * @param buffer  бинарные данные полезной нагрузки
			 * @param size    размер передаваемого буфера
			 * @param mask    флаг выполнения маскировки сообщения
			 */
			void frame(vector <char> & payload, const char * buffer, const size_t size, const bool mask) const noexcept;
		public:
			/**
			 * message Метод создание фрейма сообщения
			 * @param mess данные сообщения
			 * @return     бинарные данные фрейма
			 */
			vector <char> message(const mess_t & mess) const noexcept;
			/**
			 * message Метод извлечения сообщения из фрейма
			 * @param buffer бинарные данные сообщения
			 * @return       сообщение в текстовом виде
			 */
			mess_t message(const vector <char> & buffer) const noexcept;
		public:
			/**
			 * ping Метод создания фрейма пинга
			 * @param mess данные сообщения
			 * @param mask флаг выполнения маскировки сообщения
			 * @return     бинарные данные фрейма
			 */
			vector <char> ping(const string & mess = "", const bool mask = false) const noexcept;
			/**
			 * pong Метод создания фрейма понга
			 * @param mess данные сообщения
			 * @param mask флаг выполнения маскировки сообщения
			 * @return     бинарные данные фрейма
			 */
			vector <char> pong(const string & mess = "", const bool mask = false) const noexcept;
		public:
			/**
			 * get Метод извлечения данных фрейма
			 * @param head   заголовки фрейма
			 * @param buffer бинарные данные фрейма для извлечения
			 * @param size   размер передаваемого буфера
			 * @return       бинарные данные полезной нагрузки
			 */
			vector <char> get(head_t & head, const char * buffer, const size_t size) const noexcept;
			/**
			 * set Метод создания данных фрейма
			 * @param head   заголовки фрейма
			 * @param buffer бинарные данные полезной нагрузки
			 * @param size   размер передаваемого буфера
			 * @return       бинарные данные фрейма
			 */
			vector <char> set(const head_t & head, const char * buffer, const size_t size) const noexcept;
		public:
			/**
			 * Frame Конструктор
			 * @param fmk     объект фреймворка
			 * @param logfile адрес файла для сохранения логов
			 */
			Frame(const fmk_t * fmk, const char * logfile = nullptr) noexcept : fmk(fmk), logfile(logfile) {}
	} frame_t;
};

#endif // __AWH_FRAME__
