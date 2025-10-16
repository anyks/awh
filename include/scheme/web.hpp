/**
 * @file: web.hpp
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

#ifndef __AWH_SCHEME_WEB_SERVER__
#define __AWH_SCHEME_WEB_SERVER__

/**
 * Стандартные модули
 */
#include <vector>
#include <atomic>
#include <unordered_map>

/**
 * Наши модули
 */
#include "server.hpp"
#include "../http/server.hpp"
#include "../sys/buffer.hpp"

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
			 * @brief Структура схемы сети WEB сервера
			 *
			 */
			typedef struct AWH_SHARED_EXPORT WEB : public scheme_t {
				public:
					/**
					 * @brief Структура буфера данных
					 *
					 */
					typedef struct Buffer {
						// Бинарный буфер полезной нагрузки
						awh::buffer_t payload;
						// Буфер извлечения данных
						awh::buffer_t extraction;
						/**
						 * @brief Конструктор
						 *
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Buffer(const fmk_t * fmk, const log_t * log) noexcept :
						 payload(fmk, log), extraction(fmk, log) {}
					} buffer_t;
					/**
					 * @brief Структура параметров активного клиента
					 *
					 */
					typedef struct Options {
						bool alive;                      // Флаг долгоживущего подключения
						bool crypted;                    // Флаг шифрования сообщений
						int32_t sid;                     // Идентификатор потока
						uint32_t requests;               // Количество выполненных запросов
						uint64_t respPong;               // Контрольная точка ответа на пинг
						std::atomic_bool begin;          // Флаг открытия подключения
						std::atomic_bool close;          // Флаг требования закрыть подключение
						std::atomic_bool stopped;        // Флаг принудительной остановки
						http_t http;                     // Объект для работы с HTTP
						buffer_t buffer;                 // Объект буфера данных
						hash_t::cipher_t cipher;         // Формат шифрования
						engine_t::proto_t proto;         // Активный прототип интернета
						http_t::compressor_t compressor; // Метод компрессии данных
						/**
						 * @brief Конструктор
						 *
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Options(const fmk_t * fmk, const log_t * log) noexcept :
						 alive(false), crypted(false), 
						 sid(1), requests(0), respPong(0),
						 begin(false), close(false), stopped(false),
						 http(fmk, log), buffer(fmk, log),
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
					using clients_t = std::unordered_map <uint32_t, std::unique_ptr <options_t>>;
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
					WEB(const fmk_t * fmk, const log_t * log) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~WEB() noexcept {}
			} web_t;
		};
	};
};

#endif // __AWH_SCHEME_WEB_SERVER__
