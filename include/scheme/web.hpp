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
 * @copyright: Copyright © 2022
 */

#ifndef __AWH_SCHEME_WEB_SERVER__
#define __AWH_SCHEME_WEB_SERVER__

/**
 * Стандартная библиотека
 */
#include <map>
#include <ctime>
#include <vector>

/**
 * Наши модули
 */
#include <http/server.hpp>
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
		 * scheme серверное пространство имён
		 */
		namespace scheme {
			/**
			 * WEB Структура схемы сети WEB сервера
			 */
			typedef struct WEB : public scheme_t {
				public:
					/**
					 * Options Структура параметров активного клиента
					 */
					typedef struct Options {
						bool mode;                   // Флаг открытия подключения
						bool alive;                  // Флаг долгоживущего подключения
						bool close;                  // Флаг требования закрыть брокера
						bool crypted;                // Флаг шифрования сообщений
						bool stopped;                // Флаг принудительной остановки
						int32_t sid;                 // Идентификатор потока
						time_t point;                // Контрольная точка ответа на пинг
						size_t requests;             // Количество выполненных запросов
						http_t http;                 // Создаём объект для работы с HTTP
						vector <char> buffer;        // Буфер бинарных необработанных данных
						engine_t::proto_t proto;     // Активный прототип интернета
						http_t::compress_t compress; // Метод компрессии данных
						/**
						 * Options Конструктор
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Options(const fmk_t * fmk, const log_t * log) noexcept :
						mode(false), alive(false), close(false),
						crypted(false), stopped(false), sid(1),
						point(0), requests(0), http(fmk, log),
						proto(engine_t::proto_t::HTTP1_1),
						compress(awh::http_t::compress_t::NONE) {}
						/**
						 * ~Options Деструктор
						 */
						~Options() noexcept {}
					} options_t;
				public:
					// Список доступных компрессоров
					vector <awh::http_t::compress_t> compressors;
				private:
					// Список параметров активных клиентов
					map <uint64_t, unique_ptr <options_t>> _options;
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
					 * WEB Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					WEB(const fmk_t * fmk, const log_t * log) noexcept : scheme_t(fmk, log), _fmk(fmk), _log(log) {}
					/**
					 * ~WEB Деструктор
					 */
					~WEB() noexcept {}
			} web_t;
		};
	};
};

#endif // __AWH_SCHEME_WEB_SERVER__
