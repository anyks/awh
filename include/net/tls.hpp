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
		public:
			/**
			 * @brief Тип идентификатора события
			 *
			 */
			using id_t = uint64_t;
			/**
			 * @brief Тип буфера данных
			 *
			 */
			using buffer_t = std::pair <void *, size_t>;
			/**
			 * Функция обратного вызова срабатывающая при ошибке события
			 */
			using error_callback_t = std::function <void (const id_t, const error_t, const string &)>;
		public:
			/**
			 * Основные поддерживаемые Application-Layer Protocol Negotiation (ALPN) протоколы
			 */
			enum class alpn_t : uint8_t {
				NONE  = 0x00, // Протокол не установлен
				RAW   = 0x01, // Протокол является бинарным
				SPDY  = 0x02, // Протокол соответствует SPDY
				HTTP  = 0x03, // Протокол соответствует HTTP/1.1
				HTTP2 = 0x04, // Протокол соответствует HTTP/2.0
				HTTP3 = 0x05  // Протокол соответствует HTTP/3.0
			};
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
			 * @brief Метод извлечения активного протокола
			 *
			 * @param id идентификатор события
			 * @return   метод активного протокола
			 */
			alpn_t alpn(const id_t id) const noexcept;
		public:
			/**
			 * @brief Метод получения общей информации о TLS соединении
			 *
			 * @param id идентификатор события
			 * @return   общая информация о TLS соединении
			 */
			string info(const id_t id) const noexcept;
		public:
			/**
			 * @brief Метод получения информации о списке отзыва сертификатов
			 *
			 * @param id идентификатор события
			 * @return   информация о списке отзыва сертификатов
			 */
			string crlInfo(const id_t id) const noexcept;
		public:
			/**
			 * @brief Метод получения информации о шифре
			 *
			 * @param id идентификатор события
			 * @return   информация о шифре
			 */
			string cipherInfo(const id_t id) const noexcept;
		public:
			/**
			 * @brief Метод получения информации о сертификате
			 *
			 * @param id идентификатор события
			 * @return   информация о сертификате
			 */
			string certificateInfo(const id_t id) const noexcept;
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
			 * @param id  идентификатор события
			 * @param out буфер для записи зашифрованных данных
			 * @return    результат выполнения рукопожатия
			 */
			bool handshake(const id_t id, buffer_t out) noexcept;
		public:
			/**
			 * @brief Метод шифрования данных
			 *
			 * @param id  идентификатор события
			 * @param in  буфер данных для шифрования
			 * @param out буфер для записи зашифрованных данных
			 * @return    результат выполнения шифрования
			 */
			bool encrypt(const id_t id, const buffer_t in, buffer_t out) noexcept;
			/**
			 * @brief Метод расшифровки данных
			 *
			 * @param id  идентификатор события
			 * @param in  буфер данных для расшифровки
			 * @param out буфер для записи расшифрованных данных
			 * @return    результат выполнения расшифровки
			 */
			bool decrypt(const id_t id, const buffer_t in, buffer_t out) noexcept;
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
			 * @brief Метод установки списка отзыва сертификатов
			 *
			 * @param id       идентификатор события
			 * @param filename адрес файла списка отзыва сертификатов
			 */
			void crl(const id_t id, const string & filename) noexcept;
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
			 * @brief Метод установки функции обратного вывода получения ошибок
			 *
			 * @param id       идентификатор события
			 * @param callback объект функции обратного вызова
			 * @return        результат установки функции обратного вызова
			 */
			bool error(const id_t id, error_callback_t callback) noexcept;
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
			 * @param alpn  поддерживаемый ALPN-протокол
			 * @return      идентификатор контекста TLS
			 */
			id_t create(const event::node_t node, const event::protocol_t proto, const alpn_t alpn = alpn_t::NONE) noexcept;
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
