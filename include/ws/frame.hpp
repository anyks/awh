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
			typedef struct Message {
				u_short code; // Код сообщения
				string text;  // Текст сообщения
				/**
				 * Message Конструктор
				 * @param code код сообщения
				 * @param text текст сообщения
				 */
				Message(const u_short code = 0, const string & text = "") : code(code), text(text) {}
			} mess_t;
			/**
			 * Head Структура шапки
			 */
			typedef struct Head {
				bool fin;         // Фрейм является финальным
				bool mask;        // Маска протокола
				bool rsv[3];      // Расширения протокола
				u_short size;     // Необходимый размер полезной нагрузки
				u_short offset;   // Размер смещения в буфере
				opcode_t optcode; // Опциональные коды
				/**
				 * Head Конструктор
				 */
				Head() : fin(true), mask(true), rsv{false, false, false}, size(0), offset(0), optcode(opcode_t::TEXT) {}
			} head_t;
		private:
			// Создаём объект фреймворка
			const fmk_t * fmk = nullptr;
			// Адрес файла для сохранения логов
			const char * logfile = nullptr;
		public:
			/**
			 * head Метод извлечения заголовка фрейма
			 * @param buffer буфер с данными заголовка
			 * @return       данные заголовка фрейма
			 */
			head_t head(const vector <char> & buffer) const noexcept;
			/**
			 * size Метод получения размера фрейма
			 * @param head   заголовки фрейма
			 * @param buffer буфер с данными фрейма
			 * @return       размер полезной нагрузки фрейма
			 */
			uint8_t size(const head_t & head, const vector <char> & buffer = {}) const noexcept;
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
			 * @param buffer бинарные данные фрейма для извлечения (Без учёта заголовков и байтов размера)
			 * @return       бинарные данные полезной нагрузки
			 */
			vector <char> get(const head_t & head, const vector <char> & buffer) const noexcept;
			/**
			 * set Метод создания данных фрейма
			 * @param head   заголовки фрейма
			 * @param buffer бинарные данные полезной нагрузки
			 * @return       бинарные данные фрейма
			 */
			vector <char> set(const head_t & head, const vector <char> & buffer) const noexcept;
		public:
			/**
			 * message Метод создание фрейма сообщения
			 * @param mess данные сообщения
			 * @return     бинарные данные фрейма
			 */
			vector <char> message(const mess_t & mess) const noexcept;
			/**
			 * message Метод извлечения сообщения из фрейма
			 * @param head   заголовки фрейма
			 * @param buffer бинарные данные фрейма для извлечения (Без учёта заголовков и байтов размера)
			 * @return       сообщение в текстовом виде
			 */
			mess_t message(const head_t & head, const vector <char> & buffer) const noexcept;
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
