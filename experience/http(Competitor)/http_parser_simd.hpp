// http_parser_simd.hpp
#pragma once
#include <cstdint>
#include <cstddef>

// Platform detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define HTTP_PARSER_X86 1
#else
    #define HTTP_PARSER_X86 0
#endif

#if HTTP_PARSER_X86
    #if defined(__AVX2__)
        #include <immintrin.h>
        #define HTTP_PARSER_AVX2 1
    #else
        #define HTTP_PARSER_AVX2 0
    #endif
    
    #if defined(__SSE4_2__)
        #include <nmmintrin.h>
        #define HTTP_PARSER_SSE42 1
    #elif defined(__SSE2__)
        #include <emmintrin.h>
        #define HTTP_PARSER_SSE2 1
    #else
        #define HTTP_PARSER_SSE2 0
        #define HTTP_PARSER_SSE42 0
    #endif
#else
    #define HTTP_PARSER_AVX2 0
    #define HTTP_PARSER_SSE42 0
    #define HTTP_PARSER_SSE2 0
#endif

// Bit manipulation intrinsics
#if defined(__GNUC__) || defined(__clang__)
    #define HTTP_PARSER_CTZ32(x) __builtin_ctz(x)
    #define HTTP_PARSER_CTZ64(x) __builtin_ctzll(x)
    #define HTTP_PARSER_POPCOUNT32(x) __builtin_popcount(x)
#elif defined(_MSC_VER)
    #include <intrin.h>
    #define HTTP_PARSER_CTZ32(x) _tzcnt_u32(x)
    #define HTTP_PARSER_CTZ64(x) _tzcnt_u64(x)
    #define HTTP_PARSER_POPCOUNT32(x) __popcnt(x)
#else
    // Portable fallback
    static inline int http_parser_ctz32(uint32_t x) {
        if (x == 0) return 32;
        int n = 0;
        if ((x & 0xFFFF0000) == 0) { n += 16; x <<= 16; }
        if ((x & 0xFF000000) == 0) { n += 8; x <<= 8; }
        if ((x & 0xF0000000) == 0) { n += 4; x <<= 4; }
        if ((x & 0xC0000000) == 0) { n += 2; x <<= 2; }
        if ((x & 0x80000000) == 0) { n += 1; }
        return n;
    }
    #define HTTP_PARSER_CTZ32(x) http_parser_ctz32(x)
    #define HTTP_PARSER_CTZ64(x) HTTP_PARSER_CTZ32(static_cast<uint32_t>(x))
    #define HTTP_PARSER_POPCOUNT32(x) /* portable popcount */
#endif

namespace http_parser {
namespace simd {

// ============================================================================
// LOOKUP TABLES (Compile-time)
// ============================================================================
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

// ============================================================================
// BITMAP для валидации token characters
// ============================================================================
// Token chars: 0-9, A-Z, a-z, и !#$%&'*+-.^_`|~
// Используем 256-битную bitmap (32 байта) для быстрой проверки
static constexpr uint32_t token_bitmap[8] = {
    0x00000000, // 0x00-0x1F: control chars
    0x03FF67FA, // 0x20-0x3F: space(0), !(1), "(0), #(1), $(1), %(1), &(1), '(1), 
                //            ((0), )(0), *(1), +(1), ,(0), -(1), .(1), /(0),
                //            0-9(1)
    0x57FFFFFF, // 0x40-0x5F: @(0), A-Z(1), [(0), \(0), ](0), ^(1), _(1), `(1)
    0x47FFFFFE, // 0x60-0x7F: `(1), a-z(1), {(0), |(1), }(0), ~(1), DEL(0)
    0x00000000, // 0x80-0x9F
    0x00000000, // 0xA0-0xBF
    0x00000000, // 0xC0-0xDF
    0x00000000, // 0xE0-0xFF
};

// ============================================================================
// PREFETCH INTRINSICS
// ============================================================================
#if HTTP_PARSER_X86
    static inline void prefetch_l1(const void* ptr) noexcept {
        #if defined(__GNUC__) || defined(__clang__)
            __builtin_prefetch(ptr, 0, 3); // Read, high locality
        #elif defined(_MSC_VER)
            _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
        #endif
    }
    
