/**
 * @file: web.hpp
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

#ifndef __AWH_SCHEME_WEB_SERVER__
#define __AWH_SCHEME_WEB_SERVER__

/**
 * Стандартные модули
 */
#include <map>
#include <vector>

/**
 * Наши модули
 */
#include "server.hpp"
#include "../http/server.hpp"
#include "../sys/buffer.hpp"

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
			 * WEB Структура схемы сети WEB сервера
			 */
			typedef struct AWHSHARED_EXPORT WEB : public scheme_t {
				public:
					/**
					 * Options Структура параметров активного клиента
					 */
					typedef struct Options {
						bool mode;                       // Флаг открытия подключения
						bool alive;                      // Флаг долгоживущего подключения
						bool close;                      // Флаг требования закрыть брокера
						bool crypted;                    // Флаг шифрования сообщений
						bool stopped;                    // Флаг принудительной остановки
						int32_t sid;                     // Идентификатор потока
						uint32_t requests;               // Количество выполненных запросов
						uint64_t respPong;               // Контрольная точка ответа на пинг
						http_t http;                     // Объект для работы с HTTP
						awh::buffer_t buffer;            // Буфер бинарных необработанных данных
						hash_t::cipher_t cipher;         // Формат шифрования
						engine_t::proto_t proto;         // Активный прототип интернета
						http_t::compressor_t compressor; // Метод компрессии данных
						/**
						 * Options Конструктор
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Options(const fmk_t * fmk, const log_t * log) noexcept :
						 mode(false), alive(false), close(false),
						 crypted(false), stopped(false),
						 sid(1), requests(0), respPong(0),
						 http(fmk, log), buffer(log),
						 cipher(hash_t::cipher_t::AES128),
						 proto(engine_t::proto_t::HTTP1_1),
						 compressor(awh::http_t::compressor_t::NONE) {}
						/**
						 * ~Options Деструктор
						 */
						~Options() noexcept {}
					} options_t;
				public:
					/**
					 * Тип данных для хранения опций активных клиентов
					 */
					typedef std::map <uint64_t, std::unique_ptr <options_t>> clients_t;
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
					 * get Метод извлечения списка параметров активных клиентов
					 * @return список параметров активных клиентов
					 */
					const clients_t & get() const noexcept;
					/**
					 * get Метод получения параметров активного клиента
					 * @param bid идентификатор брокера
					 * @return    параметры активного клиента
					 */
					const options_t * get(const uint64_t bid) const noexcept;
				public:
					/**
					 * WEB Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					WEB(const fmk_t * fmk, const log_t * log) noexcept :
					 scheme_t(fmk, log), _fmk(fmk), _log(log) {}
					/**
					 * ~WEB Деструктор
					 */
					~WEB() noexcept {}
			} web_t;
		};
	};
};

#endif // __AWH_SCHEME_WEB_SERVER__
