/**
 * @file: fingerprint.cpp
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
 * Стандартные модули
 */
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <openssl/md5.h>
#include <openssl/sha.h>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <net/tls/fingerprint.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;


// ==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================

// Вспомогательная: вывод байт-массива
static void print_hex(const uint8_t* buf, size_t len) {
	for (size_t i = 0; i < len; ++i) printf("%02X ", buf[i]);
}

static inline bool is_grease(uint16_t val) {
    uint8_t hi = val >> 8, lo = val & 0xFF;
    return (hi == lo) && ((lo & 0x0F) == 0x0A);
}

static inline uint16_t read_u16(const uint8_t* p) {
    return (static_cast<uint16_t>(p[0]) << 8) | p[1];
}

// Простая таблица имён для читаемости (опционально, не влияет на парсинг)
static const char* cipher_name(uint16_t id) {
	if (is_grease(id)) return "[GREASE]";
    switch (id) {
        case 0x1301: return "TLS_AES_128_GCM_SHA256";
        case 0x1302: return "TLS_AES_256_GCM_SHA384";
        case 0x1303: return "TLS_CHACHA20_POLY1305_SHA256";
        case 0xC02B: return "ECDHE-ECDSA-AES128-GCM-SHA256";
        case 0xC02F: return "ECDHE-RSA-AES128-GCM-SHA256";
        case 0xC02C: return "ECDHE-ECDSA-AES256-GCM-SHA384";
        case 0xC030: return "ECDHE-RSA-AES256-GCM-SHA384";
        case 0xCCA9: return "ECDHE-ECDSA-CHACHA20-POLY1305";
        case 0xCCA8: return "ECDHE-RSA-CHACHA20-POLY1305";
		case 0xC013: return "ECDHE-RSA-AES128-SHA";
		case 0xC014: return "ECDHE-RSA-AES256-SHA";
		case 0x009C: return "AES128-GCM-SHA256";
		case 0x009D: return "AES256-GCM-SHA384";
		case 0x002F: return "AES128-SHA";
		case 0x0035: return "AES256-SHA";
        default:     return "UNKNOWN";
    }
}

// ==================== ТАБЛИЦЫ ИМЕН ====================

static const char* ext_name(uint16_t type) {
    if (is_grease(type)) return "TLS_GREASE";
    switch(type) {
        case 0x0000: return "server_name";                              // 0   RFC 6066
        case 0x0001: return "max_fragment_length";                      // 1   RFC 6066
        case 0x0005: return "status_request";                           // 5   RFC 6066 (OCSP)
        case 0x000A: return "supported_groups";                         // 10  RFC 8422
        case 0x000B: return "ec_point_formats";                         // 11  RFC 8422
        case 0x000D: return "signature_algorithms";                     // 13  RFC 8446
        case 0x000E: return "use_srtp";                                 // 14  RFC 5764
        case 0x000F: return "heartbeat";                                // 15  RFC 6520
        case 0x0010: return "application_layer_protocol_negotiation";   // 16  RFC 7301
        case 0x0012: return "signed_certificate_timestamp";             // 18  RFC 6962
        case 0x0013: return "client_certificate_type";                  // 19  RFC 7250
        case 0x0014: return "server_certificate_type";                  // 20  RFC 7250
        case 0x0015: return "padding";                                  // 21  RFC 7685
        case 0x0016: return "encrypt_then_mac";                         // 22  RFC 7366
        case 0x0017: return "extended_master_secret";                   // 23  RFC 7627
        case 0x001B: return "compress_certificate";                     // 27  RFC 8879
        case 0x001C: return "record_size_limit";                        // 28  RFC 8449
        case 0x0022: return "delegated_credential";                     // 34  RFC 9345
        case 0x0023: return "session_ticket";                           // 35  RFC 5077
        case 0x0029: return "pre_shared_key";                           // 41  RFC 8446
        case 0x002A: return "early_data";                               // 42  RFC 8446
        case 0x002B: return "supported_versions";                       // 43  RFC 8446
        case 0x002C: return "cookie";                                   // 44  RFC 8446
        case 0x002D: return "psk_key_exchange_modes";                   // 45  RFC 8446
        case 0x002F: return "certificate_authorities";                  // 47  RFC 8446
        case 0x0030: return "oid_filters";                              // 48  RFC 8446
        case 0x0031: return "post_handshake_auth";                      // 49  RFC 8446
        case 0x0032: return "signature_algorithms_cert";                // 50  RFC 8446
        case 0x0033: return "key_share";                                // 51  RFC 8446
        case 0x0035: return "transparency_info";                        // 53  редко
        case 0x0039: return "quic_transport_parameters";                // 57  RFC 9001
        case 0x003E: return "tls_flags";                                // 62  draft
        case 0x3374: return "next_proto_neg";                           // 13172 NPN (предшественник ALPN)
        case 0x4469: return "application_settings_old";                 // 17513 Chrome legacy ALPS
        case 0x44CD: return "application_settings";                     // 17613 ALPS новый стандарт
        case 0x7550: return "channel_id";                               // 30032 BoringSSL
        case 0xCA34: return "trust_anchors";                            // BoringSSL draft
        case 0xFD00: return "ech_outer_extensions";                     // ECH outer
        case 0xFE0D: return "extensionEncryptedClientHello";            // 65037 ECH / GREASE
        case 0xFF01: return "extensionRenegotiationInfo";               // 65281 RFC 5746
        case 0xFFA5: return "quic_transport_parameters_legacy";         // BoringSSL legacy QUIC
        default: return "UNKNOWN";
    }
}

