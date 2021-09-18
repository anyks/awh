/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_HTTP_SERVER__
#define __AWH_HTTP_SERVER__

/**
 * Наши модули
 */
#include <http/http.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * HServer Класс для работы с HTTP сервером
	 */
	typedef class HServer : public http_t {
		private:
			// Флаг проверки аутентификации
			bool checkAuth = false;
		private:
			/**
			 * updateExtensions Метод проверки полученных расширений
			 */
			void updateExtensions() noexcept;
			/**
			 * updateSubProtocol Метод извлечения доступного сабпротокола
			 */
			void updateSubProtocol() noexcept;
		private:
			/**
			 * checkKey Метод проверки ключа сервера
			 * @return результат проверки
			 */
			bool checkKey() noexcept;
			/**
			 * checkVersion Метод проверки на версию протокола WebSocket
			 * @return результат проверки соответствия
			 */
			bool checkVersion() noexcept;
		private:
			/**
			 * checkAuthenticate Метод проверки авторизации
			 * @return результат проверки авторизации
			 */
			stath_t checkAuthenticate() noexcept;
		public:
			/**
			 * HServer Конструктор
			 * @param fmk     объект фреймворка
			 * @param uri     объект работы с URI
			 * @param logfile адрес файла для сохранения логов
			 */
			HServer(const fmk_t * fmk, const uri_t * uri, const char * logfile = nullptr) noexcept : http_t(fmk, uri, logfile) {}
			/**
			 * ~HServer Деструктор
			 */
			~HServer() noexcept {}
	} shttp_t;
};

#endif // __AWH_HTTP_SERVER__
