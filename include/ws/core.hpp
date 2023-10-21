/**
 * @file: core.hpp
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

#ifndef __AWH_WS_CORE__
#define __AWH_WS_CORE__

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
	 * WCore Класс для работы с WebSocket
	 */
	typedef class WCore : public http_t {
		protected:
			// Версия протокола WebSocket
			static constexpr u_short WS_VERSION = 13;
			// Размер минимального значения окна для сжатия данных GZIP
			static constexpr short GZIP_MIN_WBITS = 8;
			// Размер максимального значения окна для сжатия данных GZIP
			static constexpr short GZIP_MAX_WBITS = 15;
		protected:
			/**
			 * Partner Структура партнёра
			 */
			typedef struct Partner {
				short wbit;    // Размер скользящего окна
				bool takeover; // Флаг скользящего контекста сжатия
				/**
				 * Partner Конструктор
				 */
				Partner() noexcept : wbit(GZIP_MAX_WBITS), takeover(false) {}
			} __attribute__((packed)) partner_t;
		protected:
			// Ключ клиента
			mutable string _key;
		protected:
			// Объект партнёра клиента
			partner_t _client;
			// Объект партнёра сервера
			partner_t _server;
		protected:
			// Метод сжатия данных запроса/ответа
			compress_t _compress;
		protected:
			// Список выбранных сабпротоколов
			set <string> _selectedProtocols;
			// Список поддерживаемых сабпротоколов
			set <string> _supportedProtocols;
		protected:
			// Список поддверживаемых расширений
			vector <vector <string>> _extensions;
		private:
			/**
			 * init Метод инициализации
			 * @param flag флаг направления передачи данных
			 */
			void init(const process_t flag) noexcept;
		private:
			/**
			 * applyExtensions Метод установки выбранных расширений
			 * @param flag флаг направления передачи данных
			 */
			void applyExtensions(const process_t flag) noexcept;
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
			 * extractExtension Метод извлечения системного расширения из заголовка
			 * @param extension запись из которой нужно извлечь расширение
			 * @return          результат извлечения
			 */
			bool extractExtension(const string & extension) noexcept;
		public:
			/**
			 * commit Метод применения полученных результатов
			 */
			virtual void commit() noexcept = 0;
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
			 * extensions Метод извлечения списка расширений
			 * @return список поддерживаемых расширений
			 */
			const vector <vector <string>> & extensions() const noexcept;
			/**
			 * extensions Метод установки списка расширений
			 * @param extensions список поддерживаемых расширений
			 */
			void extensions(const vector <vector <string>> & extensions) noexcept;
		public:
			/**
			 * checkUpgrade Метод получения флага переключения протокола
			 * @return флага переключения протокола
			 */
			bool checkUpgrade() const noexcept;
			/**
			 * isHandshake Метод выполнения проверки рукопожатия
			 * @param flag флаг выполняемого процесса
			 * @return     результат выполнения проверки рукопожатия
			 */
			bool isHandshake(const process_t flag) noexcept;
		public:
			/**
			 * wbit Метод получения размер скользящего окна
			 * @param hid тип текущего модуля
			 * @return    размер скользящего окна
			 */
			short wbit(const web_t::hid_t hid) const noexcept;
		public:
			/**
			 * reject Метод создания отрицательного ответа
			 * @param req объект параметров REST-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			vector <char> reject(const web_t::res_t & res) const noexcept;
			/**
			 * reject2 Метод создания отрицательного ответа (для протокола HTTP/2)
			 * @param req объект параметров REST-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			vector <pair <string, string>> reject2(const web_t::res_t & res) const noexcept;
		public:
			/**
			 * process Метод создания выполняемого процесса в бинарном виде
			 * @param flag     флаг выполняемого процесса
			 * @param provider параметры провайдера обмена сообщениями
			 * @return         буфер данных в бинарном виде
			 */
			vector <char> process(const process_t flag, const web_t::provider_t & provider) const noexcept;
			/**
			 * process2 Метод создания выполняемого процесса в бинарном виде (для протокола HTTP/2)
			 * @param flag     флаг выполняемого процесса
			 * @param provider параметры провайдера обмена сообщениями
			 * @return         буфер данных в бинарном виде
			 */
			vector <pair <string, string>> process2(const process_t flag, const web_t::provider_t & provider) const noexcept;
		public:
			/**
			 * subprotocol Метод установки поддерживаемого сабпротокола
			 * @param subprotocol сабпротокол для установки
			 */
			void subprotocol(const string & subprotocol) noexcept;
			/**
			 * subprotocol Метод получения списка выбранных сабпротоколов
			 * @return список выбранных сабпротоколов
			 */
			const set <string> & subprotocols() const noexcept;
			/**
			 * subprotocols Метод установки списка поддерживаемых сабпротоколов
			 * @param subprotocols сабпротоколы для установки
			 */
			void subprotocols(const set <string> & subprotocols) noexcept;
		public:
			/**
			 * takeover Метод получения флага переиспользования контекста компрессии
			 * @param hid тип текущего модуля
			 * @return    флаг запрета переиспользования контекста компрессии
			 */
			bool takeover(const web_t::hid_t hid) const noexcept;
			/**
			 * takeover Метод установки флага переиспользования контекста компрессии
			 * @param hid  тип текущего модуля
			 * @param flag флаг запрета переиспользования контекста компрессии
			 */
			void takeover(const web_t::hid_t hid, const bool flag) noexcept;
		public:
			/**
			 * WCore Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			WCore(const fmk_t * fmk, const log_t * log) noexcept : http_t(fmk, log), _key{""}, _compress(compress_t::DEFLATE) {}
			/**
			 * ~WCore Деструктор
			 */
			virtual ~WCore() noexcept {}
	} ws_core_t;
};

#endif // __AWH_WS_CORE__
