/**
 * @file: fingerprint.hpp
 * @date: 2026-04-28
 * @license: LicenseRef-AWH-1.0
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
				 * @details Расширение TLS - это дополнительная информация, которая может быть включена в процесс установления соединения TLS.
				 *          Она позволяет клиенту и серверу обмениваться дополнительными данными, которые могут быть использованы для улучшения безопасности,
				 *          совместимости и функциональности соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension {
					// Тип расширения
					extension_type_t type;
					/**
					 * @brief Конструктор
					 *
					 * @param type Тип расширения
					 */
					explicit Extension(const extension_type_t type) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension() = default;
				} extension_t;

				/**
				 * @brief Структура расширения TLS для GREASE-значений
				 *
				 * @details GREASE (Generate Random Extensions And Sustain Extensibility) - это механизм,
				 *          используемый в протоколе TLS для обеспечения совместимости и предотвращения проблем с расширениями.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Grease : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Grease() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Grease() = default;
				} extension_grease_t;

				/**
				 * @brief Структура расширения TLS для идентификатора канала (Channel ID)
				 *
				 * @details Channel ID - это механизм, используемый в протоколе TLS для обеспечения уникальной идентификации канала связи между клиентом и сервером.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Channel_ID : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Channel_ID() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Channel_ID() = default;
				} extension_channel_id_t;

				/**
				 * @brief Структура расширения TLS для фильтров OID (OID Filters)
				 *
				 * @details OID Filters - это механизм, используемый в протоколе TLS для фильтрации и управления объектными идентификаторами (Object Identifiers, OID),
				 *          которые представляют собой уникальные идентификаторы для различных объектов и алгоритмов в криптографии и безопасности.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_OID_Filters : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_OID_Filters() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_OID_Filters() = default;
				} extension_oid_filters_t;

				/**
				 * @brief Структура расширения TLS для доверенных якорей (Trust Anchors)
				 *
				 * @details Trust Anchors - это доверенные корневые сертификаты или публичные ключи,
				 *          которые используются для проверки подлинности и доверия к цифровым сертификатам в протоколе TLS.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Trust_Anchors : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Trust_Anchors() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Trust_Anchors() = default;
				} extension_trust_anchors_t;

				/**
				 * @brief Структура расширения TLS для шифрования перед MAC (Encrypt-Then-MAC)
				 *
				 * @details Encrypt-Then-MAC - это механизм, используемый в протоколе TLS для обеспечения целостности и аутентичности данных,
				 *          передаваемых по защищённому каналу связи. Он предполагает, что данные сначала шифруются,
				 *          а затем к ним применяется механизм проверки целостности (MAC - Message Authentication Code).
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Encrypt_Then_MAC : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Encrypt_Then_MAC() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Encrypt_Then_MAC() = default;
				} extension_encrypt_then_mac_t;

				/**
				 * @brief Структура расширения TLS для информации о прозрачности (Transparency Info)
				 *
				 * @details Transparency Info - это механизм, используемый в протоколе TLS для обеспечения прозрачности и отслеживаемости сертификатов и ключей,
				 *          используемых в процессе установления соединения. Он позволяет клиенту и серверу обмениваться информацией о прозрачности,
				 *          такой как журналы сертификатов и ключей, что способствует повышению доверия и безопасности соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Transparency_Info : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Transparency_Info() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Transparency_Info() = default;
				} extension_transparency_info_t;

				/**
				 * @brief Структура расширения TLS для поддержки аутентификации после рукопожатия (Post-Handshake Authentication)
				 *
				 * @details Post-Handshake Authentication - это механизм, используемый в протоколе TLS для обеспечения дополнительной аутентификации после завершения процесса рукопожатия (handshake).
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Post_Handshake_Auth : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Post_Handshake_Auth() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Post_Handshake_Auth() = default;
				} extension_post_handshake_auth_t;

				/**
				 * @brief Структура расширения TLS для указания типа сертификата клиента (Client Certificate Type)
				 *
				 * @details Client Certificate Type - это механизм, используемый в протоколе TLS для указания типа сертификата, который клиент должен предоставить серверу для аутентификации.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Client_Certificate_Type : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Client_Certificate_Type() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Client_Certificate_Type() = default;
				} extension_client_certificate_type_t;

				/**
				 * @brief Структура расширения TLS для указания типа сертификата сервера (Server Certificate Type)
				 *
				 * @details Server Certificate Type - это механизм, используемый в протоколе TLS для указания типа сертификата, который сервер должен предоставить клиенту для аутентификации.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Server_Certificate_Type : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Server_Certificate_Type() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Server_Certificate_Type() = default;
				} extension_server_certificate_type_t;

				/**
				 * @brief Структура расширения TLS для указания имени сервера (SNI)
				 *
				 * @details Server Name Indication (SNI) - это расширение протокола TLS, которое позволяет клиенту указывать имя сервера,
				 *          к которому он пытается подключиться, во время процесса установления соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Server_Name : public extension_t {
					// Список имён серверов
					vector <string> names;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Server_Name() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Server_Name() = default;
				} extension_server_name_t;

				/**
				 * @brief Структура расширения TLS для запроса статуса сертификата (OCSP)
				 *
				 * @details Certificate Status Request - это расширение протокола TLS, которое позволяет клиенту запрашивать у сервера информацию о статусе сертификата,
				 *          например, с помощью протокола OCSP (Online Certificate Status Protocol). Это расширение позволяет клиенту проверять,
				 *          действителен ли сертификат сервера, и предотвращать использование недействительных или отозванных сертификатов.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Status_Request : public extension_t {
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
					explicit Extension_Status_Request() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Status_Request() = default;
				} extension_status_request_t;

				/**
				 * @brief Структура расширения TLS для указания поддерживаемых групп (Supported Groups)
				 *
				 * @details Supported Groups - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых группах эллиптических кривых (Elliptic Curve Groups),
				 *          используемых для криптографических операций, таких как обмен ключами и цифровая подпись.
				 *          Это расширение позволяет сторонам согласовывать, какие группы эллиптических кривых они поддерживают,
				 *          и выбирать наиболее подходящую группу для установления безопасного соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Supported_Groups : public extension_t {
					// Список поддерживаемых групп
					vector <group_t> supportedGroups;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Supported_Groups() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Supported_Groups() = default;
				} extension_supported_groups_t;

				/**
				 * @brief Структура расширения TLS для указания форматов точек эллиптической кривой (EC Point Formats)
				 *
				 * @details EC Point Formats - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых форматах точек эллиптической кривой (Elliptic Curve Point Formats),
				 *		    используемых для криптографических операций, таких как обмен ключами и цифровая подпись.
				 *          Это расширение позволяет сторонам согласовывать, какие форматы точек они поддерживают,
				 *          и выбирать наиболее подходящий формат для установления безопасного соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_EC_Point : public extension_t {
					// Список поддерживаемых форматов точек эллиптической кривой
					vector <ec_point_format_t> formats;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_EC_Point() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_EC_Point() = default;
				} extension_ec_point_t;

				/**
				 * @brief Структура расширения TLS для согласования протокола прикладного уровня (ALPN)
				 *
				 * @details Application-Layer Protocol Negotiation (ALPN) - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу согласовывать протокол прикладного уровня (например, HTTP/2, HTTP/3) во время процесса установления соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_ALPN : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_ALPN() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_ALPN() = default;
				} extension_alpn_t;

				/**
				 * @brief Структура расширения TLS для передачи настроек приложения (Application Settings)
				 *
				 * @details Application Settings - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться дополнительными настройками и параметрами, связанными с приложением, во время процесса установления соединения.
				 *          Это расширение может использоваться для передачи информации о поддерживаемых функциях, конфигурации и других аспектах приложения,
				 *          что позволяет сторонам согласовывать и оптимизировать взаимодействие на уровне приложения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Application_Settings : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Application_Settings() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Application_Settings() = default;
				} extension_application_settings_t;

				/**
				 * @brief Структура расширения TLS для передачи старых настроек приложения (Application Settings Old)
				 *
				 * @details Application Settings Old - это устаревшее расширение протокола TLS,
				 *          которое использовалось для передачи настроек и параметров приложения во время процесса установления соединения.
				 *          Оно было заменено новым расширением Application Settings, которое обеспечивает более современный и безопасный способ обмена информацией о настройках приложения между клиентом и сервером.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Application_Settings_Old : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Application_Settings_Old() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Application_Settings_Old() = default;
				} extension_application_settings_old_t;

				/**
				 * @brief Структура расширения TLS для согласования следующего протокола (Next Protocol Negotiation)
				 *
				 * @details Next Protocol Negotiation (NPN) - это устаревшее расширение протокола TLS,
				 *          которое позволяло клиенту и серверу согласовывать протокол прикладного уровня (например, HTTP/2) во время процесса установления соединения.
				 *          Оно было заменено новым расширением Application-Layer Protocol Negotiation (ALPN),
				 *          которое обеспечивает более современный и безопасный способ согласования протоколов прикладного уровня между клиентом и сервером.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Next_Proto_Neg : public extension_t {
					// Список поддерживаемых протоколов ALPN
					vector <string> protocols;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Next_Proto_Neg() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Next_Proto_Neg() = default;
				} extension_next_proto_neg_t;

				/**
				 * @brief Структура расширения TLS для передачи информации о подписанных временных метках сертификатов (Signed Certificate Timestamp)
				 *
				 * @details Signed Certificate Timestamp (SCT) - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о подписанных временных метках сертификатов (SCT),
				 *          используемых в механизме Certificate Transparency (CT).
				 *          SCT обеспечивает дополнительный уровень безопасности и прозрачности для сертификатов, позволяя клиенту проверять,
				 *          были ли сертификаты зарегистрированы в публичных журналах CT и не были ли они отозваны или скомпрометированы.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Signed_Certificate_Timestamp : public extension_t {
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Signed_Certificate_Timestamp() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Signed_Certificate_Timestamp() = default;
				} extension_signed_certificate_timestamp_t;

				/**
				 * @brief Структура расширения TLS для добавления произвольного количества байтов заполнения (Padding)
				 *
				 * @details Padding - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу добавлять произвольное количество байтов заполнения (padding) в процессе установления соединения.
				 *          Это расширение может использоваться для увеличения длины сообщений,
				 *          чтобы скрыть фактическую длину передаваемых данных и предотвратить определённые типы атак, такие как атаки на основе анализа длины сообщений.
				 *          Добавление байтов заполнения может улучшить безопасность соединения, делая его более устойчивым к анализу трафика и попыткам выявления передаваемой информации.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Padding : public extension_t {
					// Размер данных заполнения
					size_t size;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Padding() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Padding() = default;
				} extension_padding_t;

				/**
				 * @brief Структура расширения TLS для использования расширенного мастер-секрета (Extended Master Secret)
				 *
				 * @details Extended Master Secret (EMS) - это расширение протокола TLS,
				 *          которое обеспечивает дополнительный уровень безопасности для процесса установления соединения.
				 *          Оно предотвращает определённые типы атак, такие как атаки на основе повторного использования мастер-секрета (replay attacks) и атаки на основе анализа длины сообщений (length extension attacks).
				 *          EMS гарантирует, что мастер-секрет, используемый для генерации ключей шифрования, зависит от всех параметров сеанса TLS,
				 *          включая идентификаторы клиента и сервера, что делает его уникальным для каждого сеанса и предотвращает возможность повторного использования мастер-секрета в других сеансах.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Extended_Master_Secret : public extension_t {
					// Данные расширения Master Secret
					string masterSecretData;
					// Данные расширения Extended Master Secret для сервера (устаревшее расширение)
					string extendedMasterSecretData;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Extended_Master_Secret() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Extended_Master_Secret() = default;
				} extension_extended_master_secret_t;

				/**
				 * @brief Структура расширения TLS для сжатия сертификатов (Compress Certificate)
				 *
				 * @details Compress Certificate - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу использовать алгоритмы сжатия для уменьшения размера передаваемых сертификатов во время процесса установления соединения.
				 *          Это расширение может улучшить производительность и снизить задержку при передаче сертификатов, особенно в случаях, когда сертификаты имеют большой размер или когда соединение осуществляется через медленные сети.
				 *          Использование сжатия сертификатов может также снизить потребление полосы пропускания и улучшить общую эффективность процесса установления соединения TLS.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Compress_Certificate : public extension_t {
					// Список поддерживаемых алгоритмов сжатия сертификатов
					vector <compressor_t> algorithms;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Compress_Certificate() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Compress_Certificate() = default;
				} extension_compress_certificate_t;

				/**
				 * @brief Структура расширения TLS для использования билета сессии (Session Ticket)
				 *
				 * @details Session Ticket - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу использовать билеты сессии для восстановления ранее установленных соединений без необходимости повторного выполнения полного процесса рукопожатия (handshake).
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Session_Ticket : public extension_t {
					// Данные расширения Session Ticket
					vector <uint8_t> data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Session_Ticket() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Session_Ticket() = default;
				} extension_session_ticket_t;

				/**
				 * @brief Структура расширения TLS для указания поддерживаемых версий (Supported Versions)
				 *
				 * @details Supported Versions - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых версиях протокола TLS во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какую версию протокола TLS они будут использовать для установления безопасного соединения, обеспечивая совместимость и улучшая безопасность соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Supported_Versions : public extension_t {
					// Список поддерживаемых версий
					vector <version_t> versions;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Supported_Versions() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Supported_Versions() = default;
				} extension_supported_versions_t;

				/**
				 * @brief Структура расширения TLS для указания режимов обмена ключами PSK (PSK Key Exchange Modes)
				 *
				 * @details PSK Key Exchange Modes - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых режимах обмена ключами с использованием предварительно совместного ключа (Pre-Shared Key, PSK) во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какой режим обмена ключами PSK они будут использовать для установления безопасного соединения, обеспечивая совместимость и улучшая безопасность соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_PSK_Key_Exchange : public extension_t {
					// Список поддерживаемых версий
					vector <psk_key_t> modes;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_PSK_Key_Exchange() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_PSK_Key_Exchange() = default;
				} extension_psk_key_exchange_t;

				/**
				 * @brief Структура расширения TLS для поддержки ранних данных (Early Data)
				 *
				 * @details Early Data - это расширение протокола TLS,
				 *          которое позволяет клиенту отправлять данные на сервер до завершения процесса установления соединения (handshake).
				 *          Это расширение может улучшить производительность и снизить задержку при установлении соединения, особенно в случаях,
				 *          когда клиент и сервер уже имеют предварительно совместный ключ (Pre-Shared Key, PSK) и могут безопасно обмениваться данными без необходимости повторного выполнения полного процесса рукопожатия.
				 *          Использование ранних данных может также снизить потребление полосы пропускания и улучшить общую эффективность процесса установления соединения TLS.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Early_Data : public extension_t {
					// Максимальный размер ранних данных
					uint32_t maxSize;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Early_Data() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Early_Data() = default;
				} extension_early_data_t;

				/**
				 * @brief Структура расширения TLS для обмена ключами (Key Share)
				 *
				 * @details Key Share - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о ключах для установления безопасного соединения с использованием алгоритмов обмена ключами,
				 *          таких как Диффи-Хеллман (Diffie-Hellman) или эллиптические кривые (Elliptic Curve Cryptography, ECC).
				 *          Это расширение позволяет сторонам согласовывать, какие ключи они будут использовать для установления безопасного соединения,
				 *          обеспечивая совместимость и улучшая безопасность соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Key_Share : public extension_t {
					// Список групп обмена ключами в порядке появления в ClientHello (порядок важен — отражает предпочтение клиента)
					vector <pair <group_t, vector <uint8_t>>> shares;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Key_Share() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Key_Share() = default;
				} extension_key_share_t;

				/**
				 * @brief Структура расширения TLS для передачи зашифрованного ClientHello (Encrypted ClientHello)
				 *
				 * @details Encrypted ClientHello - это расширение протокола TLS,
				 *          которое позволяет клиенту отправлять зашифрованный ClientHello на сервер для обеспечения дополнительной конфиденциальности и защиты от атак на основе анализа трафика.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Encryption_Client_Hello : public extension_t {
					// Данные зашифрованного ClientHello
					vector <uint8_t> data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Encryption_Client_Hello() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Encryption_Client_Hello() = default;
				} extension_encryption_client_hello_t;

				/**
				 * @brief Структура расширения TLS для передачи информации о повторной договоренности (Renegotiation Info)
				 *
				 * @details Renegotiation Info - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о повторной договоренности (renegotiation) во время процесса установления соединения.
				 *          Это расширение обеспечивает защиту от атак на основе повторной договоренности,
				 *          таких как атаки на основе повторного использования сеансов (session replay attacks) и атаки на основе анализа трафика (traffic analysis attacks).
				 *          Использование информации о повторной договоренности позволяет сторонам согласовывать,
				 *          какие параметры сеанса они будут использовать для установления безопасного соединения, обеспечивая совместимость и улучшая безопасность соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Renegotiation_Info : public extension_t {
					// Данные зашифрованного ClientHello
					vector <uint8_t> data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Renegotiation_Info() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Renegotiation_Info() = default;
				} extension_renegotiation_info_t;

				/**
				 * @brief Структура расширения TLS для указания максимального размера записи (Record Size Limit)
				 *
				 * @details Record Size Limit - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о максимальном размере записи (record size limit) во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какой максимальный размер записи они будут использовать для установления безопасного соединения,
				 *          обеспечивая совместимость и улучшая безопасность соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Record_Size_Limit : public extension_t {
					// Данные расширения Record Size Limit
					uint16_t data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Record_Size_Limit() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Record_Size_Limit() = default;
				} extension_record_size_limit_t;

				/**
				 * @brief Структура расширения DTLS для передачи cookie (Cookie)
				 *
				 * @details Cookie - это расширение протокола DTLS (Datagram Transport Layer Security),
				 *          которое позволяет клиенту и серверу обмениваться cookie во время процесса установления соединения.
				 *          Cookie используется для защиты от атак на основе подделки IP-адресов (IP address spoofing) и для предотвращения атак на основе повторного использования сеансов (session replay attacks).
				 *          Использование cookie позволяет серверу проверять подлинность клиента и предотвращать установление соединений с недействительными или поддельными клиентами, обеспечивая безопасность и целостность процесса установления соединения DTLS.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Cookie : public extension_t {
					// Данные расширения Cookie
					vector <uint8_t> data;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Cookie() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Cookie() = default;
				} extension_cookie_t;

				/**
				 * @brief Структура расширения TLS для использования предварительно совместного ключа (Pre-Shared Key)
				 *
				 * @details Pre-Shared Key (PSK) - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу использовать предварительно совместный ключ (PSK) для установления безопасного соединения.
				 *          PSK обеспечивает дополнительный уровень безопасности и конфиденциальности,
				 *          позволяя сторонам обмениваться ключами и данными без необходимости использования сертификатов и инфраструктуры открытых ключей (Public Key Infrastructure, PKI).
				 *          Использование PSK может улучшить производительность и снизить задержку при установлении соединения, особенно в случаях,
				 *          когда клиент и сервер уже имеют предварительно совместный ключ и могут безопасно обмениваться данными без необходимости повторного выполнения полного процесса рукопожатия (handshake).
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Pre_Shared_Key : public extension_t {
					/**
					 * @brief Структура идентификатора предварительно совместного ключа (PSK Identity)
					 *
					 */
					struct __AWH_SHARED_EXPORT__ Identity {
						// Время жизни билета (Ticket Age)
						uint32_t ticketAge;
						// Идентификатор предварительно совместного ключа
						vector <uint8_t> data;
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Identity() noexcept;
					};
					// Список идентификаторов предварительно совместных ключей
					vector <Identity> identities;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Pre_Shared_Key() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Pre_Shared_Key() = default;
				} extension_pre_shared_key_t;

				/**
				 * @brief Структура расширения TLS для указания доверенных центров сертификации (Certificate Authorities)
				 *
				 * @details Certificate Authorities (CAs) - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о доверенных центрах сертификации (Certificate Authorities, CAs) во время процесса установления соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Certificate_Authorities : public extension_t {
					// Список DER-кодированных Distinguished Name (DistinguishedName) из RFC 8446 §4.2.4
					vector <vector <uint8_t>> authorities;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Certificate_Authorities() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Certificate_Authorities() = default;
				} extension_certificate_authorities_t;

				/**
				 * @brief Структура расширения TLS для указания максимальной длины фрагмента (Max Fragment Length)
				 *
				 * @details Max Fragment Length - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о максимальной длине фрагмента (max fragment length) во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какой максимальный размер фрагмента они будут использовать для установления безопасного соединения,
				 *          обеспечивая совместимость и улучшая безопасность соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Max_Fragment_Length : public extension_t {
					// Максимальная длина фрагмента
					uint16_t length;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Max_Fragment_Length() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Max_Fragment_Length() = default;
				} extension_max_fragment_length_t;

				/**
				 * @brief Структура расширения TLS для использования SRTP (Use SRTP)
				 *
				 * @details Use SRTP - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых профилях Secure Real-time Transport Protocol (SRTP) во время процесса установления соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Use_SRTP : public extension_t {
					// Длина Master Key Identifier (MKI)
					uint8_t mkiLength;
					// Список поддерживаемых профилей SRTP
					vector <srtp_t> profiles;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Use_SRTP() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Use_SRTP() = default;
				} extension_use_srtp_t;

				/**
				 * @brief Структура расширения TLS для поддержки механизма heartbeat (Heartbeat)
				 *
				 * @details Heartbeat - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддержке механизма heartbeat во время процесса установления соединения.
				 *          Механизм heartbeat используется для проверки доступности и поддержания соединения между клиентом и сервером,
				 *          а также для обнаружения потери соединения. Использование heartbeat может улучшить производительность и надежность соединения,
				 *          позволяя сторонам своевременно обнаруживать проблемы с соединением и предпринимать соответствующие действия для их устранения,
				 *          обеспечивая стабильность и качество обслуживания в сетевых приложениях.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Heartbeat : public extension_t {
					// Режим heartbeat (например, peer_allowed_to_send или peer_not_allowed_to_send)
					heartbeat_t mode;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Heartbeat() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Heartbeat() = default;
				} extension_heartbeat_t;

				/**
				 * @brief Структура расширения TLS для указания поддерживаемых алгоритмов подписи (Signature Algorithms)
				 *
				 * @details Signature Algorithms - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых алгоритмах подписи во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какие алгоритмы подписи они будут использовать для установления безопасного соединения,
				 *          обеспечивая совместимость и улучшая безопасность соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Signature : public extension_t {
					// Список поддерживаемых алгоритмов подписи
					vector <signature_t> algorithms;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Signature() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Signature() = default;
				} extension_signature_t;

				/**
				 * @brief Структура расширения TLS для использования делегированных учетных данных (Delegated Credential)
				 *
				 * @details Delegated Credential - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых алгоритмах подписи для делегированных учетных данных во время процесса установления соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Delegated_Credential : public extension_t {
					// Список поддерживаемых алгоритмов подписи для делегированных учетных данных
					vector <signature_t> algorithms;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Delegated_Credential() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Delegated_Credential() = default;
				} extension_delegated_credential_t;

				/**
				 * @brief Структура расширения TLS для указания поддерживаемых алгоритмов подписи для сертификатов (Signature Algorithms for Certificates)
				 *
				 * @details Signature Algorithms for Certificates - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых алгоритмах подписи для сертификатов во время процесса установления соединения.
				 *          Это расширение позволяет сторонам согласовывать, какие алгоритмы подписи они будут использовать для проверки подлинности сертификатов, обеспечивая совместимость и улучшая безопасность соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Signature_Algorithms_Cert : public extension_t {
					// Список поддерживаемых алгоритмов подписи для сертификатов
					vector <signature_t> algorithms;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Signature_Algorithms_Cert() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Signature_Algorithms_Cert() = default;
				} extension_signature_algorithms_cert_t;

				/**
				 * @brief Структура расширения TLS для передачи информации о прозрачности (Transparency Info)
				 *
				 * @details Transparency Info - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о прозрачности (transparency) во время процесса установления соединения.
				 *          Это расширение может использоваться для передачи информации о поддерживаемых механизмах прозрачности,
				 *          таких как Certificate Transparency (CT), позволяя сторонам проверять подлинность сертификатов и обеспечивать дополнительный уровень безопасности и доверия при установлении соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_TLS_Flags : public extension_t {
					// Список флагов или параметров, специфичных для реализации TLS
					vector <uint8_t> flags;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_TLS_Flags() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_TLS_Flags() = default;
				} extension_tls_flags_t;

				/**
				 * @brief Структура расширения TLS для передачи параметров транспорта QUIC (QUIC Transport Parameters)
				 *
				 * @details QUIC Transport Parameters - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о параметрах транспорта QUIC во время процесса установления соединения.
				 *          Эти параметры могут включать информацию о поддерживаемых версиях QUIC, максимальных размерах пакетов, тайм-аутах и других настройках,
				 *          которые могут быть согласованы между клиентом и сервером для оптимизации работы протокола QUIC.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Quic_Transport_Params : public extension_t {
					/**
					 * @brief Список параметров транспорта QUIC в виде пар ключ-значение,
					 *        где ключ и значение представлены в виде 64-битных целых чисел.
					 *        Эти параметры могут включать информацию о поддерживаемых версиях QUIC,
					 *        максимальных размерах пакетов, тайм-аутах и других настройках,
					 *        которые могут быть согласованы между клиентом и сервером во время TLS-рукопожатия для оптимизации работы протокола QUIC.
					 *
					 */
					unordered_map <uint64_t, uint64_t> params;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Quic_Transport_Params() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Quic_Transport_Params() = default;
				} extension_quic_transport_params_t;

				/**
				 * @brief Структура расширения TLS для передачи параметров транспорта QUIC (QUIC Transport Parameters Legacy)
				 *
				 * @details QUIC Transport Parameters Legacy - это устаревшее расширение протокола TLS,
				 *          которое позволяло клиенту и серверу обмениваться информацией о параметрах транспорта QUIC во время процесса установления соединения.
				 *          Эти параметры могут включать информацию о поддерживаемых версиях QUIC, максимальных размерах пакетов,
				 *          тайм-аутах и других настройках, которые могли быть согласованы между клиентом и сервером для оптимизации работы протокола QUIC.
				 *          Это расширение было заменено новым расширением QUIC Transport Parameters,
				 *          которое обеспечивает более современный и безопасный способ обмена информацией о параметрах транспорта QUIC между клиентом и сервером.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_Quic_Transport_Params_Legacy : public extension_t {
					/**
					 * @brief Список параметров транспорта QUIC в виде пар ключ-значение,
					 *        где ключ и значение представлены в виде 64-битных целых чисел.
					 *        Эти параметры могут включать информацию о поддерживаемых версиях QUIC,
					 *        максимальных размерах пакетов, тайм-аутах и других настройках,
					 *        которые могут быть согласованы между клиентом и сервером во время TLS-рукопожатия для оптимизации работы протокола QUIC.
					 *
					 */
					unordered_map <uint64_t, uint64_t> params;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_Quic_Transport_Params_Legacy() noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					virtual ~Extension_Quic_Transport_Params_Legacy() = default;
				} extension_quic_transport_params_legacy_t;

				/**
				 * @brief Структура расширения TLS для передачи внешних расширений ECH (ECH Outer Extensions)
				 *
				 * @details ECH Outer Extensions - это расширение протокола TLS,
				 *          которое позволяет клиенту и серверу обмениваться информацией о внешних расширениях Encrypted ClientHello (ECH) во время процесса установления соединения.
				 *          Эти расширения могут включать информацию о поддерживаемых версиях ECH, алгоритмах шифрования, параметрах обмена ключами и других настройках,
				 *          которые могут быть согласованы между клиентом и сервером для обеспечения конфиденциальности и защиты от атак на основе анализа трафика.
				 *          Использование внешних расширений ECH позволяет сторонам согласовывать, какие расширения они будут использовать для установления безопасного соединения с использованием ECH,
				 *          обеспечивая совместимость и улучшая безопасность соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Extension_ECH_Outer_Extensions : public extension_t {
					// Список расширений, которые были зашифрованы в рамках ECH и переданы во внешнем расширении ECH Outer Extensions
					vector <extension_type_t> extensions;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Extension_ECH_Outer_Extensions() noexcept;
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
				 * @details Структура содержит информацию о версии протокола TLS,
				 *          используемой на уровне записи (record layer) и согласованной версии TLS,
				 *          которая была выбрана в процессе рукопожатия.
				 */
				typedef struct __AWH_SHARED_EXPORT__ VersionTLS {
					// Версия TLS на уровне записи (wire-код в десятичном виде, например "769")
					string record;
					// Согласованная версия TLS (наибольшая не-GREASE версия из supported_versions, десятично)
					string negotiated;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit VersionTLS() noexcept;
				} version_tls_t;

				/**
				 * @brief Структура вычисленных цифровых отпечатков TLS-соединения (JA3, JA4, PeetPrint и пр.)
				 *
				 * @details Структура содержит информацию о вычисленных цифровых отпечатках TLS-соединения,
				 *          таких как JA3, JA4, PeetPrint и другие, которые используются для идентификации и анализа TLS-соединений.
				 *          Эти отпечатки позволяют определить характеристики TLS-соединения, включая поддерживаемые версии протокола,
				 *          шифры, расширения и другие параметры, что может быть полезно для анализа безопасности, мониторинга трафика и выявления аномалий в сетевых соединениях.
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
					 * @brief Конструктор
					 *
					 */
					explicit Imprint() noexcept;
				} imprint_t;
			public:
				/**
				 * @brief Структура записи TLS
				 *
				 * @details Структура содержит информацию о записи TLS, включая эпоху записи DTLS, длину записи TLS,
				 *          порядковый номер записи, версию протокола TLS, поддерживаемую браузером,
				 *          и другие параметры, которые используются для идентификации и анализа TLS-соединений.
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
					 * @brief Конструктор
					 *
					 */
					explicit Record() noexcept;
				} __attribute__((packed)) record_t;

				/**
				 * @brief Структура фрагмента TLS
				 *
				 * @details Структура содержит информацию о фрагменте TLS,
				 *          включая смещение фрагмента в рамках записи TLS и длину фрагмента TLS.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Fragment {
					// Смещение фрагмента в рамках записи TLS
					uint32_t offset;
					// Длина фрагмента TLS
					uint32_t length;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Fragment() noexcept;
				} __attribute__((packed)) fragment_t;

				/**
				 * @brief Структура рукопожатия TLS
				 *
				 * @details Структура содержит информацию о рукопожатии TLS,
				 *          включая длину рукопожатия, порядковый номер рукопожатия и фрагмент рукопожатия TLS,
				 *          которые используются для идентификации и анализа TLS-соединений.
				 */
				typedef struct __AWH_SHARED_EXPORT__ Handshake {
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
					explicit Handshake() noexcept;
				} __attribute__((packed)) handshake_t;

				/**
				 * @brief Структура ClientHello TLS
				 *
				 * @details Структура содержит информацию о ClientHello TLS,
				 *          включая версию протокола TLS, поддерживаемую браузером в рукопожатии,
				 *          и случайные байты, которые используются для идентификации и анализа TLS-соединений.
				 */
				typedef struct __AWH_SHARED_EXPORT__ ClientHello {
					// Версия протокола TLS, поддерживаемая браузером в рукопожатии
					version_t version;
					// 32 байта случайных байта в ClientHello
					array <uint8_t, 32> random;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit ClientHello() noexcept;
				} client_hello_t;

				/**
				 * @brief Класс цифрового отпечатка браузера
				 *
				 * @details Класс содержит информацию о цифровом отпечатке браузера,
				 *          включая флаг использования GREASE, запись метаданных TLS рукопожатия, объект рукопожатия TLS,
				 *          объект ClientHello TLS, куки рукопожатия DTLS, идентификатор сессии TLS, список поддерживаемых шифров,
				 *          список компрессоров, поддерживаемых браузером, и список расширений, поддерживаемых браузером.
				 *          Этот класс используется для идентификации и анализа цифровых отпечатков браузеров,
				 *          что может быть полезно для анализа безопасности, мониторинга трафика и выявления аномалий в сетевых соединениях.
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
						explicit Browser() noexcept;
				} browser_t;
			public:
				/**
				 * @brief Одна запись параметра SETTINGS из HTTP/2-соединения (RFC 7540 §6.5)
				 *
				 * @details Структура содержит информацию о параметре SETTINGS из HTTP/2-соединения,
				 *          включая идентификатор параметра и его значение, которые используются для настройки поведения HTTP/2-соединения.
				 *          Эти параметры могут включать информацию о размере таблицы заголовков, поддержке push-уведомлений,
				 *          максимальном количестве одновременных потоков, начальном размере окна, максимальном размере фрейма и максимальном размере списка заголовков.
				 *          Использование этих параметров позволяет сторонам согласовывать настройки HTTP/2-соединения, обеспечивая совместимость и улучшая производительность и безопасность соединения.
				 */
				typedef struct __AWH_SHARED_EXPORT__ H2Setting {
					/**
					 * @details Идентификатор параметра (1=HEADER_TABLE_SIZE, 2=ENABLE_PUSH, 3=MAX_CONCURRENT_STREAMS,
					 *          4=INITIAL_WINDOW_SIZE, 5=MAX_FRAME_SIZE, 6=MAX_HEADER_LIST_SIZE).
					 */
					uint16_t id;
					// Значение параметра
					uint32_t value;
					/**
					 * @brief Конструктор
					 *
					 * @param i идентификатор параметра
					 * @param v значение параметра
					 */
					explicit H2Setting(const uint16_t i = 0, const uint32_t v = 0) noexcept;
				} __attribute__((packed)) h2_setting_t;

				/**
				 * @brief Данные PRIORITY-фрейма HTTP/2 (RFC 7540 §6.3)
				 *
				 * @details Структура содержит информацию о PRIORITY-фрейме HTTP/2,
				 *          включая флаг эксклюзивной зависимости, вес приоритета,
				 *          идентификатор потока, которому задаётся приоритет, и идентификатор потока-зависимости.
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
					 * @brief Конструктор
					 *
					 */
					explicit H2Priority() noexcept;
				} __attribute__((packed)) h2_priority_t;

				/**
				 * @brief Данные HTTP/2-соединения клиента, собранные из connection preface
				 *
				 * @details Содержит параметры, необходимые для вычисления Akamai HTTP/2 fingerprint:
				 *          SETTINGS-фрейм, WINDOW_UPDATE уровня соединения, PRIORITY-фреймы
				 *          и порядок псевдо-заголовков из первого HEADERS-фрейма.
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
					 * @brief Конструктор
					 *
					 */
					explicit H2Browser() noexcept;
				} h2_browser_t;
			public:
				/**
				 * @brief Итератор как вложенный класс
				 *
				 * @details Итератор используется для обхода элементов контейнера unordered_map <uint8_t, browser_t>,
				 *          который хранит цифровые отпечатки браузеров. Итератор предоставляет интерфейс для доступа к элементам контейнера,
				 *          включая операции разыменования, сравнения и инкрементации.
				 *          Итератор позволяет пользователю перебирать элементы контейнера в порядке их хранения,
				 *          обеспечивая удобный способ работы с цифровыми отпечатками браузеров.
				 */
				typedef class __AWH_SHARED_EXPORT__ Iterator {
					public:
						/**
						 * @brief Создаём необходимые нам типы данных
						 *
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
				const fmk_t * _fmk;
				// Объект работы с логами
				const log_t * _log;
			public:
				/**
				 * @brief Метод форматированного вывода всех данных цифрового отпечатка браузера
				 *
				 * @details Распечатывает в читаемом текстовом виде все поля browser_t (Record Layer,
				 *          Handshake, ClientHello, Cipher Suites, Compressors, Extensions), а также
				 *          вычисляет и печатает все отпечатки imprint_t (JA3, JA4, JA4_r, PeetPrint).
				 *
				 * @param browser объект с распарсенными данными ClientHello
				 * @return        форматированная строка с полным описанием отпечатка
				 */
				string print(const browser_t & browser) const noexcept;
			public:
				/**
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
				 * @details Разбирает бинарный буфер, содержащий HTTP/2 client connection preface (magic + начальные фреймы).
				 *          Извлекает SETTINGS, WINDOW_UPDATE (stream 0), PRIORITY-фреймы и порядок псевдо-заголовков
				 *          из первого HEADERS-фрейма для построения Akamai HTTP/2 fingerprint.
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
				 *
				 * @note Возвращаемый буфер — временный объект. Указатель на его данные
				 *       действителен только до конца полного выражения вызова (до возврата
				 *       из синхронного read_callback). Callback обязан скопировать данные.
				 * @note buffer должен содержать полный TLS/DTLS record layer; при неполной
				 *       записи метод возвращает пустой vector.
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
				 * @brief Метод установки режима потокобезопасности хранилища отпечатков
				 *
				 * @param mode флаг режима безопасности потоков
				 */
				void threadSafety(const bool mode) noexcept;
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
