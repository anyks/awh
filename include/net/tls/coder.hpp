/**
 * @file: coder.hpp
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

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SSL_ENGINE__
#define __AWH_SSL_ENGINE__

/**
 * Предварительное объявление типа контекста криптографической библиотеки
 *
 * @details Объявление, а не подключение заголовочного файла: публичный интерфейс
 *          модуля не должен тянуть за собой зависимости внешних библиотек на
 *          этапе подключения библиотеки потребителем.
 */
struct ssl_ctx_st;

/**
 * Подключаем заголовочные файлы проекта
 */
#include "tls.hpp"
#include "fingerprint.hpp"
#include "../addr.hpp"
#include "../event.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"
#include "../../compressor/block.hpp"

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
	 * @brief пространство имён работы с TLS
	 *
	 */
	namespace tls {
		/**
		 * @brief Структура транспортного уровня безопасности
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Coder {
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
				 * @brief Флаги поддерживаемых стандартов TLS
				 *
				 */
				enum class standard_t : uint8_t {
					NONE = 0x00, // Стандарт не поддерживается
					OLD  = 0x01, // Поддерживается старый стандарт
					NEW  = 0x02  // Поддерживается новый стандарт
				};
				/**
				 * @brief Флаги типов ошибок TLS
				 *
				 */
				enum class error_t : uint8_t {
					NONE                = 0x00, // Ошибка не установлена
					CA_FAILED           = 0x01, // Ошибка центра сертификации
					CTL_FAILED          = 0x02, // Ошибка использования уровня транспортной передачей данных
					CTS_FAILED          = 0x03, // Ошибка использования уровня шаблона контекста безопасности
					SNI_FAILED          = 0x04, // Ошибка проверки SNI
					CRL_FAILED          = 0x05, // Ошибка списка отзыва сертификатов
					BIO_FAILED          = 0x06, // Ошибка BIO
					CERT_FAILED         = 0x07, // Ошибка проверки сертификата
					ALPS_FAILED         = 0x08, // Ошибка ALPS-протокола
					READ_FAILED         = 0x09, // Ошибка чтения
					WRITE_FAILED        = 0x0A, // Ошибка записи
					COOKIE_FAILED       = 0x0B, // Ошибка проверки cookie
					CIPHER_FAILED       = 0x0C, // Ошибка шифра
					CURVE_FAILED        = 0x0D, // Ошибка кривой
					SIGNATURE_FAILED    = 0x0E, // Ошибка алгоритма подписи
					HANDSHAKE_FAILED    = 0x0F, // Ошибка рукопожатия
					STORE_X509_FAILED   = 0x10, // Ошибка хранилища X509
					FINGERPRINT_FAILED  = 0x11, // Ошибка цифрового отпечатка
					COMPRESSION_FAILED  = 0x12, // Ошибка компрессии
					TLS_SESSION_FAILED  = 0x13, // Ошибка TLS сессии
					PRIVATE_KEY_FAILED  = 0x14, // Ошибка приватного ключа
					HOSTNAME_BAD        = 0x15, // Ошибка имени хоста
					INVALID_LAYER       = 0x16, // Ошибка уровня TLS
					UNSUPPORTED_IP      = 0x17, // Ошибка неподдерживаемого IP-адреса
					HOSTNAME_VERIFY     = 0x18, // Ошибка проверки имени хоста
					MISMATCH_VERSION    = 0x19, // Ошибка версии TLS
					UNSUPPORTED_VERSION = 0x1A, // Ошибка неподдерживаемой версии TLS
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
				 * @brief Структура информации о шифре
				 *
				 * @note Содержит информацию о шифре, используемом в TLS-соединении, включая его название, стандартное название и код шифра.
				 */
				typedef struct __AWH_SHARED_EXPORT__ CipherInfo {
					bool tls13;      // Флаг, указывающий, является ли шифр TLSv1.3
					string name;     // Название шифра, например AES_128_GCM_SHA256
					string origin;   // Стандартное название шифра, например TLS_AES_128_GCM_SHA256
					cipher_t cipher; // Код шифра, например TLS_AES_128_GCM_SHA256
					/**
					 * @brief Конструктор
					 *
					 */
					explicit CipherInfo() noexcept;
				} cipher_info_t;
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
				 * @param id    идентификатор события
				 * @param state новое состояние TLS
				 */
				using state_callback_t = function <void (const id_t, const state_t)>;
				/**
				 * @brief Функция обратного вызова срабатывающая при записи
				 *
				 * @param id    идентификатор события
				 * @param event тип события TLS
				 * @param size  размер данных
				 */
				using write_callback_t = function <void (const id_t, const event_t, const size_t)>;
				/**
				 * @brief Функция обратного вызова срабатывающая при ошибке события
				 *
				 * @param id    идентификатор события
				 * @param error тип ошибки TLS
				 * @param info  дополнительная информация об ошибке
				 */
				using error_callback_t = function <void (const id_t, const error_t, const string &)>;
				/**
				 * @brief Функция обратного вызова срабатывающая при получении снимка браузера приславшего ClientHello
				 *
				 * @param id      идентификатор события
				 * @param browser объект отпечатка браузера
				 */
				using fingerprint_callback_t = function <void (const id_t, const fgp_t::browser_t &)>;
				/**
				 * @brief Функция обратного вызова срабатывающая при чтении
				 *
				 * @note Указатель buffer действителен только до возврата из callback.
				 *       Callback обязан синхронно скопировать данные; при реентрантном
				 *       вызове внутренний thread_local буфер может быть перезаписан.
				 *
				 * @param id     идентификатор события
				 * @param event  тип события TLS
				 * @param buffer буфер данных
				 * @param size   размер данных
				 */
				using read_callback_t = function <void (const id_t, const event_t, const uint8_t *, const size_t)>;
			private:
				// Объект работы с IP-адресами
				net_addr_t _addr;
				// Объект работы с компрессией
				awh::compressor::block_t _compressor;
			private:
				// Объект работы с отпечатками TLS
				const fgp_t * _fgp;
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод получения версии OpenSSL
				 *
				 * @return версия OpenSSL
				 */
				string version() const noexcept;
			public:
				/**
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @param mode флаг режима безопасности потоков
				 *
				 * @details Защищает только глобальный реестр TLS (ids, members, splice_map,
				 *          init OpenSSL). Методы модуля после закрепления id выполняются без
				 *          глобального lock; синхронизацию вызовов из разных потоков должен
				 *          обеспечивать вызывающий код.
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * @brief Метод подключения объекта для работы с отпечатками TLS
				 *
				 * @param fgp объект для работы с отпечатками TLS
				 */
				void fingerprint(const fgp_t * fgp) noexcept;
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
				 * @brief Метод получения списка доступных шифров
				 *
				 * @param id идентификатор события
				 * @return   список доступных шифров
				 */
				vector <cipher_info_t> availableCiphers(const id_t id) const noexcept;
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
				 *
				 * @note Только для CTL. На CLIENT проверяет сертификат пира (сервера)
				 *       по member->host.name (ожидаемое имя/SNI). На SERVER peer-сертификат —
				 *       сертификат клиента, host.name — SNI клиента; вызывающий код должен
				 *       понимать эту семантику (mTLS и т.п.).
				 */
				bool validateCertificate(const id_t id) const noexcept;
			public:
				/**
				 * @brief Метод установки проверки доменного имени сервера
				 *
				 * @param id   идентификатор события
				 * @param mode режим проверки доменного имени сервера
				 */
				void validateServerNameIndication(const id_t id, const bool mode) noexcept;
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
				 * @brief Метод получения доменного имени сервера
				 *
				 * @param id идентификатор события
				 * @return   доменное имя сервера
				 */
				string serverNameIndication(const id_t id) const noexcept;
				/**
				 * @brief Метод установки доменного имени сервера
				 *
				 * @param id  идентификатор события
				 * @param sni доменное имя сервера
				 */
				void serverNameIndication(const id_t id, string_view sni) noexcept;
			public:
				/**
				 * @brief Метод извлечения кэшированного билета возобновления сессии (RFC 9001 §4.6)
				 *
				 * @note Билеты возобновления хранятся на шаблонном контексте безопасности
				 *       по ключу сервера (SNI либо адрес эндпоинта), что делает
				 *       возобновление сессии прозрачным для вызывающего кода
				 *
				 * @param id      идентификатор шаблонного контекста безопасности
				 * @param key     ключ сервера (SNI либо адрес эндпоинта)
				 * @param session объект для извлечения сериализованного билета возобновления
				 * @return        результат извлечения (билет найден)
				 */
				bool session(const id_t id, string_view key, string & session) const noexcept;
				/**
				 * @brief Метод сохранения билета возобновления сессии в кэш (RFC 9001 §4.6)
				 *
				 * @param id      идентификатор шаблонного контекста безопасности
				 * @param key     ключ сервера (SNI либо адрес эндпоинта)
				 * @param session сериализованный билет возобновления для сохранения
				 */
				void session(const id_t id, string_view key, string_view session) const noexcept;
			public:
				/**
				 * @brief Метод установки адреса и порта отдалённого узла
				 *
				 * @param id   идентификатор события
				 * @param ip   IP-адрес отдалённого узла
				 * @param port порт отдалённого узла
				 * @return     результат выполнения установки
				 */
				bool peer(const id_t id, string_view ip, const uint16_t port) noexcept;
			public:
				/**
				 * @brief Метод удаления контекста TLS
				 *
				 * @param id идентификатор транспортного уровня или шаблона контекста безопасности
				 * @return   результат выполнения удаления
				 *
				 * @note После destroy() id помечается GARBAGE_MODE; дальнейшие вызовы методов
				 *       с этим id недопустимы. Физическое удаление из реестра — при refs==0.
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
				 *
				 * @note Hot path: id — валидный CTL из transport(); __awh_ssl_ids__ не проверяется.
				 *       Параллельные вызовы на один id должен сериализовать вызывающий код.
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
				 * @brief Метод получения нативного контекста криптографической библиотеки
				 *
				 * @note Предназначен для протоколов, которые ведут собственный обмен
				 *       данными поверх настроенного контекста и не пользуются
				 *       транспортным уровнем кодера - в частности для QUIC, где
                 *       криптографический слой замещает слой записей TLS целиком.
				 *       Владение контекстом остаётся за кодером
				 *
				 * @param id идентификатор транспортного уровня или шаблона контекста безопасности
				 * @return   нативный контекст криптографической библиотеки либо nullptr
				 */
				ssl_ctx_st * native(const id_t id) const noexcept;
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
				 * @brief Метод получения сериализованного ECHConfigList для публикации в DNS
				 *
				 * @param id идентификатор события
				 * @return   байты ECHConfigList для DNS HTTPS-записи (для сервера)
				 *           или байты ECHConfigList полученные из DNS (для клиента).
				 *           Возвращает пустой вектор если ECH не был настроен.
				 *
				 * @details Метод возвращает ECHConfigList, сохранённый в памяти после вызова
				 *          setKeysECH(). Для сервера содержит публичные ECHConfig, которые
				 *          нужно опубликовать через DNS HTTPS-запись (поле eckparam), чтобы
				 *          клиенты могли найти публичный ключ и зашифровать ClientHello.
				 *
				 *          Пример использования для сервера:
				 *          @code
				 *          // Шаг 1: настраиваем ECH на сервере
				 *          coder.serverNameIndication(ctx, "example.com");
				 *          coder.setKeysECH(ctx, {}); // {} = автогенерация ключа
				 *
				 *          // Шаг 2: получаем ECHConfigList для DNS
				 *          vector <uint8_t> echDns = coder.getKeysECH(ctx);
				 *          // Опубликовать echDns через DNS HTTPS-запись
				 *          @endcode
				 *
				 *          Пример использования для клиента:
				 *          @code
				 *          // ECHConfigList из DNS HTTPS-записи был передан через setKeysECH().
				 *          // getKeysECH() возвращает те же байты, которые были сохранены.
				 *          vector <uint8_t> echDns = coder.getKeysECH(ctx);
				 *          @endcode
				 */
				vector <uint8_t> getKeysECH(const id_t id) const noexcept;
				/**
				 * @brief Метод установки ключей EncryptedClientHello (ECH)
				 *
				 * @param id   идентификатор события
				 * @param keys ключи EncryptedClientHello (ECH)
				 * @return     результат выполнения установки
				 *
				 * @details Метод поддерживается только в BoringSSL. Поведение зависит от
				 *          типа узла (CLIENT/SERVER) и уровня (CTS/CTL).
				 *
				 *          <b>Клиент (CTS или CTL):</b> @p keys — сериализованный ECHConfigList
				 *          из DNS HTTPS-записи. Сохраняется и применяется к каждому новому
				 *          SSL-соединению при вызове transport().
				 *
				 *          <b>Сервер (CTS):</b> @p keys — 32-байтовый приватный X25519 ключ.
				 *          Если @p keys пустой — ключ генерируется автоматически.
				 *          После успешной установки публичная часть ECHConfigList
				 *          доступна через getKeysECH() для публикации в DNS.
				 *          Для сервера CTL-уровень ECH не поддерживается; настройка
				 *          должна выполняться через CTS до создания транспорта.
				 *
				 *          Пример для клиента:
				 *          @code
				 *          // ECHConfigList из DNS HTTPS-записи (поле eckparam)
				 *          vector <uint8_t> echFromDns = fetchEchFromDns("example.com");
				 *          coder.setKeysECH(ctx, echFromDns); // ctx = CTS клиента
				 *          // Теперь каждый transport(ctx) будет шифровать ClientHello
				 *          @endcode
				 *
				 *          Пример для сервера:
				 *          @code
				 *          // {} = автогенерация ключа; либо передать 32-байтовый X25519 приватный ключ
				 *          coder.serverNameIndication(ctx, "example.com");
				 *          coder.setKeysECH(ctx, {}); // ctx = CTS сервера
				 *          // Получить ECHConfigList для DNS:
				 *          vector <uint8_t> forDns = coder.getKeysECH(ctx);
				 *          @endcode
				 */
				bool setKeysECH(const id_t id, const vector <uint8_t> & keys) noexcept;
				/**
				 * @brief Метод установки ключей EncryptedClientHello (ECH)
				 *
				 * @param id   идентификатор события
				 * @param keys ключи EncryptedClientHello (ECH)
				 * @param size размер ключей EncryptedClientHello (ECH)
				 * @return     результат выполнения установки
				 *
				 * @details Перегрузка setKeysECH() для сырого указателя вместо vector.
				 *          Поведение идентично первой перегрузке; см. ее документацию.
				 *
				 *          Для сервера если @p keys равен nullptr или @p size равен 0 —
				 *          ключ генерируется автоматически.
				 */
				bool setKeysECH(const id_t id, const uint8_t * keys, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод шифрования данных
				 *
				 * @param id     идентификатор события
				 * @param buffer буфер данных для шифрования
				 * @param size   размер буфера данных для шифрования
				 * @return       результат выполнения шифрования
				 *
				 * @note Hot path: id — валидный CTL из transport(); __awh_ssl_ids__ не проверяется.
				 *       Параллельные вызовы на один id должен сериализовать вызывающий код.
				 */
				bool encrypt(const id_t id, const void * buffer, const size_t size) noexcept;
				/**
				 * @brief Метод расшифровки данных
				 *
				 * @param id     идентификатор события
				 * @param buffer буфер данных для расшифровки
				 * @param size   размер буфера данных для расшифровки
				 * @return       результат выполнения расшифровки
				 *
				 * @note Hot path: id — валидный CTL из transport(); __awh_ssl_ids__ не проверяется.
				 *       Параллельные вызовы на один id должен сериализовать вызывающий код.
				 */
				bool decrypt(const id_t id, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод установки поддерживаемых групп эллиптических кривых
				 *
				 * @param id     идентификатор события
				 * @param groups список поддерживаемых групп эллиптических кривых
				 */
				void groups(const id_t id, const vector <group_t> & groups) noexcept;
			public:
				/**
				 * @brief Метод установки алгоритмов шифрования
				 *
				 * @param id      идентификатор события
				 * @param ciphers список алгоритмов шифрования для установки
				 */
				void ciphers(const id_t id, const vector <cipher_t> & ciphers) noexcept;
			public:
				/**
				 * @brief Метод активации/деактивации GREASE-значений (мусорных кодов)
				 *
				 * @param id   идентификатор события
				 * @param mode режим активации/деактивации
				 */
				void grease(const id_t id, const event::mode_t mode) noexcept;
			public:
				/**
				 * @brief Метод перемешивания поддерживаемых расширений TLS для имитации поведения различных браузеров
				 *
				 * @param id   идентификатор события
				 * @param mode режим активации/деактивации перемешивания расширений
				 */
				void permuteExtensions(const id_t id, const event::mode_t mode) noexcept;
			public:
				/**
				 * @brief Метод активации поддержки SCT (Signed Certificate Timestamp)
				 *
				 * @param id идентификатор события
				 */
				void signedCertificateTimestamp(const id_t id) noexcept;
				/**
				 * @brief Метод активации поддержки Stapling (OCSP)
				 *
				 * @param id идентификатор события
				 */
				void onlineCertificateStatusProtocol(const id_t id) noexcept;
				/**
				 * @brief Метод активации поддержки расширения Next Protocol Negotiation (NPN)
				 *
				 * @param id   идентификатор события
				 * @param mode режим активации/деактивации поддержки расширения
				 */
				void nextProtocolNegotiation(const id_t id, const event::mode_t mode) noexcept;
			public:
				/**
				 * @brief Метод активации поддержки наложения цифрового отпечатка браузера на TLS-соединение
				 *
				 * @param id  идентификатор события
				 * @param fid идентификатор цифрового отпечатка браузера
				 */
				void browser(const id_t id, const fgp_t::id_t fid) noexcept;
			public:
				/**
				 * @brief Метод получения списка поддерживаемых ALPN-протоколов
				 *
				 * @note Выдаёт настроенный список, а не согласованный протокол:
				 *       согласованный отдаёт alpn() по идентификатору транспортного
				 *       уровня. Предназначен для протоколов, которые ведут собственный
				 *       обмен данными поверх настроенного контекста и применяют список
				 *       к своему объекту TLS самостоятельно
				 *
				 * @param id идентификатор транспортного уровня или шаблона контекста безопасности
				 * @return   список поддерживаемых ALPN-протоколов
				 */
				vector <alpn_t> protocols(const id_t id) const noexcept;
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
				 * @brief Метод установки поддерживаемых ALPS-протоколов
				 *
				 * @param id   идентификатор события
				 * @param alps список поддерживаемых ALPS-протоколов
				 * @param std  флаг поддерживаемого стандарта
				 */
				void alps(const id_t id, const vector <alpn_t> & alps, const standard_t std) noexcept;
			public:
				/**
				 * @brief Метод установки поддерживаемых алгоритмов подписи
				 *
				 * @param id         идентификатор события
				 * @param signatures список поддерживаемых алгоритмов подписи
				 */
				void signature(const id_t id, const vector <signature_t> & signatures) noexcept;
			public:
				/**
				 * @brief Метод установки поддерживаемых алгоритмов компрессии сертификата
				 *
				 * @param id     идентификатор события
				 * @param methods список поддерживаемых алгоритмов компрессии сертификата
				 */
				void compressors(const id_t id, const vector <awh::compressor::method_t> & methods) noexcept;
			public:
				/**
				 * @brief Метод генерации заранее клиентом эфемерного ключа и отправки серверу для поддерживаемых групп эллиптических кривых
				 *
				 * @param id     идентификатор события
				 * @param groups список поддерживаемых групп эллиптических кривых для ключевого обмена
				 * @param grease флаг активации/деактивации ложного ключа EncryptedClientHello (ECH)
				 */
				void keyShare(const id_t id, const vector <group_t> & groups, const event::mode_t grease) noexcept;
			public:
				/**
				 * @brief Метод установки сертификатов доверенных центров сертификации
				 *
				 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
				 * @param filename путь к файлу сертификата доверенных центров сертификации
				 */
				void ca(const id_t id, string_view filename) noexcept;
				/**
				 * @brief Метод установки сертификатов доверенных центров сертификации
				 *
				 * @param id   идентификатор транспортного уровня или шаблона контекста безопасности
				 * @param dir  адрес директории с сертификатами доверенных центров сертификации
				 * @param file путь к файлу сертификата доверенного центра сертификации
				 */
				void ca(const id_t id, string_view dir, string_view file) noexcept;
			public:
				/**
				 * @brief Метод установки списка отзыва сертификатов
				 *
				 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
				 * @param filename путь к файлу списка отзыва сертификатов
				 */
				void certificateRevocationList(const id_t id, string_view filename) noexcept;
				/**
				 * @brief Метод установки приватного ключа клиента
				 *
				 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
				 * @param filename путь к файлу приватного ключа клиента
				 * @param type     тип файла приватного ключа клиента
				 */
				void privateKey(const id_t id, string_view filename, const type_t type = type_t::PEM) noexcept;
				/**
				 * @brief Метод установки клиентского сертификата
				 *
				 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
				 * @param filename путь к файлу клиентского сертификата
				 * @param type     тип файла клиентского сертификата
				 */
				void certificate(const id_t id, string_view filename, const type_t type = type_t::PEM) noexcept;
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
				 * @brief Метод установки функции обратного вызова изменения состояния
				 *
				 * @param id       идентификатор транспортного уровня
				 * @param callback функция обратного вызова для установки
				 * @return         результат установки функции обратного вызова
				 */
				bool on(const id_t id, state_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова получения ошибок
				 *
				 * @param id       идентификатор транспортного уровня
				 * @param callback функция обратного вызова для установки
				 * @return         результат установки функции обратного вызова
				 */
				bool on(const id_t id, error_callback_t callback) noexcept;
				/**
				 * @brief Метод установки функции обратного вызова получения снимка браузера приславшего ClientHello
				 *
				 * @param id       идентификатор транспортного уровня
				 * @param callback функция обратного вызова для установки
				 * @return         результат установки функции обратного вызова
				 */
				bool on(const id_t id, fingerprint_callback_t callback) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit Coder(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param fgp объект для работы с отпечатками TLS
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit Coder(const fgp_t * fgp, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Coder() noexcept;
		} coder_t;
	};
};

#endif // __AWH_SSL_ENGINE__
