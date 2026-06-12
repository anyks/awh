// http_parser.hpp
#pragma once
#include <cstdint>
#include <cstddef>
#include <string_view>
#include <type_traits>

#include "http_parser_simd.hpp"

namespace http_parser {

// ============================================================================
// ENUMS & CONSTANTS
// ============================================================================
enum class parse_result_t : int8_t {
    OK = 0,
    PAUSED = 1,
    INCOMPLETE = 2,
    ERROR_INVALID_METHOD = -1,
    ERROR_INVALID_URL = -2,
    ERROR_INVALID_VERSION = -3,
    ERROR_INVALID_STATUS = -4,
    ERROR_INVALID_HEADER_NAME = -5,
    ERROR_INVALID_HEADER_VALUE = -6,
    ERROR_INVALID_CHUNK_SIZE = -7,
    ERROR_INVALID_CHUNK_EXT = -8,
    ERROR_INVALID_TRAILER = -9,
    ERROR_HEADER_OVERFLOW = -10,
    ERROR_CONTENT_LENGTH_OVERFLOW = -11,
    ERROR_UNEXPECTED_DATA = -12,
    ERROR_CLOSED_CONNECTION = -13,
    ERROR_STRICT_VALIDATION = -14,
};

enum class message_type_t : uint8_t {
    UNKNOWN = 0,
    REQUEST = 1,
    RESPONSE = 2,
};

enum class method_t : uint8_t {
    DELETE = 0, GET, HEAD, POST, PUT, CONNECT, OPTIONS, TRACE,
    COPY, LOCK, MKCOL, MOVE, PROPFIND, PROPPATCH, SEARCH, UNLOCK, BIND,
    REBIND, UNBIND, ACL, REPORT, MKACTIVITY, CHECKOUT, MERGE,
    MSEARCH, NOTIFY, SUBSCRIBE, UNSUBSCRIBE, PATCH, PURGE, MKCALENDAR,
    LINK, UNLINK, PRI, SOURCE,
    UNKNOWN_METHOD,
};

// ============================================================================
// CONFIGURATION (Compile-time)
// ============================================================================
namespace config {
    static constexpr size_t MAX_HEADER_NAME_LEN = 256;
    static constexpr size_t MAX_HEADER_VALUE_LEN = 8192;
    static constexpr size_t MAX_HEADERS = 256;
    static constexpr size_t MAX_METHOD_LEN = 32;
    static constexpr size_t MAX_URL_LEN = 8192;
    static constexpr size_t MAX_CHUNK_EXT_LEN = 256;
    static constexpr bool STRICT_VALIDATION = true;
    static constexpr bool ALLOW_OBS_FOLD = false; // Obsolete line folding
}

// ============================================================================
// INTERNAL STATE MACHINE STATES
// ============================================================================
namespace internal {
    enum class state_t : uint16_t {
        START = 0,
        
        // Request line states
        REQ_METHOD,
        REQ_METHOD_COMPLETE,
        REQ_URL,
        REQ_URL_COMPLETE,
        REQ_HTTP_VERSION,
        REQ_HTTP_H,
        REQ_HTTP_HT,
        REQ_HTTP_HTT,
        REQ_HTTP_HTTP,
        REQ_HTTP_HTTP_SLASH,
        REQ_HTTP_VERSION_MAJOR,
        REQ_HTTP_VERSION_MINOR,
        REQ_LINE_ENDING_CR,
        REQ_LINE_ENDING_LF,
        
        // Status line states (Response)
        RES_HTTP_VERSION,
        RES_STATUS_CODE,
        RES_STATUS_CODE_DIGIT_1,
        RES_STATUS_CODE_DIGIT_2,
        RES_STATUS_CODE_DIGIT_3,
        RES_REASON_PHRASE,
        RES_LINE_ENDING_CR,
        RES_LINE_ENDING_LF,
        
        // Header states
        HEADER_NAME,
        HEADER_NAME_WS,
        HEADER_COLON,
        HEADER_VALUE_START_WS,
        HEADER_VALUE,
        HEADER_VALUE_WS,
        HEADER_VALUE_ENDING_CR,
        HEADER_VALUE_ENDING_LF,
        HEADERS_ENDING_CR,
        HEADERS_ENDING_LF,
        
        // Body states
        BODY_IDENTITY,
        BODY_CHUNKED_SIZE,
        BODY_CHUNKED_SIZE_EXT,
        BODY_CHUNKED_SIZE_CR,
        BODY_CHUNKED_SIZE_LF,
        BODY_CHUNKED_DATA,
        BODY_CHUNKED_DATA_CR,
        BODY_CHUNKED_DATA_LF,
        BODY_CHUNKED_TRAILER_NAME,
        BODY_CHUNKED_TRAILER_VALUE,
        BODY_CHUNKED_TRAILER_END_CR,
        BODY_CHUNKED_TRAILER_END_LF,
        BODY_EOF,
        
        DONE,
        DEAD, // Terminal error state
    };

    // ============================================================================
    // PARSER CONTEXT (Plain Old Data - No VTable, Cache-friendly)
    // ============================================================================
    struct context_t {
        state_t state;
        message_type_t type;
        method_t method;
        
        uint16_t status_code;
        uint8_t http_major;
        uint8_t http_minor;
        
        uint64_t content_length; // -1 if not specified
        uint64_t body_received;
        uint64_t chunk_size;
        
        uint32_t header_count;
        
        // Current parsing position markers (for zero-copy)
        const char* current_mark;
        const char* header_name_start;
        const char* header_name_end;
        const char* header_value_start;
        const char* header_value_end;
        const char* url_start;
        const char* url_end;
        const char* reason_start;
        const char* reason_end;
        
        // Flags
        bool is_keep_alive;
        bool has_content_length;
        bool has_transfer_encoding_chunked;
        bool connection_close;
        bool upgrade;
        bool strict_mode;
        
        // Buffering for line folding (if allowed)
        char fold_buffer[config::MAX_HEADER_VALUE_LEN];
        size_t fold_buffer_len;
    };

