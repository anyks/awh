/**
 * @file coder.hpp
 * @date 2025-12-19
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * \~russian
 * @brief Заголовочный файл модуля транспортного уровня безопасности — класс tls::Coder,
 *        управляющий контекстами TLS и DTLS, сертификатами, наборами шифров, ALPN,
 *        верификацией пиров и выполнением защищённого рукопожатия поверх BoringSSL
 *
 * \~english
 * @brief Header file of the transport layer security module — the tls::Coder class,
 *        managing the TLS and DTLS contexts, the certificates, the cipher suites, ALPN,
 *        the verification of the peers and the performance of the secured handshake on top of BoringSSL
 *
 * \~
 *
 * @copyright Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SSL_ENGINE__
#define __AWH_SSL_ENGINE__

/**
 * \~russian
 * Предварительное объявление типа контекста криптографической библиотеки
 *
 * @details Объявление, а не подключение заголовочного файла: публичный интерфейс
 *          модуля не должен тянуть за собой зависимости внешних библиотек на
 *          этапе подключения библиотеки потребителем.
 *
 * \~english
 * @details A declaration rather than the inclusion of a header file: the public interface
 *          of the module must not drag along the dependencies of external libraries at
 *          the stage of the inclusion of the library by the consumer.
 *
 * \~
 */
struct ssl_ctx_st;

/**
 * Подключаем заголовочные файлы проекта
 */
