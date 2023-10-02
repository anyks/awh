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
		 * SchemeWEB Структура схемы сети WEB сервера
		 */
		typedef struct SchemeWEB : public scheme_t {
			public:
				/**
				 * Coffer Структура сундука параметров
				 */
				typedef struct Coffer {
					bool mode;                        // Флаг открытия подключения
					bool crypt;                       // Флаг шифрования сообщений
					bool alive;                       // Флаг долгоживущего подключения
					bool close;                       // Флаг требования закрыть адъютанта
					bool stopped;                     // Флаг принудительной остановки
					size_t requests;                  // Количество выполненных запросов
					time_t checkPoint;                // Контрольная точка ответа на пинг
					http_t http;                      // Создаём объект для работы с HTTP
					vector <char> buffer;             // Буфер бинарных необработанных данных
					awh::http_t::compress_t compress; // Метод компрессии данных
					/**
					 * Coffer Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Coffer(const fmk_t * fmk, const log_t * log) noexcept :
					 mode(false), crypt(false), alive(false),
					 close(false), stopped(false), requests(0), checkPoint(0),
					 http(fmk, log), compress(awh::http_t::compress_t::NONE) {}
					/**
					 * ~Coffer Деструктор
					 */
					~Coffer() noexcept {}
				} coffer_t;
			public:
				// Флаги работы с сжатыми данными
				awh::http_t::compress_t compress;
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
				 * SchemeWEB Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				SchemeWEB(const fmk_t * fmk, const log_t * log) noexcept :
				 scheme_t(fmk, log), compress(http_t::compress_t::NONE), _fmk(fmk), _log(log) {}
				/**
				 * ~SchemeWEB Деструктор
				 */
				~SchemeWEB() noexcept {}
		} web_scheme_t;
	};
};

#endif // __AWH_SCHEME_WEB_SERVER__
