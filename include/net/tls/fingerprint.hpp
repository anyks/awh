/**
 * @file: fingerprint.hpp
 * @date: 2026-04-28
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_SSL_FINGERPRINT__
#define __AWH_SSL_FINGERPRINT__

/**
 * Стандартные модули
 */
#include <array>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

/**
 * Наши модули
 */
#include "tls.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"
#include "../../sys/locker.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief пространство имён работы с TLS
	 *
	 */
	namespace tls {
		/**
		 * @brief Структура цифрового отпечатка устройства
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Fingerprint {
			public:
				/**
				 * @brief Тип идентификатора отпечатка браузера
				 *
				 */
				using id_t = uint8_t;
			public:
				/**
				 * @brief Структура расширения TLS
				 *
				 */
				typedef struct Extension {
					// Тип расширения
					extension_type_t type;
					/**
					 * @brief Конструктор
					 *
					 * @param type Тип расширения
					 */
					explicit Extension(const extension_type_t type) noexcept : type(type) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension() = default;
				} extension_t;

				/**
				 * @brief Структура расширения TLS для GREASE-значений
				 *
				 */
				typedef struct Extension_Grease : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Grease() noexcept :
					 extension_t(extension_type_t::GREASE) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Grease() = default;
				} extension_grease_t;

				/**
				 * @brief Структура расширения TLS для идентификатора канала (Channel ID)
				 *
				 */
				typedef struct Extension_Channel_ID : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Channel_ID() noexcept :
					 extension_t(extension_type_t::CHANNEL_ID) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Channel_ID() = default;
				} extension_channel_id_t;

				/**
				 * @brief Структура расширения TLS для фильтров OID (OID Filters)
				 *
				 */
				typedef struct Extension_OID_Filters : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_OID_Filters() noexcept :
					 extension_t(extension_type_t::OID_FILTERS) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_OID_Filters() = default;
				} extension_oid_filters_t;

				/**
				 * @brief Структура расширения TLS для доверенных якорей (Trust Anchors)
				 *
				 */
				typedef struct Extension_Trust_Anchors : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Trust_Anchors() noexcept :
					 extension_t(extension_type_t::TRUST_ANCHORS) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Trust_Anchors() = default;
				} extension_trust_anchors_t;

				/**
				 * @brief Структура расширения TLS для шифрования перед MAC (Encrypt-Then-MAC)
				 *
				 */
				typedef struct Extension_Encrypt_Then_MAC : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Encrypt_Then_MAC() noexcept :
					 extension_t(extension_type_t::ENCRYPT_THEN_MAC) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Encrypt_Then_MAC() = default;
				} extension_encrypt_then_mac_t;

				/**
				 * @brief Структура расширения TLS для информации о прозрачности (Transparency Info)
				 *
				 */
				typedef struct Extension_Transparency_Info : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Transparency_Info() noexcept :
					 extension_t(extension_type_t::TRANSPARENCY_INFO) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Transparency_Info() = default;
				} extension_transparency_info_t;

				/**
				 * @brief Структура расширения TLS для поддержки аутентификации после рукопожатия (Post-Handshake Authentication)
				 *
				 */
				typedef struct Extension_Post_Handshake_Auth : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Post_Handshake_Auth() noexcept :
					 extension_t(extension_type_t::POST_HANDSHAKE_AUTH) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Post_Handshake_Auth() = default;
				} extension_post_handshake_auth_t;

				/**
				 * @brief Структура расширения TLS для указания типа сертификата клиента (Client Certificate Type)
				 *
				 */
				typedef struct Extension_Client_Certificate_Type : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Client_Certificate_Type() noexcept :
					 extension_t(extension_type_t::CLIENT_CERTIFICATE_TYPE) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Client_Certificate_Type() = default;
				} extension_client_certificate_type_t;

				/**
				 * @brief Структура расширения TLS для указания типа сертификата сервера (Server Certificate Type)
				 *
				 */
				typedef struct Extension_Server_Certificate_Type : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Server_Certificate_Type() noexcept :
					 extension_t(extension_type_t::SERVER_CERTIFICATE_TYPE) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Server_Certificate_Type() = default;
				} extension_server_certificate_type_t;

				/**
				 * @brief Структура расширения TLS для указания имени сервера (SNI)
				 *
				 */
				typedef struct Extension_Server_Name : public extension_t {
					// Список имён серверов
					vector <string> names;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Server_Name() noexcept :
					 extension_t(extension_type_t::SERVER_NAME) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Server_Name() = default;
				} extension_server_name_t;

				/**
				 * @brief Структура расширения TLS для запроса статуса сертификата (OCSP)
				 *
				 */
				typedef struct Extension_Status_Request : public extension_t {
					// Тип статуса сертификата (например, OCSP)
					string certificateStatusType;
					// Длина списка идентификаторов ответчиков
					uint16_t responderIdListLength;
					// Длина расширений запроса статуса сертификата
					uint16_t requestExtensionsLength;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Status_Request() noexcept :
					 extension_t(extension_type_t::STATUS_REQUEST),
					 certificateStatusType{""},
					 responderIdListLength(0),
					 requestExtensionsLength(0) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Status_Request() = default;
				} extension_status_request_t;

				/**
				 * @brief Структура расширения TLS для указания поддерживаемых групп (Supported Groups)
				 *
				 */
				typedef struct Extension_Supported_Groups : public extension_t {
					// Список поддерживаемых групп
					vector <group_t> supportedGroups;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Supported_Groups() noexcept :
					 extension_t(extension_type_t::SUPPORTED_GROUPS) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Supported_Groups() = default;
				} extension_supported_groups_t;

				/**
				 * @brief Структура расширения TLS для указания форматов точек эллиптической кривой (EC Point Formats)
				 *
				 */
				typedef struct Extension_EC_Point : public extension_t {
					// Список поддерживаемых форматов точек эллиптической кривой
					vector <ec_point_format_t> formats;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_EC_Point() noexcept :
					 extension_t(extension_type_t::EC_POINT_FORMATS) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_EC_Point() = default;
				} extension_ec_point_t;

				/**
				 * @brief Структура расширения TLS для согласования протокола прикладного уровня (ALPN)
				 *
				 */
				typedef struct Extension_ALPN : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_ALPN() noexcept :
					 extension_t(extension_type_t::ALPN) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_ALPN() = default;
				} extension_alpn_t;

				/**
				 * @brief Структура расширения TLS для передачи настроек приложения (Application Settings)
				 *
				 */
				typedef struct Extension_Application_Settings : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Application_Settings() noexcept :
					 extension_t(extension_type_t::APPLICATION_SETTINGS) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Application_Settings() = default;
				} extension_application_settings_t;

				/**
				 * @brief Структура расширения TLS для передачи старых настроек приложения (Application Settings Old)
				 *
				 */
				typedef struct Extension_Application_Settings_Old : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Application_Settings_Old() noexcept :
					 extension_t(extension_type_t::APPLICATION_SETTINGS_OLD) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Application_Settings_Old() = default;
				} extension_application_settings_old_t;

				/**
				 * @brief Структура расширения TLS для согласования следующего протокола (Next Protocol Negotiation)
				 *
				 */
				typedef struct Extension_Next_Proto_Neg : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Next_Proto_Neg() noexcept :
					 extension_t(extension_type_t::NEXT_PROTO_NEG) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Next_Proto_Neg() = default;
				} extension_next_proto_neg_t;

				/**
				 * @brief Структура расширения TLS для передачи информации о подписанных временных метках сертификатов (Signed Certificate Timestamp)
				 *
				 */
				typedef struct Extension_Signed_Certificate_Timestamp : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Signed_Certificate_Timestamp() noexcept :
					 extension_t(extension_type_t::SIGNED_CERTIFICATE_TIMESTAMP) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Signed_Certificate_Timestamp() = default;
				} extension_signed_certificate_timestamp_t;

				/**
				 * @brief Структура расширения TLS для добавления произвольного количества байтов заполнения (Padding)
				 *
				 */
				typedef struct Extension_Padding : public extension_t {
					// Размер данных заполнения
					size_t size;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Padding() noexcept :
					 extension_t(extension_type_t::PADDING), size(0) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Padding() = default;
				} extension_padding_t;

				/**
				 * @brief Структура расширения TLS для использования расширенного мастер-секрета (Extended Master Secret)
				 *
				 */
				typedef struct Extension_Extended_Master_Secret : public extension_t {
					// Данные расширения Master Secret
					string masterSecretData;
					// Данные расширения Extended Master Secret для сервера (устаревшее расширение)
					string extendedMasterSecretData;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Extended_Master_Secret() noexcept :
					 extension_t(extension_type_t::EXTENDED_MASTER_SECRET),
					 masterSecretData{""}, extendedMasterSecretData{""} {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Extended_Master_Secret() = default;
				} extension_extended_master_secret_t;

				/**
				 * @brief Структура расширения TLS для сжатия сертификатов (Compress Certificate)
				 *
				 */
				typedef struct Extension_Compress_Certificate : public extension_t {
					// Список поддерживаемых алгоритмов сжатия сертификатов
					vector <compressor_t> algorithms;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Compress_Certificate() noexcept :
					 extension_t(extension_type_t::COMPRESS_CERTIFICATE) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Compress_Certificate() = default;
				} extension_compress_certificate_t;

				/**
				 * @brief Структура расширения TLS для использования билета сессии (Session Ticket)
				 *
				 */
				typedef struct Extension_Session_Ticket : public extension_t {
					// Данные расширения Session Ticket
					vector <uint8_t> data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Session_Ticket() noexcept :
					 extension_t(extension_type_t::SESSION_TICKET) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Session_Ticket() = default;
				} extension_session_ticket_t;

				/**
				 * @brief Структура расширения TLS для указания поддерживаемых версий (Supported Versions)
				 *
				 */
				typedef struct Extension_Supported_Versions : public extension_t {
					// Список поддерживаемых версий
					vector <version_t> versions;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Supported_Versions() noexcept :
					 extension_t(extension_type_t::SUPPORTED_VERSIONS) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Supported_Versions() = default;
				} extension_supported_versions_t;

				/**
				 * @brief Структура расширения TLS для указания режимов обмена ключами PSK (PSK Key Exchange Modes)
				 *
				 */
				typedef struct Extension_PSK_Key_Exchange : public extension_t {
					// Список поддерживаемых версий
					vector <psk_key_t> modes;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_PSK_Key_Exchange() noexcept :
					 extension_t(extension_type_t::PSK_KEY_EXCHANGE_MODES) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_PSK_Key_Exchange() = default;
				} extension_psk_key_exchange_t;

				/**
				 * @brief Структура расширения TLS для поддержки ранних данных (Early Data)
				 *
				 */
				typedef struct Extension_Early_Data : public extension_t {
					// Максимальный размер ранних данных
					uint32_t maxSize;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Early_Data() noexcept :
					 extension_t(extension_type_t::EARLY_DATA), maxSize(0) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Early_Data() = default;
				} extension_early_data_t;

				/**
				 * @brief Структура расширения TLS для обмена ключами (Key Share)
				 *
				 */
				typedef struct Extension_Key_Share : public extension_t {
					// Список групп обмена ключами в порядке появления в ClientHello (порядок важен — отражает предпочтение клиента)
					vector <pair <group_t, vector <uint8_t>>> shares;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Key_Share() noexcept :
					 extension_t(extension_type_t::KEY_SHARE) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Key_Share() = default;
				} extension_key_share_t;

				/**
				 * @brief Структура расширения TLS для передачи зашифрованного ClientHello (Encrypted ClientHello)
				 *
				 */
				typedef struct Extension_Encryption_Client_Hello : public extension_t {
					// Данные зашифрованного ClientHello
					vector <uint8_t> data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Encryption_Client_Hello() noexcept :
					 extension_t(extension_type_t::ENCRYPTED_CLIENT_HELLO) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Encryption_Client_Hello() = default;
				} extension_encryption_client_hello_t;

				/**
				 * @brief Структура расширения TLS для передачи информации о повторной договоренности (Renegotiation Info)
				 *
				 */
				typedef struct Extension_Renegotiation_Info : public extension_t {
					// Данные зашифрованного ClientHello
					vector <uint8_t> data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Renegotiation_Info() noexcept :
					 extension_t(extension_type_t::RENEGOTIATION_INFO) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Renegotiation_Info() = default;
				} extension_renegotiation_info_t;

				/**
				 * @brief Структура расширения TLS для указания максимального размера записи (Record Size Limit)
				 *
				 */
				typedef struct Extension_Record_Size_Limit : public extension_t {
					// Данные расширения Record Size Limit
					uint16_t data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Record_Size_Limit() noexcept :
					 extension_t(extension_type_t::RECORD_SIZE_LIMIT), data(0) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Record_Size_Limit() = default;
				} extension_record_size_limit_t;

				/**
				 * @brief Структура расширения DTLS для передачи cookie (Cookie)
				 *
				 */
				typedef struct Extension_Cookie : public extension_t {
					// Данные расширения Cookie
					vector <uint8_t> data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Cookie() noexcept :
					 extension_t(extension_type_t::COOKIE) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Cookie() = default;
				} extension_cookie_t;

				/**
				 * @brief Структура расширения TLS для использования предварительно совместного ключа (Pre-Shared Key)
				 *
				 */
				typedef struct Extension_Pre_Shared_Key : public extension_t {
					/**
					 * @brief Структура идентификатора предварительно совместного ключа (PSK Identity)
					 *
					 */
					struct Identity {
						// Время жизни билета (Ticket Age)
						uint32_t ticketAge;
						// Идентификатор предварительно совместного ключа
						vector <uint8_t> data;
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Identity() noexcept : ticketAge(0) {}
					};
					// Список идентификаторов предварительно совместных ключей
					vector <Identity> identities;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Pre_Shared_Key() noexcept :
					 extension_t(extension_type_t::PRE_SHARED_KEY) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Pre_Shared_Key() = default;
				} extension_pre_shared_key_t;

				/**
				 * @brief Структура расширения TLS для указания доверенных центров сертификации (Certificate Authorities)
				 *
				 */
				typedef struct Extension_Certificate_Authorities : public extension_t {
					// Список DER-кодированных Distinguished Name (DistinguishedName) из RFC 8446 §4.2.4
					std::vector <std::vector <uint8_t>> authorities;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Certificate_Authorities() noexcept :
					 extension_t(extension_type_t::CERTIFICATE_AUTHORITIES) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Certificate_Authorities() = default;
				} extension_certificate_authorities_t;

				/**
				 * @brief Структура расширения TLS для указания максимальной длины фрагмента (Max Fragment Length)
				 *
				 */
				typedef struct Extension_Max_Fragment_Length : public extension_t {
					// Максимальная длина фрагмента
					uint16_t length;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Max_Fragment_Length() noexcept :
					 extension_t(extension_type_t::MAX_FRAGMENT_LENGTH), length(0) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Max_Fragment_Length() = default;
				} extension_max_fragment_length_t;

				/**
				 * @brief Структура расширения TLS для использования SRTP (Use SRTP)
				 *
				 */
				typedef struct Extension_Use_SRTP : public extension_t {
					// Длина Master Key Identifier (MKI)
					uint8_t mkiLength;
					// Список поддерживаемых профилей SRTP
					vector <srtp_t> profiles;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Use_SRTP() noexcept :
					 extension_t(extension_type_t::USE_SRTP), mkiLength(0) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Use_SRTP() = default;
				} extension_use_srtp_t;

				/**
				 * @brief Структура расширения TLS для поддержки механизма heartbeat (Heartbeat)
				 *
				 */
				typedef struct Extension_Heartbeat : public extension_t {
					// Режим heartbeat (например, peer_allowed_to_send или peer_not_allowed_to_send)
					heartbeat_t mode;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Heartbeat() noexcept :
					 extension_t(extension_type_t::HEARTBEAT), mode(heartbeat_t::UNKNOWN) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Heartbeat() = default;
				} extension_heartbeat_t;

				/**
				 * @brief Структура расширения TLS для указания поддерживаемых алгоритмов подписи (Signature Algorithms)
				 *
				 */
				typedef struct Extension_Signature : public extension_t {
					// Список поддерживаемых алгоритмов подписи
					vector <signature_t> algorithms;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Signature() noexcept :
					 extension_t(extension_type_t::SIGNATURE_ALGORITHMS) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Signature() = default;
				} extension_signature_t;

				/**
				 * @brief Структура расширения TLS для использования делегированных учетных данных (Delegated Credential)
				 *
				 */
				typedef struct Extension_Delegated_Credential : public extension_t {
					// Список поддерживаемых алгоритмов подписи для делегированных учетных данных
					vector <signature_t> algorithms;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Delegated_Credential() noexcept :
					 extension_t(extension_type_t::DELEGATED_CREDENTIAL) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Delegated_Credential() = default;
				} extension_delegated_credential_t;

				/**
				 * @brief Структура расширения TLS для указания поддерживаемых алгоритмов подписи для сертификатов (Signature Algorithms for Certificates)
				 *
				 */
				typedef struct Extension_Signature_Algorithms_Cert : public extension_t {
					// Список поддерживаемых алгоритмов подписи для сертификатов
					vector <signature_t> algorithms;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Signature_Algorithms_Cert() noexcept :
					 extension_t(extension_type_t::SIGNATURE_ALGORITHMS_CERT) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Signature_Algorithms_Cert() = default;
				} extension_signature_algorithms_cert_t;

				/**
				 * @brief Структура расширения TLS для передачи информации о прозрачности (Transparency Info)
				 *
				 */
				typedef struct Extension_TLS_Flags : public extension_t {
					// Список флагов или параметров, специфичных для реализации TLS
					vector <uint8_t> flags;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_TLS_Flags() noexcept :
					 extension_t(extension_type_t::TLS_FLAGS) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_TLS_Flags() = default;
				} extension_tls_flags_t;

				/**
				 * @brief Структура расширения TLS для передачи параметров транспорта QUIC (QUIC Transport Parameters)
				 *
				 */
				typedef struct Extension_Quic_Transport_Params : public extension_t {
					/**
					 * Список параметров транспорта QUIC в виде пар ключ-значение,
					 * где ключ и значение представлены в виде 64-битных целых чисел.
					 * Эти параметры могут включать информацию о поддерживаемых версиях QUIC,
					 * максимальных размерах пакетов, тайм-аутах и других настройках,
					 * которые могут быть согласованы между клиентом и сервером во время TLS-рукопожатия для оптимизации работы протокола QUIC.
					 */
					unordered_map <uint64_t, uint64_t> params;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Quic_Transport_Params() noexcept :
					 extension_t(extension_type_t::QUIC_TRANSPORT_PARAMETERS) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Quic_Transport_Params() = default;
				} extension_quic_transport_params_t;

				/**
				 * @brief Структура расширения TLS для передачи параметров транспорта QUIC (QUIC Transport Parameters Legacy)
				 *
				 */
				typedef struct Extension_Quic_Transport_Params_Legacy : public extension_t {
					/**
					 * Список параметров транспорта QUIC в виде пар ключ-значение,
					 * где ключ и значение представлены в виде 64-битных целых чисел.
					 * Эти параметры могут включать информацию о поддерживаемых версиях QUIC,
					 * максимальных размерах пакетов, тайм-аутах и других настройках,
					 * которые могут быть согласованы между клиентом и сервером во время TLS-рукопожатия для оптимизации работы протокола QUIC.
					 */
					unordered_map <uint64_t, uint64_t> params;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Quic_Transport_Params_Legacy() noexcept :
					 extension_t(extension_type_t::QUIC_TRANSPORT_PARAMETERS_LEGACY) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Quic_Transport_Params_Legacy() = default;
				} extension_quic_transport_params_legacy_t;

				/**
				 * @brief Структура расширения TLS для передачи внешних расширений ECH (ECH Outer Extensions)
				 *
				 */
				typedef struct Extension_ECH_Outer_Extensions : public extension_t {
					// Список расширений, которые были зашифрованы в рамках ECH и переданы во внешнем расширении ECH Outer Extensions
					vector <extension_type_t> extensions;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_ECH_Outer_Extensions() noexcept :
					 extension_t(extension_type_t::ECH_OUTER_EXTENSIONS) {}
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_ECH_Outer_Extensions() = default;
				} extension_ech_outer_extensions_t;
			public:
				/**
				 * @brief Структура версии TLS
				 *
				 */
				typedef struct VersionTLS {
					// Версия TLS на уровне записи (wire-код в десятичном виде, например "769")
					string record;
					// Согласованная версия TLS (наибольшая не-GREASE версия из supported_versions, десятично)
					string negotiated;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit VersionTLS() noexcept :
					 record{""}, negotiated{""} {}
				} version_tls_t;

				/**
				 * @brief Структура вычисленных цифровых отпечатков TLS-соединения (JA3, JA4, PeetPrint и пр.)
				 *
				 */
				typedef struct Imprint {
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
					 * @brief Конструктор
					 *
					 */
					explicit Imprint() noexcept :
					 ja3{""}, ja4{""}, ja4r{""},
					 ja3Hash{""}, sessionId{""},
					 peetprint{""}, peetprintHash{""},
					 clientRandom{""}, akamai{""} {}
				} imprint_t;
			public:
				/**
				 * @brief Структура записи TLS
				 *
				 */
				typedef struct Record {
					// Эпоха записи DTLS
					uint16_t epoch;
					// Длина записи TLS
					uint16_t length;
					// Порядковый номер записи
					uint64_t sequence;
					// Версия протокола TLS, поддерживаемая браузером
					version_t version;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Record() noexcept :
					 epoch(0), length(0), sequence(0),
					 version(version_t::UNKNOWN) {}
				} __attribute__((packed)) record_t;

				/**
				 * @brief Структура фрагмента TLS
				 *
				 */
				typedef struct Fragment {
					// Смещение фрагмента в рамках записи TLS
					uint32_t offset;
					// Длина фрагмента TLS
					uint32_t length;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Fragment() noexcept : offset(0), length(0) {}
				} __attribute__((packed)) fragment_t;

				/**
				 * @brief Структура рукопожатия TLS
				 *
				 */
				typedef struct Handshake {
					// Длина рукопожатия блока рукопожатия TLS
					uint32_t length;
					// Порядковый номер рукопожатия TLS
					uint16_t sequence;
					// Фрагмент рукопожатия TLS
					fragment_t fragment;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Handshake() noexcept : length(0), sequence(0) {}
				} __attribute__((packed)) handshake_t;

				/**
				 * @brief Структура ClientHello TLS
				 *
				 */
				typedef struct ClientHello {
					// Версия протокола TLS, поддерживаемая браузером в рукопожатии
					version_t version;
					// 32 байта случайных байта в ClientHello
					array <uint8_t, 32> random;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit ClientHello() noexcept : version(version_t::UNKNOWN) {}
				} client_hello_t;

				/**
				 * @brief Класс цифрового отпечатка браузера
				 *
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
						 * @brief Оператор сравнения двух отпечатков браузеров
						 *
						 * @param browser объект цифрового отпечатка браузера для сравнения
						 * @return        результат сравнения
						 */
						bool operator == (const Browser & browser) const noexcept;
					public:
						/**
						 * @brief Оператор перемещения
						 *
						 * @param browser объект цифрового отпечатка браузера для перемещения
						 * @return        текущий объект после перемещения
						 */
						Browser & operator = (Browser && browser) noexcept;
						/**
						 * @brief Оператор копирования
						 *
						 * @param browser объект цифрового отпечатка браузера для копирования
						 * @return        текущий объект после копирования
						 */
						Browser & operator = (const Browser & browser) noexcept;
					public:
						/**
						 * @brief Конструктор перемещения
						 *
						 * @param browser объект цифрового отпечатка браузера для перемещения
						 */
						explicit Browser(Browser && browser) noexcept;
						/**
						 * @brief Конструктор копирования
						 *
						 * @param browser объект цифрового отпечатка браузера для копирования
						 */
						explicit Browser(const Browser & browser) noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Browser() noexcept : grease(false) {}
				} browser_t;
			public:
				/**
				 * @brief Одна запись параметра SETTINGS из HTTP/2-соединения (RFC 7540 §6.5)
				 *
				 */
				typedef struct H2Setting {
					/**
					 * Идентификатор параметра (1=HEADER_TABLE_SIZE, 2=ENABLE_PUSH, 3=MAX_CONCURRENT_STREAMS,
					 * 4=INITIAL_WINDOW_SIZE, 5=MAX_FRAME_SIZE, 6=MAX_HEADER_LIST_SIZE)
					 */
					uint16_t id;
					// Значение параметра
					uint32_t value;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit H2Setting(const uint16_t i = 0, const uint32_t v = 0) noexcept : id(i), value(v) {}
				} __attribute__((packed)) h2_setting_t;

				/**
				 * @brief Данные PRIORITY-фрейма HTTP/2 (RFC 7540 §6.3)
				 *
				 */
				typedef struct H2Priority {
					// Флаг эксклюзивной зависимости (E-бит)
					bool exclusive;
					// Вес приоритета: raw 0-255 (фактический вес = weight + 1, диапазон 1-256)
					uint8_t weight;
					// Идентификатор потока, которому задаётся приоритет (из заголовка фрейма)
					uint32_t streamId;
					// Идентификатор потока-зависимости (31-битный stream dependency)
					uint32_t dependency;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit H2Priority() noexcept :
					 exclusive(false), weight(0),
					 streamId(0),  dependency(0) {}
				} __attribute__((packed)) h2_priority_t;

				/**
				 * @brief Данные HTTP/2-соединения клиента, собранные из connection preface
				 *
				 * Содержит параметры, необходимые для вычисления Akamai HTTP/2 fingerprint:
				 * SETTINGS-фрейм, WINDOW_UPDATE уровня соединения, PRIORITY-фреймы
				 * и порядок псевдо-заголовков из первого HEADERS-фрейма.
				 */
				typedef struct H2Browser {
					// Инкремент от фрейма WINDOW_UPDATE уровня соединения (stream 0); 0 если фрейм не отправлялся
					uint32_t windowUpdate;
					// Псевдо-заголовки из первого HEADERS-фрейма: "m"=:method, "p"=:path, "s"=:scheme, "a"=:authority
					vector <string> pseudoHeaders;
					// Параметры SETTINGS в порядке получения из wire (порядок важен — входит в fingerprint)
					vector <h2_setting_t> settings;
					// Standalone PRIORITY-фреймы в порядке получения из wire
					vector <h2_priority_t> priorities;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit H2Browser() noexcept : windowUpdate(0) {}
				} h2_browser_t;
			public:
				/**
				 * @brief Итератор как вложенный класс
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Iterator {
					public:
						/**
						 * Создаём необходимые нам типы данных
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
						const fmk_t * _fmk;
						// Объект работы с логами
						const log_t * _log;
					public:
						/**
						 * @brief Оператор преобразования в сырой итератор
						 *
						 * @return iterator итератор для преобразования
						 */
						operator iterator() noexcept;
					public:
						/**
						 * @brief Оператор извлечения указателя заголовка
						 *
						 * @return указатель заголовка
						 */
						pointer operator -> () noexcept;
						/**
						 * @brief Оператор разыменования заголовка
						 *
						 * @return значение заголовка
						 */
						reference operator * () const noexcept;
					public:
						/**
						 * @brief Оператор смещения вперед
						 *
						 * @return значение текущего итератора
						 */
						Iterator & operator ++ () noexcept;
					public:
						/**
						 * @brief Оператор сравнения соответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 */
						bool operator == (const Iterator & other) const noexcept;
						/**
						 * @brief Оператора сравнения несоответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 */
						bool operator != (const Iterator & other) const noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param it  итератор для установки
						 * @param fmk объект фреймворка
						 * @param log объект для работы с логами
						 */
						explicit Iterator(iterator it, const fmk_t * fmk, const log_t * log) noexcept : _it(it), _fmk(fmk), _log(log) {}
				} iterator_t;
			private:
				// Список поддерживаемых цифровых отпечатков браузеров
				unordered_map <uint8_t, browser_t> _browsers;
			private:
				// Объект фреймворка
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод форматированного вывода всех данных цифрового отпечатка браузера
				 *
				 * Распечатывает в читаемом текстовом виде все поля browser_t (Record Layer,
				 * Handshake, ClientHello, Cipher Suites, Compressors, Extensions), а также
				 * вычисляет и печатает все отпечатки imprint_t (JA3, JA4, JA4_r, PeetPrint).
				 *
				 * @param browser объект с распарсенными данными ClientHello
				 * @return        форматированная строка с полным описанием отпечатка
				 */
				string print(const browser_t & browser) const noexcept;
			public:
				/**
				 * @brief Метод вычисления Akamai HTTP/2 fingerprint
				 *
				 * Формат: "{settings}|{windowUpdate}|{priorities}|{pseudoHeaders}"
				 *   - settings:      id:value пары через ';' в порядке wire
				 *   - windowUpdate:  десятичный инкремент WINDOW_UPDATE (stream 0)
				 *   - priorities:    streamId:exclusive:dependency:weight через ',' (weight = raw+1)
				 *   - pseudoHeaders: m/p/s/a через ','
				 *
				 * @param h2 объект с распарсенными данными HTTP/2-соединения (из parseH2())
				 * @return   строка Akamai fingerprint, пустая строка если h2.settings пуст
				 */
				string akamai(const h2_browser_t & h2) const noexcept;
			public:
				/**
				 * @brief Метод проверки, соответствует ли цифровой отпечаток шаблону, характерному для браузера
				 *
				 * @param imp объект цифрового отпечатка для проверки
				 * @return     результат проверки, принадлежит ли цифровой отпечаток реальному браузеру
				 */
				bool looksLikeBrowser(const imprint_t & imp) const noexcept;
			public:
				/**
				 * @brief Метод вычисления цифровых отпечатков на основе распарсенного ClientHello
				 *
				 * @param browser объект с распарсенными данными ClientHello
				 * @param result  объект для хранения всех вычисленных отпечатков
				 * @return        результат вычисления цифровых отпечатков
				 */
				bool imprint(const browser_t & browser, imprint_t & result) const noexcept;
				/**
				 * @brief Метод парсинга данных цифрового отпечатка
				 *
				 * @param buffer  бинарный буфер данных цифрового отпечатка
				 * @param size    размер бинарного буфера данных цифрового отпечатка
				 * @param browser объект для хранения распарсенных данных цифрового отпечатка
				 * @return        результат парсинга данных цифрового отпечатка
				 */
				bool parse(const uint8_t * buffer, const size_t size, browser_t & browser) const noexcept;
				/**
				 * @brief Метод парсинга connection preface и начальных фреймов HTTP/2-соединения
				 *
				 * Разбирает бинарный буфер, содержащий HTTP/2 client connection preface (magic + начальные фреймы).
				 * Извлекает SETTINGS, WINDOW_UPDATE (stream 0), PRIORITY-фреймы и порядок псевдо-заголовков
				 * из первого HEADERS-фрейма для построения Akamai HTTP/2 fingerprint.
				 *
				 * @param buffer бинарный буфер с данными HTTP/2-соединения
				 * @param size   размер буфера в байтах
				 * @param h2     объект для хранения распарсенных данных
				 * @return       true если SETTINGS-фрейм был успешно разобран, иначе false
				 */
				bool parseH2(const uint8_t * buffer, const size_t size, h2_browser_t & h2) const noexcept;
			public:
				/**
				 * @brief Метод применения данных цифрового отпечатка на запрос ClientHello
				 *
				 * @param buffer  буфер с данными цифрового отпечатка для применения к запросу ClientHello
				 * @param size    размер буфера в байтах
				 * @param browser объект с распарсенными данными ClientHello
				 * @return        буфер с данными ClientHello, модифицированными в соответствии с цифровым отпечатком
				 */
				vector <uint8_t> apply(const uint8_t * buffer, const size_t size, const browser_t & browser) const noexcept;
			public:
				/**
				 * @brief Метод очистки всех цифровых отпечатков браузеров из хранилища
				 *
				 */
				void clear() noexcept;
			public:
				/**
				 * @brief Метод проверки, пусто ли хранилище цифровых отпечатков браузеров
				 *
				 * @return результат проверки
				 */
				bool empty() const noexcept;
			public:
				/**
				 * @brief Метод получения количества цифровых отпечатков браузеров, хранящихся в хранилище
				 *
				 * @return количество цифровых отпечатков браузеров
				 */
				size_t size() const noexcept;
			public:
				/**
				 * @brief Метод получения списка идентификаторов всех цифровых отпечатков браузеров, хранящихся в хранилище
				 *
				 * @return список идентификаторов цифровых отпечатков браузеров
				 */
				vector <id_t> list() const noexcept;
			public:
				/**
				 * @brief Метод удаления цифрового отпечатка браузера из хранилища по идентификатору
				 *
				 * @param id идентификатор цифрового отпечатка
				 * @return   результат выполнения удаления цифрового отпечатка
				 */
				bool remove(const id_t id) noexcept;
			public:
				/**
				 * @brief Метод получения данных цифрового отпечатка браузера по идентификатору
				 *
				 * @param id идентификатор цифрового отпечатка
				 * @return   объект с цифровым отпечатком браузера, соответствующий указанному идентификатору
				 */
				const browser_t & get(const id_t id) const noexcept;
			public:
				/**
				 * @brief Метод добавления цифрового отпечатка браузера в хранилище
				 *
				 * @param browser объект с распарсенными данными ClientHello
				 * @return        идентификатор добавленного цифрового отпечатка
				 */
				id_t add(const browser_t & browser) noexcept;
				/**
				 * @brief Метод добавления цифрового отпечатка браузера в хранилище в бинарном виде (дамп цифрового отпечатка)
				 *
				 * @param buffer бинарный буфер с данными цифрового отпечатка
				 * @param size   размер бинарного буфера в байтах
				 * @return       идентификатор добавленного цифрового отпечатка
				 */
				id_t add(const uint8_t * buffer, const size_t size) noexcept;
			public:
				/**
				 * @brief Метод формирования бинарного дампа всех цифровых отпечатков браузеров
				 *
				 * @return бинарный буфер, содержащий дамп всех цифровых отпечатков браузеров
				 */
				vector <uint8_t> dump() const noexcept;
				/**
				 * @brief Метод загрузки бинарного дампа всех цифровых отпечатков браузеров
				 *
				 * @param buffer бинарный буфер для загрузки данных цифровых отпечатков
				 * @return       результат загрузки бинарного дампа
				 */
				bool dump(const vector <uint8_t> & buffer) noexcept;
			public:
				/**
				 * @brief Метод загрузки бинарного дампа цифрового отпечатка
				 *
				 * @param input   бинарный буфер с данными цифрового отпечатка
				 * @param browser объект для хранения данных цифрового отпечатка
				 * @return        результат загрузки бинарного дампа цифрового отпечатка
				 */
				bool dump(const vector <uint8_t> & input, browser_t & browser) const noexcept;
				/**
				 * @brief Метод формирования бинарного дампа цифрового отпечатка браузера
				 *
				 * @param browser объект с распарсенными данными ClientHello
				 * @param output  буфер для записи бинарного дампа цифрового отпечатка
				 * @return        результат формирования бинарного дампа цифрового отпечатка
				 */
				bool dump(const browser_t & browser, vector <uint8_t> & output) const noexcept;
			public:
				/**
				 * @brief Метод обмена заголовками
				 *
				 * @param fgp объект Fingerprint для обмена данными
				 */
				void swap(Fingerprint & fgp) noexcept;
			public:
				/**
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор
				 */
				iterator_t end() noexcept;
				/**
				 * @brief Метод получение начального итератора
				 *
				 * @return начальный итератор
				 */
				iterator_t begin() noexcept;
			public:
				/**
				 * @brief Метод поиска указанного заголовка
				 *
				 * @param id идентификатор заголовка для поиска
				 * @return   итератор указанного заголовка
				 */
				iterator_t find(const id_t id) noexcept;
			public:
				/**
				 * @brief Оператор извлечения цифрового отпечатка браузера
				 *
				 * @param id идентификатор цифрового отпечатка
				 * @return   цифровой отпечаток браузера
				 */
				const browser_t & operator[](const id_t id) const noexcept;
			public:
				/**
				 * @brief Проверка, пусто ли хранилище цифровых отпечатков браузеров
				 *
				 * @return результат проверки
				 */
				operator bool() const noexcept;
				/**
				 * @brief Получения количества цифровых отпечатков браузеров, хранящихся в хранилище
				 *
				 * @return количество цифровых отпечатков браузеров
				 */
				operator size_t() const noexcept;
				/**
				 * @brief Получения бинарных данных дампа всех цифровых отпечатков браузеров
				 *
				 * @return бинарные данные буфера дампа всех цифровых отпечатков браузеров
				 */
				operator vector <uint8_t> () const noexcept;
			public:
				/**
				 * @brief Оператор сравнения двух контейнеров отпечатков браузеров
				 *
				 * @param fgp отпечатки браузеров для сравнения
				 * @return    результат сравнения
				 */
				bool operator == (const Fingerprint & fgp) const noexcept;
			public:
				/**
				 * @brief Оператор установки дампа цифрового отпечатка браузера
				 *
				 * @param buffer бинарный буфер для загрузки данных цифровых отпечатков
				 * @return       текущий контейнер отпечатков браузеров
				 */
				Fingerprint & operator = (const vector <uint8_t> & buffer) noexcept;
			public:
				/**
				 * @brief Оператор перемещения
				 *
				 * @param fgp объект Fingerprint для перемещения
				 * @return    текущий контейнер отпечатков браузеров
				 */
				Fingerprint & operator = (Fingerprint && fgp) noexcept;
				/**
				 * @brief Оператор копирования
				 *
				 * @param fgp объект Fingerprint для копирования
				 * @return    текущий контейнер отпечатков браузеров
				 */
				Fingerprint & operator = (const Fingerprint & fgp) noexcept;
			public:
				/**
				 * @brief Конструктор копирования
				 *
				 */
				explicit Fingerprint(const Fingerprint &) = delete;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 */
				explicit Fingerprint(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Деструктор
				 *
				 */
				~Fingerprint() noexcept;
		} fgp_t;
	};
};

#endif // __AWH_SSL_FINGERPRINT__
