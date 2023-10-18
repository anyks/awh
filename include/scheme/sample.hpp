/**
 * @file: sample.hpp
 * @date: 2022-09-01
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

#ifndef __AWH_SCHEME_SAMPLE_SERVER__
#define __AWH_SCHEME_SAMPLE_SERVER__

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
		 * SchemeSample Структура схемы сети SAMPLE сервера
		 */
		typedef struct SchemeSample : public scheme_t {
			public:
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
				} allow_t;
			public:
				/**
				 * Options Структура параметров активного клиента
				 */
				typedef struct Options {
					bool alive;    // Флаг долгоживущего подключения
					bool close;    // Флаг требования закрыть брокера
					bool stopped;  // Флаг принудительной остановки
					allow_t allow; // Объект разрешения обмена данными
					/**
					/**
					 * Options Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Options(const fmk_t * fmk, const log_t * log) noexcept : alive(false), close(false), stopped(false) {}
					/**
					 * ~Options Деструктор
					 */
					~Options() noexcept {}
				} options_t;
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
				 * SchemeSample Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				SchemeSample(const fmk_t * fmk, const log_t * log) noexcept : scheme_t(fmk, log), _fmk(fmk), _log(log) {}
				/**
				 * ~SchemeSample Деструктор
				 */
				~SchemeSample() noexcept {}
		} sample_scheme_t;
	};
};

#endif // __AWH_SCHEME_SAMPLE_SERVER__