    static inline void prefetch_l2(const void* ptr) noexcept {
        #if defined(__GNUC__) || defined(__clang__)
            __builtin_prefetch(ptr, 0, 2); // Read, medium locality
        #elif defined(_MSC_VER)
            _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T1);
        #endif
    }
#else
    static inline void prefetch_l1(const void*) noexcept {}
    static inline void prefetch_l2(const void*) noexcept {}
#endif

// ============================================================================
// AVX2 IMPLEMENTATIONS
// ============================================================================
#if HTTP_PARSER_AVX2

[[nodiscard]] static inline const char* find_crlf_avx2(const char* begin, const char* end) noexcept {
    const size_t len = static_cast<size_t>(end - begin);
    if (len < 2) return end;
    
    const char* p = begin;
    const char* e = end - 1; // Need at least 2 bytes for \r\n
    
    const __m256i cr_vec = _mm256_set1_epi8('\r');
    const __m256i lf_vec = _mm256_set1_epi8('\n');
    
    // Process 32 bytes at a time
    while (p + 32 <= e) {
        // Prefetch next cache line
        if (p + 64 + 32 <= end) {
            prefetch_l1(p + 64);
        }
        
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i cr_cmp = _mm256_cmpeq_epi8(data, cr_vec);
        uint32_t cr_mask = _mm256_movemask_epi8(cr_cmp);
        
        if (cr_mask != 0) {
            // Found at least one \r, check each for following \n
            while (cr_mask != 0) {
                int pos = HTTP_PARSER_CTZ32(cr_mask);
                if (p + pos + 1 < end && p[pos + 1] == '\n') {
                    return p + pos;
                }
                // Clear this bit and continue searching
                cr_mask &= cr_mask - 1; // Clear lowest set bit
            }
        }
        p += 32;
    }
    
    // Handle remaining bytes with SSE2 or scalar
    while (p < e) {
        if (*p == '\r' && p[1] == '\n') {
            return p;
        }
        ++p;
    }
    
    return end;
}

[[nodiscard]] static inline bool validate_token_avx2(const char* begin, const char* end) noexcept {
    const char* p = begin;
    
    // Load bitmap into vectors for fast lookup
    // token_bitmap содержит допустимые символы
    // Мы будем проверять, что каждый символ имеет установленный бит в bitmap
    
    const __m256i mask_0x20 = _mm256_set1_epi8(0x20);
    const __m256i mask_0x7F = _mm256_set1_epi8(0x7F);
    
    // Process 32 bytes at a time
    while (p + 32 <= end) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        
        // Fast range check: all chars must be in [0x21, 0x7E]
        __m256i below_space = _mm256_cmpgt_epi8(mask_0x20, data); // data < 0x21
        __m256i above_tilde = _mm256_cmpgt_epi8(data, mask_0x7F); // data > 0x7E
        
        uint32_t invalid_mask = _mm256_movemask_epi8(_mm256_or_si256(below_space, above_tilde));
        if (invalid_mask != 0) {
            // Found invalid character, check if it's really invalid using bitmap
            int pos = HTTP_PARSER_CTZ32(invalid_mask);
            uint8_t c = static_cast<uint8_t>(p[pos]);
            if (!is_token_char[c]) {
                return false;
            }
            // Continue checking from this position
            p += pos + 1;
            continue;
        }
        
        // Additional check for specific forbidden characters
        // Forbidden in tokens: "(),/:;<=>?@[\]{}
        // These are in range [0x21, 0x7E] but not tokens
        
        // Check for '"' (0x22)
        __m256i quote_cmp = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('"'));
        if (_mm256_movemask_epi8(quote_cmp) != 0) return false;
        