static const char* sig_alg_name(uint16_t id) {
    if (is_grease(id)) return "TLS_GREASE";
    switch(id) {
        // RSA PKCS1 (RFC 8446 / BoringSSL SSL_SIGN_RSA_PKCS1_*)
        case 0x0201: return "rsa_pkcs1_sha1";
        case 0x0401: return "rsa_pkcs1_sha256";
        case 0x0501: return "rsa_pkcs1_sha384";
        case 0x0601: return "rsa_pkcs1_sha512";
        // ECDSA (RFC 8446 / BoringSSL SSL_SIGN_ECDSA_*)
        case 0x0203: return "ecdsa_sha1";
        case 0x0403: return "ecdsa_secp256r1_sha256";
        case 0x0503: return "ecdsa_secp384r1_sha384";
        case 0x0603: return "ecdsa_secp521r1_sha512";
        // RSA PSS RSAE — ключ из сертификата end-entity (RFC 8446 / BoringSSL SSL_SIGN_RSA_PSS_RSAE_*)
        case 0x0804: return "rsa_pss_rsae_sha256";
        case 0x0805: return "rsa_pss_rsae_sha384";
        case 0x0806: return "rsa_pss_rsae_sha512";
        // EdDSA (RFC 8446 / BoringSSL SSL_SIGN_ED25519)
        case 0x0807: return "ed25519";
        case 0x0808: return "ed448";
        // RSA PSS PSS — выделенный PSS-сертификат (RFC 8446)
        case 0x0809: return "rsa_pss_pss_sha256";
        case 0x080A: return "rsa_pss_pss_sha384";
        case 0x080B: return "rsa_pss_pss_sha512";
        // Прочие
        case 0x0202: return "dsa_sha1";
		case 0xFF01: return "rsa_pkcs1_md5_sha1";
        case 0x0420: return "rsa_pkcs1_sha256_legacy";
        default: return "UNKNOWN_SIG";
    }
}

static const char* group_name(uint16_t id) {
    if (is_grease(id)) return "TLS_GREASE";
    switch(id) {
        case 0x0017: return "P-256";      // secp256r1
        case 0x0018: return "P-384";      // secp384r1
        case 0x0019: return "P-521";      // secp521r1
        case 0x001D: return "X25519";
        case 0x001E: return "X448";
        case 0x001C: return "secp256k1";
        // FFDHE (RFC 7919, IANA: 0x0100-0x0104)
        case 0x0100: return "ffdhe2048";
        case 0x0101: return "ffdhe3072";
        case 0x0102: return "ffdhe4096";
        case 0x0103: return "ffdhe6144";
        case 0x0104: return "ffdhe8192";
        // Post-quantum / hybrid (BoringSSL SSL_GROUP_*)
        case 0x0202: return "mlkem1024";            // SSL_GROUP_MLKEM1024
        case 0x6399: return "X25519Kyber768Draft00"; // SSL_GROUP_X25519_KYBER768_DRAFT00
        case 0x11EC: return "X25519MLKEM768";        // SSL_GROUP_X25519_MLKEM768
        default: return "UNKNOWN_GROUP";
    }
}

static const char* tls_version_name(uint16_t ver) {
    if (is_grease(ver)) return "TLS_GREASE";
    switch(ver) {
        case 0x0300: return "SSLv3";
        case 0x0301: return "TLS 1.0";
        case 0x0302: return "TLS 1.1";
        case 0x0303: return "TLS 1.2";
        case 0x0304: return "TLS 1.3";
        case 0xFEFF: return "DTLS 1.0";
        case 0xFEFD: return "DTLS 1.2";
        default: return "UNKNOWN_VERSION";
    }
}

static const char* compress_alg_name(uint8_t id) {
    switch(id) {
        case 0x01: return "zlib";
        case 0x02: return "brotli";
        case 0x03: return "zstd";
        default: return "UNKNOWN";
    }
}

// ==================== ДЕКОДЕРЫ РАСШИРЕНИЙ ====================

// 0x0000: server_name
static void parse_server_name(const uint8_t* data, size_t len) {
    if (len < 5) { printf("\n"); return; }
    uint16_t list_len = read_u16(data);
    uint8_t name_type = data[2];
    uint16_t hostname_len = read_u16(data + 3);
    if (name_type == 0x00 && len >= 5 + hostname_len) {
        printf(", \"server_name\": \"%.*s\"", hostname_len, data + 5);
    }
    printf("\n");
}

// 0x0005: status_request (OCSP)
static void parse_status_request(const uint8_t* data, size_t len) {
    if (len >= 1) {
        const char* status_type = (data[0] == 0x01) ? "OSCP" : "UNKNOWN";
        printf(", \"status_request\": {\"certificate_status_type\": \"%s (%d)\"", status_type, data[0]);
        if (len >= 3) {
            uint16_t responder_len = read_u16(data + 1);
            printf(", \"responder_id_list_length\": %d", responder_len);
            if (len >= 5) {
                uint16_t req_ext_len = read_u16(data + 3);
                printf(", \"request_extensions_length\": %d", req_ext_len);
            }
        }
        printf("}");
    }
    printf("\n");
}

// 0x000A: supported_groups
static void parse_supported_groups(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    uint16_t list_len = read_u16(data);
    printf(", \"supported_groups\": [");
    bool first = true;
    for (size_t i = 2; i < 2 + list_len && i + 1 < len; i += 2) {
        uint16_t gid = read_u16(data + i);
        if (!first) printf(", ");
        first = false;
        if (is_grease(gid)) {
            printf("\"TLS_GREASE (0x%04X)\"", gid);
        } else {
            // Chrome формат: "Name (decimal_id)"
            printf("\"%s (%d)\"", group_name(gid), gid & 0xFFFF);
        }
    }
    printf("]\n");
}

// 0x000B: ec_point_formats
static void parse_ec_point_formats(const uint8_t* data, size_t len) {
    if (len < 1) { printf("\n"); return; }
    uint8_t fmt_count = data[0];
    printf(", \"elliptic_curves_point_formats\": [");
    for (uint8_t i = 0; i < fmt_count && i + 1 < len; ++i) {
        if (i > 0) printf(", ");
        printf("\"0x%02X\"", data[1 + i]);
    }
    printf("]\n");
}

// 0x000D: signature_algorithms
static void parse_signature_algorithms(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    uint16_t list_len = read_u16(data);
    printf(", \"signature_algorithms\": [");
    bool first = true;
    for (size_t i = 2; i < 2 + list_len && i + 1 < len; i += 2) {
        uint16_t alg = read_u16(data + i);
        if (!first) printf(", ");
        first = false;
        printf("\"%s\"", sig_alg_name(alg));
    }
    printf("]\n");
}

// 0x0010: ALPN
static void parse_alpn(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    uint16_t list_len = read_u16(data);
    printf(", \"protocols\": [");
    size_t pos = 2;
    bool first = true;
    while (pos < 2 + list_len && pos < len) {
        uint8_t proto_len = data[pos++];
        if (!first) printf(", ");
        first = false;
        printf("\"%.*s\"", proto_len, data + pos);
        pos += proto_len;
    }
    printf("]\n");
}

// 0x0012: signed_certificate_timestamp (пустое)
static void parse_sct(const uint8_t*, size_t) { printf("\n"); }

