/**
 * @file fingerprint.hpp
 * @date 2026-04-28
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
 * @brief Заголовочный файл модуля цифровых отпечатков TLS — класс tls::Fingerprint,
 *        формирующий и разбирающий расширения ClientHello (SNI, ALPN, supported_groups, GREASE, Channel ID, OCSP,
 *        SCT и другие) для эмуляции отпечатка клиента и анализа входящих рукопожатий
 *
 * \~english
 * @brief Header file of the TLS fingerprint module — the tls::Fingerprint class,
 *        which builds and parses ClientHello extensions (SNI, ALPN, supported_groups, GREASE, Channel ID, OCSP,
 *        SCT and others) to emulate a client fingerprint and analyse incoming handshakes
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SSL_FINGERPRINT__
#define __AWH_SSL_FINGERPRINT__

/**
 * Стандартные заголовочные файлы
 */
#include <array>
#include <string>
#include <vector>
#include <cstdint>
#include <shared_mutex>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "tls.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"
#include "../../sys/locker.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief пространство имён работы с TLS
	 *
	 *
	 * \~english
	 * @brief namespace of working with TLS
	 *
	 * \~
	 */
	namespace tls {
		/**
		 * \~russian
		 * @brief Структура цифрового отпечатка устройства
		 *
		 * \~english
		 * @brief Device fingerprint structure
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Fingerprint {
			public:
				/**
				 * \~russian
				 * @brief Тип идентификатора отпечатка браузера
				 *
				 * \~english
				 * @brief Browser fingerprint identifier type
				 *
				 * \~
				 */
				using id_t = uint8_t;
			public:
				/**
				 * \~russian
				 * @brief Структура расширения TLS
				 *
				 * @details Расширение TLS - это дополнительная информация, которая может быть включена в процесс установления соединения TLS.
				 *          Она позволяет клиенту и серверу обмениваться дополнительными данными, которые могут быть использованы для улучшения безопасности,
				 *          совместимости и функциональности соединения.
				 *
				 * \~english
				 * @brief TLS extension structure
				 * @details A TLS extension is additional information that can be included in the TLS connection establishment process.
				 *          It allows the client and the server to exchange extra data that can be used to improve the security,
				 *          compatibility and functionality of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension {
					// Тип расширения
					extension_type_t type;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param type Тип расширения
					 *
					 * \~english
					 * @brief Constructor
					 * @param type Extension type
					 *
					 * \~
					 */
					explicit Extension(const extension_type_t type) noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension() = default;
				} extension_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для GREASE-значений
				 *
				 * @details GREASE (Generate Random Extensions And Sustain Extensibility) - это механизм,
				 *          используемый в протоколе TLS для обеспечения совместимости и предотвращения проблем с расширениями.
				 *
				 * \~english
				 * @brief TLS extension structure for GREASE values
				 * @details GREASE (Generate Random Extensions And Sustain Extensibility) is a mechanism
				 *          used in the TLS protocol to ensure compatibility and prevent problems with extensions.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Grease : public extension_t {
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Grease() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Grease() = default;
				} extension_grease_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для идентификатора канала (Channel ID)
				 *
				 * @details Channel ID - это механизм, используемый в протоколе TLS для обеспечения уникальной идентификации канала связи между клиентом и сервером.
				 *
				 * \~english
				 * @brief TLS extension structure for the channel identifier (Channel ID)
				 * @details Channel ID is a mechanism used in the TLS protocol to uniquely identify the communication channel between the client and the server.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Channel_ID : public extension_t {
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Channel_ID() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Channel_ID() = default;
				} extension_channel_id_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для фильтров OID (OID Filters)
				 *
				 * @details OID Filters - это механизм, используемый в протоколе TLS для фильтрации и управления объектными идентификаторами (Object Identifiers, OID),
				 *          которые представляют собой уникальные идентификаторы для различных объектов и алгоритмов в криптографии и безопасности.
				 *
				 * \~english
				 * @brief TLS extension structure for OID filters (OID Filters)
				 * @details OID Filters is a mechanism used in the TLS protocol to filter and manage object identifiers (Object Identifiers, OID),
				 *          which are unique identifiers of various objects and algorithms in cryptography and security.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_OID_Filters : public extension_t {
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_OID_Filters() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_OID_Filters() = default;
				} extension_oid_filters_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для доверенных якорей (Trust Anchors)
				 *
				 * @details Trust Anchors - это доверенные корневые сертификаты или публичные ключи,
				 *          которые используются для проверки подлинности и доверия к цифровым сертификатам в протоколе TLS.
				 *
				 * \~english
				 * @brief TLS extension structure for trust anchors (Trust Anchors)
				 * @details Trust Anchors are trusted root certificates or public keys
				 *          used to verify the authenticity of and trust in digital certificates in the TLS protocol.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Trust_Anchors : public extension_t {
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Trust_Anchors() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Trust_Anchors() = default;
				} extension_trust_anchors_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для шифрования перед MAC (Encrypt-Then-MAC)
				 *
				 * @details Encrypt-Then-MAC - это механизм, используемый в протоколе TLS для обеспечения целостности и аутентичности данных,
				 *          передаваемых по защищённому каналу связи. Он предполагает, что данные сначала шифруются,
				 *          а затем к ним применяется механизм проверки целостности (MAC - Message Authentication Code).
				 *
				 * \~english
				 * @brief TLS extension structure for encryption before MAC (Encrypt-Then-MAC)
				 * @details Encrypt-Then-MAC is a mechanism used in the TLS protocol to ensure the integrity and authenticity of the data
				 *          transmitted over a secure communication channel. It implies that the data is encrypted first
				 *          and only then an integrity check mechanism (MAC — Message Authentication Code) is applied to it.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Encrypt_Then_MAC : public extension_t {
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Encrypt_Then_MAC() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Encrypt_Then_MAC() = default;
				} extension_encrypt_then_mac_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для информации о прозрачности (Transparency Info)
				 *
				 * @details Transparency Info - это механизм, используемый в протоколе TLS для обеспечения прозрачности и отслеживаемости сертификатов и ключей,
				 *          используемых в процессе установления соединения. Он позволяет клиенту и серверу обмениваться информацией о прозрачности,
				 *          такой как журналы сертификатов и ключей, что способствует повышению доверия и безопасности соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for transparency information (Transparency Info)
				 * @details Transparency Info is a mechanism used in the TLS protocol to ensure the transparency and traceability of the certificates and keys
				 *          used during connection establishment. It allows the client and the server to exchange transparency information,
				 *          such as certificate and key logs, which increases trust in and the security of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Transparency_Info : public extension_t {
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Transparency_Info() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Transparency_Info() = default;
				} extension_transparency_info_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для поддержки аутентификации после рукопожатия (Post-Handshake Authentication)
				 *
				 * @details Post-Handshake Authentication - это механизм, используемый в протоколе TLS для обеспечения дополнительной аутентификации после завершения процесса рукопожатия (handshake).
				 *
				 * \~english
				 * @brief TLS extension structure for post-handshake authentication support (Post-Handshake Authentication)
				 * @details Post-Handshake Authentication is a mechanism used in the TLS protocol to provide additional authentication after the handshake has completed.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Post_Handshake_Auth : public extension_t {
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Post_Handshake_Auth() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Post_Handshake_Auth() = default;
				} extension_post_handshake_auth_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания типа сертификата клиента (Client Certificate Type)
				 *
				 * @details Client Certificate Type - это механизм, используемый в протоколе TLS для указания типа сертификата, который клиент должен предоставить серверу для аутентификации.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the client certificate type (Client Certificate Type)
				 * @details Client Certificate Type is a mechanism used in the TLS protocol to specify the type of certificate the client must present to the server for authentication.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Client_Certificate_Type : public extension_t {
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Client_Certificate_Type() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Client_Certificate_Type() = default;
				} extension_client_certificate_type_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания типа сертификата сервера (Server Certificate Type)
				 *
				 * @details Server Certificate Type - это механизм, используемый в протоколе TLS для указания типа сертификата, который сервер должен предоставить клиенту для аутентификации.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the server certificate type (Server Certificate Type)
				 * @details Server Certificate Type is a mechanism used in the TLS protocol to specify the type of certificate the server must present to the client for authentication.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Server_Certificate_Type : public extension_t {
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Server_Certificate_Type() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Server_Certificate_Type() = default;
				} extension_server_certificate_type_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания имени сервера (SNI)
				 *
				 * @details Server Name Indication (SNI) - это расширение протокола TLS, которое позволяет клиенту указывать имя сервера,
				 *          к которому он пытается подключиться, во время процесса установления соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the server name (SNI)
				 * @details Server Name Indication (SNI) is a TLS protocol extension that allows the client to specify the name of the server
				 *          it is trying to connect to during connection establishment.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Server_Name : public extension_t {
					// Список имён серверов
					vector <string> names;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Server_Name() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Server_Name() = default;
				} extension_server_name_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для запроса статуса сертификата (OCSP)
				 *
				 * @details Certificate Status Request - это расширение протокола TLS, которое позволяет клиенту запрашивать у сервера информацию о статусе сертификата,
				 *          например, с помощью протокола OCSP (Online Certificate Status Protocol). Это расширение позволяет клиенту проверять,
				 *          действителен ли сертификат сервера, и предотвращать использование недействительных или отозванных сертификатов.
				 *
				 * \~english
				 * @brief TLS extension structure for a certificate status request (OCSP)
				 * @details Certificate Status Request is a TLS protocol extension that allows the client to ask the server for information about the certificate status,
				 *          for example by means of the OCSP protocol (Online Certificate Status Protocol). This extension lets the client check
				 *          whether the server certificate is valid and prevents the use of invalid or revoked certificates.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Status_Request : public extension_t {
					// Тип статуса сертификата (например, OCSP)
					string certificateStatusType;
					// Длина списка идентификаторов ответчиков
					uint16_t responderIdListLength;
					// Длина расширений запроса статуса сертификата
					uint16_t requestExtensionsLength;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Status_Request() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Status_Request() = default;
				} extension_status_request_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания поддерживаемых групп (Supported Groups)
				 *
				 * @details Supported Groups - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых группах эллиптических кривых (Elliptic Curve Groups),
				 *          используемых для криптографических операций, таких как обмен ключами и цифровая подпись.
				 *          Это расширение позволяет сторонам согласовывать, какие группы эллиптических кривых они поддерживают,
				 *          и выбирать наиболее подходящую группу для установления безопасного соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the supported groups (Supported Groups)
				 * @details Supported Groups is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the supported elliptic curve groups (Elliptic Curve Groups)
				 *          used for cryptographic operations such as key exchange and digital signing.
				 *          This extension lets the parties agree on which elliptic curve groups they support
				 *          and choose the most suitable group for establishing a secure connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Supported_Groups : public extension_t {
					// Список поддерживаемых групп
					vector <group_t> supportedGroups;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Supported_Groups() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Supported_Groups() = default;
				} extension_supported_groups_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания форматов точек эллиптической кривой (EC Point Formats)
				 *
				 * @details EC Point Formats - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых форматах точек эллиптической кривой (Elliptic Curve Point Formats),
				 *		    используемых для криптографических операций, таких как обмен ключами и цифровая подпись.
				 *          Это расширение позволяет сторонам согласовывать, какие форматы точек они поддерживают,
				 *          и выбирать наиболее подходящий формат для установления безопасного соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the elliptic curve point formats (EC Point Formats)
				 * @details EC Point Formats is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the supported elliptic curve point formats (Elliptic Curve Point Formats)
				 *          used for cryptographic operations such as key exchange and digital signing.
				 *          This extension lets the parties agree on which point formats they support
				 *          and choose the most suitable format for establishing a secure connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_EC_Point : public extension_t {
					// Список поддерживаемых форматов точек эллиптической кривой
					vector <ec_point_format_t> formats;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_EC_Point() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_EC_Point() = default;
				} extension_ec_point_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для согласования протокола прикладного уровня (ALPN)
				 *
				 * @details Application-Layer Protocol Negotiation (ALPN) - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу согласовывать протокол прикладного уровня (например, HTTP/2, HTTP/3) во время процесса установления соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for application-layer protocol negotiation (ALPN)
				 * @details Application-Layer Protocol Negotiation (ALPN) is a TLS protocol extension
				 *          that allows the client and the server to negotiate the application-layer protocol (for example HTTP/2, HTTP/3) during connection establishment.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_ALPN : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_ALPN() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_ALPN() = default;
				} extension_alpn_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для передачи настроек приложения (Application Settings)
				 *
				 * @details Application Settings - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться дополнительными настройками и параметрами, связанными с приложением, во время процесса установления соединения.
				 *          Это расширение может использоваться для передачи информации о поддерживаемых функциях, конфигурации и других аспектах приложения,
				 *          что позволяет сторонам согласовывать и оптимизировать взаимодействие на уровне приложения.
				 *
				 * \~english
				 * @brief TLS extension structure for passing application settings (Application Settings)
				 * @details Application Settings is a TLS protocol extension
				 *          that allows the client and the server to exchange additional application-related settings and parameters during connection establishment.
				 *          This extension can be used to convey information about supported features, configuration and other aspects of the application,
				 *          which lets the parties negotiate and optimise their interaction at the application level.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Application_Settings : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Application_Settings() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Application_Settings() = default;
				} extension_application_settings_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для передачи старых настроек приложения (Application Settings Old)
				 *
				 * @details Application Settings Old - это устаревшее расширение протокола TLS,
				 *          которое использовалось для передачи настроек и параметров приложения во время процесса установления соединения.
				 *          Оно было заменено новым расширением Application Settings, которое обеспечивает более современный и безопасный способ обмена информацией о настройках приложения между клиентом и сервером.
				 *
				 * \~english
				 * @brief TLS extension structure for passing the old application settings (Application Settings Old)
				 * @details Application Settings Old is a deprecated TLS protocol extension
				 *          that was used to pass application settings and parameters during connection establishment.
				 *          It was superseded by the new Application Settings extension, which provides a more modern and secure way of exchanging application settings between the client and the server.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Application_Settings_Old : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Application_Settings_Old() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Application_Settings_Old() = default;
				} extension_application_settings_old_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для согласования следующего протокола (Next Protocol Negotiation)
				 *
				 * @details Next Protocol Negotiation (NPN) - это устаревшее расширение протокола TLS,
				 *          которое позволяло клиенту и серверу согласовывать протокол прикладного уровня (например, HTTP/2) во время процесса установления соединения.
				 *          Оно было заменено новым расширением Application-Layer Protocol Negotiation (ALPN),
				 *          которое обеспечивает более современный и безопасный способ согласования протоколов прикладного уровня между клиентом и сервером.
				 *
				 * \~english
				 * @brief TLS extension structure for next protocol negotiation (Next Protocol Negotiation)
				 * @details Next Protocol Negotiation (NPN) is a deprecated TLS protocol extension
				 *          that allowed the client and the server to negotiate the application-layer protocol (for example HTTP/2) during connection establishment.
				 *          It was superseded by the new Application-Layer Protocol Negotiation (ALPN) extension,
				 *          which provides a more modern and secure way of negotiating application-layer protocols between the client and the server.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Next_Proto_Neg : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Next_Proto_Neg() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Next_Proto_Neg() = default;
				} extension_next_proto_neg_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для передачи информации о подписанных временных метках сертификатов (Signed Certificate Timestamp)
				 *
				 * @details Signed Certificate Timestamp (SCT) - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о подписанных временных метках сертификатов (SCT),
				 *          используемых в механизме Certificate Transparency (CT).
				 *          SCT обеспечивает дополнительный уровень безопасности и прозрачности для сертификатов, позволяя клиенту проверять,
				 *          были ли сертификаты зарегистрированы в публичных журналах CT и не были ли они отозваны или скомпрометированы.
				 *
				 * \~english
				 * @brief TLS extension structure for passing signed certificate timestamps (Signed Certificate Timestamp)
				 * @details Signed Certificate Timestamp (SCT) is a TLS protocol extension
				 *          that allows the client and the server to exchange information about signed certificate timestamps (SCT)
				 *          used in the Certificate Transparency (CT) mechanism.
				 *          SCT provides an additional level of security and transparency for certificates by letting the client check
				 *          whether the certificates were registered in public CT logs and whether they were revoked or compromised.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Signed_Certificate_Timestamp : public extension_t {
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Signed_Certificate_Timestamp() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Signed_Certificate_Timestamp() = default;
				} extension_signed_certificate_timestamp_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для добавления произвольного количества байтов заполнения (Padding)
				 *
				 * @details Padding - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу добавлять произвольное количество байтов заполнения (padding) в процессе установления соединения.
				 *          Это расширение может использоваться для увеличения длины сообщений,
				 *          чтобы скрыть фактическую длину передаваемых данных и предотвратить определённые типы атак, такие как атаки на основе анализа длины сообщений.
				 *          Добавление байтов заполнения может улучшить безопасность соединения, делая его более устойчивым к анализу трафика и попыткам выявления передаваемой информации.
				 *
				 * \~english
				 * @brief TLS extension structure for adding an arbitrary number of padding bytes (Padding)
				 * @details Padding is a TLS protocol extension
				 *          that allows the client and the server to add an arbitrary number of padding bytes during connection establishment.
				 *          This extension can be used to increase the length of messages
				 *          in order to hide the actual length of the transmitted data and prevent certain types of attacks, such as message-length analysis attacks.
				 *          Adding padding bytes can improve the security of the connection, making it more resistant to traffic analysis and attempts to reveal the transmitted information.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Padding : public extension_t {
					// Размер данных заполнения
					size_t size;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Padding() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Padding() = default;
				} extension_padding_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для использования расширенного мастер-секрета (Extended Master Secret)
				 *
				 * @details Extended Master Secret (EMS) - это расширение протокола TLS,
				 *          которое обеспечивает дополнительный уровень безопасности для процесса установления соединения.
				 *          Оно предотвращает определённые типы атак, такие как атаки на основе повторного использования мастер-секрета (replay attacks) и атаки на основе анализа длины сообщений (length extension attacks).
				 *          EMS гарантирует, что мастер-секрет, используемый для генерации ключей шифрования, зависит от всех параметров сеанса TLS,
				 *          включая идентификаторы клиента и сервера, что делает его уникальным для каждого сеанса и предотвращает возможность повторного использования мастер-секрета в других сеансах.
				 *
				 * \~english
				 * @brief TLS extension structure for using the extended master secret (Extended Master Secret)
				 * @details Extended Master Secret (EMS) is a TLS protocol extension
				 *          that provides an additional level of security for connection establishment.
				 *          It prevents certain types of attacks, such as master secret replay attacks and length extension attacks.
				 *          EMS guarantees that the master secret used to derive the encryption keys depends on all parameters of the TLS session,
				 *          including the client and server identifiers, which makes it unique for every session and rules out the reuse of the master secret in other sessions.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Extended_Master_Secret : public extension_t {
					// Данные расширения Master Secret
					string masterSecretData;
					// Данные расширения Extended Master Secret для сервера (устаревшее расширение)
					string extendedMasterSecretData;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Extended_Master_Secret() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Extended_Master_Secret() = default;
				} extension_extended_master_secret_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для сжатия сертификатов (Compress Certificate)
				 *
				 * @details Compress Certificate - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу использовать алгоритмы сжатия для уменьшения размера передаваемых сертификатов во время процесса установления соединения.
				 *          Это расширение может улучшить производительность и снизить задержку при передаче сертификатов, особенно в случаях, когда сертификаты имеют большой размер или когда соединение осуществляется через медленные сети.
				 *          Использование сжатия сертификатов может также снизить потребление полосы пропускания и улучшить общую эффективность процесса установления соединения TLS.
				 *
				 * \~english
				 * @brief TLS extension structure for certificate compression (Compress Certificate)
				 * @details Compress Certificate is a TLS protocol extension
				 *          that allows the client and the server to use compression algorithms to reduce the size of the certificates transmitted during connection establishment.
				 *          This extension can improve performance and reduce latency when transmitting certificates, especially when the certificates are large or the connection runs over slow networks.
				 *          Certificate compression can also reduce bandwidth consumption and improve the overall efficiency of TLS connection establishment.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Compress_Certificate : public extension_t {
					// Список поддерживаемых алгоритмов сжатия сертификатов
					vector <compressor_t> algorithms;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Compress_Certificate() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Compress_Certificate() = default;
				} extension_compress_certificate_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для использования билета сессии (Session Ticket)
				 *
				 * @details Session Ticket - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу использовать билеты сессии для восстановления ранее установленных соединений без необходимости повторного выполнения полного процесса рукопожатия (handshake).
				 *
				 * \~english
				 * @brief TLS extension structure for using a session ticket (Session Ticket)
				 * @details Session Ticket is a TLS protocol extension
				 *          that allows the client and the server to use session tickets to resume previously established connections without performing the full handshake again.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Session_Ticket : public extension_t {
					// Данные расширения Session Ticket
					vector <uint8_t> data;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Session_Ticket() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Session_Ticket() = default;
				} extension_session_ticket_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания поддерживаемых версий (Supported Versions)
				 *
				 * @details Supported Versions - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых версиях протокола TLS во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какую версию протокола TLS они будут использовать для установления безопасного соединения, обеспечивая совместимость и улучшая безопасность соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the supported versions (Supported Versions)
				 * @details Supported Versions is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the supported versions of the TLS protocol during connection establishment.
				 *          This extension lets the parties agree on which version of the TLS protocol they will use to establish a secure connection, ensuring compatibility and improving the security of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Supported_Versions : public extension_t {
					// Список поддерживаемых версий
					vector <version_t> versions;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Supported_Versions() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Supported_Versions() = default;
				} extension_supported_versions_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания режимов обмена ключами PSK (PSK Key Exchange Modes)
				 *
				 * @details PSK Key Exchange Modes - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых режимах обмена ключами с использованием предварительно совместного ключа (Pre-Shared Key, PSK) во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какой режим обмена ключами PSK они будут использовать для установления безопасного соединения, обеспечивая совместимость и улучшая безопасность соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the PSK key exchange modes (PSK Key Exchange Modes)
				 * @details PSK Key Exchange Modes is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the supported key exchange modes using a pre-shared key (Pre-Shared Key, PSK) during connection establishment.
				 *          This extension lets the parties agree on which PSK key exchange mode they will use to establish a secure connection, ensuring compatibility and improving the security of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_PSK_Key_Exchange : public extension_t {
					// Список поддерживаемых версий
					vector <psk_key_t> modes;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_PSK_Key_Exchange() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_PSK_Key_Exchange() = default;
				} extension_psk_key_exchange_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для поддержки ранних данных (Early Data)
				 *
				 * @details Early Data - это расширение протокола TLS,
				 *          которое позволяет клиенту отправлять данные на сервер до завершения процесса установления соединения (handshake).
				 *          Это расширение может улучшить производительность и снизить задержку при установлении соединения, особенно в случаях,
				 *          когда клиент и сервер уже имеют предварительно совместный ключ (Pre-Shared Key, PSK) и могут безопасно обмениваться данными без необходимости повторного выполнения полного процесса рукопожатия.
				 *          Использование ранних данных может также снизить потребление полосы пропускания и улучшить общую эффективность процесса установления соединения TLS.
				 *
				 * \~english
				 * @brief TLS extension structure for early data support (Early Data)
				 * @details Early Data is a TLS protocol extension
				 *          that allows the client to send data to the server before the handshake has completed.
				 *          This extension can improve performance and reduce connection establishment latency, especially when
				 *          the client and the server already share a pre-shared key (Pre-Shared Key, PSK) and can exchange data securely without performing the full handshake again.
				 *          Using early data can also reduce bandwidth consumption and improve the overall efficiency of TLS connection establishment.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Early_Data : public extension_t {
					// Максимальный размер ранних данных
					uint32_t maxSize;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Early_Data() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Early_Data() = default;
				} extension_early_data_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для обмена ключами (Key Share)
				 *
				 * @details Key Share - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о ключах для установления безопасного соединения с использованием алгоритмов обмена ключами,
				 *          таких как Диффи-Хеллман (Diffie-Hellman) или эллиптические кривые (Elliptic Curve Cryptography, ECC).
				 *          Это расширение позволяет сторонам согласовывать, какие ключи они будут использовать для установления безопасного соединения,
				 *          обеспечивая совместимость и улучшая безопасность соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for key exchange (Key Share)
				 * @details Key Share is a TLS protocol extension
				 *          that allows the client and the server to exchange key information in order to establish a secure connection using key exchange algorithms
				 *          such as Diffie-Hellman or elliptic curves (Elliptic Curve Cryptography, ECC).
				 *          This extension lets the parties agree on which keys they will use to establish a secure connection,
				 *          ensuring compatibility and improving the security of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Key_Share : public extension_t {
					// Список групп обмена ключами в порядке появления в ClientHello (порядок важен — отражает предпочтение клиента)
					vector <pair <group_t, vector <uint8_t>>> shares;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Key_Share() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Key_Share() = default;
				} extension_key_share_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для передачи зашифрованного ClientHello (Encrypted ClientHello)
				 *
				 * @details Encrypted ClientHello - это расширение протокола TLS,
				 *          которое позволяет клиенту отправлять зашифрованный ClientHello на сервер для обеспечения дополнительной конфиденциальности и защиты от атак на основе анализа трафика.
				 *
				 * \~english
				 * @brief TLS extension structure for passing an encrypted ClientHello (Encrypted ClientHello)
				 * @details Encrypted ClientHello is a TLS protocol extension
				 *          that allows the client to send an encrypted ClientHello to the server for additional confidentiality and protection against traffic analysis attacks.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Encryption_Client_Hello : public extension_t {
					// Данные зашифрованного ClientHello
					vector <uint8_t> data;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Encryption_Client_Hello() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Encryption_Client_Hello() = default;
				} extension_encryption_client_hello_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для передачи информации о повторной договоренности (Renegotiation Info)
				 *
				 * @details Renegotiation Info - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о повторной договоренности (renegotiation) во время процесса установления соединения.
				 *          Это расширение обеспечивает защиту от атак на основе повторной договоренности,
				 *          таких как атаки на основе повторного использования сеансов (session replay attacks) и атаки на основе анализа трафика (traffic analysis attacks).
				 *          Использование информации о повторной договоренности позволяет сторонам согласовывать,
				 *          какие параметры сеанса они будут использовать для установления безопасного соединения, обеспечивая совместимость и улучшая безопасность соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for passing renegotiation information (Renegotiation Info)
				 * @details Renegotiation Info is a TLS protocol extension
				 *          that allows the client and the server to exchange renegotiation information during connection establishment.
				 *          This extension provides protection against renegotiation-based attacks,
				 *          such as session replay attacks and traffic analysis attacks.
				 *          Using renegotiation information lets the parties agree on
				 *          which session parameters they will use to establish a secure connection, ensuring compatibility and improving the security of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Renegotiation_Info : public extension_t {
					// Данные зашифрованного ClientHello
					vector <uint8_t> data;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Renegotiation_Info() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Renegotiation_Info() = default;
				} extension_renegotiation_info_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания максимального размера записи (Record Size Limit)
				 *
				 * @details Record Size Limit - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о максимальном размере записи (record size limit) во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какой максимальный размер записи они будут использовать для установления безопасного соединения,
				 *          обеспечивая совместимость и улучшая безопасность соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the maximum record size (Record Size Limit)
				 * @details Record Size Limit is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the maximum record size during connection establishment.
				 *          This extension lets the parties agree on which maximum record size they will use to establish a secure connection,
				 *          ensuring compatibility and improving the security of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Record_Size_Limit : public extension_t {
					// Данные расширения Record Size Limit
					uint16_t data;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Record_Size_Limit() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Record_Size_Limit() = default;
				} extension_record_size_limit_t;

				/**
				 * \~russian
				 * @brief Структура расширения DTLS для передачи cookie (Cookie)
				 *
				 * @details Cookie - это расширение протокола DTLS (Datagram Transport Layer Security),
				 *          которое позволяет клиенту и серверу обмениваться cookie во время процесса установления соединения.
				 *          Cookie используется для защиты от атак на основе подделки IP-адресов (IP address spoofing) и для предотвращения атак на основе повторного использования сеансов (session replay attacks).
				 *          Использование cookie позволяет серверу проверять подлинность клиента и предотвращать установление соединений с недействительными или поддельными клиентами, обеспечивая безопасность и целостность процесса установления соединения DTLS.
				 *
				 * \~english
				 * @brief DTLS extension structure for passing a cookie (Cookie)
				 * @details Cookie is an extension of the DTLS protocol (Datagram Transport Layer Security)
				 *          that allows the client and the server to exchange a cookie during connection establishment.
				 *          The cookie is used to protect against IP address spoofing attacks and to prevent session replay attacks.
				 *          Using a cookie lets the server verify the authenticity of the client and prevent connections from invalid or forged clients, ensuring the security and integrity of DTLS connection establishment.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Cookie : public extension_t {
					// Данные расширения Cookie
					vector <uint8_t> data;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Cookie() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Cookie() = default;
				} extension_cookie_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для использования предварительно совместного ключа (Pre-Shared Key)
				 *
				 * @details Pre-Shared Key (PSK) - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу использовать предварительно совместный ключ (PSK) для установления безопасного соединения.
				 *          PSK обеспечивает дополнительный уровень безопасности и конфиденциальности,
				 *          позволяя сторонам обмениваться ключами и данными без необходимости использования сертификатов и инфраструктуры открытых ключей (Public Key Infrastructure, PKI).
				 *          Использование PSK может улучшить производительность и снизить задержку при установлении соединения, особенно в случаях,
				 *          когда клиент и сервер уже имеют предварительно совместный ключ и могут безопасно обмениваться данными без необходимости повторного выполнения полного процесса рукопожатия (handshake).
				 *
				 * \~english
				 * @brief TLS extension structure for using a pre-shared key (Pre-Shared Key)
				 * @details Pre-Shared Key (PSK) is a TLS protocol extension
				 *          that allows the client and the server to use a pre-shared key (PSK) to establish a secure connection.
				 *          PSK provides an additional level of security and confidentiality
				 *          by letting the parties exchange keys and data without using certificates and a public key infrastructure (Public Key Infrastructure, PKI).
				 *          Using PSK can improve performance and reduce connection establishment latency, especially when
				 *          the client and the server already share a pre-shared key and can exchange data securely without performing the full handshake again.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Pre_Shared_Key : public extension_t {
					/**
					 * \~russian
					 * @brief Структура идентификатора предварительно совместного ключа (PSK Identity)
					 *
					 * \~english
					 * @brief Pre-shared key identifier structure (PSK Identity)
					 *
					 * \~
					 */
					struct __AWH_SHARED_EXPORT__ Identity {
						// Время жизни билета (Ticket Age)
						uint32_t ticketAge;
						// Идентификатор предварительно совместного ключа
						vector <uint8_t> data;
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Identity() noexcept;
					};
					// Список идентификаторов предварительно совместных ключей
					vector <Identity> identities;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Pre_Shared_Key() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Pre_Shared_Key() = default;
				} extension_pre_shared_key_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания доверенных центров сертификации (Certificate Authorities)
				 *
				 * @details Certificate Authorities (CAs) - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о доверенных центрах сертификации (Certificate Authorities, CAs) во время процесса установления соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the trusted certificate authorities (Certificate Authorities)
				 * @details Certificate Authorities (CAs) is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the trusted certificate authorities (Certificate Authorities, CAs) during connection establishment.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Certificate_Authorities : public extension_t {
					// Список DER-кодированных Distinguished Name (DistinguishedName) из RFC 8446 §4.2.4
					vector <vector <uint8_t>> authorities;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Certificate_Authorities() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Certificate_Authorities() = default;
				} extension_certificate_authorities_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания максимальной длины фрагмента (Max Fragment Length)
				 *
				 * @details Max Fragment Length - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о максимальной длине фрагмента (max fragment length) во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какой максимальный размер фрагмента они будут использовать для установления безопасного соединения,
				 *          обеспечивая совместимость и улучшая безопасность соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the maximum fragment length (Max Fragment Length)
				 * @details Max Fragment Length is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the maximum fragment length during connection establishment.
				 *          This extension lets the parties agree on which maximum fragment size they will use to establish a secure connection,
				 *          ensuring compatibility and improving the security of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Max_Fragment_Length : public extension_t {
					// Максимальная длина фрагмента
					uint16_t length;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Max_Fragment_Length() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Max_Fragment_Length() = default;
				} extension_max_fragment_length_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для использования SRTP (Use SRTP)
				 *
				 * @details Use SRTP - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых профилях Secure Real-time Transport Protocol (SRTP) во время процесса установления соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for using SRTP (Use SRTP)
				 * @details Use SRTP is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the supported Secure Real-time Transport Protocol (SRTP) profiles during connection establishment.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Use_SRTP : public extension_t {
					// Длина Master Key Identifier (MKI)
					uint8_t mkiLength;
					// Список поддерживаемых профилей SRTP
					vector <srtp_t> profiles;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Use_SRTP() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Use_SRTP() = default;
				} extension_use_srtp_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для поддержки механизма heartbeat (Heartbeat)
				 *
				 * @details Heartbeat - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддержке механизма heartbeat во время процесса установления соединения.
				 *          Механизм heartbeat используется для проверки доступности и поддержания соединения между клиентом и сервером,
				 *          а также для обнаружения потери соединения. Использование heartbeat может улучшить производительность и надежность соединения,
				 *          позволяя сторонам своевременно обнаруживать проблемы с соединением и предпринимать соответствующие действия для их устранения,
				 *          обеспечивая стабильность и качество обслуживания в сетевых приложениях.
				 *
				 * \~english
				 * @brief TLS extension structure for heartbeat support (Heartbeat)
				 * @details Heartbeat is a TLS protocol extension
				 *          that allows the client and the server to exchange information about heartbeat support during connection establishment.
				 *          The heartbeat mechanism is used to check availability and keep the connection between the client and the server alive,
				 *          as well as to detect connection loss. Using heartbeat can improve the performance and reliability of the connection
				 *          by letting the parties detect connection problems in time and take appropriate action to resolve them,
				 *          ensuring stability and quality of service in network applications.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Heartbeat : public extension_t {
					// Режим heartbeat (например, peer_allowed_to_send или peer_not_allowed_to_send)
					heartbeat_t mode;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Heartbeat() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Heartbeat() = default;
				} extension_heartbeat_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания поддерживаемых алгоритмов подписи (Signature Algorithms)
				 *
				 * @details Signature Algorithms - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых алгоритмах подписи во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какие алгоритмы подписи они будут использовать для установления безопасного соединения,
				 *          обеспечивая совместимость и улучшая безопасность соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the supported signature algorithms (Signature Algorithms)
				 * @details Signature Algorithms is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the supported signature algorithms during connection establishment.
				 *          This extension lets the parties agree on which signature algorithms they will use to establish a secure connection,
				 *          ensuring compatibility and improving the security of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Signature : public extension_t {
					// Список поддерживаемых алгоритмов подписи
					vector <signature_t> algorithms;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Signature() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Signature() = default;
				} extension_signature_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для использования делегированных учетных данных (Delegated Credential)
				 *
				 * @details Delegated Credential - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых алгоритмах подписи для делегированных учетных данных во время процесса установления соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for using delegated credentials (Delegated Credential)
				 * @details Delegated Credential is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the supported signature algorithms for delegated credentials during connection establishment.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Delegated_Credential : public extension_t {
					// Список поддерживаемых алгоритмов подписи для делегированных учетных данных
					vector <signature_t> algorithms;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Delegated_Credential() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Delegated_Credential() = default;
				} extension_delegated_credential_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для указания поддерживаемых алгоритмов подписи для сертификатов (Signature Algorithms for Certificates)
				 *
				 * @details Signature Algorithms for Certificates - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых алгоритмах подписи для сертификатов во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какие алгоритмы подписи они будут использовать для проверки подлинности сертификатов, обеспечивая совместимость и улучшая безопасность соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for specifying the supported signature algorithms for certificates (Signature Algorithms for Certificates)
				 * @details Signature Algorithms for Certificates is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the supported signature algorithms for certificates during connection establishment.
				 *          This extension lets the parties agree on which signature algorithms they will use to verify the authenticity of certificates, ensuring compatibility and improving the security of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Signature_Algorithms_Cert : public extension_t {
					// Список поддерживаемых алгоритмов подписи для сертификатов
					vector <signature_t> algorithms;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Signature_Algorithms_Cert() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Signature_Algorithms_Cert() = default;
				} extension_signature_algorithms_cert_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для передачи информации о прозрачности (Transparency Info)
				 *
				 * @details Transparency Info - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о прозрачности (transparency) во время процесса установления соединения.
				 *          Это расширение может использоваться для передачи информации о поддерживаемых механизмах прозрачности,
				 *          таких как Certificate Transparency (CT), позволяя сторонам проверять подлинность сертификатов и обеспечивать дополнительный уровень безопасности и доверия при установлении соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for passing transparency information (Transparency Info)
				 * @details Transparency Info is a TLS protocol extension
				 *          that allows the client and the server to exchange transparency information during connection establishment.
				 *          This extension can be used to convey information about the supported transparency mechanisms,
				 *          such as Certificate Transparency (CT), letting the parties verify the authenticity of certificates and providing an additional level of security and trust during connection establishment.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_TLS_Flags : public extension_t {
					// Список флагов или параметров, специфичных для реализации TLS
					vector <uint8_t> flags;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_TLS_Flags() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_TLS_Flags() = default;
				} extension_tls_flags_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для передачи параметров транспорта QUIC (QUIC Transport Parameters)
				 *
				 * @details QUIC Transport Parameters - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о параметрах транспорта QUIC во время процесса установления соединения.
				 *          Эти параметры могут включать информацию о поддерживаемых версиях QUIC, максимальных размерах пакетов, тайм-аутах и других настройках,
				 *          которые могут быть согласованы между клиентом и сервером для оптимизации работы протокола QUIC.
				 *
				 * \~english
				 * @brief TLS extension structure for passing the QUIC transport parameters (QUIC Transport Parameters)
				 * @details QUIC Transport Parameters is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the QUIC transport parameters during connection establishment.
				 *          These parameters can include information about the supported QUIC versions, maximum packet sizes, timeouts and other settings
				 *          that can be negotiated between the client and the server to optimise the operation of the QUIC protocol.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Quic_Transport_Params : public extension_t {
					/**
					 * \~russian
					 * @brief Список параметров транспорта QUIC в виде пар ключ-значение,
					 *        где ключ и значение представлены в виде 64-битных целых чисел.
					 *        Эти параметры могут включать информацию о поддерживаемых версиях QUIC,
					 *        максимальных размерах пакетов, тайм-аутах и других настройках,
					 *        которые могут быть согласованы между клиентом и сервером во время TLS-рукопожатия для оптимизации работы протокола QUIC.
					 *
					 * \~english
					 * @brief The list of QUIC transport parameters as key-value pairs,
					 *        where both the key and the value are represented as 64-bit integers.
					 *        These parameters can include information about the supported QUIC versions,
					 *        maximum packet sizes, timeouts and other settings
					 *        that can be negotiated between the client and the server during the TLS handshake to optimise the operation of the QUIC protocol.
					 *
					 * \~
					 */
					unordered_map <uint64_t, uint64_t> params;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Quic_Transport_Params() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Quic_Transport_Params() = default;
				} extension_quic_transport_params_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для передачи параметров транспорта QUIC (QUIC Transport Parameters Legacy)
				 *
				 * @details QUIC Transport Parameters Legacy - это устаревшее расширение протокола TLS,
				 *          которое позволяло клиенту и серверу обмениваться информацией о параметрах транспорта QUIC во время процесса установления соединения.
				 *          Эти параметры могут включать информацию о поддерживаемых версиях QUIC, максимальных размерах пакетов,
				 *          тайм-аутах и других настройках, которые могли быть согласованы между клиентом и сервером для оптимизации работы протокола QUIC.
				 *          Это расширение было заменено новым расширением QUIC Transport Parameters,
				 *          которое обеспечивает более современный и безопасный способ обмена информацией о параметрах транспорта QUIC между клиентом и сервером.
				 *
				 * \~english
				 * @brief TLS extension structure for passing the QUIC transport parameters (QUIC Transport Parameters Legacy)
				 * @details QUIC Transport Parameters Legacy is a deprecated TLS protocol extension
				 *          that allowed the client and the server to exchange information about the QUIC transport parameters during connection establishment.
				 *          These parameters can include information about the supported QUIC versions, maximum packet sizes,
				 *          timeouts and other settings that could be negotiated between the client and the server to optimise the operation of the QUIC protocol.
				 *          This extension was superseded by the new QUIC Transport Parameters extension,
				 *          which provides a more modern and secure way of exchanging QUIC transport parameters between the client and the server.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Quic_Transport_Params_Legacy : public extension_t {
					/**
					 * \~russian
					 * @brief Список параметров транспорта QUIC в виде пар ключ-значение,
					 *        где ключ и значение представлены в виде 64-битных целых чисел.
					 *        Эти параметры могут включать информацию о поддерживаемых версиях QUIC,
					 *        максимальных размерах пакетов, тайм-аутах и других настройках,
					 *        которые могут быть согласованы между клиентом и сервером во время TLS-рукопожатия для оптимизации работы протокола QUIC.
					 *
					 * \~english
					 * @brief The list of QUIC transport parameters as key-value pairs,
					 *        where both the key and the value are represented as 64-bit integers.
					 *        These parameters can include information about the supported QUIC versions,
					 *        maximum packet sizes, timeouts and other settings
					 *        that can be negotiated between the client and the server during the TLS handshake to optimise the operation of the QUIC protocol.
					 *
					 * \~
					 */
					unordered_map <uint64_t, uint64_t> params;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_Quic_Transport_Params_Legacy() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_Quic_Transport_Params_Legacy() = default;
				} extension_quic_transport_params_legacy_t;

				/**
				 * \~russian
				 * @brief Структура расширения TLS для передачи внешних расширений ECH (ECH Outer Extensions)
				 *
				 * @details ECH Outer Extensions - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о внешних расширениях Encrypted ClientHello (ECH) во время процесса установления соединения.
				 *          Эти расширения могут включать информацию о поддерживаемых версиях ECH, алгоритмах шифрования, параметрах обмена ключами и других настройках,
				 *          которые могут быть согласованы между клиентом и сервером для обеспечения конфиденциальности и защиты от атак на основе анализа трафика.
				 *          Использование внешних расширений ECH позволяет сторонам согласовывать, какие расширения они будут использовать для установления безопасного соединения с использованием ECH,
				 *          обеспечивая совместимость и улучшая безопасность соединения.
				 *
				 * \~english
				 * @brief TLS extension structure for passing the ECH outer extensions (ECH Outer Extensions)
				 * @details ECH Outer Extensions is a TLS protocol extension
				 *          that allows the client and the server to exchange information about the Encrypted ClientHello (ECH) outer extensions during connection establishment.
				 *          These extensions can include information about the supported ECH versions, encryption algorithms, key exchange parameters and other settings
				 *          that can be negotiated between the client and the server to ensure confidentiality and protection against traffic analysis attacks.
				 *          Using the ECH outer extensions lets the parties agree on which extensions they will use to establish a secure connection with ECH,
				 *          ensuring compatibility and improving the security of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_ECH_Outer_Extensions : public extension_t {
					// Список расширений, которые были зашифрованы в рамках ECH и переданы во внешнем расширении ECH Outer Extensions
					vector <extension_type_t> extensions;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Extension_ECH_Outer_Extensions() noexcept;
					/**
					 * \~russian
					 * @brief Деструктор
					 *
					 *
					 * \~english
					 * @brief Destructor
					 *
					 * \~
					 */
					virtual ~Extension_ECH_Outer_Extensions() = default;
				} extension_ech_outer_extensions_t;
			public:
				/**
				 * \~russian
				 * @brief Структура версии TLS
				 *
				 * @details Структура содержит информацию о версии протокола TLS,
				 *          используемой на уровне записи (record layer) и согласованной версии TLS,
				 *          которая была выбрана в процессе рукопожатия.
				 *
				 * \~english
				 * @brief TLS version structure
				 * @details The structure holds information about the version of the TLS protocol
				 *          used at the record layer and about the negotiated TLS version
				 *          that was chosen during the handshake.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ VersionTLS {
					// Версия TLS на уровне записи (wire-код в десятичном виде, например "769")
					string record;
					// Согласованная версия TLS (наибольшая не-GREASE версия из supported_versions, десятично)
					string negotiated;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit VersionTLS() noexcept;
				} version_tls_t;

				/**
				 * \~russian
				 * @brief Структура вычисленных цифровых отпечатков TLS-соединения (JA3, JA4, PeetPrint и пр.)
				 *
				 * @details Структура содержит информацию о вычисленных цифровых отпечатках TLS-соединения,
				 *          таких как JA3, JA4, PeetPrint и другие, которые используются для идентификации и анализа TLS-соединений.
				 *          Эти отпечатки позволяют определить характеристики TLS-соединения, включая поддерживаемые версии протокола,
				 *          шифры, расширения и другие параметры, что может быть полезно для анализа безопасности, мониторинга трафика и выявления аномалий в сетевых соединениях.
				 *
				 * \~english
				 * @brief Structure of the computed fingerprints of a TLS connection (JA3, JA4, PeetPrint and others)
				 * @details The structure holds information about the computed fingerprints of a TLS connection,
				 *          such as JA3, JA4, PeetPrint and others, which are used to identify and analyse TLS connections.
				 *          These fingerprints make it possible to determine the characteristics of a TLS connection, including the supported protocol versions,
				 *          ciphers, extensions and other parameters, which can be useful for security analysis, traffic monitoring and detecting anomalies in network connections.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Imprint {
					// JA3 строка: "{sslVersion},{ciphers},{extensions},{groups},{ec_point_formats}"
					string ja3;
					// JA4 строка: "{prefix}_{md5_ciphers[:12]}_{md5_sigalgs[:12]}"
					string ja4;
					// JA4_r — расширенная форма JA4 с раскрытыми компонентами
					string ja4r;
					// MD5 хеш JA3 строки (lowercase hex, 32 символа)
					string ja3Hash;
					// Session ID — hex lowercase (пустая строка если отсутствует)
					string sessionId;
					// PeetPrint строка (8 секций, разделённых '|')
					string peetprint;
					// MD5 хеш PeetPrint строки (lowercase hex, 32 символа)
					string peetprintHash;
					// ClientHello.random — 32 байта, hex lowercase
					string clientRandom;
					// Akamai HTTP/2 fingerprint: "{settings}|{windowUpdate}|{priorities}|{pseudoHeaders}" (пусто если HTTP/2 данные не предоставлены)
					string akamai;
					// Версия TLS в расширении supported_versions (наибольшая не-GREASE версия, десятично)
					version_tls_t tls;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Imprint() noexcept;
				} imprint_t;
			public:
				/**
				 * \~russian
				 * @brief Структура записи TLS
				 *
				 * @details Структура содержит информацию о записи TLS, включая эпоху записи DTLS, длину записи TLS,
				 *          порядковый номер записи, версию протокола TLS, поддерживаемую браузером,
				 *          и другие параметры, которые используются для идентификации и анализа TLS-соединений.
				 *
				 * \~english
				 * @brief TLS record structure
				 * @details The structure holds information about a TLS record, including the DTLS record epoch, the TLS record length,
				 *          the record sequence number, the version of the TLS protocol supported by the browser,
				 *          and other parameters used to identify and analyse TLS connections.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Record {
					// Эпоха записи DTLS
					uint16_t epoch;
					// Длина записи TLS
					uint16_t length;
					// Порядковый номер записи
					uint64_t sequence;
					// Версия протокола TLS, поддерживаемая браузером
					version_t version;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Record() noexcept;
				} __attribute__((packed)) record_t;

				/**
				 * \~russian
				 * @brief Структура фрагмента TLS
				 *
				 * @details Структура содержит информацию о фрагменте TLS,
				 *          включая смещение фрагмента в рамках записи TLS и длину фрагмента TLS.
				 *
				 * \~english
				 * @brief TLS fragment structure
				 * @details The structure holds information about a TLS fragment,
				 *          including the offset of the fragment within the TLS record and the length of the TLS fragment.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Fragment {
					// Смещение фрагмента в рамках записи TLS
					uint32_t offset;
					// Длина фрагмента TLS
					uint32_t length;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Fragment() noexcept;
				} __attribute__((packed)) fragment_t;

				/**
				 * \~russian
				 * @brief Структура рукопожатия TLS
				 *
				 * @details Структура содержит информацию о рукопожатии TLS,
				 *          включая длину рукопожатия, порядковый номер рукопожатия и фрагмент рукопожатия TLS,
				 *          которые используются для идентификации и анализа TLS-соединений.
				 *
				 * \~english
				 * @brief TLS handshake structure
				 * @details The structure holds information about a TLS handshake,
				 *          including the handshake length, the handshake sequence number and the TLS handshake fragment,
				 *          which are used to identify and analyse TLS connections.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Handshake {
					// Длина рукопожатия блока рукопожатия TLS
					uint32_t length;
					// Порядковый номер рукопожатия TLS
					uint16_t sequence;
					// Фрагмент рукопожатия TLS
					fragment_t fragment;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Handshake() noexcept;
				} __attribute__((packed)) handshake_t;

				/**
				 * \~russian
				 * @brief Структура ClientHello TLS
				 *
				 * @details Структура содержит информацию о ClientHello TLS,
				 *          включая версию протокола TLS, поддерживаемую браузером в рукопожатии,
				 *          и случайные байты, которые используются для идентификации и анализа TLS-соединений.
				 *
				 * \~english
				 * @brief TLS ClientHello structure
				 * @details The structure holds information about the TLS ClientHello,
				 *          including the version of the TLS protocol supported by the browser in the handshake,
				 *          and the random bytes used to identify and analyse TLS connections.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ ClientHello {
					// Версия протокола TLS, поддерживаемая браузером в рукопожатии
					version_t version;
					// 32 байта случайных байта в ClientHello
					array <uint8_t, 32> random;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit ClientHello() noexcept;
				} client_hello_t;

				/**
				 * \~russian
				 * @brief Класс цифрового отпечатка браузера
				 *
				 * @details Класс содержит информацию о цифровом отпечатке браузера,
				 *          включая флаг использования GREASE, запись метаданных TLS рукопожатия, объект рукопожатия TLS,
				 *          объект ClientHello TLS, куки рукопожатия DTLS, идентификатор сессии TLS, список поддерживаемых шифров,
				 *          список компрессоров, поддерживаемых браузером, и список расширений, поддерживаемых браузером.
				 *          Этот класс используется для идентификации и анализа цифровых отпечатков браузеров,
				 *          что может быть полезно для анализа безопасности, мониторинга трафика и выявления аномалий в сетевых соединениях.
				 *
				 * \~english
				 * @brief Browser fingerprint class
				 * @details The class holds information about a browser fingerprint,
				 *          including the GREASE usage flag, the TLS handshake metadata record, the TLS handshake object,
				 *          the TLS ClientHello object, the DTLS handshake cookie, the TLS session identifier, the list of supported ciphers,
				 *          the list of compressors supported by the browser and the list of extensions supported by the browser.
				 *          This class is used to identify and analyse browser fingerprints,
				 *          which can be useful for security analysis, traffic monitoring and detecting anomalies in network connections.
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Browser {
					public:
						// Флаг, указывающий на использование GREASE (Generate Random Extensions And Sustain Extensibility)
						bool grease;
						// Запись метаданных TLS рукопожатия
						record_t record;
						// Объект рукопожатия TLS
						handshake_t handshake;
						// Объект ClientHello TLS
						client_hello_t clientHello;
						// Куки рукопожатия DTLS
						vector <uint8_t> cookie;
						// Идентификатор сессии TLS
						vector <uint8_t> session;
						// Список поддерживаемых шифров
						vector <cipher_t> ciphers;
						// Список компрессоров, поддерживаемых браузером
						vector <compressor_t> compressors;
						// Список расширений, поддерживаемых браузером
						vector <unique_ptr <extension_t>> extensions;
					public:
						/**
						 * \~russian
						 * @brief Оператор сравнения двух отпечатков браузеров
						 *
						 * @param browser объект цифрового отпечатка браузера для сравнения
						 * @return        результат сравнения
						 *
						 * \~english
						 * @brief Comparison operator of two browser fingerprints
						 * @param browser browser fingerprint object to compare with
						 * @return        comparison result
						 *
						 * \~
						 */
						bool operator == (const Browser & browser) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор перемещения
						 *
						 * @param browser объект цифрового отпечатка браузера для перемещения
						 * @return        текущий объект после перемещения
						 *
						 * \~english
						 * @brief Move operator
						 * @param browser browser fingerprint object to move
						 * @return        the current object after the move
						 *
						 * \~
						 */
						Browser & operator = (Browser && browser) noexcept;
						/**
						 * \~russian
						 * @brief Оператор копирования
						 *
						 * @param browser объект цифрового отпечатка браузера для копирования
						 * @return        текущий объект после копирования
						 *
						 * \~english
						 * @brief Copy operator
						 * @param browser browser fingerprint object to copy
						 * @return        the current object after the copy
						 *
						 * \~
						 */
						Browser & operator = (const Browser & browser) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор перемещения
						 *
						 * @param browser объект цифрового отпечатка браузера для перемещения
						 *
						 * \~english
						 * @brief Move constructor
						 * @param browser browser fingerprint object to move
						 *
						 * \~
						 */
						explicit Browser(Browser && browser) noexcept;
						/**
						 * \~russian
						 * @brief Конструктор копирования
						 *
						 * @param browser объект цифрового отпечатка браузера для копирования
						 *
						 * \~english
						 * @brief Copy constructor
						 * @param browser browser fingerprint object to copy
						 *
						 * \~
						 */
						explicit Browser(const Browser & browser) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Browser() noexcept;
				} browser_t;
			public:
				/**
				 * \~russian
				 * @brief Одна запись параметра SETTINGS из HTTP/2-соединения (RFC 7540 §6.5)
				 *
				 * @details Структура содержит информацию о параметре SETTINGS из HTTP/2-соединения,
				 *          включая идентификатор параметра и его значение, которые используются для настройки поведения HTTP/2-соединения.
				 *          Эти параметры могут включать информацию о размере таблицы заголовков, поддержке push-уведомлений,
				 *          максимальном количестве одновременных потоков, начальном размере окна, максимальном размере фрейма и максимальном размере списка заголовков.
				 *          Использование этих параметров позволяет сторонам согласовывать настройки HTTP/2-соединения, обеспечивая совместимость и улучшая производительность и безопасность соединения.
				 *
				 * \~english
				 * @brief A single SETTINGS parameter entry from an HTTP/2 connection (RFC 7540 §6.5)
				 * @details The structure holds information about a SETTINGS parameter of an HTTP/2 connection,
				 *          including the parameter identifier and its value, which are used to configure the behaviour of the HTTP/2 connection.
				 *          These parameters can carry information about the header table size, push notification support,
				 *          the maximum number of concurrent streams, the initial window size, the maximum frame size and the maximum header list size.
				 *          These parameters let the parties agree on the settings of the HTTP/2 connection, ensuring compatibility and improving the performance and security of the connection.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ H2Setting {
					/**
					 * \~russian
					 * @details Идентификатор параметра (1=HEADER_TABLE_SIZE, 2=ENABLE_PUSH, 3=MAX_CONCURRENT_STREAMS,
					 *          4=INITIAL_WINDOW_SIZE, 5=MAX_FRAME_SIZE, 6=MAX_HEADER_LIST_SIZE).
					 *
					 * \~english
					 * @details Parameter identifier (1=HEADER_TABLE_SIZE, 2=ENABLE_PUSH, 3=MAX_CONCURRENT_STREAMS,
					 *          4=INITIAL_WINDOW_SIZE, 5=MAX_FRAME_SIZE, 6=MAX_HEADER_LIST_SIZE).
					 *
					 * \~
					 */
					uint16_t id;
					// Значение параметра
					uint32_t value;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 * @param i идентификатор параметра
					 * @param v значение параметра
					 *
					 * \~english
					 * @brief Constructor
					 * @param i parameter identifier
					 * @param v parameter value
					 *
					 * \~
					 */
					explicit H2Setting(const uint16_t i = 0, const uint32_t v = 0) noexcept;
				} __attribute__((packed)) h2_setting_t;

				/**
				 * \~russian
				 * @brief Данные PRIORITY-фрейма HTTP/2 (RFC 7540 §6.3)
				 *
				 * @details Структура содержит информацию о PRIORITY-фрейме HTTP/2,
				 *          включая флаг эксклюзивной зависимости, вес приоритета,
				 *          идентификатор потока, которому задаётся приоритет, и идентификатор потока-зависимости.
				 *
				 * \~english
				 * @brief Data of an HTTP/2 PRIORITY frame (RFC 7540 §6.3)
				 * @details The structure holds information about an HTTP/2 PRIORITY frame,
				 *          including the exclusive dependency flag, the priority weight,
				 *          the identifier of the stream being prioritised and the identifier of the stream it depends on.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ H2Priority {
					// Флаг эксклюзивной зависимости (E-бит)
					bool exclusive;
					// Вес приоритета: raw 0-255 (фактический вес = weight + 1, диапазон 1-256)
					uint8_t weight;
					// Идентификатор потока, которому задаётся приоритет (из заголовка фрейма)
					uint32_t streamId;
					// Идентификатор потока-зависимости (31-битный stream dependency)
					uint32_t dependency;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit H2Priority() noexcept;
				} __attribute__((packed)) h2_priority_t;

				/**
				 * \~russian
				 * @brief Данные HTTP/2-соединения клиента, собранные из connection preface
				 *
				 * @details Содержит параметры, необходимые для вычисления Akamai HTTP/2 fingerprint:
				 *          SETTINGS-фрейм, WINDOW_UPDATE уровня соединения, PRIORITY-фреймы
				 *          и порядок псевдо-заголовков из первого HEADERS-фрейма.
				 *
				 * \~english
				 * @brief Data of the client HTTP/2 connection collected from the connection preface
				 * @details Holds the parameters required to compute the Akamai HTTP/2 fingerprint:
				 *          the SETTINGS frame, the connection-level WINDOW_UPDATE, the PRIORITY frames
				 *          and the order of the pseudo-headers from the first HEADERS frame.
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ H2Browser {
					// Инкремент от фрейма WINDOW_UPDATE уровня соединения (stream 0); 0 если фрейм не отправлялся
					uint32_t windowUpdate;
					// Псевдо-заголовки из первого HEADERS-фрейма: "m"=:method, "p"=:path, "s"=:scheme, "a"=:authority
					vector <string> pseudoHeaders;
					// Параметры SETTINGS в порядке получения из wire (порядок важен — входит в fingerprint)
					vector <h2_setting_t> settings;
					// Standalone PRIORITY-фреймы в порядке получения из wire
					vector <h2_priority_t> priorities;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit H2Browser() noexcept;
				} h2_browser_t;
			public:
				/**
				 * \~russian
				 * @brief Итератор как вложенный класс
				 *
				 * @details Итератор используется для обхода элементов контейнера unordered_map <uint8_t, browser_t>,
				 *          который хранит цифровые отпечатки браузеров. Итератор предоставляет интерфейс для доступа к элементам контейнера,
				 *          включая операции разыменования, сравнения и инкрементации.
				 *          Итератор позволяет пользователю перебирать элементы контейнера в порядке их хранения,
				 *          обеспечивая удобный способ работы с цифровыми отпечатками браузеров.
				 *
				 * \~english
				 * @brief Iterator as a nested class
				 * @details The iterator is used to traverse the elements of the unordered_map <uint8_t, browser_t> container
				 *          that stores the browser fingerprints. The iterator provides an interface for accessing the container elements,
				 *          including the dereference, comparison and increment operations.
				 *          The iterator lets the user walk over the container elements in their storage order,
				 *          providing a convenient way of working with browser fingerprints.
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Iterator {
					public:
						/**
						 * \~russian
						 * @brief Создаём необходимые нам типы данных
						 *
						 * \~english
						 * @brief Create the data types we need
						 *
						 * \~
						 */
						using value_type        = browser_t;
						using pointer           = browser_t *;
						using reference         = browser_t &;
						using difference_type   = std::ptrdiff_t;
						using iterator_category = std::bidirectional_iterator_tag;
					public:
						/**
						 * Создаём тип данных итератора
						 */
						using iterator = unordered_map <uint8_t, browser_t>::iterator;
					private:
						// Текущее значение итератора
						iterator _it;
					private:
						// Объект фреймворка
						[[maybe_unused]] const fmk_t * _fmk;
						// Объект работы с логами
						const log_t * _log;
					public:
						/**
						 * \~russian
						 * @brief Оператор преобразования в сырой итератор
						 *
						 * @return iterator итератор для преобразования
						 *
						 * \~english
						 * @brief Conversion operator to a raw iterator
						 * @return iterator iterator to convert
						 *
						 * \~
						 */
						operator iterator() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор извлечения указателя заголовка
						 *
						 * @return указатель заголовка
						 *
						 * \~english
						 * @brief Header pointer extraction operator
						 * @return header pointer
						 *
						 * \~
						 */
						pointer operator -> () noexcept;
						/**
						 * \~russian
						 * @brief Оператор разыменования заголовка
						 *
						 * @return значение заголовка
						 *
						 * \~english
						 * @brief Header dereference operator
						 * @return header value
						 *
						 * \~
						 */
						reference operator * () const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор смещения вперед
						 *
						 * @return значение текущего итератора
						 *
						 * \~english
						 * @brief Forward shift operator
						 * @return value of the current iterator
						 *
						 * \~
						 */
						Iterator & operator ++ () noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор сравнения соответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Iterator equality comparison operator
						 * @param other iterator to compare with
						 * @return      comparison result
						 *
						 * \~
						 */
						bool operator == (const Iterator & other) const noexcept;
						/**
						 * \~russian
						 * @brief Оператора сравнения несоответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Iterator inequality comparison operator
						 * @param other iterator to compare with
						 * @return      comparison result
						 *
						 * \~
						 */
						bool operator != (const Iterator & other) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param it  итератор для установки
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 *
						 * \~english
						 * @brief Constructor
						 * @param it  iterator to set
						 * @param fmk framework object
						 * @param log object for working with logs
						 *
						 * \~
						 */
						explicit Iterator(iterator it, const fmk_t * fmk, const log_t * log) noexcept;
				} iterator_t;
			private:
				// Список поддерживаемых цифровых отпечатков браузеров
				unordered_map <uint8_t, browser_t> _browsers;
			private:
				// Мьютекс для потокобезопасного доступа к хранилищу отпечатков
				mutable lock_state_t <std::shared_mutex> _mtx;
			private:
				// Объект фреймворка
				[[maybe_unused]] const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * \~russian
				 * @brief Метод форматированного вывода всех данных цифрового отпечатка браузера
				 *
				 * @details Распечатывает в читаемом текстовом виде все поля browser_t (Record Layer,
				 *          Handshake, ClientHello, Cipher Suites, Compressors, Extensions), а также
				 *          вычисляет и печатает все отпечатки imprint_t (JA3, JA4, JA4_r, PeetPrint).
				 *
				 * @param browser объект с распарсенными данными ClientHello
				 * @return        форматированная строка с полным описанием отпечатка
				 *
				 * \~english
				 * @brief Method of formatted output of all browser fingerprint data
				 * @details Prints in a readable text form all the fields of browser_t (Record Layer,
				 *          Handshake, ClientHello, Cipher Suites, Compressors, Extensions), and also
				 *          computes and prints all the imprint_t fingerprints (JA3, JA4, JA4_r, PeetPrint).
				 * @param browser object with the parsed ClientHello data
				 * @return        formatted string with the full description of the fingerprint
				 *
				 * \~
				 */
				string print(const browser_t & browser) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод вычисления Akamai HTTP/2 fingerprint
				 *
				 * @details Формат: "{settings}|{windowUpdate}|{priorities}|{pseudoHeaders}"
				 *           - settings:      id:value пары через ';' в порядке wire
				 *           - windowUpdate:  десятичный инкремент WINDOW_UPDATE (stream 0)
				 *           - priorities:    streamId:exclusive:dependency:weight через ',' (weight = raw+1)
				 *           - pseudoHeaders: m/p/s/a через ','
				 *
				 * @param h2 объект с распарсенными данными HTTP/2-соединения (из parseH2())
				 * @return   строка Akamai fingerprint, пустая строка если h2.settings пуст
				 *
				 * \~english
				 * @brief Method of computing the Akamai HTTP/2 fingerprint
				 * @details Format: "{settings}|{windowUpdate}|{priorities}|{pseudoHeaders}"
				 *           - settings:      id:value pairs separated by ';' in wire order
				 *           - windowUpdate:  decimal WINDOW_UPDATE increment (stream 0)
				 *           - priorities:    streamId:exclusive:dependency:weight separated by ',' (weight = raw+1)
				 *           - pseudoHeaders: m/p/s/a separated by ','
				 * @param h2 object with the parsed HTTP/2 connection data (from parseH2())
				 * @return   Akamai fingerprint string, an empty string if h2.settings is empty
				 *
				 * \~
				 */
				string akamai(const h2_browser_t & h2) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки, соответствует ли цифровой отпечаток шаблону, характерному для браузера
				 *
				 * @param imp объект цифрового отпечатка для проверки
				 * @return     результат проверки, принадлежит ли цифровой отпечаток реальному браузеру
				 *
				 * \~english
				 * @brief Method of checking whether the fingerprint matches a pattern typical of a browser
				 * @param imp fingerprint object to check
				 * @return     result of the check whether the fingerprint belongs to a real browser
				 *
				 * \~
				 */
				bool looksLikeBrowser(const imprint_t & imp) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод вычисления цифровых отпечатков на основе распарсенного ClientHello
				 *
				 * @param browser объект с распарсенными данными ClientHello
				 * @param result  объект для хранения всех вычисленных отпечатков
				 * @return        результат вычисления цифровых отпечатков
				 *
				 * \~english
				 * @brief Method of computing the fingerprints from the parsed ClientHello
				 * @param browser object with the parsed ClientHello data
				 * @param result  object for storing all the computed fingerprints
				 * @return        result of computing the fingerprints
				 *
				 * \~
				 */
				bool imprint(const browser_t & browser, imprint_t & result) const noexcept;
				/**
				 * \~russian
				 * @brief Метод парсинга данных цифрового отпечатка
				 *
				 * @param buffer  бинарный буфер данных цифрового отпечатка
				 * @param size    размер бинарного буфера данных цифрового отпечатка
				 * @param browser объект для хранения распарсенных данных цифрового отпечатка
				 * @return        результат парсинга данных цифрового отпечатка
				 *
				 * \~english
				 * @brief Method of parsing the fingerprint data
				 * @param buffer  binary buffer of the fingerprint data
				 * @param size    size of the binary buffer of the fingerprint data
				 * @param browser object for storing the parsed fingerprint data
				 * @return        result of parsing the fingerprint data
				 *
				 * \~
				 */
				bool parse(const uint8_t * buffer, const size_t size, browser_t & browser) const noexcept;
				/**
				 * \~russian
				 * @brief Метод парсинга connection preface и начальных фреймов HTTP/2-соединения
				 *
				 * @details Разбирает бинарный буфер, содержащий HTTP/2 client connection preface (magic + начальные фреймы).
				 *          Извлекает SETTINGS, WINDOW_UPDATE (stream 0), PRIORITY-фреймы и порядок псевдо-заголовков
				 *          из первого HEADERS-фрейма для построения Akamai HTTP/2 fingerprint.
				 *
				 * @param buffer бинарный буфер с данными HTTP/2-соединения
				 * @param size   размер буфера в байтах
				 * @param h2     объект для хранения распарсенных данных
				 * @return       true если SETTINGS-фрейм был успешно разобран, иначе false
				 *
				 * \~english
				 * @brief Method of parsing the connection preface and the initial frames of an HTTP/2 connection
				 * @details Parses a binary buffer holding the HTTP/2 client connection preface (magic + initial frames).
				 *          Extracts SETTINGS, WINDOW_UPDATE (stream 0), PRIORITY frames and the order of the pseudo-headers
				 *          from the first HEADERS frame in order to build the Akamai HTTP/2 fingerprint.
				 * @param buffer binary buffer with the HTTP/2 connection data
				 * @param size   size of the buffer in bytes
				 * @param h2     object for storing the parsed data
				 * @return       true if the SETTINGS frame was parsed successfully, false otherwise
				 *
				 * \~
				 */
				bool parseH2(const uint8_t * buffer, const size_t size, h2_browser_t & h2) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод применения данных цифрового отпечатка на запрос ClientHello
				 *
				 * @param buffer  буфер с данными цифрового отпечатка для применения к запросу ClientHello
				 * @param size    размер буфера в байтах
				 * @param browser объект с распарсенными данными ClientHello
				 * @return        буфер с данными ClientHello, модифицированными в соответствии с цифровым отпечатком
				 *
				 * @note Возвращаемый буфер — временный объект. Указатель на его данные
				 *       действителен только до конца полного выражения вызова (до возврата
				 *       из синхронного read_callback). Callback обязан скопировать данные.
				 * @note buffer должен содержать полный TLS/DTLS record layer; при неполной
				 *       записи метод возвращает пустой vector.
				 *
				 * \~english
				 * @brief Method of applying the fingerprint data to a ClientHello request
				 * @param buffer  buffer with the fingerprint data to apply to the ClientHello request
				 * @param size    size of the buffer in bytes
				 * @param browser object with the parsed ClientHello data
				 * @return        buffer with the ClientHello data modified according to the fingerprint
				 * @note The returned buffer is a temporary object. The pointer to its data
				 *       is valid only until the end of the full call expression (until the return
				 *       from the synchronous read_callback). The callback must copy the data.
				 * @note buffer must hold a complete TLS/DTLS record layer; on an incomplete
				 *       record the method returns an empty vector.
				 *
				 * \~
				 */
				vector <uint8_t> apply(const uint8_t * buffer, const size_t size, const browser_t & browser) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки всех цифровых отпечатков браузеров из хранилища
				 *
				 * \~english
				 * @brief Method of clearing all browser fingerprints from the storage
				 *
				 * \~
				 */
				void clear() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки, пусто ли хранилище цифровых отпечатков браузеров
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking whether the browser fingerprint storage is empty
				 * @return result of the check
				 *
				 * \~
				 */
				bool empty() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения количества цифровых отпечатков браузеров, хранящихся в хранилище
				 *
				 * @return количество цифровых отпечатков браузеров
				 *
				 * \~english
				 * @brief Method of getting the number of browser fingerprints held in the storage
				 * @return number of browser fingerprints
				 *
				 * \~
				 */
				size_t size() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения списка идентификаторов всех цифровых отпечатков браузеров, хранящихся в хранилище
				 *
				 * @return список идентификаторов цифровых отпечатков браузеров
				 *
				 * \~english
				 * @brief Method of getting the list of identifiers of all browser fingerprints held in the storage
				 * @return list of browser fingerprint identifiers
				 *
				 * \~
				 */
				vector <id_t> list() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки режима потокобезопасности хранилища отпечатков
				 *
				 * @param mode флаг режима безопасности потоков
				 *
				 * \~english
				 * @brief Method of setting the thread-safety mode of the fingerprint storage
				 * @param mode thread-safety mode flag
				 *
				 * \~
				 */
				void threadSafety(const bool mode) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод удаления цифрового отпечатка браузера из хранилища по идентификатору
				 *
				 * @param id идентификатор цифрового отпечатка
				 * @return   результат выполнения удаления цифрового отпечатка
				 *
				 * \~english
				 * @brief Method of removing a browser fingerprint from the storage by identifier
				 * @param id fingerprint identifier
				 * @return   result of removing the fingerprint
				 *
				 * \~
				 */
				bool remove(const id_t id) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения данных цифрового отпечатка браузера по идентификатору
				 *
				 * @param id идентификатор цифрового отпечатка
				 * @return   объект с цифровым отпечатком браузера, соответствующий указанному идентификатору
				 *
				 * \~english
				 * @brief Method of getting the browser fingerprint data by identifier
				 * @param id fingerprint identifier
				 * @return   browser fingerprint object matching the specified identifier
				 *
				 * \~
				 */
				const browser_t & get(const id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод добавления цифрового отпечатка браузера в хранилище
				 *
				 * @param browser объект с распарсенными данными ClientHello
				 * @return        идентификатор добавленного цифрового отпечатка
				 *
				 * \~english
				 * @brief Method of adding a browser fingerprint to the storage
				 * @param browser object with the parsed ClientHello data
				 * @return        identifier of the added fingerprint
				 *
				 * \~
				 */
				id_t add(const browser_t & browser) noexcept;
				/**
				 * \~russian
				 * @brief Метод добавления цифрового отпечатка браузера в хранилище в бинарном виде (дамп цифрового отпечатка)
				 *
				 * @param buffer бинарный буфер с данными цифрового отпечатка
				 * @param size   размер бинарного буфера в байтах
				 * @return       идентификатор добавленного цифрового отпечатка
				 *
				 * \~english
				 * @brief Method of adding a browser fingerprint to the storage in binary form (fingerprint dump)
				 * @param buffer binary buffer with the fingerprint data
				 * @param size   size of the binary buffer in bytes
				 * @return       identifier of the added fingerprint
				 *
				 * \~
				 */
				id_t add(const uint8_t * buffer, const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод формирования бинарного дампа всех цифровых отпечатков браузеров
				 *
				 * @return бинарный буфер, содержащий дамп всех цифровых отпечатков браузеров
				 *
				 * \~english
				 * @brief Method of building a binary dump of all browser fingerprints
				 * @return binary buffer holding the dump of all browser fingerprints
				 *
				 * \~
				 */
				vector <uint8_t> dump() const noexcept;
				/**
				 * \~russian
				 * @brief Метод загрузки бинарного дампа всех цифровых отпечатков браузеров
				 *
				 * @param buffer бинарный буфер для загрузки данных цифровых отпечатков
				 * @return       результат загрузки бинарного дампа
				 *
				 * \~english
				 * @brief Method of loading a binary dump of all browser fingerprints
				 * @param buffer binary buffer to load the fingerprint data from
				 * @return       result of loading the binary dump
				 *
				 * \~
				 */
				bool dump(const vector <uint8_t> & buffer) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод загрузки бинарного дампа цифрового отпечатка
				 *
				 * @param input   бинарный буфер с данными цифрового отпечатка
				 * @param browser объект для хранения данных цифрового отпечатка
				 * @return        результат загрузки бинарного дампа цифрового отпечатка
				 *
				 * \~english
				 * @brief Method of loading a binary dump of a fingerprint
				 * @param input   binary buffer with the fingerprint data
				 * @param browser object for storing the fingerprint data
				 * @return        result of loading the binary dump of the fingerprint
				 *
				 * \~
				 */
				bool dump(const vector <uint8_t> & input, browser_t & browser) const noexcept;
				/**
				 * \~russian
				 * @brief Метод формирования бинарного дампа цифрового отпечатка браузера
				 *
				 * @param browser объект с распарсенными данными ClientHello
				 * @param output  буфер для записи бинарного дампа цифрового отпечатка
				 * @return        результат формирования бинарного дампа цифрового отпечатка
				 *
				 * \~english
				 * @brief Method of building a binary dump of a browser fingerprint
				 * @param browser object with the parsed ClientHello data
				 * @param output  buffer to write the binary dump of the fingerprint to
				 * @return        result of building the binary dump of the fingerprint
				 *
				 * \~
				 */
				bool dump(const browser_t & browser, vector <uint8_t> & output) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод обмена заголовками
				 *
				 * @param fgp объект Fingerprint для обмена данными
				 *
				 * \~english
				 * @brief Method of swapping the headers
				 * @param fgp Fingerprint object to exchange the data with
				 *
				 * \~
				 */
				void swap(Fingerprint & fgp) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор
				 *
				 * \~english
				 * @brief Method of getting the end iterator
				 * @return end iterator
				 *
				 * \~
				 */
				iterator_t end() noexcept;
				/**
				 * \~russian
				 * @brief Метод получение начального итератора
				 *
				 * @return начальный итератор
				 *
				 * \~english
				 * @brief Method of getting the begin iterator
				 * @return begin iterator
				 *
				 * \~
				 */
				iterator_t begin() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод поиска указанного заголовка
				 *
				 * @param id идентификатор заголовка для поиска
				 * @return   итератор указанного заголовка
				 *
				 * \~english
				 * @brief Method of searching for the specified header
				 * @param id identifier of the header to search for
				 * @return   iterator of the specified header
				 *
				 * \~
				 */
				iterator_t find(const id_t id) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор извлечения цифрового отпечатка браузера
				 *
				 * @param id идентификатор цифрового отпечатка
				 * @return   цифровой отпечаток браузера
				 *
				 * \~english
				 * @brief Browser fingerprint extraction operator
				 * @param id fingerprint identifier
				 * @return   browser fingerprint
				 *
				 * \~
				 */
				const browser_t & operator[](const id_t id) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Проверка, пусто ли хранилище цифровых отпечатков браузеров
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Check whether the browser fingerprint storage is empty
				 * @return result of the check
				 *
				 * \~
				 */
				operator bool() const noexcept;
				/**
				 * \~russian
				 * @brief Получения количества цифровых отпечатков браузеров, хранящихся в хранилище
				 *
				 * @return количество цифровых отпечатков браузеров
				 *
				 * \~english
				 * @brief Getting the number of browser fingerprints held in the storage
				 * @return number of browser fingerprints
				 *
				 * \~
				 */
				operator size_t() const noexcept;
				/**
				 * \~russian
				 * @brief Получения бинарных данных дампа всех цифровых отпечатков браузеров
				 *
				 * @return бинарные данные буфера дампа всех цифровых отпечатков браузеров
				 *
				 * \~english
				 * @brief Getting the binary data of the dump of all browser fingerprints
				 * @return binary data of the dump buffer of all browser fingerprints
				 *
				 * \~
				 */
				operator vector <uint8_t> () const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор сравнения двух контейнеров отпечатков браузеров
				 *
				 * @param fgp отпечатки браузеров для сравнения
				 * @return    результат сравнения
				 *
				 * \~english
				 * @brief Comparison operator of two browser fingerprint containers
				 * @param fgp browser fingerprints to compare with
				 * @return    comparison result
				 *
				 * \~
				 */
				bool operator == (const Fingerprint & fgp) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор установки дампа цифрового отпечатка браузера
				 *
				 * @param buffer бинарный буфер для загрузки данных цифровых отпечатков
				 * @return       текущий контейнер отпечатков браузеров
				 *
				 * \~english
				 * @brief Browser fingerprint dump assignment operator
				 * @param buffer binary buffer to load the fingerprint data from
				 * @return       the current browser fingerprint container
				 *
				 * \~
				 */
				Fingerprint & operator = (const vector <uint8_t> & buffer) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор перемещения
				 *
				 * @param fgp объект Fingerprint для перемещения
				 * @return    текущий контейнер отпечатков браузеров
				 *
				 * \~english
				 * @brief Move operator
				 * @param fgp Fingerprint object to move
				 * @return    the current browser fingerprint container
				 *
				 * \~
				 */
				Fingerprint & operator = (Fingerprint && fgp) noexcept;
				/**
				 * \~russian
				 * @brief Оператор копирования
				 *
				 * @param fgp объект Fingerprint для копирования
				 * @return    текущий контейнер отпечатков браузеров
				 *
				 * \~english
				 * @brief Copy operator
				 * @param fgp Fingerprint object to copy
				 * @return    the current browser fingerprint container
				 *
				 * \~
				 */
				Fingerprint & operator = (const Fingerprint & fgp) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор копирования
				 *
				 *
				 * \~english
				 * @brief Copy constructor
				 *
				 * \~
				 */
				explicit Fingerprint(const Fingerprint &) = delete;
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
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Fingerprint(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Fingerprint() noexcept;
		} fgp_t;
	};
};

#endif // __AWH_SSL_FINGERPRINT__