        // Check for '(' (0x28) and ')' (0x29)
        __m256i paren_open = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('('));
        __m256i paren_close = _mm256_cmpeq_epi8(data, _mm256_set1_epi8(')'));
        if (_mm256_movemask_epi8(_mm256_or_si256(paren_open, paren_close)) != 0) return false;
        
        // Check for ',' (0x2C)
        __m256i comma_cmp = _mm256_cmpeq_epi8(data, _mm256_set1_epi8(','));
        if (_mm256_movemask_epi8(comma_cmp) != 0) return false;
        
        // Check for '/' (0x2F)
        __m256i slash_cmp = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('/'));
        if (_mm256_movemask_epi8(slash_cmp) != 0) return false;
        
        // Check for ':' (0x3A), ';' (0x3B), '<' (0x3C), '=' (0x3D), '>' (0x3E), '?' (0x3F)
        __m256i colon_cmp = _mm256_cmpeq_epi8(data, _mm256_set1_epi8(':'));
        __m256i semicolon_cmp = _mm256_cmpeq_epi8(data, _mm256_set1_epi8(';'));
        __m256i lt_cmp = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('<'));
        __m256i eq_cmp = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('='));
        __m256i gt_cmp = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('>'));
        __m256i question_cmp = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('?'));
        
        __m256i punct_invalid = _mm256_or_si256(
            _mm256_or_si256(colon_cmp, semicolon_cmp),
            _mm256_or_si256(
                _mm256_or_si256(lt_cmp, eq_cmp),
                _mm256_or_si256(gt_cmp, question_cmp)
            )
        );
        if (_mm256_movemask_epi8(punct_invalid) != 0) return false;
        
        // Check for '@' (0x40)
        __m256i at_cmp = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('@'));
        if (_mm256_movemask_epi8(at_cmp) != 0) return false;
        
        // Check for '[' (0x5B), '\' (0x5C), ']' (0x5D)
        __m256i bracket_open = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('['));
        __m256i backslash = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('\\'));
        __m256i bracket_close = _mm256_cmpeq_epi8(data, _mm256_set1_epi8(']'));
        
        __m256i brackets_invalid = _mm256_or_si256(
            bracket_open,
            _mm256_or_si256(backslash, bracket_close)
        );
        if (_mm256_movemask_epi8(brackets_invalid) != 0) return false;
        
        // Check for '{' (0x7B), '}' (0x7D)
        __m256i brace_open = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('{'));
        __m256i brace_close = _mm256_cmpeq_epi8(data, _mm256_set1_epi8('}'));
        if (_mm256_movemask_epi8(_mm256_or_si256(brace_open, brace_close)) != 0) return false;
        
        p += 32;
    }
    
    // Scalar fallback for remaining bytes
    for (; p < end; ++p) {
        if (!is_token_char[static_cast<uint8_t>(*p)]) {
            return false;
        }
    }
    
    return true;
}

[[nodiscard]] static inline bool validate_hex_string_avx2(const char* begin, const char* end) noexcept {
    const char* p = begin;
    
    const __m256i zero = _mm256_set1_epi8('0');
    const __m256i nine = _mm256_set1_epi8('9');
    const __m256i a_lower = _mm256_set1_epi8('a');
    const __m256i f_lower = _mm256_set1_epi8('f');
    const __m256i a_upper = _mm256_set1_epi8('A');
    const __m256i f_upper = _mm256_set1_epi8('F');
    
    while (p + 32 <= end) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        
        // Check: (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')
        
        // Digits: 0-9
        __m256i ge_zero = _mm256_or_si256(
            _mm256_cmpgt_epi8(data, zero),
            _mm256_cmpeq_epi8(data, zero)
        );
        __m256i le_nine = _mm256_or_si256(
            _mm256_cmpgt_epi8(nine, data),
            _mm256_cmpeq_epi8(data, nine)
        );
        __m256i is_digit = _mm256_and_si256(ge_zero, le_nine);
        
        // Lowercase hex: a-f
        __m256i ge_a_lower = _mm256_or_si256(
            _mm256_cmpgt_epi8(data, a_lower),
            _mm256_cmpeq_epi8(data, a_lower)
        );
        __m256i le_f_lower = _mm256_or_si256(
            _mm256_cmpgt_epi8(f_lower, data),
            _mm256_cmpeq_epi8(data, f_lower)
        );
        __m256i is_hex_lower = _mm256_and_si256(ge_a_lower, le_f_lower);
        
        // Uppercase hex: A-F
        __m256i ge_a_upper = _mm256_or_si256(
            _mm256_cmpgt_epi8(data, a_upper),
            _mm256_cmpeq_epi8(data, a_upper)
        );
        __m256i le_f_upper = _mm256_or_si256(
            _mm256_cmpgt_epi8(f_upper, data),
            _mm256_cmpeq_epi8(data, f_upper)
        );
        __m256i is_hex_upper = _mm256_and_si256(ge_a_upper, le_f_upper);
        
        // Combine all valid ranges
        __m256i is_valid = _mm256_or_si256(
            is_digit,
            _mm256_or_si256(is_hex_lower, is_hex_upper)
        );
        
        uint32_t invalid_mask = ~static_cast<uint32_t>(_mm256_movemask_epi8(is_valid)) & 0xFFFFFFFF;
        if (invalid_mask != 0) {
            // Found invalid hex character
            return false;
        }
        
        p += 32;
    }
    
    // Scalar fallback
    for (; p < end; ++p) {
        if (!is_hex_digit[static_cast<uint8_t>(*p)]) {
            return false;
        }
    }
    
    return true;
}