#include "tls.hpp"
#include "fingerprint.hpp"
#include "../../net/addr.hpp"
#include "../../net/event.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"
#include "../../compressor/block.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../sys/macro_push.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief пространство имён работы с TLS
	 *
	 * \~english
	 * @brief namespace of working with TLS
	 *
	 * \~
	 */
	namespace tls {
		/**
		 * \~russian
		 * @brief Структура транспортного уровня безопасности
		 *
		 * @details Ведает защищённым подключением: проводит рукопожатие,
		 * шифрует уходящее и расшифровывает приходящее, проверяет
		 * удостоверения собеседника
		 *
		 * Держится в стороне от самого обмена: данные ей передают и забирают,
		 * а чтением и записью в сокет ведает движок. Благодаря этому одна и та
		 * же защита ложится и на потоковый обмен, и на дейтаграммный
		 *
		 * @note Рукопожатие не мгновенно и требует нескольких обменов. Пока
		 * оно не завершено, подключение данных не несёт, и состояние следует
		 * проверять, а не полагать связь готовой сразу после подключения
		 *
		 * \~english
		 * @brief Structure of the transport layer security
		 *
		 * @details Takes charge of the secured connection: conducts the handshake,
		 * encrypts the outgoing and decrypts the incoming, verifies
		 * the credentials of the interlocutor
		 *
		 * Keeps aside from the exchange itself: the data is passed to it and taken from it,
		 * while the reading from and the writing into the socket is taken charge of by the engine. Thanks to that one and the same
		 * protection lies both upon the stream exchange and upon the datagram one
		 *
		 * @note The handshake is not instantaneous and requires several exchanges. As long as
		 * it is not completed, the connection carries no data, and the state should
		 * be checked rather than the link assumed ready right after the connection
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Coder {
			public:
				/**
				 * \~russian
				 * @brief Типы событий TLS
				 *
				 * \~english
				 * @brief Types of the TLS events
				 *
				 * \~
				 */
				enum class event_t : uint8_t {
					NONE       = 0x00, // Событие не установлено
					ENCRYPTION = 0x01, // Событие шифрования данных
					DECRYPTION = 0x02  // Событие расшифровки данных
				};
				/**
				 * \~russian
				 * @brief Состояния TLS работы
				 *
				 * @details Отмечает, на каком этапе защищённое подключение находится:
				 * рукопожатие идёт, состоялось либо не удалось
				 *
				 * @warning Неудача рукопожатия и неудача уже установленного
				 * подключения различаются намеренно. Первая обычно означает
				 * несогласие сторон - разошлись издания договора или не принято
				 * удостоверение, - и повторять попытку без изменений бессмысленно
				 *
				 * \~english
				 * @brief States of the TLS work
				 *
				 * @details Marks at what stage the secured connection is:
				 * the handshake is going on, has taken place or has failed
				 *
				 * @warning A failure of the handshake and a failure of an already established
				 * connection are distinguished deliberately. The first usually means
				 * a disagreement of the parties - the editions of the agreement diverged or the credential was not
				 * accepted, - and repeating the attempt without changes is pointless
				 *
				 * \~
				 */
				enum class state_t : uint8_t {
					NONE             = 0x00, // Состояние не установлено
					FAILED           = 0x01, // Состояние ошибки
					DESTROYED        = 0x02, // Состояние разрушения
					HANDSHAKED       = 0x03, // Состояние рукопожатия
					HANDSHAKE_FAILED = 0x04  // Состояние ошибки рукопожатия
				};
				/**
				 * \~russian
				 * @brief Режимы работы TLS
				 *
				 * @details Задаёт, одно ли удостоверение держит узел или несколько.
				 * Несколько нужны серверу, обслуживающему разные имена: собеседник
				 * сообщает желаемое имя в самом начале рукопожатия, и удостоверение
				 * подбирается под него
				 *
				 * @note Режим этот имеет смысл лишь для принимающей стороны:
				 * подключающийся своё удостоверение выбирает не по имени
				 *
				 * \~english
				 * @brief Modes of the TLS work
				 *
				 * @details Sets whether the node holds one credential or several.
				 * Several are needed by a server serving different names: the interlocutor
				 * reports the desired name at the very beginning of the handshake, and the credential
				 * is picked to match it
				 *
				 * @note This mode makes sense only for the accepting side:
				 * the connecting one chooses its credential not by the name
				 *
				 * \~
				 */
				enum class mode_t : uint8_t {
					NONE      = 0x00, // Режим не установлен
					UNICERT   = 0x01, // Режим единственного сертификата
					MULTICERT = 0x02  // Режим мультисертификатов
				};
				/**
				 * \~russian
				 * @brief Флаги типов файлов TLS
				 *
				 * @details Способ записи удостоверений и ключей в файле: текстовый,
				 * узнаваемый по обрамляющим строкам, либо двоичный
				 *
				 * @note Определить способ по имени файла нельзя - расширения тут не
				 * показатель, и указывать его приходится явно
				 *
				 * \~english
				 * @brief Flags of the types of the TLS files
				 *
				 * @details The way of recording the credentials and the keys in a file: textual,
				 * recognizable by the framing lines, or binary
				 *
				 * @note The way cannot be determined by the name of the file - the extensions are no
				 * indicator here, and it has to be stated explicitly
				 *
				 * \~
				 */
				enum class type_t : uint8_t {
					NONE = 0x00, // Тип не установлен
					PEM  = 0x01, // Формат PEM
					ASN1 = 0x02, // Формат ASN1
				};
				/**
				 * \~russian
				 * @brief Флаги поддерживаемых стандартов TLS
				 *
				 * @details Различает старое и новое поведение там, где издания
				 * договора расходятся между собой
				 *
				 * @note Нужно это ради совместимости: узел, знающий лишь старое
				 * поведение, иначе не сможет подключиться вовсе
				 *
				 * \~english
				 * @brief Flags of the supported TLS standards
				 *
				 * @details Distinguishes the old and the new behaviour where the editions
				 * of the agreement diverge from one another
				 *
				 * @note This is needed for the sake of compatibility: a node knowing only the old
				 * behaviour would otherwise not be able to connect at all
				 *
				 * \~
				 */
				enum class standard_t : uint8_t {
					NONE = 0x00, // Стандарт не поддерживается
					OLD  = 0x01, // Поддерживается старый стандарт
					NEW  = 0x02  // Поддерживается новый стандарт
				};
				/**
				 * \~russian
				 * @brief Флаги типов ошибок TLS
				 *
				 * @details Причины, по которым защищённое подключение не задалось.
				 * Набор подробен намеренно: за общей неудачей рукопожатия стоят вещи
				 * совершенно разные - и несогласие о шифрах, и отвергнутое
				 * удостоверение, и отозванный ключ, - а лечатся они по-разному
				 *
				 * @note Разбираются здесь на семейства: неполадки с удостоверениями и
				 * их проверкой, несогласие сторон о свойствах подключения, ошибки
				 * чтения и записи. Первые - дело настройки, вторые - несовместимости,
				 * третьи - самой сети
				 *
				 * \~english
				 * @brief Flags of the types of the TLS errors
				 *
				 * @details The reasons why the secured connection did not work out.
				 * The set is detailed deliberately: behind a general failure of the handshake stand things
				 * entirely different - a disagreement about the ciphers, and a rejected
				 * credential, and a revoked key, - and they are cured differently
				 *
				 * @note They are sorted here into families: troubles with the credentials and
				 * their verification, a disagreement of the parties about the properties of the connection, errors
				 * of reading and writing. The first are a matter of configuration, the second of incompatibilities,
				 * the third of the network itself
				 *
				 * \~
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
				 * \~russian
				 * @brief Структура ALPN-протокола
				 *
				 * \~english
				 * @brief Structure of the ALPN protocol
				 *
				 * \~
				 */
				typedef struct ALPN {
					// Идентификатор ALPN-протокола
					uint8_t id = 0;
					// Название ALPN-протокола
					string protocol = "";
				} alpn_t;
			public:
				/**
				 * \~russian
				 * @brief Структура информации о шифре
				 *
				 * @note Содержит информацию о шифре, используемом в TLS-соединении, включая его название, стандартное название и код шифра.
				 *
				 * \~english
				 * @brief Structure of the information about a cipher
				 *
				 * @note Contains the information about the cipher used in the TLS connection, including its name, standard name and cipher code.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ CipherInfo {
					bool tls13;      // Флаг, указывающий, является ли шифр TLSv1.3
					string name;     // Название шифра, например AES_128_GCM_SHA256
					string origin;   // Стандартное название шифра, например TLS_AES_128_GCM_SHA256
					cipher_t cipher; // Код шифра, например TLS_AES_128_GCM_SHA256
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit CipherInfo() noexcept;
				} cipher_info_t;
			public:
				/**
				 * \~russian
				 * @brief Тип идентификатора события
				 *
				 * \~english
				 * @brief Type of the event identifier
				 *
				 * \~
				 */
				using id_t = uint64_t;
			public:
				/**
				 * \~russian
				 * @brief Функция обратного вызова срабатывающая при изменении состояния
				 *
				 * @param id    идентификатор события
				 * @param state новое состояние TLS
				 *
				 * \~english
				 * @brief Callback function triggered upon a change of the state
				 *
				 * @param id    event identifier
				 * @param state new TLS state
				 *
				 * \~
				 */
				using state_callback_t = function <void (const id_t, const state_t)>;
				/**
				 * \~russian
				 * @brief Функция обратного вызова срабатывающая при записи
				 *
				 * @param id    идентификатор события
				 * @param event тип события TLS
				 * @param size  размер данных
				 *
				 * \~english
				 * @brief Callback function triggered upon writing
				 *
				 * @param id    event identifier
				 * @param event TLS event type
				 * @param size  data size
				 *
				 * \~
				 */
				using write_callback_t = function <void (const id_t, const event_t, const size_t)>;
				/**
				 * \~russian
				 * @brief Функция обратного вызова срабатывающая при ошибке события
				 *
				 * @param id    идентификатор события
				 * @param error тип ошибки TLS
				 * @param info  дополнительная информация об ошибке
				 *
				 * \~english
				 * @brief Callback function triggered upon an error of the event
				 *
				 * @param id    event identifier
				 * @param error TLS error type
				 * @param info  additional information about the error
				 *
				 * \~
				 */
				using error_callback_t = function <void (const id_t, const error_t, const string &)>;
				/**
				 * \~russian
				 * @brief Функция обратного вызова срабатывающая при получении снимка браузера приславшего ClientHello
				 *
				 * @param id      идентификатор события
				 * @param browser объект отпечатка браузера
				 *
				 * \~english
				 * @brief Callback function triggered upon obtaining the snapshot of the browser that sent the ClientHello
				 *
				 * @param id      event identifier
				 * @param browser object of the browser fingerprint
				 *
				 * \~
				 */
				using fingerprint_callback_t = function <void (const id_t, const fgp_t::browser_t &)>;
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Callback function triggered upon reading
				 *
				 * @note The buffer pointer is valid only until the return from the callback.
				 *       The callback is obliged to copy the data synchronously; upon a reentrant
				 *       call the internal thread_local buffer may be overwritten.
				 *
				 * @param id     event identifier
				 * @param event  TLS event type
				 * @param buffer data buffer
				 * @param size   data size
				 *
				 * \~
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
				 * \~russian
				 * @brief Метод получения версии OpenSSL
				 *
				 * @return версия OpenSSL
				 *
				 * \~english
				 * @brief Method obtaining the OpenSSL version
				 *
				 * @return OpenSSL version
				 *
				 * \~
				 */
				string version() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки безопасности работы потоков
				 *
				 * @param mode флаг режима безопасности потоков
				 *
				 * @details Защищает только глобальный реестр TLS (ids, members, splice_map,
				 *          init OpenSSL). Методы модуля после закрепления id выполняются без
				 *          глобального lock; синхронизацию вызовов из разных потоков должен
				 *          обеспечивать вызывающий код.
				 *
				 * \~english
				 * @brief Method setting the safety of the work of the threads
				 *
				 * @param mode flag of the mode of the safety of the threads
				 *
				 * @details Protects only the global TLS registry (ids, members, splice_map,
				 *          init OpenSSL). The methods of the module after the fixing of the id are performed without
				 *          a global lock; the synchronization of the calls from different threads must be
				 *          ensured by the calling code.
				 *
				 * \~
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод подключения объекта для работы с отпечатками TLS
				 *
				 * @param fgp объект для работы с отпечатками TLS
				 *
				 * \~english
				 * @brief Method connecting the object for working with the TLS fingerprints
				 *
				 * @param fgp object for working with the TLS fingerprints
				 *
				 * \~
				 */
				void fingerprint(const fgp_t * fgp) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения общей информации о TLS соединении
				 *
				 * @param id идентификатор события
				 * @return   общая информация о TLS соединении
				 *
				 * \~english
				 * @brief Method obtaining the general information about the TLS connection
				 *
				 * @param id event identifier
				 * @return   general information about the TLS connection
				 *
				 * \~
				 */
				string info(const id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения информации о одноразовом узле TLS
				 *
				 * @param id идентификатор события
				 * @return   информация о одноразовом узле TLS
				 *
				 * \~english
				 * @brief Method obtaining the information about the one-time TLS node
				 *
				 * @param id event identifier
				 * @return   information about the one-time TLS node
				 *
				 * \~
				 */
				string peerInfo(const id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения информации о шифре
				 *
				 * @param id идентификатор события
				 * @return   информация о шифре
				 *
				 * \~english
				 * @brief Method obtaining the information about the cipher
				 *
				 * @param id event identifier
				 * @return   information about the cipher
				 *
				 * \~
				 */
				string cipherInfo(const id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения информации о сертификате
				 *
				 * @param id идентификатор события
				 * @return   информация о сертификате
				 *
				 * \~english
				 * @brief Method obtaining the information about the certificate
				 *
				 * @param id event identifier
				 * @return   information about the certificate
				 *
				 * \~
				 */
				string certificateInfo(const id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения информации о списке отзыва сертификатов
				 *
				 * @param id идентификатор события
				 * @return   информация о списке отзыва сертификатов
				 *
				 * \~english
				 * @brief Method obtaining the information about the certificate revocation list
				 *
				 * @param id event identifier
				 * @return   information about the certificate revocation list
				 *
				 * \~
				 */
				string certificateRevocationListInfo(const id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения списка доступных шифров
				 *
				 * @param id идентификатор события
				 * @return   список доступных шифров
				 *
				 * \~english
				 * @brief Method obtaining the list of the available ciphers
				 *
				 * @param id event identifier
				 * @return   list of the available ciphers
				 *
				 * \~
				 */
				vector <cipher_info_t> availableCiphers(const id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения сертификата TLS
				 *
				 * @param id идентификатор события
				 * @return   активный протокол
				 *
				 * \~english
				 * @brief Method extracting the TLS certificate
				 *
				 * @param id event identifier
				 * @return   active protocol
				 *
				 * \~
				 */
				string certificateExtract(const id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки валидности сертификата
				 *
				 * @param id идентификатор события
				 * @return   результат проверки валидности сертификата
				 *
				 * @note Только для CTL. На CLIENT проверяет сертификат пира (сервера)
				 *       по member->host.name (ожидаемое имя/SNI). На SERVER peer-сертификат —
				 *       сертификат клиента, host.name — SNI клиента; вызывающий код должен
				 *       понимать эту семантику (mTLS и т.п.).
				 *
				 * \~english
				 * @brief Method checking the validity of the certificate
				 *
				 * @param id event identifier
				 * @return   result of checking the validity of the certificate
				 *
				 * @note Only for CTL. On CLIENT it checks the certificate of the peer (of the server)
				 *       against member->host.name (the expected name/SNI). On SERVER the peer certificate is
				 *       the certificate of the client, host.name is the SNI of the client; the calling code must
				 *       understand this semantics (mTLS and the like).
				 *
				 * \~
				 */
				bool validateCertificate(const id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки проверки доменного имени сервера
				 *
				 * @param id   идентификатор события
				 * @param mode режим проверки доменного имени сервера
				 *
				 * \~english
				 * @brief Method setting the verification of the domain name of the server
				 *
				 * @param id   event identifier
				 * @param mode mode of the verification of the domain name of the server
				 *
				 * \~
				 */
				void validateServerNameIndication(const id_t id, const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения режима работы TLS
				 *
				 * @param id идентификатор события
				 * @return   режим работы TLS
				 *
				 * \~english
				 * @brief Method obtaining the mode of the TLS work
				 *
				 * @param id event identifier
				 * @return   mode of the TLS work
				 *
				 * \~
				 */
				mode_t mode(const id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки режима работы TLS
				 *
				 * @param id   идентификатор события
				 * @param mode режим работы TLS
				 *
				 * \~english
				 * @brief Method setting the mode of the TLS work
				 *
				 * @param id   event identifier
				 * @param mode mode of the TLS work
				 *
				 * \~
				 */
				void mode(const id_t id, const mode_t mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения доменного имени сервера
				 *
				 * @param id идентификатор события
				 * @return   доменное имя сервера
				 *
				 * \~english
				 * @brief Method obtaining the domain name of the server
				 *
				 * @param id event identifier
				 * @return   domain name of the server
				 *
				 * \~
				 */
				string serverNameIndication(const id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки доменного имени сервера
				 *
				 * @param id  идентификатор события
				 * @param sni доменное имя сервера
				 *
				 * \~english
				 * @brief Method setting the domain name of the server
				 *
				 * @param id  event identifier
				 * @param sni domain name of the server
				 *
				 * \~
				 */
				void serverNameIndication(const id_t id, string_view sni) noexcept;
			public:
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Method extracting the cached session resumption ticket (RFC 9001 §4.6)
				 *
				 * @note The resumption tickets are stored on the template security context
				 *       by the key of the server (the SNI or the address of the endpoint), which makes
				 *       the session resumption transparent for the calling code
				 *
				 * @param id      identifier of the template security context
				 * @param key     key of the server (the SNI or the address of the endpoint)
				 * @param session object for extracting the serialized resumption ticket
				 * @return        result of the extraction (the ticket has been found)
				 *
				 * \~
				 */
				bool session(const id_t id, string_view key, string & session) const noexcept;
				/**
				 * \~russian
				 * @brief Метод сохранения билета возобновления сессии в кэш (RFC 9001 §4.6)
				 *
				 * @param id      идентификатор шаблонного контекста безопасности
				 * @param key     ключ сервера (SNI либо адрес эндпоинта)
				 * @param session сериализованный билет возобновления для сохранения
				 *
				 * \~english
				 * @brief Method saving the session resumption ticket into the cache (RFC 9001 §4.6)
				 *
				 * @param id      identifier of the template security context
				 * @param key     key of the server (the SNI or the address of the endpoint)
				 * @param session serialized resumption ticket for the saving
				 *
				 * \~
				 */
				void session(const id_t id, string_view key, string_view session) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки адреса и порта отдалённого узла
				 *
				 * @param id   идентификатор события
				 * @param ip   IP-адрес отдалённого узла
				 * @param port порт отдалённого узла
				 * @return     результат выполнения установки
				 *
				 * \~english
				 * @brief Method setting the address and the port of the remote node
				 *
				 * @param id   event identifier
				 * @param ip   IP address of the remote node
				 * @param port port of the remote node
				 * @return     result of performing the setting
				 *
				 * \~
				 */
				bool peer(const id_t id, string_view ip, const uint16_t port) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод удаления контекста TLS
				 *
				 * @param id идентификатор транспортного уровня или шаблона контекста безопасности
				 * @return   результат выполнения удаления
				 *
				 * @note После destroy() id помечается GARBAGE_MODE; дальнейшие вызовы методов
				 *       с этим id недопустимы. Физическое удаление из реестра — при refs==0.
				 *
				 * \~english
				 * @brief Method removing the TLS context
				 *
				 * @param id identifier of the transport layer or of the template security context
				 * @return   result of performing the removal
				 *
				 * @note After destroy() the id is marked GARBAGE_MODE; further calls of the methods
				 *       with this id are inadmissible. The physical removal from the registry happens at refs==0.
				 *
				 * \~
				 */
				bool destroy(const id_t id) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод завершения TLS соединения
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения завершения
				 *
				 * \~english
				 * @brief Method terminating the TLS connection
				 *
				 * @param id event identifier
				 * @return   result of performing the termination
				 *
				 * \~
				 */
				bool shutdown(const id_t id) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод выполнения TLS рукопожатия
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения рукопожатия
				 *
				 * @note Hot path: id — валидный CTL из transport(); __awh_ssl_ids__ не проверяется.
				 *       Параллельные вызовы на один id должен сериализовать вызывающий код.
				 *
				 * \~english
				 * @brief Method performing the TLS handshake
				 *
				 * @param id event identifier
				 * @return   result of performing the handshake
				 *
				 * @note Hot path: the id is a valid CTL from transport(); __awh_ssl_ids__ is not checked.
				 *       Parallel calls on one id must be serialized by the calling code.
				 *
				 * \~
				 */
				bool handshake(const id_t id) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод повторной передачи данных
				 *
				 * @param id идентификатор события
				 * @return   результат выполнения повторной передачи
				 *
				 * \~english
				 * @brief Method of the repeated transmission of the data
				 *
				 * @param id event identifier
				 * @return   result of performing the repeated transmission
				 *
				 * \~
				 */
				bool retransmit(const id_t id) noexcept;
			public:
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Method obtaining the native context of the cryptographic library
				 *
				 * @note Intended for the protocols that conduct their own exchange of
				 *       data on top of a configured context and do not use
				 *       the transport layer of the coder - in particular for QUIC, where
				 *       the cryptographic layer replaces the TLS record layer entirely.
				 *       The ownership of the context remains with the coder
				 *
				 * @param id identifier of the transport layer or of the template security context
				 * @return   native context of the cryptographic library or nullptr
				 *
				 * \~
				 */
				ssl_ctx_st * native(const id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод создания идентификатора транспортного уровня
				 *
				 * @param id идентификатор шаблона контекста безопасности
				 * @return   идентификатор транспортного уровня
				 *
				 * \~english
				 * @brief Method creating the identifier of the transport layer
				 *
				 * @param id identifier of the template security context
				 * @return   identifier of the transport layer
				 *
				 * \~
				 */
				id_t transport(const id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод создания идентификатора шаблона контекста безопасности
				 *
				 * @param node  тип узла события
				 * @param proto тип протокола события
				 * @return      идентификатор шаблона контекста безопасности
				 *
				 * \~english
				 * @brief Method creating the identifier of the template security context
				 *
				 * @param node  type of the event node
				 * @param proto type of the event protocol
				 * @return      identifier of the template security context
				 *
				 * \~
				 */
				id_t context(const event::node_t node, const event::protocol_t proto) noexcept;
			public:
				/**
				 * \~russian
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
				 *          Пример использования для клиента:
				 *
				 * @code
				 * // Шаг 1: настраиваем ECH на сервере
				 * coder.serverNameIndication(ctx, "example.com");
				 * coder.setKeysECH(ctx, {}); // {} = автогенерация ключа
				 *
				 * // Шаг 2: получаем ECHConfigList для DNS
				 * vector <uint8_t> echDns = coder.getKeysECH(ctx);
				 * // Опубликовать echDns через DNS HTTPS-запись
				 * @endcode
				 *
				 * @code
				 * // ECHConfigList из DNS HTTPS-записи был передан через setKeysECH().
				 * // getKeysECH() возвращает те же байты, которые были сохранены.
				 * vector <uint8_t> echDns = coder.getKeysECH(ctx);
				 * @endcode
				 *
				 * \~english
				 * @brief Method obtaining the serialized ECHConfigList for the publication in DNS
				 *
				 * @param id event identifier
				 * @return   bytes of the ECHConfigList for the DNS HTTPS record (for the server)
				 *           or bytes of the ECHConfigList obtained from DNS (for the client).
				 *           Returns an empty vector if ECH has not been configured.
				 *
				 * @details The method returns the ECHConfigList stored in memory after the call of
				 *          setKeysECH(). For the server it contains the public ECHConfig which
				 *          needs to be published through a DNS HTTPS record (the eckparam field) so that
				 *          the clients can find the public key and encrypt the ClientHello.
				 *
				 *          An example of the usage for the server:
				 *
				 * @code
				 * // Step 1: setting up ECH on the server
				 * coder.serverNameIndication(ctx, "example.com");
				 * coder.setKeysECH(ctx, {}); // {} = the automatic generation of the key
				 *
				 * // Step 2: getting the ECHConfigList for DNS
				 * vector <uint8_t> echDns = coder.getKeysECH(ctx);
				 * // To publish echDns through a DNS HTTPS record
				 * @endcode
				 *
				 * @code
				 * // The ECHConfigList from the DNS HTTPS record was passed through setKeysECH().
				 * // getKeysECH() returns the same bytes that were saved.
				 * vector <uint8_t> echDns = coder.getKeysECH(ctx);
				 * @endcode
				 *
				 */
				vector <uint8_t> getKeysECH(const id_t id) const noexcept;
				/**
				 * \~russian
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
				 *          Пример для сервера:
				 *
				 * @code
				 * // ECHConfigList из DNS HTTPS-записи (поле eckparam)
				 * vector <uint8_t> echFromDns = fetchEchFromDns("example.com");
				 * coder.setKeysECH(ctx, echFromDns); // ctx = CTS клиента
				 * // Теперь каждый transport(ctx) будет шифровать ClientHello
				 * @endcode
				 *
				 * @code
				 * // {} = автогенерация ключа; либо передать 32-байтовый X25519 приватный ключ
				 * coder.serverNameIndication(ctx, "example.com");
				 * coder.setKeysECH(ctx, {}); // ctx = CTS сервера
				 * // Получить ECHConfigList для DNS:
				 * vector <uint8_t> forDns = coder.getKeysECH(ctx);
				 * @endcode
				 *
				 * \~english
				 * @brief Method setting the EncryptedClientHello (ECH) keys
				 *
				 * @param id   event identifier
				 * @param keys EncryptedClientHello (ECH) keys
				 * @return     result of performing the setting
				 *
				 * @details The method is supported only in BoringSSL. The behaviour depends on
				 *          the type of the node (CLIENT/SERVER) and on the layer (CTS/CTL).
				 *
				 *          <b>Client (CTS or CTL):</b> @p keys is the serialized ECHConfigList
				 *          from the DNS HTTPS record. It is stored and applied to every new
				 *          SSL connection upon the call of transport().
				 *
				 *          <b>Server (CTS):</b> @p keys is a 32-byte private X25519 key.
				 *          If @p keys is empty — the key is generated automatically.
				 *          After a successful setting the public part of the ECHConfigList
				 *          is available through getKeysECH() for the publication in DNS.
				 *          For the server the CTL-level ECH is not supported; the configuration
				 *          must be performed through CTS before the creation of the transport.
				 *
				 *          An example for the client:
				 *
				 * @code
				 * // The ECHConfigList from the DNS HTTPS record (the eckparam field)
				 * vector <uint8_t> echFromDns = fetchEchFromDns("example.com");
				 * coder.setKeysECH(ctx, echFromDns); // ctx = the CTS of the client
				 * // Now every transport(ctx) will encrypt the ClientHello
				 * @endcode
				 *
				 * @code
				 * // {} = the automatic generation of the key; or to pass a 32-byte X25519 private key
				 * coder.serverNameIndication(ctx, "example.com");
				 * coder.setKeysECH(ctx, {}); // ctx = the CTS of the server
				 * // To get the ECHConfigList for DNS:
				 * vector <uint8_t> forDns = coder.getKeysECH(ctx);
				 * @endcode
				 *
				 */
				bool setKeysECH(const id_t id, const vector <uint8_t> & keys) noexcept;
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Method setting the EncryptedClientHello (ECH) keys
				 *
				 * @param id   event identifier
				 * @param keys EncryptedClientHello (ECH) keys
				 * @param size size of the EncryptedClientHello (ECH) keys
				 * @return     result of performing the setting
				 *
				 * @details An overload of setKeysECH() for a raw pointer instead of a vector.
				 *          The behaviour is identical to the first overload; see its documentation.
				 *
				 *          For the server, if @p keys equals nullptr or @p size equals 0 —
				 *          the key is generated automatically.
				 *
				 * \~
				 */
				bool setKeysECH(const id_t id, const uint8_t * keys, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод шифрования данных
				 *
				 * @param id     идентификатор события
				 * @param buffer буфер данных для шифрования
				 * @param size   размер буфера данных для шифрования
				 * @return       результат выполнения шифрования
				 *
				 * @note Hot path: id — валидный CTL из transport(); __awh_ssl_ids__ не проверяется.
				 *       Параллельные вызовы на один id должен сериализовать вызывающий код.
				 *
				 * \~english
				 * @brief Method encrypting the data
				 *
				 * @param id     event identifier
				 * @param buffer data buffer for the encryption
				 * @param size   size of the data buffer for the encryption
				 * @return       result of performing the encryption
				 *
				 * @note Hot path: the id is a valid CTL from transport(); __awh_ssl_ids__ is not checked.
				 *       Parallel calls on one id must be serialized by the calling code.
				 *
				 * \~
				 */
				bool encrypt(const id_t id, const void * buffer, const size_t size) noexcept;
				/**
				 * \~russian
				 * @brief Метод расшифровки данных
				 *
				 * @param id     идентификатор события
				 * @param buffer буфер данных для расшифровки
				 * @param size   размер буфера данных для расшифровки
				 * @return       результат выполнения расшифровки
				 *
				 * @note Hot path: id — валидный CTL из transport(); __awh_ssl_ids__ не проверяется.
				 *       Параллельные вызовы на один id должен сериализовать вызывающий код.
				 *
				 * \~english
				 * @brief Method decrypting the data
				 *
				 * @param id     event identifier
				 * @param buffer data buffer for the decryption
				 * @param size   size of the data buffer for the decryption
				 * @return       result of performing the decryption
				 *
				 * @note Hot path: the id is a valid CTL from transport(); __awh_ssl_ids__ is not checked.
				 *       Parallel calls on one id must be serialized by the calling code.
				 *
				 * \~
				 */
				bool decrypt(const id_t id, const void * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки поддерживаемых групп эллиптических кривых
				 *
				 * @param id     идентификатор события
				 * @param groups список поддерживаемых групп эллиптических кривых
				 *
				 * \~english
				 * @brief Method setting the supported groups of elliptic curves
				 *
				 * @param id     event identifier
				 * @param groups list of the supported groups of elliptic curves
				 *
				 * \~
				 */
				void groups(const id_t id, const vector <group_t> & groups) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки алгоритмов шифрования
				 *
				 * @param id      идентификатор события
				 * @param ciphers список алгоритмов шифрования для установки
				 *
				 * \~english
				 * @brief Method setting the encryption algorithms
				 *
				 * @param id      event identifier
				 * @param ciphers list of the encryption algorithms to set
				 *
				 * \~
				 */
				void ciphers(const id_t id, const vector <cipher_t> & ciphers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации/деактивации GREASE-значений (мусорных кодов)
				 *
				 * @param id   идентификатор события
				 * @param mode режим активации/деактивации
				 *
				 * \~english
				 * @brief Method activating/deactivating the GREASE values (the junk codes)
				 *
				 * @param id   event identifier
				 * @param mode activation/deactivation mode
				 *
				 * \~
				 */
				void grease(const id_t id, const event::mode_t mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод перемешивания поддерживаемых расширений TLS для имитации поведения различных браузеров
				 *
				 * @param id   идентификатор события
				 * @param mode режим активации/деактивации перемешивания расширений
				 *
				 * \~english
				 * @brief Method shuffling the supported TLS extensions for imitating the behaviour of various browsers
				 *
				 * @param id   event identifier
				 * @param mode mode of the activation/deactivation of the shuffling of the extensions
				 *
				 * \~
				 */
				void permuteExtensions(const id_t id, const event::mode_t mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации поддержки SCT (Signed Certificate Timestamp)
				 *
				 * @param id идентификатор события
				 *
				 * \~english
				 * @brief Method activating the support of SCT (Signed Certificate Timestamp)
				 *
				 * @param id event identifier
				 *
				 * \~
				 */
				void signedCertificateTimestamp(const id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод активации поддержки Stapling (OCSP)
				 *
				 * @param id идентификатор события
				 *
				 * \~english
				 * @brief Method activating the support of Stapling (OCSP)
				 *
				 * @param id event identifier
				 *
				 * \~
				 */
				void onlineCertificateStatusProtocol(const id_t id) noexcept;
				/**
				 * \~russian
				 * @brief Метод активации поддержки расширения Next Protocol Negotiation (NPN)
				 *
				 * @param id   идентификатор события
				 * @param mode режим активации/деактивации поддержки расширения
				 *
				 * \~english
				 * @brief Method activating the support of the Next Protocol Negotiation (NPN) extension
				 *
				 * @param id   event identifier
				 * @param mode mode of the activation/deactivation of the support of the extension
				 *
				 * \~
				 */
				void nextProtocolNegotiation(const id_t id, const event::mode_t mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод активации поддержки наложения цифрового отпечатка браузера на TLS-соединение
				 *
				 * @param id  идентификатор события
				 * @param fid идентификатор цифрового отпечатка браузера
				 *
				 * \~english
				 * @brief Method activating the support of imposing the browser digital fingerprint upon the TLS connection
				 *
				 * @param id  event identifier
				 * @param fid identifier of the browser digital fingerprint
				 *
				 * \~
				 */
				void browser(const id_t id, const fgp_t::id_t fid) noexcept;
			public:
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Method obtaining the list of the supported ALPN protocols
				 *
				 * @note Gives out the configured list rather than the negotiated protocol:
				 *       the negotiated one is given out by alpn() by the identifier of the transport
				 *       layer. Intended for the protocols that conduct their own
				 *       exchange of data on top of a configured context and apply the list
				 *       to their own TLS object themselves
				 *
				 * @param id identifier of the transport layer or of the template security context
				 * @return   list of the supported ALPN protocols
				 *
				 * \~
				 */
				vector <alpn_t> protocols(const id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения активного протокола
				 *
				 * @param id идентификатор события
				 * @return   метод активного протокола
				 *
				 * \~english
				 * @brief Method extracting the active protocol
				 *
				 * @param id event identifier
				 * @return   method of the active protocol
				 *
				 * \~
				 */
				uint8_t alpn(const id_t id) const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки поддерживаемых ALPN-протоколов
				 *
				 * @param id   идентификатор события
				 * @param alpn список поддерживаемых ALPN-протоколов
				 *
				 * \~english
				 * @brief Method setting the supported ALPN protocols
				 *
				 * @param id   event identifier
				 * @param alpn list of the supported ALPN protocols
				 *
				 * \~
				 */
				void alpn(const id_t id, const vector <alpn_t> & alpn) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки поддерживаемых ALPS-протоколов
				 *
				 * @param id   идентификатор события
				 * @param alps список поддерживаемых ALPS-протоколов
				 * @param std  флаг поддерживаемого стандарта
				 *
				 * \~english
				 * @brief Method setting the supported ALPS protocols
				 *
				 * @param id   event identifier
				 * @param alps list of the supported ALPS protocols
				 * @param std  flag of the supported standard
				 *
				 * \~
				 */
				void alps(const id_t id, const vector <alpn_t> & alps, const standard_t std) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки поддерживаемых алгоритмов подписи
				 *
				 * @param id         идентификатор события
				 * @param signatures список поддерживаемых алгоритмов подписи
				 *
				 * \~english
				 * @brief Method setting the supported signature algorithms
				 *
				 * @param id         event identifier
				 * @param signatures list of the supported signature algorithms
				 *
				 * \~
				 */
				void signature(const id_t id, const vector <signature_t> & signatures) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки поддерживаемых алгоритмов компрессии сертификата
				 *
				 * @param id     идентификатор события
				 * @param methods список поддерживаемых алгоритмов компрессии сертификата
				 *
				 * \~english
				 * @brief Method setting the supported algorithms of the certificate compression
				 *
				 * @param id     event identifier
				 * @param methods list of the supported algorithms of the certificate compression
				 *
				 * \~
				 */
				void compressors(const id_t id, const vector <awh::compressor::method_t> & methods) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод генерации заранее клиентом эфемерного ключа и отправки серверу для поддерживаемых групп эллиптических кривых
				 *
				 * @param id     идентификатор события
				 * @param groups список поддерживаемых групп эллиптических кривых для ключевого обмена
				 * @param grease флаг активации/деактивации ложного ключа EncryptedClientHello (ECH)
				 *
				 * \~english
				 * @brief Method of the generation in advance by the client of an ephemeral key and of sending it to the server for the supported groups of elliptic curves
				 *
				 * @param id     event identifier
				 * @param groups list of the supported groups of elliptic curves for the key exchange
				 * @param grease flag of the activation/deactivation of a false EncryptedClientHello (ECH) key
				 *
				 * \~
				 */
				void keyShare(const id_t id, const vector <group_t> & groups, const event::mode_t grease) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки сертификатов доверенных центров сертификации
				 *
				 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
				 * @param filename путь к файлу сертификата доверенных центров сертификации
				 *
				 * \~english
				 * @brief Method setting the certificates of the trusted certification authorities
				 *
				 * @param id       identifier of the transport layer or of the template security context
				 * @param filename path to the file of the certificate of the trusted certification authorities
				 *
				 * \~
				 */
				void ca(const id_t id, string_view filename) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки сертификатов доверенных центров сертификации
				 *
				 * @param id   идентификатор транспортного уровня или шаблона контекста безопасности
				 * @param dir  адрес директории с сертификатами доверенных центров сертификации
				 * @param file путь к файлу сертификата доверенного центра сертификации
				 *
				 * \~english
				 * @brief Method setting the certificates of the trusted certification authorities
				 *
				 * @param id   identifier of the transport layer or of the template security context
				 * @param dir  address of the directory with the certificates of the trusted certification authorities
				 * @param file path to the file of the certificate of a trusted certification authority
				 *
				 * \~
				 */
				void ca(const id_t id, string_view dir, string_view file) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки списка отзыва сертификатов
				 *
				 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
				 * @param filename путь к файлу списка отзыва сертификатов
				 *
				 * \~english
				 * @brief Method setting the certificate revocation list
				 *
				 * @param id       identifier of the transport layer or of the template security context
				 * @param filename path to the file of the certificate revocation list
				 *
				 * \~
				 */
				void certificateRevocationList(const id_t id, string_view filename) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки приватного ключа клиента
				 *
				 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
				 * @param filename путь к файлу приватного ключа клиента
				 * @param type     тип файла приватного ключа клиента
				 *
				 * \~english
				 * @brief Method setting the private key of the client
				 *
				 * @param id       identifier of the transport layer or of the template security context
				 * @param filename path to the file of the private key of the client
				 * @param type     type of the file of the private key of the client
				 *
				 * \~
				 */
				void privateKey(const id_t id, string_view filename, const type_t type = type_t::PEM) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки клиентского сертификата
				 *
				 * @param id       идентификатор транспортного уровня или шаблона контекста безопасности
				 * @param filename путь к файлу клиентского сертификата
				 * @param type     тип файла клиентского сертификата
				 *
				 * \~english
				 * @brief Method setting the client certificate
				 *
				 * @param id       identifier of the transport layer or of the template security context
				 * @param filename path to the file of the client certificate
				 * @param type     type of the file of the client certificate
				 *
				 * \~
				 */
				void certificate(const id_t id, string_view filename, const type_t type = type_t::PEM) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова получения данных
				 *
				 * @param id       идентификатор транспортного уровня
				 * @param callback функция обратного вызова для установки
				 * @return         результат установки функции обратного вызова
				 *
				 * \~english
				 * @brief Method setting the callback function of the receiving of the data
				 *
				 * @param id       identifier of the transport layer
				 * @param callback callback function to set
				 * @return         result of setting the callback function
				 *
				 * \~
				 */
				bool on(const id_t id, read_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова передачи данных
				 *
				 * @param id       идентификатор транспортного уровня
				 * @param callback функция обратного вызова для установки
				 * @return         результат установки функции обратного вызова
				 *
				 * \~english
				 * @brief Method setting the callback function of the transmission of the data
				 *
				 * @param id       identifier of the transport layer
				 * @param callback callback function to set
				 * @return         result of setting the callback function
				 *
				 * \~
				 */
				bool on(const id_t id, write_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова изменения состояния
				 *
				 * @param id       идентификатор транспортного уровня
				 * @param callback функция обратного вызова для установки
				 * @return         результат установки функции обратного вызова
				 *
				 * \~english
				 * @brief Method setting the callback function of the change of the state
				 *
				 * @param id       identifier of the transport layer
				 * @param callback callback function to set
				 * @return         result of setting the callback function
				 *
				 * \~
				 */
				bool on(const id_t id, state_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова получения ошибок
				 *
				 * @param id       идентификатор транспортного уровня
				 * @param callback функция обратного вызова для установки
				 * @return         результат установки функции обратного вызова
				 *
				 * \~english
				 * @brief Method setting the callback function of the receiving of the errors
				 *
				 * @param id       identifier of the transport layer
				 * @param callback callback function to set
				 * @return         result of setting the callback function
				 *
				 * \~
				 */
				bool on(const id_t id, error_callback_t callback) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки функции обратного вызова получения снимка браузера приславшего ClientHello
				 *
				 * @param id       идентификатор транспортного уровня
				 * @param callback функция обратного вызова для установки
				 * @return         результат установки функции обратного вызова
				 *
				 * \~english
				 * @brief Method setting the callback function of the obtaining of the snapshot of the browser that sent the ClientHello
				 *
				 * @param id       identifier of the transport layer
				 * @param callback callback function to set
				 * @return         result of setting the callback function
				 *
				 * \~
				 */
				bool on(const id_t id, fingerprint_callback_t callback) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Coder(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fgp объект для работы с отпечатками TLS
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * @param fgp object for working with the TLS fingerprints
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Coder(const fgp_t * fgp, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Coder() noexcept;
		} coder_t;
	};
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_SSL_ENGINE__
