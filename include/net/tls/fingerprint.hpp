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
		 * @brief Структура цифрового отпечатка устройства
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Fingerprint {
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
				 * @brief Структура расширения TLS для указания поддерживаемых алгоритмов подписи для сертификатов (Signature Algorithms for Certificates)
				 *
				 */
				typedef struct Extension_Signature_Algorithms_Cert : public extension_t {
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
					// Длина данных заполнения
					size_t length;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Padding() noexcept :
					 extension_t(extension_type_t::PADDING), length(0) {}
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
					// Данные расширения Extended Master Secret для сервера
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
					// Список поддерживаемых групп обмена ключами и соответствующих данных ключей
					unordered_map <group_t, vector <uint8_t>> keyShares;
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
					// Количество идентификаторов предварительно совместных ключей
					uint32_t count;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Pre_Shared_Key() noexcept :
					 extension_t(extension_type_t::PRE_SHARED_KEY), count(0) {}
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
					// Количество идентификаторов авторитетов сертификатов
					uint32_t count;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Certificate_Authorities() noexcept :
					 extension_t(extension_type_t::CERTIFICATE_AUTHORITIES), count(0) {}
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
				 * @brief Структура цифрового отпечатка браузера
				 *
				 */
				typedef struct Browser {
					// Флаг, указывающий на использование GREASE (Generate Random Extensions And Sustain Extensibility)
					bool grease;
					// Запись TLS рукопожатия
					record_t record;
					// Рукопожатие TLS
					handshake_t handshake;
					// ClientHello TLS
					client_hello_t clientHello;
					// Куки DTLS, если используется протокол DTLS
					vector <uint8_t> cookie;
					// Идентификатор сессии TLS
					vector <uint8_t> session;
					// Список поддерживаемых шифров
					vector <cipher_t> ciphers;
					// Список компрессоров, поддерживаемых браузером
					vector <compressor_t> compressors;
					// Список расширений, поддерживаемых браузером
					vector <unique_ptr <extension_t>> extensions;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Browser() noexcept : grease(false) {}
				} browser_t;
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
				 * @brief Метод парсинга данных цифрового отпечатка
				 *
				 * @param buffer  бинарный буфер данных цифрового отпечатка
				 * @param size    размер бинарного буфера данных цифрового отпечатка
				 * @param browser объект для хранения распарсенных данных цифрового отпечатка
				 * @return        результат парсинга данных цифрового отпечатка
				 */
				bool parse(const uint8_t * buffer, const size_t size, browser_t & browser) noexcept;
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