[[nodiscard]] static inline bool validate_header_value_avx2(const char* begin, const char* end) noexcept {
    const char* p = begin;
    
    // Valid characters according to RFC 7230:
    // field-content = field-vchar [ 1*( SP / HTAB ) field-vchar ]
    // field-vchar   = VCHAR / obs-text
    // VCHAR         = %x21-7E
    // obs-text      = %x80-FF
    // SP            = 0x20
    // HTAB          = 0x09
    
    // Invalid: 0x00-0x08, 0x0A-0x0C, 0x0E-0x1F, 0x7F (DEL)
    
    const __m256i vec_0x08 = _mm256_set1_epi8(0x08);
    const __m256i vec_0x0A = _mm256_set1_epi8(0x0A);
    const __m256i vec_0x0C = _mm256_set1_epi8(0x0C);
    const __m256i vec_0x0E = _mm256_set1_epi8(0x0E);
    const __m256i vec_0x1F = _mm256_set1_epi8(0x1F);
    const __m256i vec_0x7F = _mm256_set1_epi8(0x7F);
    
    while (p + 32 <= end) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        
        // Check for control characters (0x00-0x08)
        __m256i ctrl_low = _mm256_cmpgt_epi8(vec_0x08, data);
        
        // Check for newline (0x0A)
        __m256i is_lf = _mm256_cmpeq_epi8(data, vec_0x0A);
        
        // Check for carriage return (0x0D) and form feed (0x0C)
        __m256i is_cr = _mm256_cmpeq_epi8(data, _mm256_set1_epi8(0x0D));
        __m256i is_ff = _mm256_cmpeq_epi8(data, vec_0x0C);
        
        // Check for 0x0B (VT)
        __m256i is_vt = _mm256_cmpeq_epi8(data, _mm256_set1_epi8(0x0B));
        
        // Check for 0x0E-0x1F
        __m256i ge_0x0E = _mm256_cmpgt_epi8(data, _mm256_set1_epi8(0x0D));
        __m256i le_0x1F = _mm256_cmpgt_epi8(_mm256_set1_epi8(0x20), data);
        __m256i ctrl_high = _mm256_and_si256(ge_0x0E, le_0x1F);
        
        // Check for DEL (0x7F)
        __m256i is_del = _mm256_cmpeq_epi8(data, vec_0x7F);
        
        // Combine all invalid characters
        __m256i invalid = _mm256_or_si256(
            ctrl_low,
            _mm256_or_si256(
                _mm256_or_si256(is_lf, is_cr),
                _mm256_or_si256(
                    _mm256_or_si256(is_ff, is_vt),
                    _mm256_or_si256(ctrl_high, is_del)
                )
            )
        );
        
        if (_mm256_movemask_epi8(invalid) != 0) {
            return false;
        }
        
        p += 32;
    }
    
    // Scalar fallback
    for (; p < end; ++p) {
        uint8_t c = static_cast<uint8_t>(*p);
        if (c < 0x09 || (c > 0x0A && c < 0x0D) || (c > 0x0D && c < 0x20) || c == 0x7F) {
            return false;
        }
    }
    
    return true;
}

#endif // HTTP_PARSER_AVX2

// ============================================================================
// SSE4.2 / SSE2 IMPLEMENTATIONS
// ============================================================================
#if HTTP_PARSER_SSE2 || HTTP_PARSER_SSE42