// 0x0015: padding
static void parse_padding(const uint8_t*, size_t len) {
    printf(", \"length\": %zu\n", len);
}

// 0x0017: extended_master_secret (пустое)
static void parse_extended_master_secret(const uint8_t*, size_t) {
    printf(", \"master_secret_data\": \"\", \"extended_master_secret_data\": \"\"\n");
}

// 0x001B: compress_certificate (RFC 8879)
// Формат: list_bytes(1) + алгоритмы в виде uint16 (2 байта каждый)
static void parse_compress_certificate(const uint8_t* data, size_t len) {
    if (len < 1) { printf("\n"); return; }
    const uint8_t list_bytes = data[0];  // суммарное число байт списка
    printf(", \"algorithms\": [");
    bool first = true;
    for (size_t i = 0; i + 2 <= static_cast<size_t>(list_bytes) && 1 + i + 2 <= len; i += 2) {
        if (!first) printf(", ");
        first = false;
        const uint16_t alg_id = read_u16(data + 1 + i);
        const char* name;
        switch (alg_id) {
            case 1: name = "zlib";    break;
            case 2: name = "brotli";  break;
            case 3: name = "zstd";    break;
            default: name = "UNKNOWN"; break;
        }
        printf("\"%s (%d)\"", name, alg_id);
    }
    printf("]\n");
}

// 0x0023: session_ticket (пустое или с данными)
static void parse_session_ticket(const uint8_t* data, size_t len) {
    if (len == 0) {
        printf(", \"data\": \"\"\n");
    } else {
        printf(", \"data\": \"");
        for (size_t i = 0; i < std::min<size_t>(32, len); ++i) printf("%02X", data[i]);
        if (len > 32) printf("...");
        printf("\"\n");
    }
}

// 0x002B: supported_versions
static void parse_supported_versions(const uint8_t* data, size_t len) {
    if (len < 1) { printf("\n"); return; }
    uint8_t list_len = data[0];
    printf(", \"versions\": [");
    bool first = true;
    for (size_t i = 1; i < 1 + list_len && i + 1 < len; i += 2) {
        uint16_t ver = read_u16(data + i);
        if (!first) printf(", ");
        first = false;
        if (is_grease(ver)) {
            printf("\"TLS_GREASE (0x%04X)\"", ver);
        } else {
            printf("\"%s\"", tls_version_name(ver));
        }
    }
    printf("]\n");
}

// 0x002C: psk_key_exchange_modes
static void parse_psk_key_exchange_modes(const uint8_t* data, size_t len) {
    if (len < 1) { printf("\n"); return; }
    uint8_t mode_count = data[0];
    printf(", \"PSK_Key_Exchange_Mode\": \"");
    for (uint8_t i = 0; i < mode_count && 1 + i < len; ++i) {
        if (i > 0) printf(", ");
        uint8_t mode = data[1 + i];
        switch (mode) {
            case 0x00: printf("PSK-only key establishment (psk_ke) (%d)", mode); break;
            case 0x01: printf("PSK with (EC)DHE key establishment (psk_dhe_ke) (%d)", mode); break;
            default:   printf("UNKNOWN_MODE_%d (%d)", mode, mode); break;
        }
    }
    printf("\"\n");
}

// 0x002D: early_data
static void parse_early_data(const uint8_t* data, size_t len) {
    if (len == 0) {
        printf("\n");
    } else if (len >= 4) {
        uint32_t max_size = (static_cast<uint32_t>(data[0]) << 24) |
                           (static_cast<uint32_t>(data[1]) << 16) |
                           (static_cast<uint32_t>(data[2]) << 8) | data[3];
        printf(", \"max_early_data_size\": %u\n", max_size);
    } else {
        printf("\n");
    }
}

// 0x0033: key_share
static void parse_key_share(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    uint16_t total_len = read_u16(data);
    printf(", \"shared_keys\": [");
    size_t off = 2;
    bool first = true;
    while (off + 4 <= len && off < 2 + total_len) {
        uint16_t group = read_u16(data + off);
        uint16_t key_len = read_u16(data + off + 2);
        off += 4;
        if (!first) printf(", ");
        first = false;
        
        printf("{\n          \"");
        if (is_grease(group)) {
            printf("TLS_GREASE (0x%04X)", group);
        } else {
            printf("%s (%d)", group_name(group), group & 0xFFFF);
        }
        printf("\": \"");
        
        // Вывод ключа в hex
        if (off + key_len <= len) {
            for (size_t i = 0; i < std::min<size_t>(64, key_len); ++i) {
                printf("%02X", data[off + i]);
            }
            if (key_len > 64) printf("...");
        }
        printf("\"\n        }");
        
        off += key_len;
    }
    printf("\n        ]\n");
}

// 0x4449: application_settings_old (Chrome legacy)
static void parse_alps_old(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    uint16_t list_len = read_u16(data);
    printf(", \"protocols\": [");
    size_t pos = 2;
    bool first = true;
    while (pos < 2 + list_len && pos < len) {
        uint8_t proto_len = data[pos++];
        if (!first) printf(", ");
        first = false;
        printf("\"%.*s\"", proto_len, data + pos);
        pos += proto_len;
    }
    printf("]\n");
}

// 0xFE0D: ECH (Encrypted Client Hello)
static void parse_ech(const uint8_t* data, size_t len) {
    printf(", \"data\": \"");
    // Выводим все данные в hex, как в Chrome-дампе
    for (size_t i = 0; i < len; ++i) {
        printf("%02X", data[i]);
    }
    printf("\"\n");
}

// 0xFF01: renegotiation_info
static void parse_renegotiation_info(const uint8_t* data, size_t len) {
    printf(", \"data\": \"");
    for (size_t i = 0; i < std::min<size_t>(32, len); ++i) {
        printf("%02X", data[i]);
    }
    if (len > 32) printf("...");
    printf("\"\n");
}

// 0x001C: record_size_limit (RFC 8449) — ограничение размера TLS-записи
static void parse_record_size_limit(const uint8_t* data, size_t len) {
    if (len >= 2) {
        printf(", \"record_size_limit\": %u\n", read_u16(data));
    } else {
        printf("\n");
    }
}

// 0x002C: cookie (RFC 8446) — только в ClientHello2 (ответ на HelloRetryRequest)
static void parse_cookie(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    uint16_t cookie_len = read_u16(data);
    printf(", \"cookie\": \"");
    for (size_t i = 0; i < cookie_len && 2 + i < len; ++i)
        printf("%02X", data[2 + i]);
    printf("\"\n");
}