    // Static assertions for cache line alignment
    static_assert(sizeof(context_t) <= 128, "context_t should fit in 2 cache lines");
}

// ============================================================================
// PUBLIC CONTEXT
// ============================================================================
using context_t = internal::context_t;
using state_t = internal::state_t;

// ============================================================================
// INITIALIZATION & RESET
// ============================================================================
static void init_context(context_t& ctx) noexcept {
    ctx.state = internal::state_t::START;
    ctx.type = message_type_t::UNKNOWN;
    ctx.method = method_t::UNKNOWN_METHOD;
    ctx.status_code = 0;
    ctx.http_major = 0;
    ctx.http_minor = 0;
    ctx.content_length = UINT64_MAX;
    ctx.body_received = 0;
    ctx.chunk_size = 0;
    ctx.header_count = 0;
    ctx.current_mark = nullptr;
    ctx.header_name_start = nullptr;
    ctx.header_name_end = nullptr;
    ctx.header_value_start = nullptr;
    ctx.header_value_end = nullptr;
    ctx.url_start = nullptr;
    ctx.url_end = nullptr;
    ctx.reason_start = nullptr;
    ctx.reason_end = nullptr;
    ctx.is_keep_alive = false;
    ctx.has_content_length = false;
    ctx.has_transfer_encoding_chunked = false;
    ctx.connection_close = false;
    ctx.upgrade = false;
    ctx.strict_mode = config::STRICT_VALIDATION;
    ctx.fold_buffer_len = 0;
}

static void reset_context(context_t& ctx) noexcept {
    auto strict = ctx.strict_mode;
    init_context(ctx);
    ctx.strict_mode = strict;
}

// ============================================================================
// VECTORIZED CHARACTER SEARCH (SSE4.2 / AVX2 / Fallback)
// ============================================================================
namespace simd {
    
    // Compile-time lookup tables for fast validation
    static constexpr bool is_token_char[256] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,1,0,1,1,1,1,1,0,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
        0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    };

    static constexpr bool is_hex_digit[256] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
        0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    };

    static constexpr bool is_digit[256] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    };

    static constexpr uint8_t hex_to_nibble[256] = {
        255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
        255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
        255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
        0,1,2,3,4,5,6,7,8,9,255,255,255,255,255,255,
        255,10,11,12,13,14,15,255,255,255,255,255,255,255,255,255,
        255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
        255,10,11,12,13,14,15,255,255,255,255,255,255,255,255,255,
        255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    };

    // Fast scan for CRLF using bit manipulation
    // Returns pointer to the first '\r' in "\r\n" sequence, or end if not found
    [[nodiscard]] static inline const char* find_crlf(const char* begin, const char* end) noexcept {
        const size_t len = static_cast<size_t>(end - begin);
        
        #if defined(__SSE4_2__) || defined(__AVX2__)
        // Vectorized search for CR (0x0D)
        if (len >= 16) {
            // Implementation using SSE4.2 _mm_cmpestrm or AVX2
            // For brevity, using fallback in this skeleton
            // Real implementation would use SIMD here
        }
        #endif
        
        // Optimized scalar fallback with loop unrolling
        const char* p = begin;
        const char* e = end - 1; // Need at least 2 bytes for \r\n
        for (; p < e; ++p) {
            if (*p == '\r' && p[1] == '\n') {
                return p;
            }
        }
        return end;
    }

    // Find first occurrence of character using SIMD
    [[nodiscard]] static inline const char* find_char(const char* begin, const char* end, char c) noexcept {
        for (const char* p = begin; p < end; ++p) {
            if (*p == c) return p;
        }
        return end;
    }

    // Skip linear whitespace (SP / HTAB)
    [[nodiscard]] static inline const char* skip_lws(const char* p, const char* end) noexcept {
        while (p < end && (*p == ' ' || *p == '\t')) ++p;
        return p;
    }
    
    // Validate token string
    [[nodiscard]] static inline bool validate_token(const char* begin, const char* end) noexcept {
        for (const char* p = begin; p < end; ++p) {
            if (!is_token_char[static_cast<uint8_t>(*p)]) return false;
        }
        return true;
    }

    // Validate URI string (simplified)
    [[nodiscard]] static inline bool validate_uri(const char* begin, const char* end) noexcept {
        // RFC 3986 validation
        for (const char* p = begin; p < end; ++p) {
            uint8_t c = static_cast<uint8_t>(*p);
            // Allow printable ASCII except control characters
            if (c < 0x21 || c == 0x7F) return false;
        }
        return true;
    }

    // Validate header value (RFC 7230 Section 3.2.6)
    [[nodiscard]] static inline bool validate_header_value(const char* begin, const char* end) noexcept {
        for (const char* p = begin; p < end; ++p) {
            uint8_t c = static_cast<uint8_t>(*p);
            // field-content = field-vchar [ 1*( SP / HTAB ) field-vchar ]
            // field-vchar   = VCHAR / obs-text
            // obs-text      = %x80-FF
            if (c < 0x09 || (c > 0x0A && c < 0x0D) || (c > 0x0D && c < 0x20) || c == 0x7F) {
                return false;
            }
        }
        return true;
    }
}

// ============================================================================
// METHOD RESOLUTION
// ============================================================================
namespace method_resolver {
    
