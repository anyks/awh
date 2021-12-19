/**
 * @file: socks5.hpp
 * @date: 2021-12-19
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2021
 */

#ifndef __AWH_WORKER_SOCKS5_SERVER__
#define __AWH_WORKER_SOCKS5_SERVER__

/**
 * Стандартная библиотека
 */
#include <map>
#include <ctime>
#include <vector>
#include <event2/event.h>

/**
 * Наши модули
 */
#include <worker/client.hpp>
#include <worker/server.hpp>
#include <socks5/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/*
	 * server серверное пространство имён
	 */
	namespace server {
		/**
		 * WorkerSocks5 Структура Socks5 сервера воркера
		 */
		typedef struct WorkerSocks5 : public worker_t {
			public:
				/**
				 * AdjParam Структура параметров адъютанта
				 */
				typedef struct AdjParam {
					bool close;              // Флаг требования закрыть адъютанта
					bool locked;             // Флаг блокировки обработки запроса
					bool connect;            // Флаг выполненного подключения
					size_t readBytes;        // Количество полученных байт для закрытия подключения
					size_t stopBytes;        // Количество байт для закрытия подключения
					client::worker_t worker; // Объект рабочего для клиента
					server::socks5_t socks5; // Объект для работы с Socks5
					vector <char> buffer;    // Буфер бинарных необработанных данных
					/**
					 * AdjParam Конструктор
					 * @param fmk объект фреймворка
					 * @param log объект для работы с логами
					 * @param uri объект работы с URI ссылками
					 */
					AdjParam(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept :
						close(false),
						locked(false),
						connect(false),
						readBytes(0),
						stopBytes(0),
						worker(fmk, log),
						socks5(fmk, log, uri) {}
					/**
					 * ~AdjParam Деструктор
					 */
					~AdjParam() noexcept {}
				} adjp_t;
			public:
				// Создаём объект работы с URI ссылками
				uri_t uri;
				// Создаем объект для работы с сетью
				network_t nwk;
			public:
				// Список пар клиентов
				map <size_t, size_t> pairs;
			private:
				// Параметры подключения адъютантов
				map <size_t, adjp_t> adjParams;
			private:
				// Создаём объект фреймворка
				const fmk_t * fmk = nullptr;
				// Создаём объект работы с логами
				const log_t * log = nullptr;
			public:
				/**
				 * clear Метод очистки
				 */
				void clear() noexcept;
			public:
				/**
				 * createAdj Метод создания параметров адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void createAdj(const size_t aid) noexcept;
				/**
				 * removeAdj Метод удаления параметров подключения адъютанта
				 * @param aid идентификатор адъютанта
				 */
				void removeAdj(const size_t aid) noexcept;
				/**
				 * getAdj Метод получения параметров подключения адъютанта
				 * @param aid идентификатор адъютанта
				 * @return    параметры подключения адъютанта
				 */
				const adjp_t * getAdj(const size_t aid) const noexcept;
			public:
				/**
				 * WorkerSocks5 Конструктор
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				WorkerSocks5(const fmk_t * fmk, const log_t * log) noexcept : worker_t(fmk, log), nwk(fmk), uri(fmk, &nwk), fmk(fmk), log(log) {}
				/**
				 * ~WorkerSocks5 Деструктор
				 */
				~WorkerSocks5() noexcept {}
		} workerSocks5_t;
	};
};

#endif // __AWH_WORKER_SOCKS5_SERVER__
