/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_WS_SERVER_HTTP__
#define __AWH_WS_SERVER_HTTP__

/**
 * Наши модули
 */
#include <ws/ws.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * WSServer Класс для работы с сервером WebSocket
	 */
	typedef class WSServer : public ws_t {
		private:
			/**
			 * update Метод обновления входящих данных
			 */
			void update() noexcept;
		public:
			/**
			 * checkKey Метод проверки ключа сервера
			 * @return результат проверки
			 */
			bool checkKey() noexcept;
			/**
			 * checkVer Метод проверки на версию протокола
			 * @return результат проверки соответствия
			 */
			bool checkVer() noexcept;
			/**
			 * checkAuth Метод проверки авторизации
			 * @return результат проверки авторизации
			 */
			stath_t checkAuth() noexcept;
		public:
			/**
			 * WSServer Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			WSServer(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : ws_t(fmk, log, uri) {}
			/**
			 * ~WSServer Деструктор
			 */
			~WSServer() noexcept {}
	} wss_t;
};

#endif // __AWH_WS_SERVER_HTTP__
