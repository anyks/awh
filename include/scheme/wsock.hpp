/**
 * @file: websocket.hpp
 * @date: 2022-09-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_SCHEME_WEBSOCKET_SERVER__
#define __AWH_SCHEME_WEBSOCKET_SERVER__

/**
 * Стандартная библиотека
 */
#include <map>
#include <ctime>
#include <vector>

/**
 * Наши модули
 */
#include <ws/frame.hpp>
#include <ws/server.hpp>
#include <scheme/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * SchemeWS Структура схемы сети WebSocket сервера
		 */
		typedef struct SchemeWebSocket : public scheme_t {
			public:
				/**
				 * Buffer Структура буфера данных
				 */
				typedef struct Buffer {
					vector <char> payload; // Бинарный буфер полезной нагрузки
					vector <char> fragmes; // Данные фрагметрированного сообщения
				} buffer_t;
				/**
				 * Allow Структура флагов разрешения обменом данных
				 */
				typedef struct Allow {
					bool send;    // Флаг разрешения отправки данных
					bool receive; // Флаг разрешения чтения данных
					/**
					 * Allow Конструктор
					 */
					Allow() noexcept : send(true), receive(true) {}
				} __attribute__((packed)) allow_t;
				/**
				 * Partner Структура партнёра
				 */
				typedef struct Partner {
					short wbit;    // Размер скользящего окна
					bool takeover; // Флаг скользящего контекста сжатия
					/**
					 * Partner Конструктор
					 */
					Partner() noexcept : wbit(0), takeover(false) {}
				} __attribute__((packed)) partner_t;
				/**
				 * Frame Объект фрейма WebSocket
				 */
				typedef struct Frame {
					size_t size;                  // Минимальный размер сегмента
					ws::frame_t methods;          // Методы работы с фреймом WebSocket
					ws::frame_t::opcode_t opcode; // Полученный опкод сообщения
					/**
					 * Frame Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Frame(const fmk_t * fmk, const log_t * log) noexcept :
					 size(0xFA000), methods(fmk, log), opcode(ws::frame_t::opcode_t::TEXT) {}
				} frame_t;
			public:
				/**
				 * Coffer Структура сундука параметров
				 */
				typedef struct Coffer {
					bool crypt;                  // Флаг шифрования сообщений
					bool close;                  // Флаг требования закрыть адъютанта
					bool shake;                  // Флаг выполненного рукопожатия
					bool deflate;                // Флаг переданных сжатых данных
					bool stopped;                // Флаг принудительной остановки
					time_t point;                // Контрольная точка ответа на пинг
					hash_t hash;                 // Создаём объект для компрессии-декомпрессии данных
					allow_t allow;               // Объект разрешения обмена данными
					frame_t frame;               // Объект для работы с фреймом WebSocket
					ws::mess_t mess;             // Объект отправляемого сообщения
					buffer_t buffer;             // Объект буфера данных
					partner_t client;            // Объект партнёра клиента
					partner_t server;            // Объект партнёра сервера
					server::ws_t http;           // Создаём объект для работы с HTTP
					recursive_mutex mtx;         // Мютекс для блокировки потока
					http_t::compress_t compress; // Метод компрессии данных
					/**
					 * Coffer Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Coffer(const fmk_t * fmk, const log_t * log) noexcept :
					 crypt(false), close(false), shake(false),
					 deflate(false), stopped(false), point(0),
					 hash(log), frame(fmk, log), http(fmk, log),
					 compress(http_t::compress_t::NONE) {}
					/**
					 * ~Coffer Деструктор
					 */
					~Coffer() noexcept {}
				} coffer_t;
			public:
				// Флаги работы с сжатыми данными
				http_t::compress_t compress;
			private:
				// Параметры подключения адъютантов
				map <uint64_t, unique_ptr <coffer_t>> _coffers;
			private:
				// Создаём объект фреймворка
				const fmk_t * _fmk;
				// Создаём объект работы с логами
				const log_t * _log;
			public:
				/**
				 * clear Метод очистки
				 */
				void clear() noexcept;
			public:
				/**
				 * set Метод создания параметров адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void set(const uint64_t aid) noexcept;
				/**
				 * rm Метод удаления параметров подключения адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void rm(const uint64_t aid) noexcept;
				/**
				 * get Метод получения параметров подключения адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    параметры подключения адъютанта
				 */
				const coffer_t * get(const uint64_t aid) const noexcept;
			public:
				/**
				 * SchemeWebSocket Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				SchemeWebSocket(const fmk_t * fmk, const log_t * log) noexcept :
				 scheme_t(fmk, log), compress(http_t::compress_t::NONE), _fmk(fmk), _log(log) {}
				/**
				 * ~SchemeWebSocket Деструктор
				 */
				~SchemeWebSocket() noexcept {}
		} ws_scheme_t;
	};
};

#endif // __AWH_SCHEME_WEBSOCKET_SERVER__
