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
 * @brief Основное пространство имён
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
	typedef class __AWH_SHARED_EXPORT__ TransportLayerSecurity {
		public:
			/**
			 * @brief Типы событий TLS
			 *
			 */
			enum class event_t : uint8_t {
				NONE       = 0x00, // Событие не установлено
				ENCRYPTION = 0x01, // Событие шифрования данных
				DECRYPTION = 0x02  // Событие расшифровки данных
			};
			/**
			 * @brief Состояния TLS работы
			 *
			 */
			enum class state_t : uint8_t {
				NONE             = 0x00, // Состояние не установлено
				FAILED           = 0x01, // Состояние ошибки
				DESTROYED        = 0x02, // Состояние разрушения
				HANDSHAKED       = 0x03, // Состояние рукопожатия
				HANDSHAKE_FAILED = 0x04  // Состояние ошибки рукопожатия
			};
			/**
			 * @brief Режимы работы TLS
			 *
			 */
			enum class mode_t : uint8_t {
				NONE      = 0x00, // Режим не установлен
				UNICERT   = 0x01, // Режим единственного сертификата
				MULTICERT = 0x02  // Режим мультисертификатов
			};
			/**
			 * @brief Флаги типов файлов TLS
			 *
			 */
			enum class type_t : uint8_t {
				NONE = 0x00, // Тип не установлен
				PEM  = 0x01, // Формат PEM
				ASN1 = 0x02, // Формат ASN1
			};
			/**
			 * @brief Флаги типов ошибок TLS
			 *
			 */
			enum class error_t : uint8_t {
				NONE                = 0x00, // Ошибка не установлена
				CA_FAILED           = 0x01, // Ошибка центра сертификации
				SNI_FAILED          = 0x02, // Ошибка проверки SNI
				CRL_FAILED          = 0x03, // Ошибка списка отзыва сертификатов
				BIO_FAILED          = 0x04, // Ошибка BIO
				CERT_FAILED         = 0x05, // Ошибка проверки сертификата
				READ_FAILED         = 0x06, // Ошибка чтения
				WRITE_FAILED        = 0x07, // Ошибка записи
				COOKIE_FAILED       = 0x08, // Ошибка проверки cookie
				CIPHER_FAILED       = 0x09, // Ошибка шифра
				HANDSHAKE_FAILED    = 0x0A, // Ошибка рукопожатия
				STORE_X509_FAILED   = 0x0B, // Ошибка хранилища X509
				TLS_SESSION_FAILED  = 0x0C, // Ошибка TLS сессии
				PRIVATE_KEY_FAILED  = 0x0D, // Ошибка приватного ключа
				HOSTNAME_BAD        = 0x0E, // Ошибка имени хоста
				INVALID_LAYER       = 0x0F, // Ошибка уровня TLS
				UNSUPPORTED_IP      = 0x10, // Ошибка неподдерживаемого IP-адреса
				HOSTNAME_VERIFY     = 0x11, // Ошибка проверки имени хоста
				MISMATCH_VERSION    = 0x12, // Ошибка версии TLS
				UNSUPPORTED_VERSION = 0x13, // Ошибка неподдерживаемой версии TLS
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
			 * @brief Функция обратного вызова срабатывающая при изменении состояния
			 *
			 */
			using state_callback_t = std::function <void (const id_t, const state_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при ошибке события
			 *
			 */
			using error_callback_t = std::function <void (const id_t, const error_t, const string &)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при записи
			 *
			 */
			using write_callback_t = std::function <void (const id_t, const event_t, const size_t)>;
			/**
			 * @brief Функция обратного вызова срабатывающая при чтении
			 *
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
			 * @brief Метод получения информации о одноразовом узле TLS
			 *
			 * @param id идентификатор события
			 * @return   информация о одноразовом узле TLS
			 */
			string peerInfo(const id_t id) const noexcept;
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
			 * @brief Метод извлечения сертификата TLS
			 *
			 * @param id идентификатор события
			 * @return   активный протокол
			 */
			string certificateExtract(const id_t id) const noexcept;
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
		public:
			/**
			 * @brief Метод получения режима работы TLS
			 *
			 * @param id идентификатор события
			 * @return   режим работы TLS
			 */
			mode_t mode(const id_t id) const noexcept;
			/**
			 * @brief Метод установки режима работы TLS
			 *
			 * @param id   идентификатор события
			 * @param mode режим работы TLS
			 */
			void mode(const id_t id, const mode_t mode) noexcept;
		public:
			/**
			 * @brief Метод получения имени хоста сервера
			 *
			 * @param id идентификатор события
			 * @return   имя хоста сервера
			 */
			string hostname(const id_t id) const noexcept;
			/**
			 * @brief Метод установки имени хоста сервера
			 *
			 * @param id       идентификатор события
			 * @param hostname имя хоста сервера
			 */
			void hostname(const id_t id, const string & hostname) noexcept;
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
			 * @brief Метод удаления контекста TLS
			 *
			 * @param id идентификатор транспортного уровня или шаблона контекста безопасности
			 * @return   результат выполнения удаления
			 */
			bool destroy(const id_t id) noexcept;
		public:
			/**
			 * @brief Метод завершения TLS соединения
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения завершения
			 */
			bool shutdown(const id_t id) noexcept;
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
			 * @brief Метод повторной передачи данных
			 *
			 * @param id идентификатор события
			 * @return   результат выполнения повторной передачи
			 */
			bool retransmit(const id_t id) noexcept;
		public:
			/**
			 * @brief Метод создания идентификатора транспортного уровня
			 *
			 * @param id идентификатор шаблона контекста безопасности
			 * @return   идентификатор транспортного уровня
			 */
			id_t transport(const id_t id) noexcept;
			/**
			 * @brief Метод создания идентификатора шаблона контекста безопасности
			 *
			 * @param node  тип узла события
			 * @param proto тип протокола события
			 * @return      идентификатор шаблона контекста безопасности
			 */
			id_t context(const event::node_t node, const event::protocol_t proto) noexcept;
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
			 * @brief Метод установки сертификатов доверенных центров сертификации
			 *
			 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
			 * @param filename адрес файла сертификата доверенных центров сертификации
			 */
			void ca(const id_t id, const string & filename) noexcept;
			/**
			 * @brief Метод установки сертификатов доверенных центров сертификации
			 *
			 * @param id   идентификатор транспортного уровня или шаблона контекста безопасности
			 * @param dir  адрес директории с сертификатами доверенных центров сертификации
			 * @param file адрес файла сертификата доверенного центра сертификации
			 */
			void ca(const id_t id, const string & dir, const string & file = "") noexcept;
		public:
			/**
			 * @brief Метод установки списка отзыва сертификатов
			 *
			 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
			 * @param filename адрес файла списка отзыва сертификатов
			 */
			void certificateRevocationList(const id_t id, const string & filename) noexcept;
			/**
			 * @brief Метод установки приватного ключа клиента
			 *
			 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
			 * @param filename адрес файла приватного ключа клиента
			 * @param type     тип файла приватного ключа клиента
			 */
			void privateKey(const id_t id, const string & filename, const type_t type = type_t::PEM) noexcept;
			/**
			 * @brief Метод установки клиентского сертификата
			 *
			 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
			 * @param filename адрес файла клиентского сертификата
			 * @param type     тип файла клиентского сертификата
			 */
			void certificate(const id_t id, const string & filename, const type_t type = type_t::PEM) noexcept;
		public:
			/**
			 * @brief Метод установки функции обратного вызова получения данных
			 *
			 * @param id       идентификатор транспортного уровня
			 * @param callback функция обратного вызова для установки
			 * @return         результат установки функции обратного вызова
			 */
			bool on(const id_t id, read_callback_t callback) noexcept;
			/**
			 * @brief Метод установки функции обратного вызова передачи данных
			 *
			 * @param id       идентификатор транспортного уровня
			 * @param callback функция обратного вызова для установки
			 * @return         результат установки функции обратного вызова
			 */
			bool on(const id_t id, write_callback_t callback) noexcept;
			/**
			 * @brief Метод установки функции обратного вызова получения ошибок
			 *
			 * @param id       идентификатор транспортного уровня
			 * @param callback функция обратного вызова для установки
			 * @return         результат установки функции обратного вызова
			 */
			bool on(const id_t id, error_callback_t callback) noexcept;
			/**
			 * @brief Метод установки функции обратного вызова изменения состояния
			 *
			 * @param id       идентификатор транспортного уровня
			 * @param callback функция обратного вызова для установки
			 * @return         результат установки функции обратного вызова
			 */
			bool on(const id_t id, state_callback_t callback) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			explicit TransportLayerSecurity(const fmk_t * fmk, const log_t * log) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~TransportLayerSecurity() noexcept;
	} tls_t;
};

#endif // __AWH_SSL_ENGINE__
