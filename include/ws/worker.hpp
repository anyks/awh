/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_WORKER_WSS_SERVER__
#define __AWH_WORKER_WSS_SERVER__

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
#include <ws/frame.hpp>
#include <ws/server.hpp>
#include <server/worker.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * WorkerServerWS Структура WebSocket сервера воркера
	 */
	typedef struct WorkerServerWS : public workSrv_t {
		public:
			/**
			 * AdjParam Структура параметров адъютанта
			 */
			typedef struct AdjParam {
				wss_t http;                  // Создаём объект для работы с HTTP
				bool crypt;                  // Флаг шифрования сообщений
				bool compressed;             // Флаг переданных сжатых данных
				size_t stopBytes;            // Количество байт для закрытия подключения
				time_t checkPoint;           // Контрольная точка ответа на пинг
				vector <char> fragmes;       // Данные фрагметрированного сообщения
				frame_t::opcode_t opcode;    // Полученный опкод сообщения
				http_t::compress_t compress; // Флаги работы с сжатыми данными
				/**
				 * AdjParam Конструктор
				 */
				AdjParam(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept :
					http(fmk, log, uri),
					crypt(false),
					compressed(false),
					stopBytes(0),
					checkPoint(0),
					opcode(frame_t::opcode_t::TEXT),
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
		private:
			// Параметры подключения адъютантов
			map <size_t, adjp_t> adjParams;
		public:
			// Полученный опкод сообщения
			frame_t::opcode_t opcode = frame_t::opcode_t::TEXT;
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
			 * WorkerServerWS Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			WorkerServerWS(const fmk_t * fmk, const log_t * log) noexcept : workSrv_t(fmk, log), nwk(fmk), uri(fmk, &nwk), fmk(fmk), log(log) {}
			/**
			 * ~WorkerServerWS Деструктор
			 */
			~WorkerServerWS() noexcept {}
	} workSrvWss_t;
};

#endif // __AWH_WORKER_WSS_SERVER__
