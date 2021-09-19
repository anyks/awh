/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_HTTP_CLIENT__
#define __AWH_HTTP_CLIENT__

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
	 * HClient Класс для работы с HTTP клиентом
	 */
	typedef class HClient : public http_t {
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
			 * HClient Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 * @param url объект URL адреса сервера
			 */
			HClient(const fmk_t * fmk, const log_t * log, const uri_t * uri, const uri_t::url_t * url) noexcept : http_t(fmk, log, uri, url) {}
			/**
			 * ~HClient Деструктор
			 */
			~HClient() noexcept {}
	} chttp_t;
};

#endif // __AWH_HTTP_CLIENT__