// 0x0029: pre_shared_key (RFC 8446) — TLS 1.3 resumption
static void parse_pre_shared_key(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    uint16_t ids_len = read_u16(data);
    int count = 0;
    size_t off = 2;
    while (off + 2 <= static_cast<size_t>(2 + ids_len) && off + 2 <= len) {
        uint16_t id_len = read_u16(data + off);
        off += 2 + id_len + 4; // identity + obfuscated_ticket_age (4 байта)
        count++;
    }
    printf(", \"identities_count\": %d\n", count);
}

// 0x002F: certificate_authorities (RFC 8446)
static void parse_certificate_authorities(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    uint16_t list_len = read_u16(data);
    int count = 0;
    size_t off = 2;
    while (off + 2 <= static_cast<size_t>(2 + list_len) && off + 2 <= len) {
        uint16_t dn_len = read_u16(data + off);
        off += 2 + dn_len;
        count++;
    }
    printf(", \"authorities_count\": %d\n", count);
}

// 0x3374: next_proto_neg (NPN) — в ClientHello обычно пустое (сигнализирует поддержку)
static void parse_npn(const uint8_t* data, size_t len) {
    if (len == 0) {
        printf("\n");
        return;
    }
    printf(", \"protocols\": [");
    size_t pos = 0;
    bool first = true;
    while (pos < len) {
        uint8_t proto_len = data[pos++];
        if (pos + proto_len > len) break;
        if (!first) printf(", ");
        first = false;
        printf("\"%.*s\"", proto_len, data + pos);
        pos += proto_len;
    }
    printf("]\n");
}

// 0x0001: max_fragment_length (RFC 6066)
static void parse_max_fragment_length(const uint8_t* data, size_t len) {
    if (len < 1) { printf("\n"); return; }
    uint8_t code = data[0];
    const char* sz;
    switch(code) {
        case 1: sz = "512";  break;
        case 2: sz = "1024"; break;
        case 3: sz = "2048"; break;
        case 4: sz = "4096"; break;
        default: sz = "UNKNOWN"; break;
    }
    printf(", \"max_fragment_length\": \"%s (%d)\"\n", sz, code);
}

// 0x000E: use_srtp (RFC 5764)
static void parse_use_srtp(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    uint16_t profiles_len = read_u16(data);
    printf(", \"protection_profiles\": [");
    bool first = true;
    for (size_t i = 2; i + 1 < 2 + profiles_len && i + 1 < len; i += 2) {
        uint16_t profile = read_u16(data + i);
        if (!first) printf(", ");
        first = false;
        const char* name;
        switch (profile) {
            case 0x0001: name = "SRTP_AES128_CM_HMAC_SHA1_80"; break;
            case 0x0002: name = "SRTP_AES128_CM_HMAC_SHA1_32"; break;
            case 0x0005: name = "SRTP_AES128_F8_HMAC_SHA1_80"; break;
            case 0x0007: name = "SRTP_NULL_HMAC_SHA1_80"; break;
            case 0x0008: name = "SRTP_NULL_HMAC_SHA1_32"; break;
            case 0x0009: name = "SRTP_AEAD_AES_128_GCM"; break;
            case 0x000A: name = "SRTP_AEAD_AES_256_GCM"; break;
            default:     name = "UNKNOWN"; break;
        }
        printf("\"%s (0x%04X)\"", name, profile);
    }
    size_t mki_off = 2 + profiles_len;
    if (mki_off < len)
        printf("], \"mki_length\": %d\n", data[mki_off]);
    else
        printf("]\n");
}

// 0x000F: heartbeat (RFC 6520)
static void parse_heartbeat(const uint8_t* data, size_t len) {
    if (len < 1) { printf("\n"); return; }
    uint8_t mode = data[0];
    const char* s = (mode == 1) ? "peer_allowed_to_send" :
                    (mode == 2) ? "peer_not_allowed_to_send" : "UNKNOWN";
    printf(", \"mode\": \"%s (%d)\"\n", s, mode);
}

// 0x0022: delegated_credential (RFC 9345)
// В ClientHello — список поддерживаемых алгоритмов подписи (SignatureSchemeList)
static void parse_delegated_credential(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    uint16_t list_len = read_u16(data);
    printf(", \"signature_algorithms\": [");
    bool first = true;
    for (size_t i = 2; i + 1 < 2 + list_len && i + 1 < len; i += 2) {
        uint16_t alg = read_u16(data + i);
        if (!first) printf(", ");
        first = false;
        printf("\"%s\"", sig_alg_name(alg));
    }
    printf("]\n");
}

// 0x003E: tls_flags (draft-ietf-tls-tlsflags)
// Данные — битовый вектор (переменная длина), каждый бит — отдельный флаг
static void parse_tls_flags(const uint8_t* data, size_t len) {
    if (len == 0) { printf("\n"); return; }
    printf(", \"flags\": \"0x");
    for (size_t i = 0; i < len; ++i) printf("%02X", data[i]);
    printf("\"\n");
}

// 0x0039 / 0xFFA5: quic_transport_parameters (RFC 9001)
// Параметры кодируются как последовательность TLV с QUIC-varint (RFC 9000 §16)
static void parse_quic_transport_params(const uint8_t* data, size_t len) {
    printf(", \"transport_params\": [");
    size_t off = 0;
    bool first = true;
    while (off < len) {
        // Читаем тип параметра (QUIC varint)
        uint8_t b = data[off];
        size_t vlen = 1u << (b >> 6);
        if (off + vlen > len) break;
        uint64_t param_id = b & 0x3F;
        for (size_t j = 1; j < vlen; ++j) param_id = (param_id << 8) | data[off + j];
        off += vlen;
        // Читаем длину значения (QUIC varint)
        if (off >= len) break;
        b = data[off];
        vlen = 1u << (b >> 6);
        if (off + vlen > len) break;
        uint64_t param_len = b & 0x3F;
        for (size_t j = 1; j < vlen; ++j) param_len = (param_len << 8) | data[off + j];
        off += vlen;
        if (off + (size_t)param_len > len) break;
        if (!first) printf(", ");
        first = false;
        printf("{\"id\": \"0x%04llX\", \"len\": %llu}",
               (unsigned long long)param_id, (unsigned long long)param_len);
        off += (size_t)param_len;
    }
    printf("]\n");
}

