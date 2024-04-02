/**
 * @file: web2.hpp
 * @date: 2022-11-29
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

#ifndef __AWH_SCHEME_WEB2_SERVER__
#define __AWH_SCHEME_WEB2_SERVER__

/**
 * Стандартные модули
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
			 * WEB2 Структура схемы сети WEB/2 сервера
			 */
			typedef struct WEB2 : public scheme_t {
				public:
					/**
					 * Stream Структура потока
					 */
					typedef struct Stream {
						bool crypted;                    // Флаг шифрования сообщений
						int32_t sid;                     // Идентификатор потока
						http_t http;                     // Создаём объект для работы с HTTP
						http_t::compressor_t compressor; // Метод компрессии данных
						/**
						 * Stream Конструктор
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						Stream(const fmk_t * fmk, const log_t * log) noexcept :
						 crypted(false), sid(1), http(fmk, log),
						 compressor(awh::http_t::compressor_t::NONE) {}
					} stream_t;
					/**
					 * Options Класс параметров активного клиента
					 */
					typedef class Options {
						public:
							bool alive;                                   // Флаг долгоживущего подключения
							bool close;                                   // Флаг требования закрыть брокера
							bool stopped;                                 // Флаг принудительной остановки
							time_t point;                                 // Контрольная точка ответа на пинг
							time_t pinging;                               // Время отправленного пинга
							size_t requests;                              // Количество выполненных запросов
							engine_t::proto_t proto;                      // Активный прототип интернета
							map <int32_t, unique_ptr <stream_t>> streams; // Список активных потоков
						public:
							// Объект фреймворка
							const fmk_t * fmk;
							// Объект работы с логами
							const log_t * log;
						public:
							/**
							 * Options Конструктор
							 * @param fmk объект фреймворка
							 * @param log объект для работы с логами
							 */
							Options(const fmk_t * fmk, const log_t * log) noexcept :
							 alive(false), close(false), stopped(false), point(0), pinging(0),
							 requests(0), proto(engine_t::proto_t::HTTP1_1), fmk(fmk), log(log) {}
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
					 * openStream Метод открытия потока
					 * @param sid идентификатор потока
					 * @param bid идентификатор брокера
					 */
					void openStream(const int32_t sid, const uint64_t bid) noexcept;
					/**
					 * closeStream Метод закрытия потока
					 * @param sid идентификатор потока
					 * @param bid идентификатор брокера
					 */
					void closeStream(const int32_t sid, const uint64_t bid) noexcept;
				public:
					/**
					 * getStream Метод извлечения данных потока
					 * @param sid идентификатор потока
					 * @param bid идентификатор брокера
					 * @return    данные запрашиваемого потока
					 */
					const stream_t * getStream(const int32_t sid, const uint64_t bid) const noexcept;
				public:
					/**
					 * WEB2 Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					WEB2(const fmk_t * fmk, const log_t * log) noexcept : scheme_t(fmk, log), _fmk(fmk), _log(log) {}
					/**
					 * ~WEB2 Деструктор
					 */
					~WEB2() noexcept {}
			} web2_t;
		};
	};
};

#endif // __AWH_SCHEME_WEB2_SERVER__
