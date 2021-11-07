/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_WORKER_PROXY_SERVER__
#define __AWH_WORKER_PROXY_SERVER__

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
#include <http/proxy.hpp>
#include <worker/client.hpp>
#include <worker/server.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * WorkerServerProxy Структура REST сервера воркера
	 */
	typedef struct WorkerServerProxy : public workSrv_t {
		public:
			/**
			 * AdjParam Структура параметров адъютанта
			 */
			typedef struct AdjParam {
				httpCli_t cli;               // Создаём объект для работы с HTTP клиентом
				httpProxy_t srv;             // Создаём объект для работы с HTTP сервером
				workCli_t worker;            // Объект рабочего для клиента
				bool crypt;                  // Флаг шифрования сообщений
				bool alive;                  // Флаг долгоживущего подключения
				bool close;                  // Флаг требования закрыть адъютанта
				bool locked;                 // Флаг блокировки обработки запроса
				bool connect;                // Флаг выполненного подключения
				size_t requests;             // Количество выполненных запросов
				size_t readBytes;            // Количество полученных байт для закрытия подключения
				size_t stopBytes;            // Количество байт для закрытия подключения
				time_t checkPoint;           // Контрольная точка ответа на пинг
				web_t::method_t method;      // Метод HTTP выполняемого запроса
				http_t::compress_t compress; // Флаги работы с сжатыми данными
				vector <char> client;        // Буфер бинарных необработанных данных клиента
				vector <char> server;        // Буфер бинарных необработанных данных сервера
				/**
				 * AdjParam Конструктор
				 */
				AdjParam(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept :
					cli(fmk, log, uri),
					srv(fmk, log, uri),
					worker(fmk, log),
					crypt(false),
					alive(false),
					close(false),
					locked(false),
					connect(false),
					requests(0),
					readBytes(0),
					stopBytes(0),
					checkPoint(0),
					method(web_t::method_t::NONE),
					compress(http_t::compress_t::NONE) {}
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
		public:
			// Флаги работы с сжатыми данными
			http_t::compress_t compress = http_t::compress_t::NONE;
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
			 * WorkerServerProxy Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			WorkerServerProxy(const fmk_t * fmk, const log_t * log) noexcept : workSrv_t(fmk, log), nwk(fmk), uri(fmk, &nwk), fmk(fmk), log(log) {}
			/**
			 * ~WorkerServerProxy Деструктор
			 */
			~WorkerServerProxy() noexcept {}
	} workSrvProxy_t;
};

#endif // __AWH_WORKER_PROXY_SERVER__