    // Perfect hash / trie-based method lookup
    [[nodiscard]] static inline method_t resolve(const char* begin, size_t len) noexcept {
        // Fast path for common methods using first character + length
        if (len == 0) return method_t::UNKNOWN_METHOD;
        
        switch (begin[0]) {
            case 'G':
                if (len == 3 && begin[1] == 'E' && begin[2] == 'T') 
                    return method_t::GET;
                break;
            case 'P':
                if (len == 4) {
                    if (begin[1] == 'O' && begin[2] == 'S' && begin[3] == 'T') 
                        return method_t::POST;
                    if (begin[1] == 'U' && begin[2] == 'T') 
                        return method_t::PUT; // PUT is 3, but handle 4-char variants
                }
                if (len == 3 && begin[1] == 'U' && begin[2] == 'T')
                    return method_t::PUT;
                if (len == 5 && begin[1] == 'A' && begin[2] == 'T' && begin[3] == 'C' && begin[4] == 'H')
                    return method_t::PATCH;
                if (len == 5 && begin[1] == 'R' && begin[2] == 'O' && begin[3] == 'P' && begin[4] == 'F')
                    return method_t::PROPFIND;
                if (len == 6 && begin[1] == 'U' && begin[2] == 'R' && begin[3] == 'G' && begin[4] == 'E')
                    return method_t::PURGE;
                break;
            case 'D':
                if (len == 6 && begin[1] == 'E' && begin[2] == 'L' && begin[3] == 'E' && 
                    begin[4] == 'T' && begin[5] == 'E')
                    return method_t::DELETE;
                break;
            case 'H':
                if (len == 4 && begin[1] == 'E' && begin[2] == 'A' && begin[3] == 'D')
                    return method_t::HEAD;
                break;
            case 'C':
                if (len == 7 && begin[1] == 'O' && begin[2] == 'N' && begin[3] == 'N' &&
                    begin[4] == 'E' && begin[5] == 'C' && begin[6] == 'T')
                    return method_t::CONNECT;
                break;
            case 'O':
                if (len == 7 && begin[1] == 'P' && begin[2] == 'T' && begin[3] == 'I' &&
                    begin[4] == 'O' && begin[5] == 'N' && begin[6] == 'S')
                    return method_t::OPTIONS;
                break;
            case 'T':
                if (len == 5 && begin[1] == 'R' && begin[2] == 'A' && begin[3] == 'C' && begin[4] == 'E')
                    return method_t::TRACE;
                break;
        }
        
        // Generic fallback: string comparison for all other methods
        std::string_view sv(begin, len);
        if (sv == "COPY") return method_t::COPY;
        if (sv == "LOCK") return method_t::LOCK;
        if (sv == "MKCOL") return method_t::MKCOL;
        if (sv == "MOVE") return method_t::MOVE;
        if (sv == "PROPFIND") return method_t::PROPFIND;
        if (sv == "PROPPATCH") return method_t::PROPPATCH;
        if (sv == "SEARCH") return method_t::SEARCH;
        if (sv == "UNLOCK") return method_t::UNLOCK;
        if (sv == "BIND") return method_t::BIND;
        if (sv == "REBIND") return method_t::REBIND;
        if (sv == "UNBIND") return method_t::UNBIND;
        if (sv == "ACL") return method_t::ACL;
        if (sv == "REPORT") return method_t::REPORT;
        if (sv == "MKACTIVITY") return method_t::MKACTIVITY;
        if (sv == "CHECKOUT") return method_t::CHECKOUT;
        if (sv == "MERGE") return method_t::MERGE;
        if (sv == "M-SEARCH") return method_t::MSEARCH;
        if (sv == "NOTIFY") return method_t::NOTIFY;
        if (sv == "SUBSCRIBE") return method_t::SUBSCRIBE;
        if (sv == "UNSUBSCRIBE") return method_t::UNSUBSCRIBE;
        if (sv == "MKCALENDAR") return method_t::MKCALENDAR;
        if (sv == "LINK") return method_t::LINK;
        if (sv == "UNLINK") return method_t::UNLINK;
        if (sv == "PRI") return method_t::PRI;
        if (sv == "SOURCE") return method_t::SOURCE;
        
        return method_t::UNKNOWN_METHOD;
    }

    [[nodiscard]] static inline std::string_view method_to_string(method_t m) noexcept {
        switch (m) {
            case method_t::GET: return "GET";
            case method_t::POST: return "POST";
            case method_t::PUT: return "PUT";
            case method_t::DELETE: return "DELETE";
            case method_t::HEAD: return "HEAD";
            case method_t::OPTIONS: return "OPTIONS";
            case method_t::TRACE: return "TRACE";
            case method_t::CONNECT: return "CONNECT";
            case method_t::PATCH: return "PATCH";
            case method_t::COPY: return "COPY";
            case method_t::LOCK: return "LOCK";
            case method_t::MKCOL: return "MKCOL";
            case method_t::MOVE: return "MOVE";
            case method_t::PROPFIND: return "PROPFIND";
            case method_t::PROPPATCH: return "PROPPATCH";
            case method_t::SEARCH: return "SEARCH";
            case method_t::UNLOCK: return "UNLOCK";
            case method_t::BIND: return "BIND";
            case method_t::REBIND: return "REBIND";
            case method_t::UNBIND: return "UNBIND";
            case method_t::ACL: return "ACL";
            case method_t::REPORT: return "REPORT";
            case method_t::MKACTIVITY: return "MKACTIVITY";
            case method_t::CHECKOUT: return "CHECKOUT";
            case method_t::MERGE: return "MERGE";
            case method_t::MSEARCH: return "M-SEARCH";
            case method_t::NOTIFY: return "NOTIFY";
            case method_t::SUBSCRIBE: return "SUBSCRIBE";
            case method_t::UNSUBSCRIBE: return "UNSUBSCRIBE";
            case method_t::MKCALENDAR: return "MKCALENDAR";
            case method_t::LINK: return "LINK";
            case method_t::UNLINK: return "UNLINK";
            case method_t::PRI: return "PRI";
            case method_t::SOURCE: return "SOURCE";
            default: return "";
        }
    }
}

// ============================================================================
// HEADER NAME NORMALIZATION (Case-insensitive matching)
// ============================================================================
namespace header_names {
    
