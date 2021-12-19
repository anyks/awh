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
#include <base64.hpp>
#include <http/core.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
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
			short wbitClient = GZIP_MAX_WBITS;
			// Размер скользящего окна сервера
			short wbitServer = GZIP_MAX_WBITS;
			// Метод сжатия данных запроса/ответа
			compress_t compress = compress_t::DEFLATE;
		protected:
			// Поддерживаемый сабпротокол
			string sub = "";
			// Ключ клиента
			mutable string key = "";
		protected:
			// Поддерживаемые сабпротоколы
			set <string> subs;
		protected:
			/**
			 * getKey Метод генерации ключа
			 * @return сгенерированный ключ
			 */
			const string getKey() const noexcept;
			/**
			 * getHash Метод генерации хэша ключа
			 * @return сгенерированный хэш ключа клиента
			 */
			const string getHash() const noexcept;
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
			 * clean Метод очистки собранных данных
			 */
			void clean() noexcept;
		public:
			/**
			 * getCompress Метод получения метода сжатия
			 * @return метод сжатия сообщений
			 */
			compress_t getCompress() const noexcept;
			/**
			 * setCompress Метод установки метода сжатия
			 * @param метод сжатия сообщений
			 */
			void setCompress(const compress_t compress) noexcept;
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
			 * getWbitClient Метод получения размер скользящего окна для клиента
			 * @return размер скользящего окна
			 */
			short getWbitClient() const noexcept;
			/**
			 * getWbitServer Метод получения размер скользящего окна для сервера
			 * @return размер скользящего окна
			 */
			short getWbitServer() const noexcept;
		public:
			/**
			 * getSub Метод получения выбранного сабпротокола
			 * @return выбранный сабпротокол
			 */
			const string & getSub() const noexcept;
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
			 * setSub Метод установки подпротокола поддерживаемого сервером
			 * @param sub подпротокол для установки
			 */
			void setSub(const string & sub) noexcept;
			/**
			 * setSubs Метод установки списка подпротоколов поддерживаемых сервером
			 * @param subs подпротоколы для установки
			 */
			void setSubs(const vector <string> & subs) noexcept;
		public:
			/**
			 * WS Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект работы с URI
			 */
			WS(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : http_t(fmk, log, uri) {}
			/**
			 * ~WS Деструктор
			 */
			virtual ~WS() noexcept {}
	} ws_t;
};

#endif // __AWH_WS_HTTP__
