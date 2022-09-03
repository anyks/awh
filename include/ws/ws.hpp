/**
 * @file: ws.hpp
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

#ifndef __AWH_WS_HTTP__
#define __AWH_WS_HTTP__

/**
 * Стандартная библиотека
 */
#include <set>
#include <string>
#include <vector>
#include <random>
#include <algorithm>

/**
 * Наши модули
 */
#include <http/core.hpp>
#include <hash/base64.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * WS Класс для работы с WebSocket
	 */
	typedef class WS : public http_t {
		protected:
			// Версия протокола WebSocket
			static constexpr u_short WS_VERSION = 13;
			// Размер максимального значения окна для сжатия данных GZIP
			static constexpr int GZIP_MAX_WBITS = 15;
		protected:
			// Размер скользящего окна клиента
			short _wbitClient;
			// Размер скользящего окна сервера
			short _wbitServer;
		protected:
			// Метод сжатия данных запроса/ответа
			compress_t _compress;
		protected:
			// Поддерживаемый сабпротокол
			string _sub;
			// Ключ клиента
			mutable string _key;
		protected:
			// Флаг запрета переиспользования контекста компрессии для клиента
			bool _noClientTakeover;
			// Флаг запрета переиспользования контекста компрессии для сервера
			bool _noServerTakeover;
		protected:
			// Поддерживаемые сабпротоколы
			set <string> _subs;
		protected:
			/**
			 * key Метод генерации ключа
			 * @return сгенерированный ключ
			 */
			const string key() const noexcept;
			/**
			 * sha1 Метод генерации хэша SHA1 ключа
			 * @return сгенерированный хэш ключа клиента
			 */
			const string sha1() const noexcept;
		protected:
			/**
			 * update Метод обновления входящих данных
			 */
			virtual void update() noexcept = 0;
		public:
			/**
			 * checkKey Метод проверки ключа сервера
			 * @return результат проверки
			 */
			virtual bool checkKey() noexcept = 0;
			/**
			 * checkVer Метод проверки на версию протокола
			 * @return результат проверки соответствия
			 */
			virtual bool checkVer() noexcept = 0;
			/**
			 * checkAuth Метод проверки авторизации
			 * @return результат проверки авторизации
			 */
			virtual stath_t checkAuth() noexcept = 0;
		public:
			/**
			 * dump Метод получения бинарного дампа
			 * @return бинарный дамп данных
			 */
			vector <char> dump() const noexcept;
			/**
			 * dump Метод установки бинарного дампа
			 * @param data бинарный дамп данных
			 */
			void dump(const vector <char> & data) noexcept;
		public:
			/**
			 * clean Метод очистки собранных данных
			 */
			void clean() noexcept;
		public:
			/**
			 * compress Метод получения метода компрессии
			 * @return метод компрессии сообщений
			 */
			compress_t compress() const noexcept;
			/**
			 * compress Метод установки метода компрессии
			 * @param compress метод компрессии сообщений
			 */
			void compress(const compress_t compress) noexcept;
		public:
			/**
			 * isHandshake Метод получения флага рукопожатия
			 * @return флаг получения рукопожатия
			 */
			bool isHandshake() noexcept;
			/**
			 * checkUpgrade Метод получения флага переключения протокола
			 * @return флага переключения протокола
			 */
			bool checkUpgrade() const noexcept;
		public:
			/**
			 * wbitClient Метод получения размер скользящего окна для клиента
			 * @return размер скользящего окна
			 */
			short wbitClient() const noexcept;
			/**
			 * wbitServer Метод получения размер скользящего окна для сервера
			 * @return размер скользящего окна
			 */
			short wbitServer() const noexcept;
		public:
			/**
			 * response Метод создания ответа
			 * @return буфер данных запроса в бинарном виде
			 */
			vector <char> response() noexcept;
			/**
			 * request Метод создания запроса
			 * @param url объект параметров REST запроса
			 * @return    буфер данных запроса в бинарном виде
			 */
			vector <char> request(const uri_t::url_t & url) noexcept;
		public:
			/**
			 * sub Метод получения выбранного сабпротокола
			 * @return выбранный сабпротокол
			 */
			const string & sub() const noexcept;
			/**
			 * sub Метод установки подпротокола поддерживаемого сервером
			 * @param sub подпротокол для установки
			 */
			void sub(const string & sub) noexcept;
			/**
			 * subs Метод установки списка подпротоколов поддерживаемых сервером
			 * @param subs подпротоколы для установки
			 */
			void subs(const vector <string> & subs) noexcept;
		public:
			/**
			 * clientTakeover Метод получения флага переиспользования контекста компрессии для клиента
			 * @return флаг запрета переиспользования контекста компрессии для клиента
			 */
			bool clientTakeover() const noexcept;
			/**
			 * clientTakeover Метод установки флага переиспользования контекста компрессии для клиента
			 * @param flag флаг запрета переиспользования контекста компрессии для клиента
			 */
			void clientTakeover(const bool flag) noexcept;
		public:
			/**
			 * serverTakeover Метод получения флага переиспользования контекста компрессии для сервера
			 * @return флаг запрета переиспользования контекста компрессии для сервера
			 */
			bool serverTakeover() const noexcept;
			/**
			 * serverTakeover Метод установки флага переиспользования контекста компрессии для сервера
			 * @param flag флаг запрета переиспользования контекста компрессии для сервера
			 */
			void serverTakeover(const bool flag) noexcept;
		public:
			/**
			 * WS Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			WS(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept :
			 http_t(fmk, log, uri), _wbitClient(GZIP_MAX_WBITS),
			 _wbitServer(GZIP_MAX_WBITS), _compress(compress_t::DEFLATE),
			 _sub(""), _key(""), _noClientTakeover(true), _noServerTakeover(true) {}
			/**
			 * ~WS Деструктор
			 */
			virtual ~WS() noexcept {}
	} ws_t;
};

#endif // __AWH_WS_HTTP__