    // Fast case-insensitive comparison
    [[nodiscard]] static inline bool icase_equal(std::string_view a, std::string_view b) noexcept {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            char ca = a[i];
            char cb = b[i];
            if (ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca + 32);
            if (cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb + 32);
            if (ca != cb) return false;
        }
        return true;
    }

    // Well-known header IDs for fast dispatch
    enum class known_header_t : uint8_t {
        UNKNOWN = 0,
        CONTENT_LENGTH,
        TRANSFER_ENCODING,
        CONNECTION,
        HOST,
        UPGRADE,
        CONTENT_TYPE,
        CONTENT_ENCODING,
        ACCEPT,
        ACCEPT_ENCODING,
        ACCEPT_LANGUAGE,
        AUTHORIZATION,
        COOKIE,
        SET_COOKIE,
        CACHE_CONTROL,
        DATE,
        ETAG,
        IF_MATCH,
        IF_MODIFIED_SINCE,
        IF_NONE_MATCH,
        IF_RANGE,
        IF_UNMODIFIED_SINCE,
        LAST_MODIFIED,
        LOCATION,
        RANGE,
        REFERER,
        SERVER,
        USER_AGENT,
        WWW_AUTHENTICATE,
        VARY,
        VIA,
        X_FORWARDED_FOR,
        X_FORWARDED_PROTO,
        X_REAL_IP,
    };

    [[nodiscard]] static inline known_header_t identify(std::string_view name) noexcept {
        if (name.empty()) return known_header_t::UNKNOWN;
        
        switch (name[0]) {
            case 'C': case 'c':
                if (icase_equal(name, "Content-Length")) return known_header_t::CONTENT_LENGTH;
                if (icase_equal(name, "Content-Type")) return known_header_t::CONTENT_TYPE;
                if (icase_equal(name, "Content-Encoding")) return known_header_t::CONTENT_ENCODING;
                if (icase_equal(name, "Connection")) return known_header_t::CONNECTION;
                if (icase_equal(name, "Cookie")) return known_header_t::COOKIE;
                if (icase_equal(name, "Cache-Control")) return known_header_t::CACHE_CONTROL;
                break;
            case 'T': case 't':
                if (icase_equal(name, "Transfer-Encoding")) return known_header_t::TRANSFER_ENCODING;
                break;
            case 'H': case 'h':
                if (icase_equal(name, "Host")) return known_header_t::HOST;
                break;
            case 'U': case 'u':
                if (icase_equal(name, "Upgrade")) return known_header_t::UPGRADE;
                if (icase_equal(name, "User-Agent")) return known_header_t::USER_AGENT;
                break;
            case 'A': case 'a':
                if (icase_equal(name, "Authorization")) return known_header_t::AUTHORIZATION;
                if (icase_equal(name, "Accept")) return known_header_t::ACCEPT;
                if (icase_equal(name, "Accept-Encoding")) return known_header_t::ACCEPT_ENCODING;
                if (icase_equal(name, "Accept-Language")) return known_header_t::ACCEPT_LANGUAGE;
                break;
            case 'S': case 's':
                if (icase_equal(name, "Set-Cookie")) return known_header_t::SET_COOKIE;
                if (icase_equal(name, "Server")) return known_header_t::SERVER;
                break;
            case 'D': case 'd':
                if (icase_equal(name, "Date")) return known_header_t::DATE;
                break;
            case 'E': case 'e':
                if (icase_equal(name, "ETag")) return known_header_t::ETAG;
                break;
            case 'I': case 'i':
                if (icase_equal(name, "If-Match")) return known_header_t::IF_MATCH;
                if (icase_equal(name, "If-Modified-Since")) return known_header_t::IF_MODIFIED_SINCE;
                if (icase_equal(name, "If-None-Match")) return known_header_t::IF_NONE_MATCH;
                if (icase_equal(name, "If-Range")) return known_header_t::IF_RANGE;
                if (icase_equal(name, "If-Unmodified-Since")) return known_header_t::IF_UNMODIFIED_SINCE;
                break;
            case 'L': case 'l':
                if (icase_equal(name, "Last-Modified")) return known_header_t::LAST_MODIFIED;
                if (icase_equal(name, "Location")) return known_header_t::LOCATION;
                break;
            case 'R': case 'r':
                if (icase_equal(name, "Range")) return known_header_t::RANGE;
                if (icase_equal(name, "Referer")) return known_header_t::REFERER;
                break;
            case 'W': case 'w':
                if (icase_equal(name, "WWW-Authenticate")) return known_header_t::WWW_AUTHENTICATE;
                break;
            case 'V': case 'v':
                if (icase_equal(name, "Vary")) return known_header_t::VARY;
                if (icase_equal(name, "Via")) return known_header_t::VIA;
                break;
            case 'X': case 'x':
                if (icase_equal(name, "X-Forwarded-For")) return known_header_t::X_FORWARDED_FOR;
                if (icase_equal(name, "X-Forwarded-Proto")) return known_header_t::X_FORWARDED_PROTO;
                if (icase_equal(name, "X-Real-IP")) return known_header_t::X_REAL_IP;
                break;
        }
        return known_header_t::UNKNOWN;
    }
}

// ============================================================================
// CALLBACKS INTERFACE (Function pointers for zero-overhead dispatch)
// ============================================================================
namespace callbacks {
    
    // Callback types
    using on_message_begin_t = parse_result_t (*)(context_t&);
    using on_url_t = parse_result_t (*)(context_t&, std::string_view);
    using on_status_t = parse_result_t (*)(context_t&, uint16_t status_code, std::string_view reason);
    using on_header_t = parse_result_t (*)(context_t&, std::string_view name, std::string_view value);
    using on_headers_complete_t = parse_result_t (*)(context_t&);
    using on_body_t = parse_result_t (*)(context_t&, std::string_view);
    using on_chunk_header_t = parse_result_t (*)(context_t&, uint64_t chunk_size);
    using on_chunk_complete_t = parse_result_t (*)(context_t&);
    using on_message_complete_t = parse_result_t (*)(context_t&);
    using on_trailer_t = parse_result_t (*)(context_t&, std::string_view name, std::string_view value);

    struct handler_t {
        on_message_begin_t on_message_begin = nullptr;
        on_url_t on_url = nullptr;
        on_status_t on_status = nullptr;
        on_header_t on_header = nullptr;
        on_headers_complete_t on_headers_complete = nullptr;
        on_body_t on_body = nullptr;
        on_chunk_header_t on_chunk_header = nullptr;
        on_chunk_complete_t on_chunk_complete = nullptr;
        on_message_complete_t on_message_complete = nullptr;
        on_trailer_t on_trailer = nullptr;
    };
}

using handler_t = callbacks::handler_t;

// ============================================================================
// MAIN PARSING FUNCTION
// ============================================================================
namespace internal {
    