[[nodiscard]] static inline const char* find_crlf_sse2(const char* begin, const char* end) noexcept {
    const size_t len = static_cast<size_t>(end - begin);
    if (len < 2) return end;
    
    const char* p = begin;
    const char* e = end - 1;
    
    const __m128i cr_vec = _mm_set1_epi8('\r');
    
    // Process 16 bytes at a time
    while (p + 16 <= e) {
        // Prefetch next cache line
        if (p + 32 + 16 <= end) {
            prefetch_l1(p + 32);
        }
        
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        __m128i cr_cmp = _mm_cmpeq_epi8(data, cr_vec);
        uint16_t cr_mask = static_cast<uint16_t>(_mm_movemask_epi8(cr_cmp));
        
        if (cr_mask != 0) {
            // Found at least one \r
            while (cr_mask != 0) {
                int pos = HTTP_PARSER_CTZ32(cr_mask);
                if (p + pos + 1 < end && p[pos + 1] == '\n') {
                    return p + pos;
                }
                cr_mask &= cr_mask - 1;
            }
        }
        p += 16;
    }
    
    // Scalar fallback
    while (p < e) {
        if (*p == '\r' && p[1] == '\n') {
            return p;
        }
        ++p;
    }
    
    return end;
}

[[nodiscard]] static inline bool validate_token_sse2(const char* begin, const char* end) noexcept {
    const char* p = begin;
    
    const __m128i mask_0x20 = _mm_set1_epi8(0x20);
    const __m128i mask_0x7F = _mm_set1_epi8(0x7F);
    
    while (p + 16 <= end) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        
        // Range check
        __m128i below_space = _mm_cmpgt_epi8(mask_0x20, data);
        __m128i above_tilde = _mm_cmpgt_epi8(data, mask_0x7F);
        
        uint16_t invalid_mask = static_cast<uint16_t>(
            _mm_movemask_epi8(_mm_or_si128(below_space, above_tilde))
        );
        
        if (invalid_mask != 0) {
            int pos = HTTP_PARSER_CTZ32(invalid_mask);
            uint8_t c = static_cast<uint8_t>(p[pos]);
            if (!is_token_char[c]) {
                return false;
            }
            p += pos + 1;
            continue;
        }
        
        // Check forbidden characters
        __m128i quote_cmp = _mm_cmpeq_epi8(data, _mm_set1_epi8('"'));
        __m128i paren_open = _mm_cmpeq_epi8(data, _mm_set1_epi8('('));
        __m128i paren_close = _mm_cmpeq_epi8(data, _mm_set1_epi8(')'));
        __m128i comma_cmp = _mm_cmpeq_epi8(data, _mm_set1_epi8(','));
        __m128i slash_cmp = _mm_cmpeq_epi8(data, _mm_set1_epi8('/'));
        __m128i colon_cmp = _mm_cmpeq_epi8(data, _mm_set1_epi8(':'));
        __m128i semicolon_cmp = _mm_cmpeq_epi8(data, _mm_set1_epi8(';'));
        __m128i lt_cmp = _mm_cmpeq_epi8(data, _mm_set1_epi8('<'));
        __m128i eq_cmp = _mm_cmpeq_epi8(data, _mm_set1_epi8('='));
        __m128i gt_cmp = _mm_cmpeq_epi8(data, _mm_set1_epi8('>'));
        __m128i question_cmp = _mm_cmpeq_epi8(data, _mm_set1_epi8('?'));
        __m128i at_cmp = _mm_cmpeq_epi8(data, _mm_set1_epi8('@'));
        __m128i bracket_open = _mm_cmpeq_epi8(data, _mm_set1_epi8('['));
        __m128i backslash = _mm_cmpeq_epi8(data, _mm_set1_epi8('\\'));
        __m128i bracket_close = _mm_cmpeq_epi8(data, _mm_set1_epi8(']'));
        __m128i brace_open = _mm_cmpeq_epi8(data, _mm_set1_epi8('{'));
        __m128i brace_close = _mm_cmpeq_epi8(data, _mm_set1_epi8('}'));
        
        __m128i forbidden = _mm_or_si128(
            _mm_or_si128(quote_cmp, paren_open),
            _mm_or_si128(
                _mm_or_si128(paren_close, comma_cmp),
                _mm_or_si128(
                    _mm_or_si128(slash_cmp, colon_cmp),
                    _mm_or_si128(
                        _mm_or_si128(semicolon_cmp, lt_cmp),
                        _mm_or_si128(
                            _mm_or_si128(eq_cmp, gt_cmp),
                            _mm_or_si128(
                                _mm_or_si128(question_cmp, at_cmp),
                                _mm_or_si128(
                                    _mm_or_si128(bracket_open, backslash),
                                    _mm_or_si128(bracket_close, _mm_or_si128(brace_open, brace_close))
                                )
                            )
                        )
                    )
                )
            )
        );
        
        if (_mm_movemask_epi8(forbidden) != 0) {
            return false;
        }
        
        p += 16;
    }
    
    for (; p < end; ++p) {
        if (!is_token_char[static_cast<uint8_t>(*p)]) {
            return false;
        }
    }
    
    return true;
}

