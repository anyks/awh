/**
 * @file: ws.hpp
 * @date: 2022-09-03
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

#ifndef __AWH_SCHEME_WEBSOCKET_SERVER__
#define __AWH_SCHEME_WEBSOCKET_SERVER__

/**
 * Стандартные модули
 */
#include <map>
#include <vector>

/**
 * Наши модули
 */
#include <ws/frame.hpp>
#include <ws/server.hpp>
#include <sys/buffer.hpp>
#include <scheme/server.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * scheme серверное пространство имён
		 */
		namespace scheme {
			/**
			 * WebSocket Структура схемы сети WebSocket сервера
			 */
			typedef struct AWHSHARED_EXPORT WebSocket : public scheme_t {
				public:
					/**
					 * Buffer Структура буфера данных
					 */
					typedef struct Buffer {
						// Бинарный буфер полезной нагрузки
						awh::buffer_t payload;
						// Данные фрагметрированного сообщения
						vector <char> fragmes;
						/**
						 * Buffer Конструктор
						 * @param log объект для работы с логами
						 */
						Buffer(const log_t * log) noexcept : payload(log) {}
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
						int16_t wbit;  // Размер скользящего окна
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
						size_t size;                  // Размер отправляемого сегмента
						ws::frame_t methods;          // Методы работы с фреймом WebSocket
						ws::frame_t::opcode_t opcode; // Полученный опкод сообщения
						/**
						 * Frame Конструктор
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Frame(const fmk_t * fmk, const log_t * log) noexcept :
						 size(AWH_CHUNK_SIZE), methods(fmk, log),
						 opcode(ws::frame_t::opcode_t::TEXT) {}
					} frame_t;
				public:
					/**
					 * Options Структура параметров активного клиента
					 */
					typedef struct Options {
						bool close;                      // Флаг требования закрыть брокера
						bool shake;                      // Флаг выполненного рукопожатия
						bool crypted;                    // Флаг шифрования сообщений
						bool inflate;                    // Флаг переданных сжатых данных
						bool stopped;                    // Флаг принудительной остановки
						int32_t sid;                     // Идентификатор потока
						uint64_t respPong;               // Контрольная точка ответа на пинг
						uint64_t sendPing;               // Время отправленного пинга
						hash_t hash;                     // Объект хэширования
						allow_t allow;                   // Объект разрешения обмена данными
						frame_t frame;                   // Объект фрейма WebSocket
						ws::mess_t mess;                 // Объект отправляемого сообщения
						buffer_t buffer;                 // Объект буфера данных
						partner_t client;                // Объект партнёра клиента
						partner_t server;                // Объект партнёра сервера
						server::ws_t http;               // Объект для работы с HTTP
						recursive_mutex mtx;             // Мютекс для блокировки потока
						hash_t::cipher_t cipher;         // Формат шифрования
						engine_t::proto_t proto;         // Активный прототип интернета
						http_t::compressor_t compressor; // Метод компрессии данных
						/**
						 * Options Конструктор
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Options(const fmk_t * fmk, const log_t * log) noexcept :
						 close(false), shake(false),
						 crypted(false), inflate(false), stopped(false),
						 sid(1), respPong(0), sendPing(0), hash(log),
						 frame(fmk, log), buffer(log), http(fmk, log),
						 cipher(hash_t::cipher_t::AES128),
						 proto(engine_t::proto_t::HTTP1_1),
						 compressor(http_t::compressor_t::NONE) {}
						/**
						 * ~Options Деструктор
						 */
						~Options() noexcept {}
					} options_t;
				public:
					// Список доступных компрессоров
					vector <awh::http_t::compressor_t> compressors;
				private:
					// Список параметров активных клиентов
					map <uint64_t, unique_ptr <options_t>> _options;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * clear Метод очистки
					 */
					void clear() noexcept;
				public:
					/**
					 * set Метод создания параметров активного клиента
					 * @param bid идентификатор брокера
					 */
					void set(const uint64_t bid) noexcept;
					/**
					 * rm Метод удаления параметров активного клиента
					 * @param bid идентификатор брокера
					 */
					void rm(const uint64_t bid) noexcept;
				public:
					/**
					 * get Метод получения параметров активного клиента
					 * @param bid идентификатор брокера
					 * @return    параметры активного клиента
					 */
					const options_t * get(const uint64_t bid) const noexcept;
					/**
					 * get Метод извлечения списка параметров активных клиентов
					 * @return список параметров активных клиентов
					 */
					const map <uint64_t, unique_ptr <options_t>> & get() const noexcept;
				public:
					/**
					 * WebSocket Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					WebSocket(const fmk_t * fmk, const log_t * log) noexcept :
					 scheme_t(fmk, log), _fmk(fmk), _log(log) {}
					/**
					 * ~WebSocket Деструктор
					 */
					~WebSocket() noexcept {}
			} ws_t;
		};
	};
};

#endif // __AWH_SCHEME_WEBSOCKET_SERVER__
