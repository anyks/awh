/**
 * @file: frame.hpp
 * @date: 2021-12-19
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

#ifndef __AWH_FRAME__
#define __AWH_FRAME__

/**
 * Стандартные модули
 */
#include <map>
#include <vector>
#include <string>
#include <random>
#include <cstring>
#include <algorithm>

/**
 * Для операционной системы OS Windows
 */
#if _WIN32 || _WIN64
	#include <winsock2.h>
/**
 * Для операционной системы не являющейся OS Windows
 */
#else
	#include <arpa/inet.h>
#endif

/**
 * Наши модули
 */
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * ws пространство имён
	 */
	namespace ws {
		/**
		 * Message Структура сообщений удалённой стороны
		 */
		typedef class AWHSHARED_EXPORT Message {
			public:
				// Код сообщения
				uint16_t code;
				// Текст и тип сообщения
				string text, type;
			private:
				/**
				 * Коды сообщений
				 */
				std::map <uint16_t, pair <string, string>> _codes;
			private:
				/**
				 * find Метод поиска типа сообщения
				 */
				void find() noexcept;
			private:
				/**
				 * init Метод инициализации модуля
				 */
				void init() noexcept;
			public:
				/**
				 * operator= Оператор установки текстового сообщения
				 * @param text текст сообщения
				 * @return     ссылка на контекст объекта
				 */
				Message & operator = (const string & text) noexcept;
				/**
				 * operator= Оператор установки кода сообщения
				 * @param code код сообщения
				 * @return     ссылка на контекст объекта
				 */
				Message & operator = (const uint16_t code) noexcept;
			public:
				/**
				 * Message Конструктор
				 */
				Message() noexcept;
				/**
				 * Message Конструктор
				 * @param code код сообщения
				 */
				Message(const uint16_t code) noexcept;
				/**
				 * Message Конструктор
				 * @param text текст сообщения
				 */
				Message(const string & text) noexcept;
				/**
				 * Message Конструктор
				 * @param code код сообщения
				 * @param text текст сообщения
				 */
				Message(const uint16_t code, const string & text) noexcept;
		} mess_t;
		/**
		 * Frame Класс для работы с фреймом WebSocket
		 */
		typedef class AWHSHARED_EXPORT Frame {
			private:
				// Устанавливаем максимальную версию фрейма
				static constexpr uint32_t MAX_FRAME_SIZE = numeric_limits <uint32_t>::max();
			public:
				/**
				 * Состояние фрейма
				 */
				enum class state_t : uint8_t {
					NONE = 0x00, // Состояние фрейма не понятно
					BAD  = 0x01, // Состояние фрейма плохое
					GOOD = 0x02  // Состояние фрейма отличное
				};
				/**
				 * Опкоды запроса
				 */
				enum class opcode_t : uint8_t {
					TEXT          = 0x01, // Текстовый фрейм
					PING          = 0x09, // Проверка подключения от сервера
					PONG          = 0x0A, // Ответ серверу на проверку подключения
					CLOSE         = 0x08, // Выполнить закрытие соединения этим фреймом
					BINARY        = 0x02, // Двоичный фрейм
					DATAUNUSED    = 0x03, // Для будущих фреймов с данными
					CONTINUATION  = 0x00, // Работа продолжается в текущем режиме
					CONTROLUNUSED = 0x0B  // Зарезервированы для будущих управляющих фреймов
				};
			public:
				/**
				 * Header Структура шапки
				 */
				typedef struct Header {
					bool fin;         // Фрейм является финальным
					bool mask;        // Маска протокола
					bool rsv[3];      // Расширения протокола
					uint8_t size;     // Размер блока заголовков
					state_t state;    // Состояние фрейма
					uint64_t frame;   // Размер всего фрейма данных
					uint64_t payload; // Размер полезной нагрузки
					opcode_t optcode; // Опциональные коды
					/**
					 * Header Конструктор
					 * @param fin  флаг финального фрейма
					 * @param mask флаг маскирования сообщения
					 */
					Header(const bool fin = true, const bool mask = true) noexcept :
					 fin(fin), mask(mask), rsv{false, false, false},
					 size(0x00), state(state_t::NONE), frame(0x00),
					 payload(0x00), optcode(opcode_t::TEXT) {}
				} __attribute__((packed)) head_t;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
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
				 * @param size   размер буфера данных сообщения
				 * @return       сообщение в текстовом виде
				 */
				mess_t message(const void * buffer, const size_t size) const noexcept;
				/**
				 * message Метод извлечения сообщения из заголовка фрейма
				 * @param head       заголовки фрейма
				 * @param code       код сообщения
				 * @param compressed флаг сжатых ожидаемых данных
				 * @return           сообщение в текстовом виде
				 */
				mess_t message(const head_t & head, const uint16_t code = 0, const bool compressed = false) const noexcept;
			public:
				/**
				 * ping Метод создания фрейма пинга
				 * @param mess данные сообщения
				 * @param mask флаг выполнения маскировки сообщения
				 * @return     бинарные данные фрейма
				 */
				vector <char> ping(const string & mess, const bool mask = false) const noexcept;
				/**
				 * ping Метод создания фрейма пинга
				 * @param buffer бинарный буфер данных для создания фрейма
				 * @param size   размер буфера данных для создания фрейма
				 * @param mask   флаг выполнения маскировки сообщения
				 * @return       бинарные данные фрейма
				 */
				vector <char> ping(const void * buffer, const size_t size, const bool mask = false) const noexcept;
			public:
				/**
				 * pong Метод создания фрейма понга
				 * @param mess данные сообщения
				 * @param mask флаг выполнения маскировки сообщения
				 * @return     бинарные данные фрейма
				 */
				vector <char> pong(const string & mess, const bool mask = false) const noexcept;
				/**
				 * pong Метод создания фрейма понга
				 * @param buffer бинарный буфер данных для создания фрейма
				 * @param size   размер буфера данных для создания фрейма
				 * @param mask   флаг выполнения маскировки сообщения
				 * @return       бинарные данные фрейма
				 */
				vector <char> pong(const void * buffer, const size_t size, const bool mask = false) const noexcept;
			public:
				/**
				 * get Метод извлечения данных фрейма
				 * @param head   заголовки фрейма
				 * @param buffer бинарные данные фрейма для извлечения
				 * @param size   размер передаваемого буфера
				 * @return       бинарные данные полезной нагрузки
				 */
				vector <char> get(head_t & head, const void * buffer, const size_t size) const noexcept;
				/**
				 * set Метод создания данных фрейма
				 * @param head   заголовки фрейма
				 * @param buffer бинарные данные полезной нагрузки
				 * @param size   размер передаваемого буфера
				 * @return       бинарные данные фрейма
				 */
				vector <char> set(const head_t & head, const void * buffer, const size_t size) const noexcept;
			public:
				/**
				 * Frame Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				Frame(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
		} frame_t;
	};
};

#endif // __AWH_FRAME__
