/**
 * @file: ws.hpp
 * @date: 2025-10-07
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

#ifndef __AWH_WS__
#define __AWH_WS__

/**
 * Стандартные модули
 */
#include <string>
#include <vector>
#include <unordered_set>

/**
 * Наши модули
 */
#include "../sys/hash.hpp"
#include "../http/http.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * @brief Класс для работы с протоколом Websocket
	 *
	 */
	typedef class AWH_SHARED_EXPORT Websocket : public http_t {
		protected:
			/**
			 * Версия протокола WebSocket
			 */
			static constexpr uint16_t VERSION = 13;
			/**
			 * Размер минимального значения окна для сжатия данных GZIP
			 */
			static constexpr int16_t GZIP_MIN_WBITS = 8;
			/**
			 * Размер максимального значения окна для сжатия данных GZIP
			 */
			static constexpr int16_t GZIP_MAX_WBITS = 15;
		public:
			/**
			 * Шаги проверок переключения протокола
			 */
			enum class step_t : uint8_t {
				NONE    = 0x00, // Шаг не установлен
				KEY     = 0x01, // Шаг проверки соответствия ключа запроса
				VERSION = 0x02, // Шаг проверки версии протокола
				UPGRADE = 0x03  // Шаг выполнения переключения протокола
			};
		protected:
			/**
			 * @brief Структура настроек компрессора
			 *
			 */
			typedef struct Deflate {
				// Флаг активации накопления контекста
				bool takeover;
				// Размер скользящего окна
				int16_t wbits;
				/**
				 * @brief Конструктор
				 *
				 */
				Deflate() noexcept :
				 takeover(false),
				 wbits(GZIP_MAX_WBITS) {}
			} __attribute__((packed)) deflate_t;
			/**
			 * @brief Структура параметров компрессии сообщений
			 * 
			 */
			typedef struct Permessage {
				// Объект партнёра клиента
				deflate_t client;
				// Объект партнёра сервера
				deflate_t server;
			} __attribute__((packed)) permessage_t;
			/**
			 * @brief Структура протоколов
			 * 
			 */
			typedef struct Protocols {
				// Список выбранных сабпротоколов
				std::unordered_set <string> selected;
				// Список поддерживаемых сабпротоколов
				std::unordered_set <string> supported;
			} protocols_t;
		protected:
			// Флаг зашифрованных данных
			bool _encryption;
		private:
			// Объект работы с временным буфером
			buffer_t _buffer;
		protected:
			// Ключ клиента
			mutable string _key;
		protected:
			// Наборы активных протоколов
			protocols_t _protocols;
			// Объект параметров компрессии сообщений
			permessage_t _permessage;
			// Компрессор для жатия данных
			compressors_t _compressors;
		protected:
			// Список поддверживаемых расширений
			vector <vector <string>> _extensions;
		private:
			/**
			 * @brief Метод инициализации
			 *
			 * @param flag флаг направления передачи данных
			 */
			void init(const process_t flag) noexcept;
		private:
			/**
			 * @brief Метод установки выбранных расширений
			 *
			 * @param flag флаг направления передачи данных
			 */
			void extensions(const process_t flag) noexcept;
		protected:
			/**
			 * @brief Метод генерации ключа
			 *
			 * @return сгенерированный ключ
			 */
			string key() const noexcept;
			/**
			 * @brief Метод генерации хэша SHA1 ключа
			 *
			 * @return сгенерированный хэш ключа клиента
			 */
			string sha1() const noexcept;
		protected:
			/**
			 * @brief Метод извлечения системного расширения из заголовка
			 *
			 * @param extension запись из которой нужно извлечь расширение
			 * @return          результат извлечения
			 */
			bool extract(const string & extension) noexcept;
		public:
			/**
			 * @brief Метод очистки собранных данных
			 *
			 */
			void clean() noexcept;
		public:
			/**
			 * @brief Метод применения полученных результатов
			 *
			 */
			virtual void commit() noexcept = 0;
		protected:
			/**
			 * @brief Метод проверки выполнения рукопожатия
			 *
			 * @return результат выполнения рукопожатия
			 */
			virtual handshake_t handshake() noexcept = 0;
		public:
			/**
			 * @brief Метод получения бинарного дампа
			 *
			 * @return бинарный дамп данных
			 */
			buffer_t & dump() noexcept;
			/**
			 * @brief Метод установки бинарного дампа
			 *
			 * @param data бинарный дамп данных
			 */
			void dump(const buffer_t & data) noexcept;
			/**
			 * @brief Метод установки бинарного дампа
			 *
			 * @param buffer буфер бинарных данных
			 * @param size   размер бинарных данных
			 */
			void dump(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод проверки шагов рукопожатия
			 *
			 * @param step флаг выполнения проверки
			 * @return     результат проверки соответствия
			 */
			virtual bool step(const step_t step) noexcept;
		public:
			/**
			 * @brief Метод выполнения проверки рукопожатия
			 *
			 * @param flag флаг выполняемого процесса
			 * @return     результат выполнения проверки рукопожатия
			 */
			bool isHandshake(const process_t flag) noexcept;
		public:
			/**
			 * @brief Метод получения размер скользящего окна
			 *
			 * @param hid тип текущего модуля
			 * @return    размер скользящего окна
			 */
			int16_t wbits(const web_t::hid_t hid) const noexcept;
		public:
			/**
			 * @brief Метод получения флага переиспользования контекста компрессии
			 *
			 * @param hid тип текущего модуля
			 * @return    флаг запрета переиспользования контекста компрессии
			 */
			bool takeover(const web_t::hid_t hid) const noexcept;
			/**
			 * @brief Метод установки флага переиспользования контекста компрессии
			 *
			 * @param hid  тип текущего модуля
			 * @param mode режим запрета переиспользования контекста компрессии
			 */
			void takeover(const web_t::hid_t hid, const bool mode) noexcept;
		public:
			/**
			 * @brief Метод извлечения выбранного метода компрессии
			 *
			 * @return метод компрессии
			 */
			compressor_t compression() const noexcept;
			/**
			 * @brief Метод установки выбранного метода компрессии
			 *
			 * @param compressor метод компрессии
			 */
			void compression(const compressor_t compressor) noexcept;
			/**
			 * @brief Метод установки списка поддерживаемых компрессоров
			 *
			 * @param compressors методы компрессии данных полезной нагрузки
			 */
			void compressors(const vector <compressor_t> & compressors) noexcept;
		public:
			/**
			 * @brief Метод извлечения списка расширений
			 *
			 * @return список поддерживаемых расширений
			 */
			const vector <vector <string>> & extensions() const noexcept;
			/**
			 * @brief Метод установки списка расширений
			 *
			 * @param extensions список поддерживаемых расширений
			 */
			void extensions(const vector <vector <string>> & extensions) noexcept;
		public:
			/**
			 * @brief Метод установки поддерживаемого сабпротокола
			 *
			 * @param subprotocol сабпротокол для установки
			 */
			void subprotocol(const string & subprotocol) noexcept;
			/**
			 * @brief Метод получения списка выбранных сабпротоколов
			 *
			 * @return список выбранных сабпротоколов
			 */
			const std::unordered_set <string> & subprotocols() const noexcept;
			/**
			 * @brief Метод установки списка поддерживаемых сабпротоколов
			 *
			 * @param subprotocols сабпротоколы для установки
			 */
			void subprotocols(const std::unordered_set <string> & subprotocols) noexcept;
		public:
			/**
			 * @brief Метод создания отрицательного ответа
			 *
			 * @param res объект параметров HTTP-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			buffer_t & reject(const web_t::res_t & res) noexcept;
			/**
			 * @brief Метод создания отрицательного ответа (для протокола HTTP/2)
			 *
			 * @param res объект параметров HTTP-ответа
			 * @return    буфер данных ответа в бинарном виде
			 */
			vector <std::pair <string, string>> reject2(const web_t::res_t & res) noexcept;
		public:
			/**
			 * @brief Метод создания выполняемого процесса в бинарном виде
			 *
			 * @param flag флаг выполняемого процесса
			 * @param prov параметры провайдера обмена сообщениями
			 * @return     буфер данных в бинарном виде
			 */
			buffer_t & process(const process_t flag, const web_t::provider_t & prov) noexcept;
			/**
			 * @brief Метод создания выполняемого процесса в бинарном виде (для протокола HTTP/2)
			 *
			 * @param flag флаг выполняемого процесса
			 * @param prov параметры провайдера обмена сообщениями
			 * @return     буфер данных в бинарном виде
			 */
			vector <std::pair <string, string>> process2(const process_t flag, const web_t::provider_t & prov) noexcept;
		public:
			/**
			 * @brief Метод проверки на зашифрованные данные
			 *
			 * @return флаг проверки на зашифрованные данные
			 */
			bool crypted() const noexcept;
		public:
			/**
			 * @brief Метод активации шифрования
			 *
			 * @param mode флаг активации шифрования
			 */
			void encryption(const bool mode) noexcept;
			/**
			 * @brief Метод установки параметров шифрования
			 *
			 * @param pass   пароль шифрования передаваемых данных
			 * @param salt   соль шифрования передаваемых данных
			 * @param cipher размер шифрования передаваемых данных
			 */
			void encryption(const string & pass, const string & salt = "", const hash_t::cipher_t cipher = hash_t::cipher_t::AES128) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Websocket(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			virtual ~Websocket() noexcept;
	} ws_t;
};

#endif // __AWH_WS__