[[nodiscard]] static inline bool validate_hex_string_sse2(const char* begin, const char* end) noexcept {
    const char* p = begin;
    
    const __m128i zero = _mm_set1_epi8('0');
    const __m128i nine = _mm_set1_epi8('9');
    const __m128i a_lower = _mm_set1_epi8('a');
    const __m128i f_lower = _mm_set1_epi8('f');
    const __m128i a_upper = _mm_set1_epi8('A');
    const __m128i f_upper = _mm_set1_epi8('F');
    
    while (p + 16 <= end) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        
        __m128i ge_zero = _mm_or_si128(_mm_cmpgt_epi8(data, zero), _mm_cmpeq_epi8(data, zero));
        __m128i le_nine = _mm_or_si128(_mm_cmpgt_epi8(nine, data), _mm_cmpeq_epi8(data, nine));
        __m128i is_digit = _mm_and_si128(ge_zero, le_nine);
        
        __m128i ge_a_lower = _mm_or_si128(_mm_cmpgt_epi8(data, a_lower), _mm_cmpeq_epi8(data, a_lower));
        __m128i le_f_lower = _mm_or_si128(_mm_cmpgt_epi8(f_lower, data), _mm_cmpeq_epi8(data, f_lower));
        __m128i is_hex_lower = _mm_and_si128(ge_a_lower, le_f_lower);
        
        __m128i ge_a_upper = _mm_or_si128(_mm_cmpgt_epi8(data, a_upper), _mm_cmpeq_epi8(data, a_upper));
        __m128i le_f_upper = _mm_or_si128(_mm_cmpgt_epi8(f_upper, data), _mm_cmpeq_epi8(data, f_upper));
        __m128i is_hex_upper = _mm_and_si128(ge_a_upper, le_f_upper);
        
        __m128i is_valid = _mm_or_si128(is_digit, _mm_or_si128(is_hex_lower, is_hex_upper));
        
        uint16_t invalid_mask = ~static_cast<uint16_t>(_mm_movemask_epi8(is_valid)) & 0xFFFF;
        if (invalid_mask != 0) {
            return false;
        }
        
        p += 16;
    }
    
    for (; p < end; ++p) {
        if (!is_hex_digit[static_cast<uint8_t>(*p)]) {
            return false;
        }
    }
    
    return true;
}

[[nodiscard]] static inline bool validate_header_value_sse2(const char* begin, const char* end) noexcept {
    const char* p = begin;
    
    const __m128i vec_0x08 = _mm_set1_epi8(0x08);
    const __m128i vec_0x0A = _mm_set1_epi8(0x0A);
    const __m128i vec_0x0C = _mm_set1_epi8(0x0C);
    const __m128i vec_0x0E = _mm_set1_epi8(0x0E);
    const __m128i vec_0x1F = _mm_set1_epi8(0x1F);
    const __m128i vec_0x7F = _mm_set1_epi8(0x7F);
    
    while (p + 16 <= end) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        
        __m128i ctrl_low = _mm_cmpgt_epi8(vec_0x08, data);
        __m128i is_lf = _mm_cmpeq_epi8(data, vec_0x0A);
        __m128i is_cr = _mm_cmpeq_epi8(data, _mm_set1_epi8(0x0D));
        __m128i is_ff = _mm_cmpeq_epi8(data, vec_0x0C);
        __m128i is_vt = _mm_cmpeq_epi8(data, _mm_set1_epi8(0x0B));
        
        __m128i ge_0x0E = _mm_cmpgt_epi8(data, _mm_set1_epi8(0x0D));
        __m128i le_0x1F = _mm_cmpgt_epi8(_mm_set1_epi8(0x20), data);
        __m128i ctrl_high = _mm_and_si128(ge_0x0E, le_0x1F);
        
        __m128i is_del = _mm_cmpeq_epi8(data, vec_0x7F);
        
        __m128i invalid = _mm_or_si128(
            ctrl_low,
            _mm_or_si128(
                _mm_or_si128(is_lf, is_cr),
                _mm_or_si128(
                    _mm_or_si128(is_ff, is_vt),
                    _mm_or_si128(ctrl_high, is_del)
                )
            )
        );
        
        if (_mm_movemask_epi8(invalid) != 0) {
            return false;
        }
        
        p += 16;
    }
    
    for (; p < end; ++p) {
        uint8_t c = static_cast<uint8_t>(*p);
        if (c < 0x09 || (c > 0x0A && c < 0x0D) || (c > 0x0D && c < 0x20) || c == 0x7F) {
            return false;
        }
    }
    
    return true;
}