// 0xFD00: ech_outer_extensions (RFC 9420)
// Список типов расширений, которые вынесены из inner ClientHello в outer
static void parse_ech_outer_extensions(const uint8_t* data, size_t len) {
    if (len < 1) { printf("\n"); return; }
    uint8_t list_bytes = data[0];
    printf(", \"outer_extensions\": [");
    bool first = true;
    for (size_t i = 1; i + 1 < len && i <= list_bytes; i += 2) {
        uint16_t ext_type = read_u16(data + i);
        if (!first) printf(", ");
        first = false;
        printf("\"%s (0x%04X)\"", ext_name(ext_type), ext_type);
    }
    printf("]\n");
}

// ==================== ГЛАВНЫЙ ДИСПЕТЧЕР ====================

void parse_extensions_chrome_style(const uint8_t* data, size_t data_len, size_t start_offset) {
    size_t off = start_offset;
    if (off + 2 > data_len) { printf("[ERR] Truncated at extensions_length\n"); return; }
    
    uint16_t ext_total_len = read_u16(data + off);
    off += 2;
    if (off + ext_total_len > data_len) { printf("[ERR] Truncated inside extensions\n"); return; }
    size_t ext_end = off + ext_total_len;
    
    printf("  \"extensions\": [\n");
    
    int idx = 0;
    while (off < ext_end) {
        if (off + 4 > ext_end) break;
        
        uint16_t type = read_u16(data + off);
        uint16_t len  = read_u16(data + off + 2);
        off += 4;
        if (off + len > ext_end) break;
        
        if (idx > 0) printf(",\n");
        printf("      {\n        \"name\": \"%s (%d)\"", ext_name(type), type);
        
        // Диспетчер по типам
        switch(type) {
            case 0x0000: parse_server_name(data + off, len); break;
            case 0x0001: parse_max_fragment_length(data + off, len); break;  // max_fragment_length
            case 0x0005: parse_status_request(data + off, len); break;
            case 0x000A: parse_supported_groups(data + off, len); break;
            case 0x000B: parse_ec_point_formats(data + off, len); break;
            case 0x000D: parse_signature_algorithms(data + off, len); break;
            case 0x000E: parse_use_srtp(data + off, len); break;             // use_srtp (DTLS)
            case 0x000F: parse_heartbeat(data + off, len); break;            // heartbeat
            case 0x0010: parse_alpn(data + off, len); break;
            case 0x0012: parse_sct(data + off, len); break;
            case 0x0015: parse_padding(data + off, len); break;
            case 0x0016: printf("\n"); break;                                // encrypt_then_mac (пустое)
            case 0x0017: parse_extended_master_secret(data + off, len); break;
            case 0x001B: parse_compress_certificate(data + off, len); break;
            case 0x001C: parse_record_size_limit(data + off, len); break;
            case 0x0022: parse_delegated_credential(data + off, len); break; // delegated_credential
            case 0x0023: parse_session_ticket(data + off, len); break;
            case 0x0029: parse_pre_shared_key(data + off, len); break;
            case 0x002A: parse_early_data(data + off, len); break;
            case 0x002B: parse_supported_versions(data + off, len); break;
            case 0x002C: parse_cookie(data + off, len); break;
            case 0x002D: parse_psk_key_exchange_modes(data + off, len); break;
            case 0x002F: parse_certificate_authorities(data + off, len); break;
            case 0x0031: printf("\n"); break;                                // post_handshake_auth (пустое)
            case 0x0032: parse_signature_algorithms(data + off, len); break; // signature_algorithms_cert
            case 0x0033: parse_key_share(data + off, len); break;
            case 0x0039: parse_quic_transport_params(data + off, len); break; // quic_transport_parameters
            case 0x003E: parse_tls_flags(data + off, len); break;            // tls_flags
            case 0x3374: parse_npn(data + off, len); break;
            case 0x4469: parse_alps_old(data + off, len); break;
            case 0x44CD: parse_alps_old(data + off, len); break;
            case 0x7550: printf("\n"); break;                                // channel_id (пустое в ClientHello)
            case 0xCA34: printf("\n"); break;                                // trust_anchors
            case 0xFD00: parse_ech_outer_extensions(data + off, len); break; // ech_outer_extensions
            case 0xFE0D: parse_ech(data + off, len); break;
            case 0xFF01: parse_renegotiation_info(data + off, len); break;
            case 0xFFA5: parse_quic_transport_params(data + off, len); break; // quic_transport_parameters_legacy
            default:
                // Для GREASE и неизвестных — просто закрываем объект
                printf("\n");
        }
        
        printf("      }");
        off += len;
        idx++;
    }
    printf("\n    ]\n");
}


/**
 * @brief Метод парсинга данных цифрового отпечатка
 *
 * @param buffer бинарный буфер данных цифрового отпечатка
 * @param size   размер бинарного буфера данных цифрового отпечатка
 * @return       результат парсинга данных цифрового отпечатка
 */
