/**
 * @file: tls.hpp
 * @date: 2026-04-30
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
#ifndef __AWH_TLS__
#define __AWH_TLS__

/**
 * Стандартные модули
 */
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

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
		 * @brief Типы компрессоров TLS
		 *
		 */
		enum class compressor_t : uint8_t {
			NONE    = 0x00, // Сжатие не используется
			ZLIB    = 0x01, // Сжатие zlib
			ZSTD    = 0x02, // Сжатие Zstandard
			BROTLI  = 0x03, // Сжатие Brotli
			UNKNOWN = 0xFF  // Неизвестный или нераспознанный алгоритм сжатия
		};

		/**
		 * @brief Типы ключей обмена TLS
		 *
		 */
		enum class psk_key_t : uint8_t {
			PSK_ONLY = 0x00, // Только PSK-ключи
			PSK_DHE  = 0x01, // PSK с (EC)DHE-ключами
			UNKNOWN  = 0xFF  // Неизвестный или нераспознанный режим обмена ключами PSK
		};

		/**
		 * @brief Тип формата точек эллиптической кривой TLS
		 *
		 */
		enum class ec_point_format_t : uint8_t {
			UNCOMPRESSED = 0x00, // Не сжатый формат
			ANSIX962     = 0x01, // Сжатый формат ANSI X9.62
			ANSIX962_2   = 0x02, // Сжатый формат ANSI X9.62 с использованием битовой маски
			UNKNOWN      = 0xFF  // Неизвестный или нераспознанный формат точек
		};

		/**
		 * @brief Тип версии TLS
		 *
		 */
		enum class version_t : uint8_t {
			UNKNOWN  = 0x00, // Неизвестная или нераспознанная версия TLS.
			GREASE   = 0x01, // Специальный код для обозначения GREASE-значений, которые не являются реальными версиями, а используются для тестирования устойчивости TLS-стека к неизвестным значениям.
			SSL_V3   = 0x02, // Версия SSLv3.
			TLS_1_0  = 0x03, // Версия TLS 1.0.
			TLS_1_1  = 0x04, // Версия TLS 1.1.
			TLS_1_2  = 0x05, // Версия TLS 1.2.
			TLS_1_3  = 0x06, // Версия TLS 1.3.
			DTLS_1_0 = 0x07, // Версия DTLS 1.0.
			DTLS_1_2 = 0x08  // Версия DTLS 1.2.
		};

		/**
		 * @brief Типы криптографических профилей SRTP для TLS
		 *
		 */
		enum class srtp_t : uint8_t {
			UNKNOWN                = 0x00,
			AEAD_AES_128_GCM       = 0x01,
			AEAD_AES_256_GCM       = 0x02,
			NULL_HMAC_SHA1_80      = 0x03,
			NULL_HMAC_SHA1_32      = 0x04,
			AES128_CM_HMAC_SHA1_80 = 0x05,
			AES128_CM_HMAC_SHA1_32 = 0x06,
			AES128_F8_HMAC_SHA1_80 = 0x07
		};

		/**
		 * @brief Типы режимов поддержки активного подключения (heartbeat) в TLS
		 *
		 */
		enum class heartbeat_t : uint8_t {
			UNKNOWN                  = 0x00, // Неизвестный или нераспознанный режим heartbeat
			PEER_ALLOWED_TO_SEND     = 0x01, // Режим, при котором обе стороны могут отправлять heartbeat-запросы
			PEER_NOT_ALLOWED_TO_SEND = 0x02  // Режим, при котором только одна сторона может отправлять heartbeat-запросы
		};

		/**
		 * @brief Тип доступных шифров TLS
		 *
		 */
		enum class cipher_t : uint8_t {
			UNKNOWN                       = 0x00, // Неизвестный или нераспознанный шифр.
			GREASE                        = 0x01, // Специальный код для обозначения GREASE-значений, которые не являются реальными шифрами, а используются для тестирования устойчивости TLS-стека к неизвестным значениям.
			AES128_SHA                    = 0x02, // Шифр TLS 1.2, поддерживающий AES128-SHA.
			AES256_SHA                    = 0x03, // Шифр TLS 1.2, поддерживающий AES256-SHA.
			AES128_GCM_SHA256             = 0x04, // Шифр TLS 1.2, поддерживающий AES128-GCM-SHA256.
			AES256_GCM_SHA384             = 0x05, // Шифр TLS 1.2, поддерживающий AES256-GCM-SHA384.
			PSK_AES128_CBC_SHA            = 0x06, // Шифр TLS 1.2, поддерживающий PSK и AES128-SHA CBC и SHA.
			PSK_AES256_CBC_SHA            = 0x07, // Шифр TLS 1.2, поддерживающий PSK и AES256-SHA CBC и SHA.
			ECDHE_RSA_AES128_SHA          = 0x08, // Шифр TLS 1.2, поддерживающий ECDHE RSA и AES128-SHA.
			ECDHE_RSA_AES256_SHA          = 0x09, // Шифр TLS 1.2, поддерживающий ECDHE RSA и AES256-SHA.
			ECDHE_ECDSA_AES128_SHA        = 0x0A, // Шифр TLS 1.2, поддерживающий ECDHE ECDSA и AES128-SHA.
			ECDHE_ECDSA_AES256_SHA        = 0x0B, // Шифр TLS 1.2, поддерживающий ECDHE ECDSA и AES256-SHA.
			ECDHE_RSA_AES128_SHA256       = 0x0C, // Шифр TLS 1.2, поддерживающий ECDHE RSA и AES128-SHA256.
			ECDHE_PSK_AES128_CBC_SHA      = 0x0D, // Шифр TLS 1.2, поддерживающий ECDHE PSK и AES128-SHA CBC и SHA.
			ECDHE_PSK_AES256_CBC_SHA      = 0x0E, // Шифр TLS 1.2, поддерживающий ECDHE PSK и AES256-SHA CBC и SHA.
			ECDHE_ECDSA_AES128_SHA256     = 0x0F, // Шифр TLS 1.2, поддерживающий ECDHE ECDSA и AES128-SHA256.
			ECDHE_RSA_AES128_GCM_SHA256   = 0x10, // Шифр TLS 1.2, поддерживающий ECDHE RSA и AES128-GCM-SHA256.
			ECDHE_RSA_AES256_GCM_SHA384   = 0x11, // Шифр TLS 1.2, поддерживающий ECDHE RSA и AES256-GCM-SHA384.
			ECDHE_RSA_CHACHA20_POLY1305   = 0x12, // Шифр TLS 1.2, поддерживающий ECDHE RSA и ChaCha20-Poly1305.
			ECDHE_PSK_CHACHA20_POLY1305   = 0x13, // Шифр TLS 1.2, поддерживающий ECDHE PSK и ChaCha20-Poly1305.
			ECDHE_ECDSA_AES128_GCM_SHA256 = 0x14, // Шифр TLS 1.2, поддерживающий ECDHE ECDSA и AES128-GCM-SHA256.
			ECDHE_ECDSA_AES256_GCM_SHA384 = 0x15, // Шифр TLS 1.2, поддерживающий ECDHE ECDSA и AES256-GCM-SHA384.
			ECDHE_ECDSA_CHACHA20_POLY1305 = 0x16, // Шифр TLS 1.2, поддерживающий ECDHE ECDSA и ChaCha20-Poly1305.
			TLS_AES_256_GCM_SHA384        = 0x17, // Шифр TLS 1.3, поддерживающий AES-256-GCM-SHA384.
			TLS_AES_128_GCM_SHA256        = 0x18, // Шифр TLS 1.3, поддерживающий AES-128-GCM-SHA256.
			TLS_CHACHA20_POLY1305_SHA256  = 0x19  // Шифр TLS 1.3, поддерживающий ChaCha20-Poly1305-SHA256.
		};

		/**
		 * @brief Тип группы TLS-сессий
		 *
		 */
		enum class group_t : uint8_t {
			UNKNOWN                 = 0x00, // Неизвестная или нераспознанная группа.
			GREASE                  = 0x01, // Специальный код для обозначения GREASE-значений, которые не являются реальными группами, а используются для тестирования устойчивости TLS-стека к неизвестным значениям.
			P_256                   = 0x02, // Эллиптическая кривая, относится к стандартам NIST (National Institute of Standards and Technology) и применяется в алгоритмах обмена ключами (ECDHE — Elliptic Curve Diffie-Hellman) и цифровых подписях (ECDSA — Elliptic Curve Digital Signature Algorithm). Обеспечивает хороший баланс между уровнем безопасности и производительностью, что делает её популярным выбором для широкого спектра приложений. P-256 также известна как secp256r1 или prime256v1.
			P_384                   = 0x03, // Эллиптическая кривая, относится к стандартам NIST (National Institute of Standards and Technology) и применяется в алгоритмах обмена ключами (ECDHE — Elliptic Curve Diffie-Hellman) и цифровых подписях (ECDSA — Elliptic Curve Digital Signature Algorithm). Обеспечивает более высокий уровень безопасности по сравнению с P-256, но при этом требует больше вычислительных ресурсов. P-384 также известна как secp384r1.
			P_521                   = 0x04, // Эллиптическая кривая, относится к стандартам NIST (National Institute of Standards and Technology) и применяется в алгоритмах обмена ключами (ECDHE — Elliptic Curve Diffie-Hellman) и цифровых подписях (ECDSA — Elliptic Curve Digital Signature Algorithm). Обеспечивает более высокий уровень безопасности по сравнению с P-256 и P-384, но при этом требует больше вычислительных ресурсов. P-521 также известна как secp521r1.
			X448                    = 0x05, // Эллиптическая кривая, разработанная Дэниелом Дж. Бернштейном и Тони Андерсоном. Она предназначена для использования в алгоритмах обмена ключами (ECDH — Elliptic Curve Diffie-Hellman) и цифровых подписях (EdDSA — Edwards-curve Digital Signature Algorithm). X448 обеспечивает высокий уровень безопасности, эквивалентный примерно 224 битам, и оптимизирован для производительности на широком спектре платформ. X448 также известна как Curve448 или Goldilocks.
			X25519                  = 0x06, // Эллиптическая кривая, разработанная Дэниелом Дж. Бернштейном и Тони Андерсоном. Она предназначена для использования в алгоритмах обмена ключами (ECDH — Elliptic Curve Diffie-Hellman) и цифровых подписях (EdDSA — Edwards-curve Digital Signature Algorithm). X25519 обеспечивает высокий уровень безопасности, эквивалентный примерно 128 битам, и оптимизирован для производительности на широком спектре платформ. X25519 также известна как Curve25519.
			SECP256K1               = 0x07, // Эллиптическая кривая, разработанная Сатоши Накамото и используемая в криптовалюте Биткойн. Она предназначена для использования в алгоритмах обмена ключами (ECDH — Elliptic Curve Diffie-Hellman) и цифровых подписях (ECDSA — Elliptic Curve Digital Signature Algorithm). SECP256K1 обеспечивает высокий уровень безопасности, эквивалентный примерно 128 битам, и оптимизирован для производительности на широком спектре платформ. SECP256K1 также известна как secp256k1 или prime256k1.
			FFDHE2048               = 0x08, // Группа для обмена ключами, использующая алгоритм Diffie-Hellman с фиксированными параметрами (FFDHE — Finite Field Diffie-Hellman Ephemeral) и 2048-битным модулем. FFDHE2048 обеспечивает высокий уровень безопасности, эквивалентный примерно 112 битам, и является одним из рекомендуемых вариантов для использования в TLS 1.3. FFDHE2048 также известна как ffdhe2048 или DH group 14.
			FFDHE3072               = 0x09, // Группа для обмена ключами, использующая алгоритм Diffie-Hellman с фиксированными параметрами (FFDHE — Finite Field Diffie-Hellman Ephemeral) и 3072-битным модулем. FFDHE3072 обеспечивает высокий уровень безопасности, эквивалентный примерно 128 битам, и является одним из рекомендуемых вариантов для использования в TLS 1.3. FFDHE3072 также известна как ffdhe3072 или DH group 15.
			FFDHE4096               = 0x0A, // Группа для обмена ключами, использующая алгоритм Diffie-Hellman с фиксированными параметрами (FFDHE — Finite Field Diffie-Hellman Ephemeral) и 4096-битным модулем. FFDHE4096 обеспечивает высокий уровень безопасности, эквивалентный примерно 256 битам, и является одним из рекомендуемых вариантов для использования в TLS 1.3. FFDHE4096 также известна как ffdhe4096 или DH group 16.
			FFDHE6144               = 0x0B, // Группа для обмена ключами, использующая алгоритм Diffie-Hellman с фиксированными параметрами (FFDHE — Finite Field Diffie-Hellman Ephemeral) и 6144-битным модулем. FFDHE6144 обеспечивает высокий уровень безопасности, эквивалентный примерно 384 битам, и является одним из рекомендуемых вариантов для использования в TLS 1.3. FFDHE6144 также известна как ffdhe6144 или DH group 17.
			FFDHE8192               = 0x0C, // Группа для обмена ключами, использующая алгоритм Diffie-Hellman с фиксированными параметрами (FFDHE — Finite Field Diffie-Hellman Ephemeral) и 8192-битным модулем. FFDHE8192 обеспечивает высокий уровень безопасности, эквивалентный примерно 512 битам, и является одним из рекомендуемых вариантов для использования в TLS 1.3. FFDHE8192 также известна как ffdhe8192 или DH group 18.
			MLKEM1024               = 0x0D, // Группа для обмена ключами, использующая алгоритм ML-KEM (Multi-Level Key Encapsulation Mechanism) с 1024-битным параметром. MLKEM1024 обеспечивает высокий уровень безопасности и является одним из рекомендуемых вариантов для использования в TLS 1.3. MLKEM1024 также известна как mlkem1024 или KEM group 32.
			X25519_MLKEM768         = 0x0E, // Группа для обмена ключами, использующая алгоритм ML-KEM (Multi-Level Key Encapsulation Mechanism) с 768-битным параметром и кривой X25519. X25519_MLKEM768 обеспечивает высокий уровень безопасности и является одним из рекомендуемых вариантов для использования в TLS 1.3. X25519_MLKEM768 также известна как x25519_mlkem768 или KEM group 33.
			X25519_KYBER768_DRAFT00 = 0x0F  // Группа для обмена ключами, использующая алгоритм Kyber (постквантовый криптографический алгоритм) с 768-битным параметром и кривой X25519. X25519_KYBER768_DRAFT00 обеспечивает высокий уровень безопасности и является одним из рекомендуемых вариантов для использования в TLS 1.3. X25519_KYBER768_DRAFT00 также известна как x25519_kyber768_draft00 или KEM group 34.
		};

		/**
		 * @brief Тип доступных подписей TLS
		 *
		 */
		enum class signature_t : uint8_t {
			UNKNOWN                 = 0x00, // Неизвестная или нераспознанная подпись
			GREASE                  = 0x01, // Специальный код для обозначения GREASE-значений, которые не являются реальными подписями, а используются для тестирования устойчивости TLS-стека к неизвестным значениям.
			ED448                   = 0x02, // Алгоритм цифровой подписи, основанный на эллиптической кривой (ECC — Elliptic Curve Cryptography). Он использует кривую Ed448-Goldilocks, разработанную Майком Гамбургом.
			ED25519                 = 0x03, // Конкретная реализация алгоритма цифровой подписи Edwards-curve Digital Signature Algorithm (EdDSA), основанная на эллиптических кривых Эдвардса. Разработана группой исследователей во главе с Дэниелом Дж. Бернштейном в 2011 году.
			DSA_SHA1                = 0x04, // Алгоритм цифровой подписи, который использует алгоритм DSA (Digital Signature Algorithm) в сочетании с хеш-функцией SHA-1. Он используется для создания и проверки электронных подписей, обеспечивая аутентификацию и целостность данных.
			RSA_PKCS1_SHA1          = 0x06, // Алгоритм цифровой подписи, который использует RSA в сочетании с хеш-функцией SHA-1 и схемой заполнения PKCS#1 версии 1.5.
			RSA_PKCS1_SHA256        = 0x07, // Алгоритм цифровой подписи, который использует RSA в сочетании с хеш-функцией SHA-256 и схемой заполнения PKCS#1 версии 1.5.
			RSA_PKCS1_SHA384        = 0x08, // Алгоритм цифровой подписи, который использует RSA в сочетании с хеш-функцией SHA-384 и схемой заполнения PKCS#1 версии 1.5.
			RSA_PKCS1_SHA512        = 0x09, // Алгоритм цифровой подписи, который использует RSA в сочетании с хеш-функцией SHA-512 и схемой заполнения PKCS#1 версии 1.5.
			RSA_PSS_PSS_SHA256      = 0x0A, // Схема цифровой подписи, использующая алгоритм RSA с вероятностной схемой подписи (Probabilistic Signature Scheme, PSS) и хеш-функцию SHA-256.
			RSA_PSS_PSS_SHA384      = 0x0B, // Схема цифровой подписи, использующая алгоритм RSA с вероятностной схемой подписи (Probabilistic Signature Scheme, PSS) и хеш-функцию SHA-384.
			RSA_PSS_PSS_SHA512      = 0x0C, // Схема цифровой подписи, использующая алгоритм RSA с вероятностной схемой подписи (Probabilistic Signature Scheme, PSS) и хеш-функцию SHA-512.
			RSA_PSS_RSAE_SHA256     = 0x0D, // Алгоритм цифровой подписи, который используется в контексте криптографии, в частности в протоколе TLS 1.3. Он относится к семейству алгоритмов RSASSA-PSS с функцией генерации маски MGF1 и хеш-функцией SHA-256.
			RSA_PSS_RSAE_SHA384     = 0x0E, // Алгоритм цифровой подписи, который используется в контексте криптографии, в частности в протоколе TLS 1.3. Он относится к семейству алгоритмов RSASSA-PSS с функцией генерации маски MGF1 и хеш-функцией SHA-384.
			RSA_PSS_RSAE_SHA512     = 0x0F, // Алгоритм цифровой подписи, который используется в контексте криптографии, в частности в протоколе TLS 1.3. Он относится к семейству алгоритмов RSASSA-PSS с функцией генерации маски MGF1 и хеш-функцией SHA-512.
			ECDSA_SHA1              = 0x05, // Алгоритм цифровой подписи, сочетающий эллиптическую кривую цифровой подписи (ECDSA) с хеш-функцией SHA-1. Он используется для создания и проверки электронных подписей, обеспечивая аутентификацию и целостность данных.
			ECDSA_SECP256R1_SHA256  = 0x10, // Алгоритм цифровой подписи, сочетающий эллиптическую кривую SECP256R1 (также известную как prime256v1) с хеш-функцией SHA-256. Он используется для создания и проверки электронных подписей, обеспечивая аутентификацию и целостность данных.
			ECDSA_SECP384R1_SHA384  = 0x11, // Алгоритм цифровой подписи, сочетающий эллиптическую кривую SECP384R1 с хеш-функцией SHA-384. Он используется для создания и проверки электронных подписей, обеспечивая аутентификацию и целостность данных.
			ECDSA_SECP521R1_SHA512  = 0x12, // Алгоритм цифровой подписи, сочетающий эллиптическую кривую SECP521R1 с хеш-функцией SHA-512. Он используется для создания и проверки электронных подписей, обеспечивая аутентификацию и целостность данных.
			RSA_PKCS1_MD5_SHA1	    = 0x13, // Алгоритм цифровой подписи RSA PKCS1, который использует комбинацию хеш-функций MD5 и SHA-1. Этот алгоритм был широко использован в ранних версиях TLS, но сейчас считается небезопасным из-за уязвимостей в обеих хеш-функциях (например, уязвимость к атакам типа "collision attack").
			RSA_PKCS1_SHA256_LEGACY = 0x14  // Алгоритм цифровой подписи RSA PKCS1 SHA256, который может быть использован в TLS 1.2, но не рекомендуется из-за слабой безопасности (например, уязвимость к атакам типа "Bleichenbacher's attack").
		};

		/**
		 * @brief Тип доступных расширений TLS
		 *
		 */
		enum class extension_type_t : uint8_t {
			UNKNOWN                          = 0x00, // Неизвестное или нераспознанное расширение.
			GREASE                           = 0x01, // Специальный код для обозначения GREASE-значений, которые не являются реальными расширениями, а используются для тестирования устойчивости TLS-стека к неизвестным значениям.
			ALPN                             = 0x02, // Application-Layer Protocol Negotiation, используется для согласования протокола прикладного уровня (например, h2,http/1.1) во время TLS-рукопожатия в RFC 7301.
			COOKIE                           = 0x03, // Расширение TLS для передачи cookie, используемое в DTLS для защиты от атак с повторным воспроизведением и DoS-атак в RFC 6347.
			PADDING                          = 0x04, // Расширение TLS для добавления произвольного количества байтов заполнения в сообщения TLS, что может помочь скрыть фактическую длину сообщений и затруднить анализ трафика в RFC 7685.
			USE_SRTP                         = 0x05, // Расширение TLS для использования SRTP (Secure Real-time Transport Protocol), которое позволяет согласовать параметры безопасности для SRTP во время TLS-рукопожатия в RFC 5764.
			HEARTBEAT                        = 0x06, // Расширение TLS для поддержки механизма heartbeat, который позволяет проверять доступность соединения без необходимости отправки данных в RFC 6520.
			TLS_FLAGS                        = 0x07, // Расширение TLS для передачи флагов или параметров, специфичных для реализации, которые не входят в стандартные расширения TLS (например, может использоваться для передачи информации о поддерживаемых функциях или настройках безопасности).
			KEY_SHARE                        = 0x08, // Расширение TLS 1.3 для обмена ключами, которое позволяет клиенту и серверу обмениваться информацией о поддерживаемых группами обмена ключами (например, ECDHE) и соответствующими параметрами во время TLS-рукопожатия в RFC 8446.
			CHANNEL_ID                       = 0x09, // Расширение TLS для передачи идентификатора канала, используемое в BoringSSL.
			EARLY_DATA                       = 0x0A, // Расширение TLS для поддержки ранних данных, позволяющее клиенту отправлять данные до завершения полного TLS-рукопожатия в RFC 8446.
			OID_FILTERS                      = 0x0B, // Расширение TLS для фильтрации идентификаторов объектов (OID), используемое в RFC 8446.
			SERVER_NAME                      = 0x0C, // Расширение TLS для указания имени сервера, используемое в RFC 6066.
			TRUST_ANCHORS                    = 0x0D, // Расширение TLS для передачи доверенных якорей, используемое в BoringSSL draft.
			NEXT_PROTO_NEG                   = 0x0E, // Расширение TLS для согласования следующего протокола, используемое в старых версиях TLS и заменённое на ALPN в RFC 7301.
			PRE_SHARED_KEY                   = 0x0F, // Расширение TLS для использования предварительно совместного ключа, используемое в RFC 8446.
			SESSION_TICKET                   = 0x10, // Расширение TLS для использования билета сессии, используемое в RFC 5077.
			STATUS_REQUEST                   = 0x11, // Расширение TLS для запроса статуса сертификата, используемое в RFC 6066 (OCSP).
			SUPPORTED_GROUPS                 = 0x12, // Расширение TLS для указания поддерживаемых групп, используемое в RFC 8422.
			EC_POINT_FORMATS                 = 0x13, // Расширение TLS для указания форматов точек эллиптической кривой, используемое в RFC 8422.
			ENCRYPT_THEN_MAC                 = 0x14, // Расширение TLS для указания необходимости шифрования перед MAC, используемое в RFC 7366.
			RECORD_SIZE_LIMIT                = 0x15, // Расширение TLS для указания максимального размера записи, используемое в RFC 8449.
			TRANSPARENCY_INFO                = 0x16, // Расширение TLS для передачи информации о прозрачности, редко используемое.
			RENEGOTIATION_INFO               = 0x17, // Расширение TLS для передачи информации о повторной договоренности, используемое в RFC 5746.
			SUPPORTED_VERSIONS               = 0x18, // Расширение TLS для указания поддерживаемых версий, используемое в RFC 8446.
			MAX_FRAGMENT_LENGTH              = 0x19, // Расширение TLS для указания максимальной длины фрагмента, используемое в RFC 6066.
			POST_HANDSHAKE_AUTH              = 0x1A, // Расширение TLS для поддержки аутентификации после рукопожатия, используемое в RFC 8446.
			ECH_OUTER_EXTENSIONS             = 0x1B, // Расширение TLS для передачи внешних расширений ECH.
			COMPRESS_CERTIFICATE             = 0x1C, // Расширение TLS для сжатия сертификатов, используемое в RFC 8879.
			DELEGATED_CREDENTIAL             = 0x1D, // Расширение TLS для использования делегированных учетных данных, используемое в RFC 9345.
			SIGNATURE_ALGORITHMS             = 0x1E, // Расширение TLS для указания поддерживаемых алгоритмов подписи, используемое в RFC 8446.
			APPLICATION_SETTINGS             = 0x1F, // Расширение TLS для передачи настроек приложения, используемое в ALPS.
			ENCRYPTED_CLIENT_HELLO           = 0x29, // Расширение TLS для передачи зашифрованного ClientHello, используемое в RFC 65037 (ECH) / GREASE.
			EXTENDED_MASTER_SECRET           = 0x20, // Расширение TLS для использования расширенного мастер-секрета, используемое в RFC 7627.
			PSK_KEY_EXCHANGE_MODES           = 0x21, // Расширение TLS для указания режимов обмена ключами PSK, используемое в RFC 8446.
			CLIENT_CERTIFICATE_TYPE          = 0x22, // Расширение TLS для указания типа сертификата клиента, используемое в RFC 7250.
			SERVER_CERTIFICATE_TYPE          = 0x23, // Расширение TLS для указания типа сертификата сервера, используемое в RFC 7250.
			CERTIFICATE_AUTHORITIES          = 0x24, // Расширение TLS для указания доверенных центров сертификации, используемое в RFC 8446.
			APPLICATION_SETTINGS_OLD         = 0x25, // Расширение TLS для передачи старых настроек приложения, используемое в Chrome legacy ALPS.
			QUIC_TRANSPORT_PARAMETERS        = 0x26, // Расширение TLS для передачи параметров транспорта QUIC, используемое в RFC 9001.
			SIGNATURE_ALGORITHMS_CERT        = 0x27, // Расширение TLS для указания поддерживаемых алгоритмов подписи для сертификатов, используемое в RFC 8446.
			SIGNED_CERTIFICATE_TIMESTAMP     = 0x28, // Расширение TLS для передачи информации о подписанных временных метках сертификатов, используемое в RFC 6962.
			QUIC_TRANSPORT_PARAMETERS_LEGACY = 0x2A  // Расширение TLS для передачи устаревших параметров транспорта QUIC, используемое в BoringSSL legacy QUIC.
		};

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
			// Имя сервера
			string name;
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Extension_Server_Name() noexcept :
			 extension_t(extension_type_t::SERVER_NAME), name{""} {}
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
			vector <ec_point_format_t> supportedFormats;
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
	};
};

#endif // __AWH_TLS__
