/**
 * @file: ssl.hpp
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
			 * @brief Тип идентификатора события
			 *
			 */
			using id_t = uint64_t;
			/**
			 * @brief Тип буфера данных
			 *
			 */
			using buffer_t = std::pair <void *, size_t>;
		public:
			/**
			 * Основные поддерживаемые протоколы
			 */
			enum class proto_t : uint8_t {
				NONE  = 0x00, // Протокол не установлен
				RAW   = 0x01, // Протокол является бинарным
				SPDY  = 0x02, // Протокол соответствует SPDY
				HTTP  = 0x03, // Протокол соответствует HTTP/1.1
				HTTP2 = 0x04, // Протокол соответствует HTTP/2.0
				HTTP3 = 0x05  // Протокол соответствует HTTP/3.0
			};
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * @brief Метод создания контекста TLS
			 *
			 * @return идентификатор контекста TLS
			 */
			id_t create() noexcept;
			/**
			 * @brief Метод удаления контекста TLS
			 *
			 * @param id идентификатор контекста TLS
			 * @return   результат выполнения удаления
			 */
			bool destroy(const id_t id) noexcept;
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
			 * @brief Метод получения ошибки TLS
			 *
			 * @param id идентификатор события
			 * @return   текст ошибки TLS
			 */
			string error(const id_t id) const noexcept;
		public:
			/**
			 * @brief Метод получения версии протокола TLS
			 *
			 * @param id идентификатор события
			 * @return   версия протокола TLS
			 */
			string version(const id_t id) const noexcept;
		public:
			/**
			 * @brief Метод извлечения активного протокола
			 *
			 * @param id идентификатор события
			 * @return   метод активного протокола
			 */
			proto_t proto(const id_t id) const noexcept;
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
			 * @brief Метод разрешающий/запрещающий, выполнять проверку соответствия, доменному имени хоста сертификату сервера
			 *
			 * @param mode флаг состояния разрешения проверки
			 */
			void verifyHost(const bool mode) noexcept;
		public:
			/**
			 * @brief Метод установки алгоритмов шифрования
			 *
			 * @param ciphers список алгоритмов шифрования для установки
			 */
			void ciphers(const vector <string> & ciphers) noexcept;
		public:
			/**
			 * @brief Метод установки списка отзыва сертификатов
			 *
			 * @param path адрес файла списка отзыва сертификатов
			 */
			void crl(const string & path) noexcept;
			/**
			 * @brief Метод установки приватного ключа клиента
			 *
			 * @param path адрес файла приватного ключа клиента
			 */
			void privateKey(const string & path) noexcept;
			/**
			 * @brief Метод установки клиентского сертификата
			 *
			 * @param path адрес файла клиентского сертификата
			 */
			void certificate(const string & path) noexcept;
		public:
			/**
			 * @brief Метод установки сертификатов доверенных центров сертификации
			 *
			 * @param path адрес файла сертификата доверенных центров сертификации
			 */
			void ca(const string & path) noexcept;
			/**
			 * @brief Метод установки сертификатов доверенных центров сертификации
			 *
			 * @param dir  адрес директории с сертификатами доверенных центров сертификации
			 * @param file адрес файла сертификата доверенного центра сертификации
			 */
			void ca(const string & dir, const string & file = "") noexcept;
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