#endif // HTTP_PARSER_SSE2 || HTTP_PARSER_SSE42

// ============================================================================
// SCALAR FALLBACK IMPLEMENTATIONS
// ============================================================================
[[nodiscard]] static inline const char* find_crlf_scalar(const char* begin, const char* end) noexcept {
    const char* p = begin;
    const char* e = end - 1;
    
    while (p < e) {
        if (*p == '\r' && p[1] == '\n') {
            return p;
        }
        ++p;
    }
    
    return end;
}

[[nodiscard]] static inline bool validate_token_scalar(const char* begin, const char* end) noexcept {
    for (const char* p = begin; p < end; ++p) {
        if (!is_token_char[static_cast<uint8_t>(*p)]) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] static inline bool validate_hex_string_scalar(const char* begin, const char* end) noexcept {
    for (const char* p = begin; p < end; ++p) {
        if (!is_hex_digit[static_cast<uint8_t>(*p)]) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] static inline bool validate_header_value_scalar(const char* begin, const char* end) noexcept {
    for (const char* p = begin; p < end; ++p) {
        uint8_t c = static_cast<uint8_t>(*p);
        if (c < 0x09 || (c > 0x0A && c < 0x0D) || (c > 0x0D && c < 0x20) || c == 0x7F) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] static inline bool validate_uri_scalar(const char* begin, const char* end) noexcept {
    for (const char* p = begin; p < end; ++p) {
        uint8_t c = static_cast<uint8_t>(*p);
        if (c < 0x21 || c == 0x7F) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// DISPATCH FUNCTIONS (Auto-select best implementation)
// ============================================================================

// Поиск CRLF в буфере
// Возвращает указатель на первый '\r' в последовательности "\r\n", или end если не найдено
[[nodiscard]] static inline const char* find_crlf(const char* begin, const char* end) noexcept {
    #if HTTP_PARSER_AVX2
        return find_crlf_avx2(begin, end);
    #elif HTTP_PARSER_SSE2 || HTTP_PARSER_SSE42
        return find_crlf_sse2(begin, end);
    #else
        return find_crlf_scalar(begin, end);
    #endif
}

// Валидация token string (RFC 7230 Section 3.2.6)
[[nodiscard]] static inline bool validate_token(const char* begin, const char* end) noexcept {
    #if HTTP_PARSER_AVX2
        return validate_token_avx2(begin, end);
    #elif HTTP_PARSER_SSE2 || HTTP_PARSER_SSE42
        return validate_token_sse2(begin, end);
    #else
        return validate_token_scalar(begin, end);
    #endif
}

// Валидация hex string (для chunked encoding)
[[nodiscard]] static inline bool validate_hex_string(const char* begin, const char* end) noexcept {
    #if HTTP_PARSER_AVX2
        return validate_hex_string_avx2(begin, end);
    #elif HTTP_PARSER_SSE2 || HTTP_PARSER_SSE42
        return validate_hex_string_sse2(begin, end);
    #else
        return validate_hex_string_scalar(begin, end);
    #endif
}

// Валидация header value (RFC 7230 Section 3.2.6)
[[nodiscard]] static inline bool validate_header_value(const char* begin, const char* end) noexcept {
    #if HTTP_PARSER_AVX2
        return validate_header_value_avx2(begin, end);
    #elif HTTP_PARSER_SSE2 || HTTP_PARSER_SSE42
        return validate_header_value_sse2(begin, end);
    #else
        return validate_header_value_scalar(begin, end);
    #endif
}

// Валидация URI (упрощенная)
[[nodiscard]] static inline bool validate_uri(const char* begin, const char* end) noexcept {
    // URI validation is complex, using scalar for now
    return validate_uri_scalar(begin, end);
}

// Пропуск linear whitespace (SP / HTAB)
[[nodiscard]] static inline const char* skip_lws(const char* p, const char* end) noexcept {
    while (p < end && (*p == ' ' || *p == '\t')) {
        ++p;
    }
    return p;
}

// Поиск первого вхождения символа
[[nodiscard]] static inline const char* find_char(const char* begin, const char* end, char c) noexcept {
    // For single character search, scalar is often faster than SIMD
    for (const char* p = begin; p < end; ++p) {
        if (*p == c) {
            return p;
        }
    }
    return end;
}

// Поиск любого символа из набора (для future optimization)
[[nodiscard]] static inline const char* find_any_of(const char* begin, const char* end, 
                                                     const char* chars, size_t num_chars) noexcept {
    // Scalar implementation
    for (const char* p = begin; p < end; ++p) {
        for (size_t i = 0; i < num_chars; ++i) {
            if (*p == chars[i]) {
                return p;
            }
        }
    }
    return end;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Быстрое преобразование hex строки в число (для chunked encoding)
// Возвращает false если строка невалидна или переполнение
[[nodiscard]] static inline bool parse_hex_uint64(const char* begin, const char* end, uint64_t& result) noexcept {
    result = 0;
    
    if (begin == end) {
        return false;
    }
    
    // Валидация через SIMD
    if (!validate_hex_string(begin, end)) {
        return false;
    }
    
    // Преобразование
    for (const char* p = begin; p < end; ++p) {
        uint8_t nibble = hex_to_nibble[static_cast<uint8_t>(*p)];
        if (nibble == 255) {
            return false;
        }
        
        // Проверка переполнения
        if (result > (UINT64_MAX - nibble) / 16) {
            return false;
        }
        
        result = result * 16 + nibble;
    }
    
    return true;
}

// Быстрое преобразование десятичной строки в число (для Content-Length)
[[nodiscard]] static inline bool parse_decimal_uint64(const char* begin, const char* end, uint64_t& result) noexcept {
    result = 0;
    
    if (begin == end) {
        return false;
    }
    
    for (const char* p = begin; p < end; ++p) {
        uint8_t c = static_cast<uint8_t>(*p);
        if (!is_digit[c]) {
            return false;
        }
        
        uint64_t digit = static_cast<uint64_t>(c - '0');
        
        // Проверка переполнения
        if (result > (UINT64_MAX - digit) / 10) {
            return false;
        }
        
        result = result * 10 + digit;
    }
    
    return true;
}

// Case-insensitive сравнение (ASCII only)
[[nodiscard]] static inline bool icase_equal(const char* a_begin, const char* a_end,
                                             const char* b_begin, const char* b_end) noexcept {
    const size_t a_len = static_cast<size_t>(a_end - a_begin);
    const size_t b_len = static_cast<size_t>(b_end - b_begin);
    
    if (a_len != b_len) {
        return false;
    }
    
    for (size_t i = 0; i < a_len; ++i) {
        char ca = a_begin[i];
        char cb = b_begin[i];
        
        // Convert to lowercase
        if (ca >= 'A' && ca <= 'Z') {
            ca = static_cast<char>(ca + 32);
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = static_cast<char>(cb + 32);
        }
        
        if (ca != cb) {
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// BENCHMARKING HELPERS (Для тестирования производительности)
// ============================================================================

// Подсчет количества CRLF в буфере (для статистики)
[[nodiscard]] static inline size_t count_crlf(const char* begin, const char* end) noexcept {
    size_t count = 0;
    const char* p = begin;
    
    while (p < end - 1) {
        const char* found = find_crlf(p, end);
        if (found == end) {
            break;
        }
        ++count;
        p = found + 2;
    }
    
    return count;
}

// Проверка, содержит ли буфер только валидные token characters
[[nodiscard]] static inline bool is_all_tokens(const char* begin, const char* end) noexcept {
    return validate_token(begin, end);
}

} // namespace simd
} // namespace http_parser
