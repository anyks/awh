/**
 * @file: tls.hpp
 * @date: 2025-12-19
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

#ifndef __AWH_SSL_ENGINE__
#define __AWH_SSL_ENGINE__

/**
 * Наши модули
 */
#include "addr.hpp"
#include "event.hpp"
#include "../sys/fmk.hpp"
#include "../sys/log.hpp"

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
	 * @brief Структура транспортного уровня безопасности
	 *
	 */
	typedef class TransportLayerSecurity {
		public:
			/**
			 * Флаги типов ошибок TLS
			 */
			enum class error_t : uint8_t {
				NONE     = 0x00, // Флаг не установлен
				WARNING  = 0x01, // Предупреждающее сообщение
				CRITICAL = 0x02  // Критическое сообщение
			};
			/**
			 * Типы событий TLS
			 *
			 */
			enum class event_t : uint8_t {
				NONE       = 0x00, // Событие не установлено
				ENCRYPTION = 0x01, // Событие шифрования данных
				DECRYPTION = 0x02  // Событие расшифровки данных
			};
		public:
			/**
			 * @brief Структура ALPN-протокола
			 *
			 */
			typedef struct ALPN {
				// Идентификатор ALPN-протокола
				uint8_t id = 0;
				// Название ALPN-протокола
				string protocol = "";
			} alpn_t;
		public:
			/**
			 * @brief Тип идентификатора события
			 *
			 */
			using id_t = uint64_t;
		public:
			/**
			 * Функция обратного вызова срабатывающая при успешном завершении рукопожатия
			 */
			using handshake_callback_t = std::function <void (const id_t)>;
			/**
			 * Функция обратного вызова срабатывающая при ошибке события
			 */
			using error_callback_t = std::function <void (const id_t, const error_t, const string &)>;
			/**
			 * Функция обратного вызова срабатывающая при записи
			 */
			using write_callback_t = std::function <void (const id_t, const event_t, const size_t)>;
			/**
			 * Функция обратного вызова срабатывающая при чтении
			 */
			using read_callback_t = std::function <void (const id_t, const event_t, const uint8_t *, const size_t)>;
		private:
			// Объект работы с IP-адресами
			net_addr_t _addr;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод получения версии протокола TLS
			 *
			 * @return версия протокола TLS
			 */
			string version() const noexcept;
		public:
			/**
			 * @brief Метод получения общей информации о TLS соединении
			 *
			 * @param id идентификатор события
			 * @return   общая информация о TLS соединении
			 */
			string info(const id_t id) const noexcept;
			/**
			 * @brief Метод получения информации о шифре
			 *
			 * @param id идентификатор события
			 * @return   информация о шифре
			 */
			string cipherInfo(const id_t id) const noexcept;
			/**
			 * @brief Метод получения информации о сертификате
			 *
			 * @param id идентификатор события
			 * @return   информация о сертификате
			 */
			string certificateInfo(const id_t id) const noexcept;
			/**
			 * @brief Метод получения информации о списке отзыва сертификатов
			 *
			 * @param id идентификатор события
			 * @return   информация о списке отзыва сертификатов
			 */
			string certificateRevocationListInfo(const id_t id) const noexcept;
		public:
			/**
			 * @brief Метод проверки валидности сертификата
			 *
			 * @param id идентификатор события
			 * @return   результат проверки валидности сертификата
			 */
			bool validateCertificate(const id_t id) const noexcept;
		public:
			/**
			 * @brief Метод установки проверки хоста сервера
			 *
			 * @param id   идентификатор события
			 * @param mode режим проверки хоста сервера
			 */
			void validateHostname(const id_t id, const bool mode) noexcept;
			/**
			 * @brief Метод установки имени хоста сервера
			 *
			 * @param id       идентификатор события
			 * @param hostname имя хоста сервера
			 */
			void setHostname(const id_t id, const string & hostname) noexcept;
		public:
			/**
			 * @brief Метод установки адреса и порта отдалённого узла
			 *
			 * @param id   идентификатор события
			 * @param ip   IP-адрес отдалённого узла
			 * @param port порт отдалённого узла
			 * @return     результат выполнения установки
			 */
			bool peer(const id_t id, const string & ip, const uint16_t port) noexcept;
		public:
			/**
			 * @brief Метод выполнения TLS рукопожатия
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения рукопожатия
			 */
			bool handshake(const id_t id) noexcept;
		public:
			/**
			 * @brief Метод шифрования данных
			 *
			 * @param id     идентификатор события
			 * @param buffer буфер данных для шифрования
			 * @param size   размер буфера данных для шифрования
			 * @return       результат выполнения шифрования
			 */
			bool encrypt(const id_t id, const void * buffer, const size_t size) noexcept;
			/**
			 * @brief Метод расшифровки данных
			 *
			 * @param id     идентификатор события
			 * @param buffer буфер данных для расшифровки
			 * @param size   размер буфера данных для расшифровки
			 * @return       результат выполнения расшифровки
			 */
			bool decrypt(const id_t id, const void * buffer, const size_t size) noexcept;
		public:
			/**
			 * @brief Метод установки безопасности работы потоков
			 *
			 * @param id   идентификатор события
			 * @param mode режим безопасности потоков
			 */
			void threadSafety(const id_t id, const event::mode_t mode) noexcept;
		public:
			/**
			 * @brief Метод установки алгоритмов шифрования
			 *
			 * @param id      идентификатор события
			 * @param ciphers список алгоритмов шифрования для установки
			 */
			void ciphers(const id_t id, const vector <string> & ciphers) noexcept;
		public:
			/**
			 * @brief Метод установки приватного ключа клиента
			 *
			 * @param id       идентификатор события
			 * @param filename адрес файла приватного ключа клиента
			 */
			void privateKey(const id_t id, const string & filename) noexcept;
			/**
			 * @brief Метод установки клиентского сертификата
			 *
			 * @param id       идентификатор события
			 * @param filename адрес файла клиентского сертификата
			 */
			void certificate(const id_t id, const string & filename) noexcept;
			/**
			 * @brief Метод установки списка отзыва сертификатов
			 *
			 * @param id       идентификатор события
			 * @param filename адрес файла списка отзыва сертификатов
			 */
			void certificateRevocationList(const id_t id, const string & filename) noexcept;
		public:
			/**
			 * @brief Метод установки сертификатов доверенных центров сертификации
			 *
			 * @param id       идентификатор события
			 * @param filename адрес файла сертификата доверенных центров сертификации
			 */
			void ca(const id_t id, const string & filename) noexcept;
			/**
			 * @brief Метод установки сертификатов доверенных центров сертификации
			 *
			 * @param id   идентификатор события
			 * @param dir  адрес директории с сертификатами доверенных центров сертификации
			 * @param file адрес файла сертификата доверенного центра сертификации
			 */
			void ca(const id_t id, const string & dir, const string & file = "") noexcept;
		public:
			/**
			 * @brief Метод извлечения активного протокола
			 *
			 * @param id идентификатор события
			 * @return   метод активного протокола
			 */
			uint8_t alpn(const id_t id) const noexcept;
			/**
			 * @brief Метод установки поддерживаемых ALPN-протоколов
			 *
			 * @param id   идентификатор события
			 * @param alpn список поддерживаемых ALPN-протоколов
			 */
			void alpn(const id_t id, const vector <alpn_t> & alpn) noexcept;
		public:
			/**
			 * @brief Метод установки функции обратного вывода получения данных
			 *
			 * @param id       идентификатор события
			 * @param callback объект функции обратного вызова
			 * @return         результат установки функции обратного вызова
			 */
			bool on(const id_t id, read_callback_t callback) noexcept;
			/**
			 * @brief Метод установки функции обратного вывода передачи данных
			 *
			 * @param id       идентификатор события
			 * @param callback объект функции обратного вызова
			 * @return         результат установки функции обратного вызова
			 */
			bool on(const id_t id, write_callback_t callback) noexcept;
			/**
			 * @brief Метод установки функции обратного вывода получения ошибок
			 *
			 * @param id       идентификатор события
			 * @param callback объект функции обратного вызова
			 * @return         результат установки функции обратного вызова
			 */
			bool on(const id_t id, error_callback_t callback) noexcept;
			/**
			 * @brief Метод установки функции обратного вывода выполнения рукопожатия
			 *
			 * @param id       идентификатор события
			 * @param callback объект функции обратного вызова
			 * @return         результат установки функции обратного вызова
			 */
			bool on(const id_t id, handshake_callback_t callback) noexcept;
		public:
			/**
			 * @brief Метод удаления контекста TLS
			 *
			 * @param id идентификатор контекста TLS
			 * @return   результат выполнения удаления
			 */
			bool destroy(const id_t id) noexcept;
			/**
			 * @brief Метод создания контекста TLS
			 *
			 * @param node  тип узла события
			 * @param proto тип протокола события
			 * @return      идентификатор контекста TLS
			 */
			id_t create(const event::node_t node, const event::protocol_t proto) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			TransportLayerSecurity(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~TransportLayerSecurity() noexcept;
	} tls_t;
};

#endif // __AWH_SSL_ENGINE__
