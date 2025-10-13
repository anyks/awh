/**
 * @file: ws.hpp
 * @date: 2025-10-08
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
#include <atomic>

/**
 * Наши модули
 */
#include "server.hpp"
#include "../ws/frame.hpp"
#include "../ws/server.hpp"
#include "../sys/guard.hpp"
#include "../sys/buffer.hpp"
#include "../http/server.hpp"

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
	 * @brief серверное пространство имён
	 *
	 */
	namespace server {
		/**
		 * @brief серверное пространство имён
		 *
		 */
		namespace scheme {
			/**
			 * @brief Структура схемы сети Websocket-сервера
			 *
			 */
			typedef struct AWH_SHARED_EXPORT Websocket : public scheme_t {
				public:
					/**
					 * @brief Структура флагов разрешения обменом данных
					 *
					 */
					typedef struct Allow {
						// Флаг разрешения чтения данных
						bool receive;
						// Объект охранника
						guard_t guard;
						/**
						 * @brief Конструктор
						 *
						 */
						Allow() noexcept : receive(true) {}
					} allow_t;
					/**
					 * @brief Структура партнёра
					 *
					 */
					typedef struct Partner {
						int16_t wbit;  // Размер скользящего окна
						bool takeover; // Флаг скользящего контекста сжатия
						/**
						 * @brief Конструктор
						 *
						 */
						Partner() noexcept : wbit(0), takeover(false) {}
					} __attribute__((packed)) partner_t;
					/**
					 * @brief Объект фрейма Websocket
					 *
					 */
					typedef struct Frame {
						size_t size;                  // Размер отправляемого сегмента
						ws::frame_t methods;          // Методы работы с фреймом Websocket
						ws::frame_t::opcode_t opcode; // Полученный опкод сообщения
						/**
						 * @brief Конструктор
						 *
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Frame(const fmk_t * fmk, const log_t * log) noexcept :
						 size(AWH_CHUNK_SIZE), methods(fmk, log),
						 opcode(ws::frame_t::opcode_t::TEXT) {}
					} frame_t;
					/**
					 * @brief Структура работы с HTTP-протоколом
					 * 
					 */
					typedef struct Http {
						// Объект для работы с HTTP-запросом
						server::ws_t req;
						// Объект для работы с HTTP-ответом
						server::http_t res;
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Объект для формирования отладочной информации
							server::http_t debug;
							/**
							 * @brief Конструктор
							 * 
							 * @param fmk объект фреймворка
							 * @param log объект для работы с логами
							 */
							Http(const fmk_t * fmk, const log_t * log) noexcept :
							 req(fmk, log), res(fmk, log), debug(fmk, log) {}
						/**
						 * Если режим отладки не включён
						 */
						#else
							/**
							 * @brief Конструктор
							 * 
							 * @param fmk объект фреймворка
							 * @param log объект для работы с логами
							 */
							Http(const fmk_t * fmk, const log_t * log) noexcept :
							 req(fmk, log), res(fmk, log) {}
						#endif
					} http_t;
					/**
					 * @brief Структура буфера данных
					 *
					 */
					typedef struct Buffer {
						// Бинарный буфер полезной нагрузки
						awh::buffer_t payload;
						// Буфер фрагментированного сообщения
						awh::buffer_t fragments;
						// Буфер извлечения данных
						awh::buffer_t extraction;
						/**
						 * @brief Конструктор
						 *
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Buffer(const fmk_t * fmk, const log_t * log) noexcept :
						 payload(fmk, log), fragments(fmk, log), extraction(fmk, log) {}
					} buffer_t;
				public:
					/**
					 * @brief Структура параметров активного клиента
					 *
					 */
					typedef struct Options {
						bool shake;                           // Флаг выполненного рукопожатия
						bool crypted;                         // Флаг шифрования сообщений
						bool inflate;                         // Флаг переданных сжатых данных
						int32_t sid;                          // Идентификатор потока
						uint64_t respPong;                    // Контрольная точка ответа на пинг
						uint64_t sendPing;                    // Время отправленного пинга
						std::atomic_bool close;               // Флаг требования закрыть брокера
						std::atomic_bool stopped;             // Флаг принудительной остановки
						http_t http;                          // Объект для работы с HTTP
						hash_t hash;                          // Объект хэширования
						allow_t allow;                        // Объект разрешения обмена данными
						frame_t frame;                        // Объект фрейма Websocket
						ws::mess_t mess;                      // Объект отправляемого сообщения
						buffer_t buffer;                      // Объект буфера данных
						partner_t client;                     // Объект партнёра клиента
						partner_t server;                     // Объект партнёра сервера
						hash_t::cipher_t cipher;              // Формат шифрования
						engine_t::proto_t proto;              // Активный прототип интернета
						awh::http_t::compressor_t compressor; // Метод компрессии данных
						/**
						 * @brief Конструктор
						 *
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Options(const fmk_t * fmk, const log_t * log) noexcept :
						 shake(false), crypted(false), inflate(false), sid(1),
						 respPong(0), sendPing(0), close(false), stopped(false),
						 http(fmk, log), hash(log), frame(fmk, log), buffer(fmk, log),
						 cipher(hash_t::cipher_t::AES128),
						 proto(engine_t::proto_t::HTTP1_1),
						 compressor(awh::http_t::compressor_t::NONE) {}
						/**
						 * @brief Деструктор
						 *
						 */
						~Options() noexcept {}
					} options_t;
				public:
					/**
					 * Тип данных для хранения опций активных клиентов
					 */
					using clients_t = std::map <uint32_t, std::unique_ptr <options_t>>;
				private:
					// Список параметров активных клиентов
					clients_t _clients;
				public:
					// Список доступных компрессоров
					vector <awh::http_t::compressor_t> compressors;
				private:
					// Объект фреймворка
					const fmk_t * _fmk;
					// Объект работы с логами
					const log_t * _log;
				public:
					/**
					 * @brief Метод очистки
					 *
					 */
					void clear() noexcept;
				public:
					/**
					 * @brief Метод создания параметров активного клиента
					 *
					 * @param bid идентификатор брокера
					 */
					void set(const uint32_t bid) noexcept;
					/**
					 * @brief Метод удаления параметров активного клиента
					 *
					 * @param bid идентификатор брокера
					 */
					void rm(const uint32_t bid) noexcept;
				public:
					/**
					 * @brief Метод извлечения списка параметров активных клиентов
					 *
					 * @return список параметров активных клиентов
					 */
					const clients_t & get() const noexcept;
					/**
					 * @brief Метод получения параметров активного клиента
					 *
					 * @param bid идентификатор брокера
					 * @return    параметры активного клиента
					 */
					const options_t * get(const uint32_t bid) const noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Websocket(const fmk_t * fmk, const log_t * log) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Websocket() noexcept {}
			} ws_t;
		};
	};
};

#endif // __AWH_SCHEME_WEBSOCKET_SERVER__
