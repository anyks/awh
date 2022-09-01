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

#ifndef __AWH_WORKER_SAMPLE_SERVER__
#define __AWH_WORKER_SAMPLE_SERVER__

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
#include <worker/server.hpp>

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
		 * WorkerSample Структура SAMPLE сервера воркера
		 */
		typedef struct WorkerSample : public worker_t {
			public:
				/**
				 * Основные экшены
				 */
				enum class action_t : uint8_t {
					NONE       = 0x01, // Отсутствие события
					READ       = 0x02, // Событие чтения с сервера
					CONNECT    = 0x03, // Событие подключения к серверу
					DISCONNECT = 0x04  // Событие отключения от сервера
				};
			public:
				/**
				 * Locker Структура локера
				 */
				typedef struct Locker {
					bool mode;           // Флаг блокировки
					recursive_mutex mtx; // Мютекс для блокировки потока
					/**
					 * Locker Конструктор
					 */
					Locker() noexcept : mode(false) {}
				} locker_t;
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
				 * Coffer Структура сундука параметров
				 */
				typedef struct Coffer {
					bool alive;           // Флаг долгоживущего подключения
					bool close;           // Флаг требования закрыть адъютанта
					bool stopped;         // Флаг принудительной остановки
					action_t action;      // Экшен активного события
					allow_t allow;        // Объект разрешения обмена данными
					locker_t locker;      // Объект блокировщика
					vector <char> buffer; // Буфер бинарных необработанных данных
					/**
					 * Coffer Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 */
					Coffer(const fmk_t * fmk, const log_t * log) noexcept :
					 alive(false), close(false), stopped(false), action(action_t::NONE) {}
					/**
					 * ~Coffer Деструктор
					 */
					~Coffer() noexcept {}
				} coffer_t;
			public:
				// Создаем объект для работы с сетью
				network_t nwk;
			public:
				// Создаём объект работы с URI ссылками
				uri_t uri;
			private:
				// Параметры подключения адъютантов
				map <size_t, unique_ptr <coffer_t>> _coffers;
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
				void set(const size_t aid) noexcept;
				/**
				 * rm Метод удаления параметров подключения адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void rm(const size_t aid) noexcept;
				/**
				 * get Метод получения параметров подключения адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    параметры подключения адъютанта
				 */
				const coffer_t * get(const size_t aid) const noexcept;
			public:
				/**
				 * WorkerSample Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				WorkerSample(const fmk_t * fmk, const log_t * log) noexcept :
				 worker_t(fmk, log), nwk(fmk), uri(fmk, &nwk), _fmk(fmk), _log(log) {}
				/**
				 * ~WorkerSample Деструктор
				 */
				~WorkerSample() noexcept {}
		} sample_worker_t;
	};
};

#endif // __AWH_WORKER_SAMPLE_SERVER__
