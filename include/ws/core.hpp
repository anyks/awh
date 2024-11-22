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
 * Стандартные модули
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
	typedef class AWHSHARED_EXPORT WCore : public http_t {
		protected:
			// Версия протокола WebSocket
			static constexpr uint16_t WS_VERSION = 13;
			// Размер минимального значения окна для сжатия данных GZIP
			static constexpr int16_t GZIP_MIN_WBITS = 8;
			// Размер максимального значения окна для сжатия данных GZIP
			static constexpr int16_t GZIP_MAX_WBITS = 15;
		public:
			/**
			 * Флаги проверок переключения протокола
			 */
			enum class flag_t : uint8_t {
				NONE    = 0x00, // Флаг не установлен
				KEY     = 0x01, // Флаг проверки соответствия ключа запроса
				VERSION = 0x02, // Флаг проверки версии протокола
				UPGRADE = 0x03  // Флаг выполнения переключения протокола
			};
		protected:
			/**
			 * Partner Структура партнёра
			 */
			typedef struct Partner {
				int16_t wbit;  // Размер скользящего окна
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
			// Флаг зашифрованных данных
			bool _encryption;
		protected:
			// Объект партнёра клиента
			partner_t _client;
			// Объект партнёра сервера
			partner_t _server;
		protected:
			// Компрессор для жатия данных
			compressors_t _compressors;
		protected:
			// Список выбранных сабпротоколов
			std::set <string> _selectedProtocols;
			// Список поддерживаемых сабпротоколов
			std::set <string> _supportedProtocols;
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
			 * status Метод проверки текущего статуса
			 * @return результат проверки текущего статуса
			 */
			virtual status_t status() noexcept = 0;
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
			 * crypted Метод проверки на зашифрованные данные
			 * @return флаг проверки на зашифрованные данные
			 */
			bool crypted() const noexcept;
		public:
			/**
			 * encryption Метод активации шифрования
			 * @param mode флаг активации шифрования
			 */
			void encryption(const bool mode) noexcept;
			/**
			 * encryption Метод установки параметров шифрования
			 * @param pass   пароль шифрования передаваемых данных
			 * @param salt   соль шифрования передаваемых данных
			 * @param cipher размер шифрования передаваемых данных
			 */
			void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
		public:
			/**
			 * compression Метод извлечения выбранного метода компрессии
			 * @return метод компрессии
			 */
			compressor_t compression() const noexcept;
			/**
			 * compression Метод установки выбранного метода компрессии
			 * @param compressor метод компрессии
			 */
			void compression(const compressor_t compressor) noexcept;
			/**
			 * compressors Метод установки списка поддерживаемых компрессоров
			 * @param compressors методы компрессии данных полезной нагрузки
			 */
			void compressors(const vector <compressor_t> & compressors) noexcept;
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
			 * handshake Метод выполнения проверки рукопожатия
			 * @param flag флаг выполняемого процесса
			 * @return     результат выполнения проверки рукопожатия
			 */
			bool handshake(const process_t flag) noexcept;
		public:
			/**
			 * check Метод проверки шагов рукопожатия
			 * @param flag флаг выполнения проверки
			 * @return     результат проверки соответствия
			 */
			virtual bool check(const flag_t flag) noexcept;
		public:
			/**
			 * wbit Метод получения размер скользящего окна
			 * @param hid тип текущего модуля
			 * @return    размер скользящего окна
			 */
			int16_t wbit(const web_t::hid_t hid) const noexcept;
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
			vector <std::pair <string, string>> reject2(const web_t::res_t & res) const noexcept;
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
			vector <std::pair <string, string>> process2(const process_t flag, const web_t::provider_t & provider) const noexcept;
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
			const std::set <string> & subprotocols() const noexcept;
			/**
			 * subprotocols Метод установки списка поддерживаемых сабпротоколов
			 * @param subprotocols сабпротоколы для установки
			 */
			void subprotocols(const std::set <string> & subprotocols) noexcept;
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
			WCore(const fmk_t * fmk, const log_t * log) noexcept : http_t(fmk, log), _key{""}, _encryption(false) {}
			/**
			 * ~WCore Деструктор
			 */
			virtual ~WCore() noexcept {}
	} ws_core_t;
};

#endif // __AWH_WS_CORE__