    // Forward declarations of state handlers
    static parse_result_t state_start(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept;
    static parse_result_t state_req_method(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept;
    static parse_result_t state_req_url(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept;
    static parse_result_t state_req_version(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept;
    static parse_result_t state_res_status_line(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept;
    static parse_result_t state_header_name(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept;
    static parse_result_t state_header_value(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept;
    static parse_result_t state_body_identity(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept;
    static parse_result_t state_body_chunked(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept;
    static parse_result_t state_trailers(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept;
    
    // Macro for callback invocation with error handling
    #define INVOKE_CALLBACK(ctx, handler, name, ...) \
        do { \
            if (handler.name) { \
                auto _res = handler.name(ctx, ##__VA_ARGS__); \
                if (_res != parse_result_t::OK) { \
                    ctx.state = state_t::DEAD; \
                    return _res; \
                } \
            } \
        } while(0)

    static parse_result_t state_start(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept {
        if (p >= end) return parse_result_t::INCOMPLETE;
        
        INVOKE_CALLBACK(ctx, h, on_message_begin);
        
        char c = *p;
        
        // Detect message type
        if (c == 'H') {
            // Could be HTTP/ response
            ctx.type = message_type_t::RESPONSE;
            ctx.state = state_t::RES_HTTP_VERSION;
        } else if (simd::is_token_char[static_cast<uint8_t>(c)]) {
            // Request method
            ctx.type = message_type_t::REQUEST;
            ctx.method_start = p;
            ctx.state = state_t::REQ_METHOD;
        } else {
            ctx.state = state_t::DEAD;
            return parse_result_t::ERROR_INVALID_METHOD;
        }
        
        return parse_result_t::OK;
    }

    static parse_result_t state_req_method(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept {
        const char* start = p;
        
        while (p < end) {
            char c = *p;
            if (c == ' ') {
                // End of method
                size_t len = static_cast<size_t>(p - ctx.method_start);
                if (len == 0 || len > config::MAX_METHOD_LEN) {
                    ctx.state = state_t::DEAD;
                    return parse_result_t::ERROR_INVALID_METHOD;
                }
                
                if (config::STRICT_VALIDATION && !simd::validate_token(ctx.method_start, p)) {
                    ctx.state = state_t::DEAD;
                    return parse_result_t::ERROR_INVALID_METHOD;
                }
                
                ctx.method = method_resolver::resolve(ctx.method_start, len);
                ctx.state = state_t::REQ_URL;
                ctx.url_start = p + 1; // Skip space
                ++p;
                return parse_result_t::OK;
            }
            
            if (!simd::is_token_char[static_cast<uint8_t>(c)]) {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_INVALID_METHOD;
            }
            ++p;
        }
        
        return parse_result_t::INCOMPLETE;
    }

    static parse_result_t state_req_url(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept {
        while (p < end) {
            char c = *p;
            if (c == ' ') {
                // End of URL, start of HTTP version
                size_t len = static_cast<size_t>(p - ctx.url_start);
                if (len == 0 || len > config::MAX_URL_LEN) {
                    ctx.state = state_t::DEAD;
                    return parse_result_t::ERROR_INVALID_URL;
                }
                
                if (config::STRICT_VALIDATION && !simd::validate_uri(ctx.url_start, p)) {
                    ctx.state = state_t::DEAD;
                    return parse_result_t::ERROR_INVALID_URL;
                }
                
                ctx.url_end = p;
                INVOKE_CALLBACK(ctx, h, on_url, std::string_view(ctx.url_start, ctx.url_end - ctx.url_start));
                
                ctx.state = state_t::REQ_HTTP_VERSION;
                ++p;
                return parse_result_t::OK;
            }
            ++p;
        }
        return parse_result_t::INCOMPLETE;
    }

    static parse_result_t state_req_version(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept {
        // Expect "HTTP/X.Y\r\n"
        static constexpr const char* HTTP_PREFIX = "HTTP/";
        const size_t remaining = static_cast<size_t>(end - p);
        
        // Match "HTTP/"
        const size_t match_pos = static_cast<size_t>(p - ctx.url_end - 1);
        if (match_pos < 5) {
            if (remaining == 0) return parse_result_t::INCOMPLETE;
            if (*p != HTTP_PREFIX[match_pos]) {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_INVALID_VERSION;
            }
            ++p;
            return parse_result_t::OK;
        }
        
        if (match_pos == 5) {
            if (remaining == 0) return parse_result_t::INCOMPLETE;
            if (!simd::is_digit[static_cast<uint8_t>(*p)]) {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_INVALID_VERSION;
            }
            ctx.http_major = static_cast<uint8_t>(*p - '0');
            ++p;
            return parse_result_t::OK;
        }
        
        if (match_pos == 6) {
            if (remaining == 0) return parse_result_t::INCOMPLETE;
            if (*p != '.') {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_INVALID_VERSION;
            }
            ++p;
            return parse_result_t::OK;
        }
        
        if (match_pos == 7) {
            if (remaining == 0) return parse_result_t::INCOMPLETE;
            if (!simd::is_digit[static_cast<uint8_t>(*p)]) {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_INVALID_VERSION;
            }
            ctx.http_minor = static_cast<uint8_t>(*p - '0');
            ++p;
            return parse_result_t::OK;
        }
        
        if (match_pos == 8) {
            if (remaining == 0) return parse_result_t::INCOMPLETE;
            if (*p == '\r') {
                ctx.state = state_t::REQ_LINE_ENDING_CR;
            } else {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_INVALID_VERSION;
            }
            ++p;
            return parse_result_t::OK;
        }
        
        if (match_pos == 9) {
            if (remaining == 0) return parse_result_t::INCOMPLETE;
            if (*p != '\n') {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_INVALID_VERSION;
            }
            ++p;
            ctx.state = state_t::HEADER_NAME;
            return parse_result_t::OK;
        }
        
        ctx.state = state_t::DEAD;
        return parse_result_t::ERROR_INVALID_VERSION;
    }

    static parse_result_t state_header_name(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept {
        if (p >= end) return parse_result_t::INCOMPLETE;
        
        // Check for end of headers (empty line)
        if (*p == '\r') {
            if (p + 1 >= end) return parse_result_t::INCOMPLETE;
            if (p[1] != '\n') {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_INVALID_HEADER_NAME;
            }
            p += 2;
            
            // Headers complete
            ctx.header_count = 0; // Reset for potential trailers
            INVOKE_CALLBACK(ctx, h, on_headers_complete);
            
            // Determine body handling strategy
            if (ctx.has_transfer_encoding_chunked) {
                ctx.state = state_t::BODY_CHUNKED_SIZE;
            } else if (ctx.has_content_length) {
                if (ctx.content_length == 0) {
                    ctx.state = state_t::DONE;
                    INVOKE_CALLBACK(ctx, h, on_message_complete);
                } else {
                    ctx.state = state_t::BODY_IDENTITY;
                    ctx.body_received = 0;
                }
            } else {
                // No body for requests (unless CONNECT/TRACE)
                // For responses, read until EOF if no content-length
                if (ctx.type == message_type_t::RESPONSE && 
                    ctx.status_code != 204 && ctx.status_code != 304 &&
                    ctx.status_code < 200) {
                    ctx.state = state_t::BODY_EOF;
                } else {
                    ctx.state = state_t::DONE;
                    INVOKE_CALLBACK(ctx, h, on_message_complete);
                }
            }
            return parse_result_t::OK;
        }
        
        ctx.header_name_start = p;
        
        while (p < end) {
            char c = *p;
            if (c == ':') {
                ctx.header_name_end = p;
                size_t name_len = static_cast<size_t>(p - ctx.header_name_start);
                
                if (name_len == 0 || name_len > config::MAX_HEADER_NAME_LEN) {
                    ctx.state = state_t::DEAD;
                    return parse_result_t::ERROR_INVALID_HEADER_NAME;
                }
                
                if (config::STRICT_VALIDATION && !simd::validate_token(ctx.header_name_start, p)) {
                    ctx.state = state_t::DEAD;
                    return parse_result_t::ERROR_INVALID_HEADER_NAME;
                }
                
                ctx.state = state_t::HEADER_VALUE_START_WS;
                ++p;
                return parse_result_t::OK;
            }
            
            if (!simd::is_token_char[static_cast<uint8_t>(c)]) {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_INVALID_HEADER_NAME;
            }
            ++p;
        }
        
        return parse_result_t::INCOMPLETE;
    }

    static parse_result_t state_header_value(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept {
        // Skip leading whitespace
        p = simd::skip_lws(p, end);
        if (p >= end) return parse_result_t::INCOMPLETE;
        
        ctx.header_value_start = p;
        
        // Find CRLF
        // const char* crlf = simd::find_crlf(p, end);
        const char* crlf = simd::find_crlf_scalar(p, end);
        if (crlf == end) {
            // Check if we have exceeded max value length
            if (static_cast<size_t>(end - ctx.header_value_start) > config::MAX_HEADER_VALUE_LEN) {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_HEADER_OVERFLOW;
            }
            return parse_result_t::INCOMPLETE;
        }
        
        // Trim trailing whitespace
        ctx.header_value_end = crlf;
        while (ctx.header_value_end > ctx.header_value_start && 
               (ctx.header_value_end[-1] == ' ' || ctx.header_value_end[-1] == '\t')) {
            --ctx.header_value_end;
        }
        
        size_t value_len = static_cast<size_t>(ctx.header_value_end - ctx.header_value_start);
        if (value_len > config::MAX_HEADER_VALUE_LEN) {
            ctx.state = state_t::DEAD;
            return parse_result_t::ERROR_HEADER_OVERFLOW;
        }
        
        std::string_view name(ctx.header_name_start, 
                              static_cast<size_t>(ctx.header_name_end - ctx.header_name_start));
        std::string_view value(ctx.header_value_start, value_len);
        
        // Process special headers
        auto known = header_names::identify(name);
        switch (known) {
            case header_names::known_header_t::CONTENT_LENGTH: {
                // Parse content-length
                if (ctx.has_content_length) {
                    // Duplicate Content-Length - error in strict mode
                    if (ctx.strict_mode) {
                        ctx.state = state_t::DEAD;
                        return parse_result_t::ERROR_STRICT_VALIDATION;
                    }
                }
                
                uint64_t len = 0;
                for (char c : value) {
                    if (!simd::is_digit[static_cast<uint8_t>(c)]) {
                        ctx.state = state_t::DEAD;
                        return parse_result_t::ERROR_INVALID_HEADER_VALUE;
                    }
                    uint64_t digit = static_cast<uint64_t>(c - '0');
                    if (len > (UINT64_MAX - digit) / 10) {
                        ctx.state = state_t::DEAD;
                        return parse_result_t::ERROR_CONTENT_LENGTH_OVERFLOW;
                    }
                    len = len * 10 + digit;
                }
                ctx.content_length = len;
                ctx.has_content_length = true;
                break;
            }
            case header_names::known_header_t::TRANSFER_ENCODING: {
                if (header_names::icase_equal(value, "chunked")) {
                    ctx.has_transfer_encoding_chunked = true;
                }
                break;
            }
            case header_names::known_header_t::CONNECTION: {
                // Parse connection tokens
                std::string_view token = value;
                size_t pos = 0;
                while (pos < token.size()) {
                    size_t comma = token.find(',', pos);
                    if (comma == std::string_view::npos) comma = token.size();
                    
                    std::string_view t = token.substr(pos, comma - pos);
                    // Trim whitespace
                    while (!t.empty() && (t.front() == ' ' || t.front() == '\t')) t.remove_prefix(1);
                    while (!t.empty() && (t.back() == ' ' || t.back() == '\t')) t.remove_suffix(1);
                    
                    if (header_names::icase_equal(t, "close")) {
                        ctx.connection_close = true;
                        ctx.is_keep_alive = false;
                    } else if (header_names::icase_equal(t, "keep-alive")) {
                        ctx.is_keep_alive = true;
                    } else if (header_names::icase_equal(t, "upgrade")) {
                        ctx.upgrade = true;
                    }
                    
                    pos = comma + 1;
                }
                break;
            }
            default:
                break;
        }
        
        INVOKE_CALLBACK(ctx, h, on_header, name, value);
        ++ctx.header_count;
        
        if (ctx.header_count > config::MAX_HEADERS) {
            ctx.state = state_t::DEAD;
            return parse_result_t::ERROR_HEADER_OVERFLOW;
        }
        
        p = crlf + 2; // Skip \r\n
        ctx.state = state_t::HEADER_NAME;
        return parse_result_t::OK;
    }

    static parse_result_t state_body_identity(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept {
        const uint64_t remaining = ctx.content_length - ctx.body_received;
        const uint64_t available = static_cast<uint64_t>(end - p);
        const uint64_t to_read = remaining < available ? remaining : available;
        
        if (to_read > 0) {
            std::string_view body_chunk(p, static_cast<size_t>(to_read));
            INVOKE_CALLBACK(ctx, h, on_body, body_chunk);
            p += to_read;
            ctx.body_received += to_read;
        }
        
        if (ctx.body_received >= ctx.content_length) {
            ctx.state = state_t::DONE;
            INVOKE_CALLBACK(ctx, h, on_message_complete);
        }
        
        return parse_result_t::OK;
    }

    static parse_result_t state_body_chunked(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept {
        // Simplified chunked encoding parser
        // Full implementation would handle all sub-states
        
        if (p >= end) return parse_result_t::INCOMPLETE;
        
        switch (ctx.state) {
            case state_t::BODY_CHUNKED_SIZE: {
                // Parse hex chunk size
                const char* start = p;
                uint64_t size = 0;
                bool ext_found = false;
                
                while (p < end) {
                    char c = *p;
                    if (c == '\r') {
                        if (p + 1 >= end) return parse_result_t::INCOMPLETE;
                        if (p[1] != '\n') {
                            ctx.state = state_t::DEAD;
                            return parse_result_t::ERROR_INVALID_CHUNK_SIZE;
                        }
                        
                        ctx.chunk_size = size;
                        INVOKE_CALLBACK(ctx, h, on_chunk_header, size);
                        
                        p += 2;
                        if (size == 0) {
                            ctx.state = state_t::BODY_CHUNKED_TRAILER_NAME;
                        } else {
                            ctx.state = state_t::BODY_CHUNKED_DATA;
                            ctx.body_received = 0;
                        }
                        return parse_result_t::OK;
                    }
                    
                    if (c == ';') {
                        // Chunk extension
                        ext_found = true;
                        ++p;
                        // Skip extension (simplified)
                        while (p < end && *p != '\r') ++p;
                        continue;
                    }
                    
                    if (!ext_found) {
                        uint8_t nibble = simd::hex_to_nibble[static_cast<uint8_t>(c)];
                        if (nibble == 255) {
                            ctx.state = state_t::DEAD;
                            return parse_result_t::ERROR_INVALID_CHUNK_SIZE;
                        }
                        if (size > (UINT64_MAX - nibble) / 16) {
                            ctx.state = state_t::DEAD;
                            return parse_result_t::ERROR_CONTENT_LENGTH_OVERFLOW;
                        }
                        size = size * 16 + nibble;
                    }
                    ++p;
                }
                return parse_result_t::INCOMPLETE;
            }
            
            case state_t::BODY_CHUNKED_DATA: {
                const uint64_t remaining = ctx.chunk_size - ctx.body_received;
                const uint64_t available = static_cast<uint64_t>(end - p);
                const uint64_t to_read = remaining < available ? remaining : available;
                
                if (to_read > 0) {
                    std::string_view chunk(p, static_cast<size_t>(to_read));
                    INVOKE_CALLBACK(ctx, h, on_body, chunk);
                    p += to_read;
                    ctx.body_received += to_read;
                }
                
                if (ctx.body_received >= ctx.chunk_size) {
                    // Expect \r\n after chunk data
                    if (p + 2 > end) return parse_result_t::INCOMPLETE;
                    if (p[0] != '\r' || p[1] != '\n') {
                        ctx.state = state_t::DEAD;
                        return parse_result_t::ERROR_INVALID_CHUNK_SIZE;
                    }
                    p += 2;
                    INVOKE_CALLBACK(ctx, h, on_chunk_complete);
                    ctx.state = state_t::BODY_CHUNKED_SIZE;
                }
                return parse_result_t::OK;
            }
            
            case state_t::BODY_CHUNKED_TRAILER_NAME:
                return state_trailers(ctx, p, end, h);
            
            default:
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_UNEXPECTED_DATA;
        }
    }

    static parse_result_t state_trailers(context_t& ctx, const char*& p, const char* end, const handler_t& h) noexcept {
        // Similar to header parsing but with trailer callbacks
        // Simplified implementation
        if (p >= end) return parse_result_t::INCOMPLETE;
        
        if (*p == '\r') {
            if (p + 1 >= end) return parse_result_t::INCOMPLETE;
            if (p[1] != '\n') {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_INVALID_TRAILER;
            }
            p += 2;
            ctx.state = state_t::DONE;
            INVOKE_CALLBACK(ctx, h, on_message_complete);
            return parse_result_t::OK;
        }
        
        ctx.header_name_start = p;
        while (p < end && *p != ':') {
            if (!simd::is_token_char[static_cast<uint8_t>(*p)]) {
                ctx.state = state_t::DEAD;
                return parse_result_t::ERROR_INVALID_TRAILER;
            }
            ++p;
        }
        if (p >= end) return parse_result_t::INCOMPLETE;
        
        ctx.header_name_end = p;
        ++p; // Skip ':'
        p = simd::skip_lws(p, end);
        
        ctx.header_value_start = p;
        // const char* crlf = simd::find_crlf(p, end);
        const char* crlf = simd::find_crlf_scalar(p, end);
        if (crlf == end) return parse_result_t::INCOMPLETE;
        
        ctx.header_value_end = crlf;
        while (ctx.header_value_end > ctx.header_value_start &&
               (ctx.header_value_end[-1] == ' ' || ctx.header_value_end[-1] == '\t')) {
            --ctx.header_value_end;
        }
        
        std::string_view name(ctx.header_name_start, 
                              static_cast<size_t>(ctx.header_name_end - ctx.header_name_start));
        std::string_view value(ctx.header_value_start,
                              static_cast<size_t>(ctx.header_value_end - ctx.header_value_start));
        
        INVOKE_CALLBACK(ctx, h, on_trailer, name, value);
        
        p = crlf + 2;
        return parse_result_t::OK;
    }

} // namespace internal

// ============================================================================
// PUBLIC PARSE FUNCTION
// ============================================================================
[[nodiscard]] static inline parse_result_t parse(
    context_t& ctx,
    const char* data,
    size_t len,
    const handler_t& handler
) noexcept {
    const char* p = data;
    const char* end = data + len;
    
    while (p < end && ctx.state != internal::state_t::DEAD && ctx.state != internal::state_t::DONE) {
        parse_result_t result = parse_result_t::OK;
        
        // Dispatch to appropriate state handler
        switch (ctx.state) {
            case internal::state_t::START:
                result = internal::state_start(ctx, p, end, handler);
                break;
            case internal::state_t::REQ_METHOD:
                result = internal::state_req_method(ctx, p, end, handler);
                break;
            case internal::state_t::REQ_URL:
                result = internal::state_req_url(ctx, p, end, handler);
                break;
            case internal::state_t::REQ_HTTP_VERSION:
            case internal::state_t::REQ_LINE_ENDING_CR:
            case internal::state_t::REQ_LINE_ENDING_LF:
                result = internal::state_req_version(ctx, p, end, handler);
                break;
            case internal::state_t::RES_HTTP_VERSION:
            case internal::state_t::RES_STATUS_CODE:
            case internal::state_t::RES_STATUS_CODE_DIGIT_1:
            case internal::state_t::RES_STATUS_CODE_DIGIT_2:
            case internal::state_t::RES_STATUS_CODE_DIGIT_3:
            case internal::state_t::RES_REASON_PHRASE:
            case internal::state_t::RES_LINE_ENDING_CR:
            case internal::state_t::RES_LINE_ENDING_LF:
                result = internal::state_res_status_line(ctx, p, end, handler);
                break;
            case internal::state_t::HEADER_NAME:
            case internal::state_t::HEADER_COLON:
            case internal::state_t::HEADER_VALUE_START_WS:
                result = internal::state_header_name(ctx, p, end, handler);
                break;
            case internal::state_t::HEADER_VALUE:
            case internal::state_t::HEADER_VALUE_WS:
            case internal::state_t::HEADER_VALUE_ENDING_CR:
            case internal::state_t::HEADER_VALUE_ENDING_LF:
                result = internal::state_header_value(ctx, p, end, handler);
                break;
            case internal::state_t::BODY_IDENTITY:
                result = internal::state_body_identity(ctx, p, end, handler);
                break;
            case internal::state_t::BODY_CHUNKED_SIZE:
            case internal::state_t::BODY_CHUNKED_SIZE_EXT:
            case internal::state_t::BODY_CHUNKED_SIZE_CR:
            case internal::state_t::BODY_CHUNKED_SIZE_LF:
            case internal::state_t::BODY_CHUNKED_DATA:
            case internal::state_t::BODY_CHUNKED_DATA_CR:
            case internal::state_t::BODY_CHUNKED_DATA_LF:
            case internal::state_t::BODY_CHUNKED_TRAILER_NAME:
            case internal::state_t::BODY_CHUNKED_TRAILER_VALUE:
            case internal::state_t::BODY_CHUNKED_TRAILER_END_CR:
            case internal::state_t::BODY_CHUNKED_TRAILER_END_LF:
                result = internal::state_body_chunked(ctx, p, end, handler);
                break;
            case internal::state_t::BODY_EOF:
                // Read until connection close
                if (p < end) {
                    std::string_view body_chunk(p, static_cast<size_t>(end - p));
                    INVOKE_CALLBACK(ctx, handler, on_body, body_chunk);
                    p = end;
                }
                break;
            default:
                ctx.state = internal::state_t::DEAD;
                return parse_result_t::ERROR_UNEXPECTED_DATA;
        }
        
        if (result != parse_result_t::OK) {
            return result;
        }
    }
    
    return (ctx.state == internal::state_t::DONE) ? parse_result_t::OK : parse_result_t::INCOMPLETE;
}

// Check if message should keep connection alive
[[nodiscard]] static inline bool should_keep_alive(const context_t& ctx) noexcept {
    if (ctx.connection_close) return false;
    if (ctx.http_major > 1 || (ctx.http_major == 1 && ctx.http_minor >= 1)) {
        // HTTP/1.1 defaults to keep-alive
        return !ctx.connection_close;
    }
    // HTTP/1.0 requires explicit keep-alive
    return ctx.is_keep_alive;
}

// Check if message expects no body
[[nodiscard]] static inline bool expects_no_body(const context_t& ctx) noexcept {
    if (ctx.type == message_type_t::RESPONSE) {
        if (ctx.status_code == 204 || ctx.status_code == 304) return true;
        if (ctx.status_code < 200) return true; // 1xx informational
    }
    if (ctx.type == message_type_t::REQUEST) {
        if (ctx.method == method_t::HEAD) return true;
        if (ctx.method == method_t::CONNECT) return true;
        if (ctx.method == method_t::TRACE) return true;
    }
    return false;
}

// Get HTTP version as string
[[nodiscard]] static inline std::string_view http_version_string(const context_t& ctx, char* buffer) noexcept {
    buffer[0] = 'H';
    buffer[1] = 'T';
    buffer[2] = 'T';
    buffer[3] = 'P';
    buffer[4] = '/';
    buffer[5] = static_cast<char>('0' + ctx.http_major);
    buffer[6] = '.';
    buffer[7] = static_cast<char>('0' + ctx.http_minor);
    buffer[8] = '\0';
    return std::string_view(buffer, 8);
}

} // namespace http_parser