bool awh::Fingerprint::parse(const uint8_t * buffer, const size_t size) noexcept {

	// Данные для вычисления отпечатков (заполняются по ходу парсинга)
	std::vector<uint16_t> fp_ciphers;
	std::vector<uint16_t> fp_ext_types;
	std::vector<uint16_t> fp_groups;
	std::vector<uint16_t> fp_sig_algs;
	std::vector<uint16_t> fp_versions;
	std::vector<uint8_t>  fp_point_fmts;
	std::string fp_alpn_first;
	bool fp_has_sni = false;
	bool fp_has_session_ticket = false;
	uint8_t fp_comp_raw_len = 0;
	size_t fp_ext_start = 0;
	std::string fp_session_id_hex;
	std::string fp_random_hex;

	if(size < 11){
		printf("[ERR] Too short: %zu bytes (need >= 11)\n", size);
		return false;
	}

	// 1. Поле версии в record header определяет TLS vs DTLS.
	// DTLS использует «инвертированные» номера версий: 0xFEFF = DTLS 1.0, 0xFEFD = DTLS 1.2.
	const uint8_t  content_type   = buffer[0];
	const uint16_t record_version = read_u16(buffer + 1);
	const bool     is_dtls        = (record_version == 0xFEFF || record_version == 0xFEFD);

	// Размеры заголовков зависят от протокола:
	//   TLS:  record header  =  5 байт (type:1 + version:2 + length:2)
	//   DTLS: record header  = 13 байт (type:1 + version:2 + epoch:2 + seq:6 + length:2)
	//   TLS:  handshake hdr  =  4 байта (type:1 + length:3)
	//   DTLS: handshake hdr  = 12 байт  (type:1 + length:3 + msg_seq:2 + frag_off:3 + frag_len:3)
	const size_t rec_hdr = is_dtls ? 13u : 5u;
	const size_t hs_hdr  = is_dtls ? 12u : 4u;

	if(size < rec_hdr + hs_hdr + 2u + 32u){
		printf("[ERR] Too short for %s headers: %zu bytes\n", is_dtls ? "DTLS" : "TLS", size);
		return false;
	}

	// Длина полезной нагрузки record: в TLS — offset 3, в DTLS — offset 11
	const uint16_t record_length = is_dtls ? read_u16(buffer + 11) : read_u16(buffer + 3);

	// Строковое представление версии record
	string version_str;
	if(is_dtls){
		version_str = (record_version == 0xFEFF) ? "DTLS 1.0" : "DTLS 1.2";
	} else {
		switch(record_version){
			case 0x0300: version_str = "SSL 3.0"; break;
			case 0x0301: version_str = "TLS 1.0"; break;
			case 0x0302: version_str = "TLS 1.1"; break;
			case 0x0303: version_str = "TLS 1.2"; break;
			case 0x0304: version_str = "TLS 1.3"; break;
			default:
				printf("[ERR] Unsupported record version: 0x%04X\n", record_version);
				return false;
		}
	}

	printf("Record:\n");
	printf("  content_type       = %d (0x%02X) %s\n",
	       content_type, content_type, content_type == 0x16 ? "(handshake)" : "");
	printf("  legacy_version     = %d (0x%04X) %s\n", record_version, record_version, version_str.c_str());
	if(is_dtls){
		const uint16_t epoch = read_u16(buffer + 3);
		printf("  epoch              = %d\n", epoch);
		printf("  sequence_number    = %02X%02X%02X%02X%02X%02X\n",
		       buffer[5], buffer[6], buffer[7], buffer[8], buffer[9], buffer[10]);
	}
	printf("  record_length      = %d bytes\n", record_length);

	if(content_type != 0x16){
		printf("[WARN] Not a handshake record. Skipping.\n");
		return false;
	}

	// 2. Handshake Header (начинается сразу после record header)
	const uint8_t  msg_type         = buffer[rec_hdr];
	const uint32_t handshake_length = (static_cast<uint32_t>(buffer[rec_hdr + 1]) << 16) |
	                                  (static_cast<uint32_t>(buffer[rec_hdr + 2]) << 8)  |
	                                   static_cast<uint32_t>(buffer[rec_hdr + 3]);

	printf("Handshake:\n");
	printf("  msg_type           = %d (0x%02X) %s\n",
	       msg_type, msg_type, msg_type == 0x01 ? "(client_hello)" : "");
	printf("  handshake_length   = %d bytes\n", handshake_length);
	if(is_dtls){
		const uint16_t message_seq     = read_u16(buffer + rec_hdr + 4);
		const uint32_t fragment_offset = (static_cast<uint32_t>(buffer[rec_hdr + 6]) << 16) |
		                                 (static_cast<uint32_t>(buffer[rec_hdr + 7]) << 8)  |
		                                  static_cast<uint32_t>(buffer[rec_hdr + 8]);
		const uint32_t fragment_length = (static_cast<uint32_t>(buffer[rec_hdr + 9])  << 16) |
		                                 (static_cast<uint32_t>(buffer[rec_hdr + 10]) << 8)  |
		                                  static_cast<uint32_t>(buffer[rec_hdr + 11]);
		printf("  message_seq        = %d\n",  message_seq);
		printf("  fragment_offset    = %d\n",  fragment_offset);
		printf("  fragment_length    = %d\n",  fragment_length);
	}

	if(msg_type != 0x01){
		printf("[WARN] Not a ClientHello.\n");
		return false;
	}

	// 3. ClientHello payload начинается сразу после обоих заголовков
	const size_t ch_base = rec_hdr + hs_hdr;

	const uint16_t client_version = read_u16(buffer + ch_base);

	if(is_dtls){
		version_str = (client_version == 0xFEFF) ? "DTLS 1.0" :
		              (client_version == 0xFEFD) ? "DTLS 1.2" : "DTLS (unknown)";
	} else {
		switch(client_version){
			case 0x0300: version_str = "SSL 3.0"; break;
			case 0x0301: version_str = "TLS 1.0"; break;
			case 0x0302: version_str = "TLS 1.1"; break;
			case 0x0303: version_str = "TLS 1.2"; break;
			case 0x0304: version_str = "TLS 1.3"; break;
			default:
				printf("[ERR] Unsupported ClientHello version: 0x%04X\n", client_version);
				return false;
		}
	}

	// Random: 32 байта после client_version
	const size_t random_off = ch_base + 2;

	printf("ClientHello:\n");
	printf("  legacy_version     = %d (0x%04X) %s\n", client_version, client_version, version_str.c_str());
	printf("  random             = ");
	for (size_t i = 0; i < 32; ++i) {
		if (i > 0 && i % 8 == 0) printf("\n                       ");
		printf("%02X ", buffer[random_off + i]);
	}
	printf(" (first 4 bytes = gmt_unix_time)\n");
	{
		char _rnd[65] = {};
		for (size_t _i = 0; _i < 32; ++_i) snprintf(_rnd + 2*_i, 3, "%02x", buffer[random_off + _i]);
		fp_random_hex = _rnd;
	}

	// off = начало переменных полей (session_id_len)
	// TLS:  rec(5)  + hs(4)  + version(2) + random(32) = 43
	// DTLS: rec(13) + hs(12) + version(2) + random(32) = 59
	size_t off = ch_base + 2 + 32;

	if(size < off + 1){ printf("[ERR] Truncated at session_id_len\n"); return false; }

	// 4. legacy_session_id
	const uint8_t sess_len = buffer[off++];
	if(sess_len > 32){ printf("[ERR] session_id_len > 32 (%d)\n", sess_len); return false; }
	if(off + sess_len > size){ printf("[ERR] Truncated at session_id\n"); return false; }

	printf("Session ID:\n");
	printf("  length    = %d\n", sess_len);
	printf("  value     = "); print_hex(buffer + off, sess_len); printf("\n");
	{
		char _sid[65] = {};
		for (int _i = 0; _i < sess_len; _i++) snprintf(_sid + 2*_i, 3, "%02x", buffer[off + _i]);
		fp_session_id_hex = _sid;
	}
	off += sess_len;

	// 5. DTLS: cookie (только для DTLS ClientHello, RFC 6347 §4.2.1)
	if(is_dtls){
		if(off >= size){ printf("[ERR] Truncated at cookie_len\n"); return false; }
		const uint8_t cookie_len = buffer[off++];
		if(off + cookie_len > size){ printf("[ERR] Truncated at cookie data\n"); return false; }
		printf("Cookie (DTLS):\n");
		printf("  length    = %d\n", cookie_len);
		printf("  value     = "); print_hex(buffer + off, cookie_len); printf("\n");
		off += cookie_len;
	}

	// 6. cipher_suites
	if(off + 2 > size){ printf("[ERR] Truncated at cipher_suites_len\n"); return false; }
	const uint16_t cs_len = read_u16(buffer + off);
	off += 2;
	if(off + cs_len > size || cs_len % 2 != 0){
		printf("[ERR] Invalid cipher_suites length (%d)\n", cs_len); return false;
	}

	printf("Cipher Suites:\n");
	printf("  total_bytes = %d\n", cs_len);
	printf("  count       = %d\n", cs_len / 2);
	for(size_t i = 0; i < cs_len; i += 2){
		const uint16_t id = read_u16(buffer + off + i);
		printf("    [%2zu] 0x%04X %-45s\n", i / 2, id, cipher_name(id));
		fp_ciphers.push_back(id);
	}
	off += cs_len;

	// 7. legacy_compression_methods
	if(off + 1 > size){ printf("[ERR] Truncated at compression_methods_len\n"); return false; }
	const uint8_t comp_len = buffer[off++];
	fp_comp_raw_len = static_cast<uint8_t>(1 + comp_len);
	if(off + comp_len > size){ printf("[ERR] Truncated at compression_methods\n"); return false; }
	if(comp_len != 1 || buffer[off] != 0x00){
		printf("[WARN] Non-standard compression_methods (len=%d, val=0x%02X)\n", comp_len, buffer[off]);
	}

	printf("Compression Methods:\n");
	printf("  length    = %d\n", comp_len);
	printf("  value     = 0x%02X %s\n", buffer[off],
	       (comp_len == 1 && buffer[off] == 0x00) ? "(null)" : "(unknown)");
	off += comp_len;

	// 8. Extensions
	printf("\n[Next] extensions_length begins at offset %zu\n", off);
	fp_ext_start = off;
	parse_extensions_chrome_style(buffer, size, off);

	// ==================== ВТОРОЙ ПРОХОД: СБОР ДАННЫХ ДЛЯ ОТПЕЧАТКОВ ====================
	if (fp_ext_start + 2 <= size) {
		const uint16_t ext_total = read_u16(buffer + fp_ext_start);
		size_t eoff = fp_ext_start + 2;
		const size_t eend = eoff + ext_total;
		while (eoff + 4 <= eend && eoff + 4 <= size) {
			const uint16_t etype = read_u16(buffer + eoff);
			const uint16_t elen  = read_u16(buffer + eoff + 2);
			eoff += 4;
			if (eoff + elen > size) break;
			const uint8_t* edata = buffer + eoff;
			fp_ext_types.push_back(etype);
			switch (etype) {
				case 0x0000: fp_has_sni = true; break;
				case 0x0023: fp_has_session_ticket = true; break;
				case 0x000A: // supported_groups
					if (elen >= 2) {
						const uint16_t gl = read_u16(edata);
						for (size_t i = 2; i + 1 < elen && i < static_cast<size_t>(2 + gl); i += 2)
							fp_groups.push_back(read_u16(edata + i));
					}
					break;
				case 0x000B: // ec_point_formats
					if (elen >= 1) {
						const uint8_t cnt = edata[0];
						for (uint8_t i = 0; i < cnt && 1 + i < elen; ++i)
							fp_point_fmts.push_back(edata[1 + i]);
					}
					break;
				case 0x000D: // signature_algorithms
					if (elen >= 2) {
						const uint16_t sl = read_u16(edata);
						for (size_t i = 2; i + 1 < elen && i < static_cast<size_t>(2 + sl); i += 2)
							fp_sig_algs.push_back(read_u16(edata + i));
					}
					break;
				case 0x0010: // ALPN
					if (elen >= 4 && fp_alpn_first.empty()) {
						const uint8_t plen = edata[2];
						if (3u + plen <= elen)
							fp_alpn_first = std::string(reinterpret_cast<const char*>(edata + 3), plen);
					}
					break;
				case 0x002B: // supported_versions
					if (elen >= 1) {
						const uint8_t vl = edata[0];
						for (size_t i = 1; i + 1 < elen && i < static_cast<size_t>(1 + vl); i += 2)
							fp_versions.push_back(read_u16(edata + i));
					}
					break;
			}
			eoff += elen;
		}
	}

	// ==================== ВЫЧИСЛЕНИЕ ОТПЕЧАТКОВ ====================

	// Вспомогательные лямбды (без захвата — используют только file-scope функции)
	auto join_dash = [](const std::vector<uint16_t>& v, bool skip_grease) -> std::string {
		std::string r;
		for (auto x : v) {
			if (skip_grease && is_grease(x)) continue;
			if (!r.empty()) r += '-';
			r += std::to_string(x);
		}
		return r;
	};
	auto join_hex4_comma = [](const std::vector<uint16_t>& v, bool skip_grease) -> std::string {
		std::string r;
		for (auto x : v) {
			if (skip_grease && is_grease(x)) continue;
			if (!r.empty()) r += ',';
			char buf[5]; snprintf(buf, sizeof(buf), "%04x", static_cast<unsigned>(x));
			r += buf;
		}
		return r;
	};
	auto join_peet = [](const std::vector<uint16_t>& v) -> std::string {
		std::string r;
		for (auto x : v) {
			if (!r.empty()) r += '-';
			if (is_grease(x)) r += "GREASE";
			else r += std::to_string(x);
		}
		return r;
	};
	auto md5_hex = [](const std::string& s) -> std::string {
		uint8_t d[16];
		MD5(reinterpret_cast<const uint8_t*>(s.data()), s.size(), d);
		char out[33] = {};
		for (int i = 0; i < 16; ++i) snprintf(out + 2*i, 3, "%02x", d[i]);
		return std::string(out, 32);
	};
	auto sha256_12 = [](const std::string& s) -> std::string {
		uint8_t d[32];
		SHA256(reinterpret_cast<const uint8_t*>(s.data()), s.size(), d);
		char out[13] = {};
		for (int i = 0; i < 6; ++i) snprintf(out + 2*i, 3, "%02x", d[i]);
		return std::string(out, 12);
	};

	// Определяем согласованную версию (первая не-GREASE из supported_versions)
	uint16_t neg_ver = client_version;
	for (auto v : fp_versions) {
		if (!is_grease(v)) { neg_ver = v; break; }
	}

	printf("\nFingerprints:\n");
	printf("  tls_version_record     = \"%u\"\n",  static_cast<unsigned>(record_version));
	printf("  tls_version_negotiated = \"%u\"\n",  static_cast<unsigned>(neg_ver));

	// --- JA3 ---
	{
		std::string ja3;
		ja3 += std::to_string(client_version);         ja3 += ',';
		ja3 += join_dash(fp_ciphers,   true);           ja3 += ',';
		ja3 += join_dash(fp_ext_types, true);           ja3 += ',';
		ja3 += join_dash(fp_groups,    true);           ja3 += ',';
		std::string pts;
		for (auto p : fp_point_fmts) { if (!pts.empty()) pts += '-'; pts += std::to_string(p); }
		ja3 += pts;
		printf("  ja3                    = \"%s\"\n", ja3.c_str());
		printf("  ja3_hash               = \"%s\"\n", md5_hex(ja3).c_str());
	}

	// --- JA4 / JA4_r ---
	{
		const char proto = is_dtls ? 'd' : 't';
		const char* ver_str;
		switch (neg_ver) {
			case 0x0304: ver_str = "13"; break;
			case 0x0303: ver_str = "12"; break;
			case 0x0302: ver_str = "11"; break;
			case 0x0301: ver_str = "10"; break;
			case 0xFEFD: ver_str = "d2"; break;
			case 0xFEFF: ver_str = "d1"; break;
			default:     ver_str = "00"; break;
		}
		const char sni_flag = fp_has_sni ? 'd' : 'i';

		int cipher_cnt = 0;
		for (auto c : fp_ciphers)   if (!is_grease(c)) cipher_cnt++;
		int ext_cnt = 0;
		for (auto e : fp_ext_types) if (!is_grease(e)) ext_cnt++;

		// ALPN: первые 2 символа (или "00")
		std::string alpn_part = "00";
		if (!fp_alpn_first.empty()) {
			alpn_part = fp_alpn_first.substr(0, 2);
			while (alpn_part.size() < 2) alpn_part += '0';
		}

		// Отсортированные cipher (не-GREASE) → хэш
		std::vector<uint16_t> sc;
		for (auto c : fp_ciphers) if (!is_grease(c)) sc.push_back(c);
		std::sort(sc.begin(), sc.end());
		const std::string sc_hex4 = join_hex4_comma(sc, false);

		// Отсортированные ext (не-GREASE, без SNI и ALPN) → хэш
		std::vector<uint16_t> se;
		for (auto e : fp_ext_types) if (!is_grease(e) && e != 0x0000 && e != 0x0010) se.push_back(e);
		std::sort(se.begin(), se.end());
		const std::string se_hex4  = join_hex4_comma(se, false);
		const std::string sig_hex4 = join_hex4_comma(fp_sig_algs, false);

		char ja4_buf[128];
		snprintf(ja4_buf, sizeof(ja4_buf), "%c%s%c%02d%02d%s_%s_%s",
		         proto, ver_str, sni_flag, cipher_cnt, ext_cnt, alpn_part.c_str(),
		         sha256_12(sc_hex4).c_str(),
		         sha256_12(se_hex4 + "_" + sig_hex4).c_str());
		printf("  ja4                    = \"%s\"\n", ja4_buf);

		char ja4r_pref[32];
		snprintf(ja4r_pref, sizeof(ja4r_pref), "%c%s%c%02d%02d%s",
		         proto, ver_str, sni_flag, cipher_cnt, ext_cnt, alpn_part.c_str());
		printf("  ja4_r                  = \"%s_%s_%s_%s\"\n",
		       ja4r_pref, sc_hex4.c_str(), se_hex4.c_str(), sig_hex4.c_str());
	}

	// --- PeetPrint ---
	{
		// Field 1: supported_versions (incl. GREASE, в порядке, decimal)
		const std::string f1 = join_peet(fp_versions);

		// Field 2: {sess_len>>4}-{sess_id_present}.{session_ticket_present}
		// sess_len>>4 = 32/16=2 для Chrome (fake 32-byte session ID)
		char f2[16];
		snprintf(f2, sizeof(f2), "%d-%d.%d",
		         sess_len >> 4,
		         (sess_len > 0) ? 1 : 0,
		         fp_has_session_ticket ? 1 : 0);

		// Field 3: supported_groups (incl. GREASE, в порядке, decimal)
		const std::string f3 = join_peet(fp_groups);

		// Field 4: sig_algs (в порядке, decimal, без GREASE — у sig_algs GREASE не бывает)
		const std::string f4 = join_dash(fp_sig_algs, false);

		// Field 5: количество EC point formats
		const std::string f5 = std::to_string(fp_point_fmts.size());

		// Field 6: raw-длина поля compression_methods (1 байт счётчика + comp_len байт значений)
		const std::string f6 = std::to_string(fp_comp_raw_len);

		// Field 7: cipher_suites (incl. GREASE, в порядке, decimal)
		const std::string f7 = join_peet(fp_ciphers);

		// Field 8: extension_types (incl. GREASE, в порядке, decimal)
		const std::string f8 = join_peet(fp_ext_types);

		const std::string peet = f1+"|"+f2+"|"+f3+"|"+f4+"|"+f5+"|"+f6+"|"+f7+"|"+f8;
		printf("  peetprint              = \"%s\"\n", peet.c_str());
		printf("  peetprint_hash         = \"%s\"\n", md5_hex(peet).c_str());
	}

	// --- client_random / session_id ---
	printf("  client_random          = \"%s\"\n", fp_random_hex.c_str());
	printf("  session_id             = \"%s\"\n", fp_session_id_hex.c_str());

	return true;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Fingerprint::Fingerprint(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::Fingerprint::~Fingerprint() noexcept {}
