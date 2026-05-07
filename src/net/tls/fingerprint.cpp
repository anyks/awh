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
#include <cstdint>
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>

/**
 * Модули OpenSSL для вычисления хешей
 */
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

/**
 * Инкапсулируем статические объекты в пространство имён локальных вспомогательных функций
 */
namespace local {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Вспомогательная функция проверки GREASE-значений
	 *
	 * @param value 16-битное значение для проверки
	 * @return      true, если значение является GREASE-значением, иначе false
	 */
	static inline bool isGrease(const uint16_t value) noexcept {
		// Получаем старший и младший байт значения
		const uint8_t hi = (value >> 8), lo = (value & 0xFF);
		// GREASE-значения имеют вид 0xXYXY, где X = Y и Y & 0x0F == 0x0A
		return ((hi == lo) && ((lo & 0x0F) == 0x0A));
	}

	/**
	 * @brief Вспомогательная функция чтения 16-битного значения из буфера в формате big-endian
	 *
	 * @param buffer бинарный буфер с данными handshake-сообщения
	 * @return       значение в формате big-endian
	 */
	static inline uint16_t u16(const uint8_t * buffer) noexcept {
		// Читаем 16-битное значение из буфера в формате big-endian
		return ((static_cast <uint16_t> (buffer[0]) << 8) | buffer[1]);
	}

	/**
	 * @brief Вспомогательная функция вычисления MD5 строки
	 *
	 * @param input входная строка
	 * @return      MD5 hex lowercase
	 */
	static string md5(const string & input) noexcept {
		// Результат работы функции
		char result[33] = {'\0'};
		// Буфер для хранения MD5 хеша
		uint8_t digest[MD5_DIGEST_LENGTH];
		// Вычисляем MD5 хеш строки
		::MD5(reinterpret_cast <const uint8_t *> (input.data()), input.size(), digest);
		/**
		 * Преобразуем бинарный MD5 хеш в lowercase hex-строку из 32 символов
		 */
		for(uint32_t i = 0; i < MD5_DIGEST_LENGTH; i++)
			// Преобразуем каждый байт MD5 хеша в 2 символа hex и записываем в результат
			::snprintf(result + 2 * i, 3, "%02x", digest[i]);
		// Возвращаем итоговый результат
		return result;
	}

	/**
	 * @brief Вспомогательная функция преобразования бинарного буфера в lowercase hex-строку
	 *
	 * @param buffer бинарный буфер данных
	 * @param size   размер бинарного буфера
	 * @return       hex-строка
	 */
	static string tohex(const uint8_t * buffer, const size_t size) noexcept {
		// Результат работы функции
		string result(2 * size, '0');
		/**
		 * Преобразуем каждый байт бинарного буфера в 2 символа hex
		 */
		for(size_t i = 0; i < size; i++)
			// Преобразуем каждый байт в 2 символа hex и записываем в результат
			::snprintf(&result[2 * i], 3, "%02x", buffer[i]);
		// Возвращаем итоговый результат
		return result;
	}

	/**
	 * @brief Вспомогательная функция возвращения TLS wire-кода для код шифра
	 *
	 * @param cipher тип шифра
	 * @return       wire-код (0 = неизвестный)
	 */
	static inline uint16_t cipherWire(const awh::tls::cipher_t cipher) noexcept {
		/**
		 * Возвращаем TLS wire-код для типа шифра
		 */
		switch(static_cast <uint8_t> (cipher)){
			// Если алгоритм шифрования соответствует AES128-SHA
			case static_cast <uint8_t> (awh::tls::cipher_t::AES128_SHA):
				// Возвращаем TLS wire-код для AES128-SHA
				return 0x002F;
			// Если алгоритм шифрования соответствует AES256-SHA
			case static_cast <uint8_t> (awh::tls::cipher_t::AES256_SHA):
				// Возвращаем TLS wire-код для AES256-SHA
				return 0x0035;
			// Если алгоритм шифрования соответствует AES128-GCM-SHA256
			case static_cast <uint8_t> (awh::tls::cipher_t::AES128_GCM_SHA256):
				// Возвращаем TLS wire-код для AES128-GCM-SHA256
				return 0x009C;
			// Если алгоритм шифрования соответствует AES256-GCM-SHA384
			case static_cast <uint8_t> (awh::tls::cipher_t::AES256_GCM_SHA384):
				// Возвращаем TLS wire-код для AES256-GCM-SHA384
				return 0x009D;
			// Если алгоритм шифрования соответствует PSK-AES128-CBC-SHA
			case static_cast <uint8_t> (awh::tls::cipher_t::PSK_AES128_CBC_SHA):
				// Возвращаем TLS wire-код для PSK-AES128-CBC-SHA
				return 0x008C;
			// Если алгоритм шифрования соответствует PSK-AES256-CBC-SHA
			case static_cast <uint8_t> (awh::tls::cipher_t::PSK_AES256_CBC_SHA):
				// Возвращаем TLS wire-код для PSK-AES256-CBC-SHA
				return 0x008D;
			// Если алгоритм шифрования соответствует ECDHE-RSA-AES128-SHA
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_RSA_AES128_SHA):
				// Возвращаем TLS wire-код для ECDHE-RSA-AES128-SHA
				return 0xC013;
			// Если алгоритм шифрования соответствует ECDHE-RSA-AES256-SHA
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_RSA_AES256_SHA):
				// Возвращаем TLS wire-код для ECDHE-RSA-AES256-SHA
				return 0xC014;
			// Если алгоритм шифрования соответствует ECDHE-ECDSA-AES128-SHA
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_ECDSA_AES128_SHA):
				// Возвращаем TLS wire-код для ECDHE-ECDSA-AES128-SHA
				return 0xC009;
			// Если алгоритм шифрования соответствует ECDHE-ECDSA-AES256-SHA
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_ECDSA_AES256_SHA):
				// Возвращаем TLS wire-код для ECDHE-ECDSA-AES256-SHA
				return 0xC00A;
			// Если алгоритм шифрования соответствует ECDHE-RSA-AES128-SHA256
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_RSA_AES128_SHA256):
				// Возвращаем TLS wire-код для ECDHE-RSA-AES128-SHA256
				return 0xC027;
			// Если алгоритм шифрования соответствует ECDHE-PSK-AES128-CBC-SHA
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_PSK_AES128_CBC_SHA):
				// Возвращаем TLS wire-код для ECDHE-PSK-AES128-CBC-SHA
				return 0xC035;
			// Если алгоритм шифрования соответствует ECDHE-PSK-AES256-CBC-SHA
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_PSK_AES256_CBC_SHA):
				// Возвращаем TLS wire-код для ECDHE-PSK-AES256-CBC-SHA
				return 0xC036;
			// Если алгоритм шифрования соответствует ECDHE-ECDSA-AES128-SHA256
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_ECDSA_AES128_SHA256):
				// Возвращаем TLS wire-код для ECDHE-ECDSA-AES128-SHA256
				return 0xC023;
			// Если алгоритм шифрования соответствует ECDHE-RSA-AES128-GCM-SHA256
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_RSA_AES128_GCM_SHA256):
				// Возвращаем TLS wire-код для ECDHE-RSA-AES128-GCM-SHA256
				return 0xC02F;
			// Если алгоритм шифрования соответствует ECDHE-RSA-AES256-GCM-SHA384
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_RSA_AES256_GCM_SHA384):
				// Возвращаем TLS wire-код для ECDHE-RSA-AES256-GCM-SHA384
				return 0xC030;
			// Если алгоритм шифрования соответствует ECDHE-RSA-CHACHA20-POLY1305
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_RSA_CHACHA20_POLY1305):
				// Возвращаем TLS wire-код для ECDHE-RSA-CHACHA20-POLY1305
				return 0xCCA8;
			// Если алгоритм шифрования соответствует ECDHE-PSK-CHACHA20-POLY1305
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_PSK_CHACHA20_POLY1305):
				// Возвращаем TLS wire-код для ECDHE-PSK-CHACHA20-POLY1305
				return 0xCCAC;
			// Если алгоритм шифрования соответствует ECDHE-ECDSA-AES128-GCM-SHA256
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_ECDSA_AES128_GCM_SHA256):
				// Возвращаем TLS wire-код для ECDHE-ECDSA-AES128-GCM-SHA256
				return 0xC02B;
			// Если алгоритм шифрования соответствует ECDHE-ECDSA-AES256-GCM-SHA384
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_ECDSA_AES256_GCM_SHA384):
				// Возвращаем TLS wire-код для ECDHE-ECDSA-AES256-GCM-SHA384
				return 0xC02C;
			// Если алгоритм шифрования соответствует ECDHE-ECDSA-CHACHA20-POLY1305
			case static_cast <uint8_t> (awh::tls::cipher_t::ECDHE_ECDSA_CHACHA20_POLY1305):
				// Возвращаем TLS wire-код для ECDHE-ECDSA-CHACHA20-POLY1305
				return 0xCCA9;
			// Если алгоритм шифрования соответствует TLS_AES_256_GCM_SHA384
			case static_cast <uint8_t> (awh::tls::cipher_t::TLS_AES_256_GCM_SHA384):
				// Возвращаем TLS wire-код для TLS_AES_256_GCM_SHA384
				return 0x1302;
			// Если алгоритм шифрования соответствует TLS_AES_128_GCM_SHA256
			case static_cast <uint8_t> (awh::tls::cipher_t::TLS_AES_128_GCM_SHA256):
				// Возвращаем TLS wire-код для TLS_AES_128_GCM_SHA256
				return 0x1301;
			// Если алгоритм шифрования соответствует TLS_CHACHA20_POLY1305_SHA256
			case static_cast <uint8_t> (awh::tls::cipher_t::TLS_CHACHA20_POLY1305_SHA256):
				// Возвращаем TLS wire-код для TLS_CHACHA20_POLY1305_SHA256
				return 0x1303;
			// Если алгоритм шифрования неопределён, возвращаем 0
			default: return 0;
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения TLS wire-кода для кода версии TLS
	 *
	 * @param version тип версии
	 * @return        wire-код (0 = неизвестный)
	 */
	static inline uint16_t versionWire(const awh::tls::version_t version) noexcept {
		/**
		 * Возвращаем TLS wire-код для типа версии
		 */
		switch(static_cast <uint8_t> (version)){
			// Если версия соответствует SSL 3.0
			case static_cast <uint8_t> (awh::tls::version_t::SSL_V3):
				// Возвращаем TLS wire-код для SSL 3.0
				return 0x0300;
			// Если версия соответствует TLS 1.0
			case static_cast <uint8_t> (awh::tls::version_t::TLS_1_0):
				// Возвращаем TLS wire-код для TLS 1.0
				return 0x0301;
			// Если версия соответствует TLS 1.1
			case static_cast <uint8_t> (awh::tls::version_t::TLS_1_1):
				// Возвращаем TLS wire-код для TLS 1.1
				return 0x0302;
			// Если версия соответствует TLS 1.2
			case static_cast <uint8_t> (awh::tls::version_t::TLS_1_2):
				// Возвращаем TLS wire-код для TLS 1.2
				return 0x0303;
			// Если версия соответствует TLS 1.3
			case static_cast <uint8_t> (awh::tls::version_t::TLS_1_3):
				// Возвращаем TLS wire-код для TLS 1.3
				return 0x0304;
			// Если версия соответствует DTLS 1.0
			case static_cast <uint8_t> (awh::tls::version_t::DTLS_1_0):
				// Возвращаем TLS wire-код для DTLS 1.0
				return 0xFEFF;
			// Если версия соответствует DTLS 1.2
			case static_cast <uint8_t> (awh::tls::version_t::DTLS_1_2):
				// Возвращаем TLS wire-код для DTLS 1.2
				return 0xFEFD;
			// Если версия не определена, возвращаем 0
			default: return 0;
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения TLS wire-кода для кода группы эллиптических кривых
	 *
	 * @param gid тип группы
	 * @return    wire-код (0 = неизвестный)
	 */
	static inline uint16_t groupWire(const awh::tls::group_t gid) noexcept {
		/**
		 * Возвращаем TLS wire-код для типа группы эллиптических кривых
		 */
		switch(static_cast <uint8_t> (gid)){
			// Если группа соответствует secp256r1 (NIST P-256)
			case static_cast <uint8_t> (awh::tls::group_t::P_256):
				// Возвращаем TLS wire-код для secp256r1 (NIST P-256)
				return 0x0017;
			// Если группа соответствует secp384r1 (NIST P-384)
			case static_cast <uint8_t> (awh::tls::group_t::P_384):
				// Возвращаем TLS wire-код для secp384r1 (NIST P-384)
				return 0x0018;
			// Если группа соответствует secp521r1 (NIST P-521)
			case static_cast <uint8_t> (awh::tls::group_t::P_521):
				// Возвращаем TLS wire-код для secp521r1 (NIST P-521)
				return 0x0019;
			// Если группа соответствует X448
			case static_cast <uint8_t> (awh::tls::group_t::X448):
				// Возвращаем TLS wire-код для X448
				return 0x001E;
			// Если группа соответствует X25519
			case static_cast <uint8_t> (awh::tls::group_t::X25519):
				// Возвращаем TLS wire-код для X25519
				return 0x001D;
			// Если группа соответствует SECP256K1
			case static_cast <uint8_t> (awh::tls::group_t::SECP256K1):
				// Возвращаем TLS wire-код для SECP256K1
				return 0x001C;
			// Если группа соответствует FFDHE2048
			case static_cast <uint8_t> (awh::tls::group_t::FFDHE2048):
				// Возвращаем TLS wire-код для FFDHE2048
				return 0x0100;
			// Если группа соответствует FFDHE3072
			case static_cast <uint8_t> (awh::tls::group_t::FFDHE3072):
				// Возвращаем TLS wire-код для FFDHE3072
				return 0x0101;
			// Если группа соответствует FFDHE4096
			case static_cast <uint8_t> (awh::tls::group_t::FFDHE4096):
				// Возвращаем TLS wire-код для FFDHE4096
				return 0x0102;
			// Если группа соответствует FFDHE6144
			case static_cast <uint8_t> (awh::tls::group_t::FFDHE6144):
				// Возвращаем TLS wire-код для FFDHE6144
				return 0x0103;
			// Если группа соответствует FFDHE8192
			case static_cast <uint8_t> (awh::tls::group_t::FFDHE8192):
				// Возвращаем TLS wire-код для FFDHE8192
				return 0x0104;
			// Если группа соответствует MLKEM1024
			case static_cast <uint8_t> (awh::tls::group_t::MLKEM1024):
				// Возвращаем TLS wire-код для MLKEM1024
				return 0x0202;
			// Если группа соответствует x25519_mlkem768
			case static_cast <uint8_t> (awh::tls::group_t::X25519_MLKEM768):
				// Возвращаем TLS wire-код для x25519_mlkem768
				return 0x11EC;
			// Если группа соответствует x25519_kyber768_draft00
			case static_cast <uint8_t> (awh::tls::group_t::X25519_KYBER768_DRAFT00):
				// Возвращаем TLS wire-код для x25519_kyber768_draft00
				return 0x6399;
			// Если группа не определена, возвращаем 0
			default: return 0;
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения TLS wire-кода для алгоритма подписи
	 *
	 * @param sign тип алгоритма
	 * @return     wire-код (0 = неизвестный)
	 */
	static inline uint16_t signatureWire(const awh::tls::signature_t sign) noexcept {
		/**
		 * Возвращаем TLS wire-код для типа алгоритма подписи
		 */
		switch(static_cast <uint8_t> (sign)){
			// Если алгоритм подписи соответствует ED448
			case static_cast <uint8_t> (awh::tls::signature_t::ED448):
				// Возвращаем TLS wire-код для ED448
				return 0x0808;
			// Если алгоритм подписи соответствует ED25519
			case static_cast <uint8_t> (awh::tls::signature_t::ED25519):
				// Возвращаем TLS wire-код для ED25519
				return 0x0807;
			// Если алгоритм подписи соответствует DSA_SHA1
			case static_cast <uint8_t> (awh::tls::signature_t::DSA_SHA1):
				// Возвращаем TLS wire-код для DSA_SHA1
				return 0x0202;
			// Если алгоритм подписи соответствует RSA_PKCS1_SHA1
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PKCS1_SHA1):
				// Возвращаем TLS wire-код для RSA_PKCS1_SHA1
				return 0x0201;
			// Если алгоритм подписи соответствует RSA_PKCS1_SHA256
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PKCS1_SHA256):
				// Возвращаем TLS wire-код для RSA_PKCS1_SHA256
				return 0x0401;
			// Если алгоритм подписи соответствует RSA_PKCS1_SHA384
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PKCS1_SHA384):
				// Возвращаем TLS wire-код для RSA_PKCS1_SHA384
				return 0x0501;
			// Если алгоритм подписи соответствует RSA_PKCS1_SHA512
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PKCS1_SHA512):
				// Возвращаем TLS wire-код для RSA_PKCS1_SHA512
				return 0x0601;
			// Если алгоритм подписи соответствует RSA_PSS_PSS_SHA256
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PSS_PSS_SHA256):
				// Возвращаем TLS wire-код для RSA_PSS_PSS_SHA256
				return 0x0809;
			// Если алгоритм подписи соответствует RSA_PSS_PSS_SHA384
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PSS_PSS_SHA384):
				// Возвращаем TLS wire-код для RSA_PSS_PSS_SHA384
				return 0x080A;
			// Если алгоритм подписи соответствует RSA_PSS_PSS_SHA512
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PSS_PSS_SHA512):
				// Возвращаем TLS wire-код для RSA_PSS_PSS_SHA512
				return 0x080B;
			// Если алгоритм подписи соответствует RSA_PSS_RSAE_SHA256
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PSS_RSAE_SHA256):
				// Возвращаем TLS wire-код для RSA_PSS_RSAE_SHA256
				return 0x0804;
			// Если алгоритм подписи соответствует RSA_PSS_RSAE_SHA384
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PSS_RSAE_SHA384):
				// Возвращаем TLS wire-код для RSA_PSS_RSAE_SHA384
				return 0x0805;
			// Если алгоритм подписи соответствует RSA_PSS_RSAE_SHA512
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PSS_RSAE_SHA512):
				// Возвращаем TLS wire-код для RSA_PSS_RSAE_SHA512
				return 0x0806;
			// Если алгоритм подписи соответствует ECDSA_SHA1
			case static_cast <uint8_t> (awh::tls::signature_t::ECDSA_SHA1):
				// Возвращаем TLS wire-код для ECDSA_SHA1
				return 0x0203;
			// Если алгоритм подписи соответствует ECDSA_SECP256R1_SHA256
			case static_cast <uint8_t> (awh::tls::signature_t::ECDSA_SECP256R1_SHA256):
				// Возвращаем TLS wire-код для ECDSA_SECP256R1_SHA256
				return 0x0403;
			// Если алгоритм подписи соответствует ECDSA_SECP384R1_SHA384
			case static_cast <uint8_t> (awh::tls::signature_t::ECDSA_SECP384R1_SHA384):
				// Возвращаем TLS wire-код для ECDSA_SECP384R1_SHA384
				return 0x0503;
			// Если алгоритм подписи соответствует ECDSA_SECP521R1_SHA512
			case static_cast <uint8_t> (awh::tls::signature_t::ECDSA_SECP521R1_SHA512):
				// Возвращаем TLS wire-код для ECDSA_SECP521R1_SHA512
				return 0x0603;
			// Если алгоритм подписи соответствует RSA_PKCS1_MD5_SHA1
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PKCS1_MD5_SHA1):
				// Возвращаем TLS wire-код для RSA_PKCS1_MD5_SHA1
				return 0xFF01;
			// Если алгоритм подписи соответствует RSA_PKCS1_SHA256_LEGACY
			case static_cast <uint8_t> (awh::tls::signature_t::RSA_PKCS1_SHA256_LEGACY):
				// Возвращаем TLS wire-код для RSA_PKCS1_SHA256_LEGACY
				return 0x0420;
			// Если алгоритм подписи не определён, возвращаем 0
			default: return 0;
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения TLS wire-кода для типа расширения
	 *
	 * @param ext тип расширения
	 * @return    wire-код (0xFFFF = неизвестный/непривязанный)
	 */
	static inline uint16_t extensionWire(const awh::tls::extension_type_t ext) noexcept {
		/**
		 * Возвращаем TLS wire-код для типа расширения
		 */
		switch(static_cast <uint8_t> (ext)){
			// Если тип расширения соответствует server_name
			case static_cast <uint8_t> (awh::tls::extension_type_t::SERVER_NAME):
				// Возвращаем TLS wire-код для server_name
				return 0x0000;
			// Если тип расширения соответствует max_fragment_length
			case static_cast <uint8_t> (awh::tls::extension_type_t::MAX_FRAGMENT_LENGTH):
				// Возвращаем TLS wire-код для max_fragment_length
				return 0x0001;
			// Если тип расширения соответствует status_request
			case static_cast <uint8_t> (awh::tls::extension_type_t::STATUS_REQUEST):
				// Возвращаем TLS wire-код для status_request
				return 0x0005;
			// Если тип расширения соответствует supported_groups
			case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_GROUPS):
				// Возвращаем TLS wire-код для supported_groups
				return 0x000A;
			// Если тип расширения соответствует ec_point_formats
			case static_cast <uint8_t> (awh::tls::extension_type_t::EC_POINT_FORMATS):
				// Возвращаем TLS wire-код для ec_point_formats
				return 0x000B;
			// Если тип расширения соответствует signature_algorithms
			case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS):
				// Возвращаем TLS wire-код для signature_algorithms
				return 0x000D;
			// Если тип расширения соответствует use_srtp
			case static_cast <uint8_t> (awh::tls::extension_type_t::USE_SRTP):
				// Возвращаем TLS wire-код для use_srtp
				return 0x000E;
			// Если тип расширения соответствует heartbeat
			case static_cast <uint8_t> (awh::tls::extension_type_t::HEARTBEAT):
				// Возвращаем TLS wire-код для heartbeat
				return 0x000F;
			// Если тип расширения соответствует alpn
			case static_cast <uint8_t> (awh::tls::extension_type_t::ALPN):
				// Возвращаем TLS wire-код для alpn
				return 0x0010;
			// Если тип расширения соответствует signed_certificate_timestamp
			case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNED_CERTIFICATE_TIMESTAMP):
				// Возвращаем TLS wire-код для signed_certificate_timestamp
				return 0x0012;
			// Если тип расширения соответствует padding
			case static_cast <uint8_t> (awh::tls::extension_type_t::PADDING):
				// Возвращаем TLS wire-код для padding
				return 0x0015;
			// Если тип расширения соответствует encrypt_then_mac
			case static_cast <uint8_t> (awh::tls::extension_type_t::ENCRYPT_THEN_MAC):
				// Возвращаем TLS wire-код для encrypt_then_mac
				return 0x0016;
			// Если тип расширения соответствует extended_master_secret
			case static_cast <uint8_t> (awh::tls::extension_type_t::EXTENDED_MASTER_SECRET):
				// Возвращаем TLS wire-код для extended_master_secret
				return 0x0017;
			// Если тип расширения соответствует compress_certificate
			case static_cast <uint8_t> (awh::tls::extension_type_t::COMPRESS_CERTIFICATE):
				// Возвращаем TLS wire-код для compress_certificate
				return 0x001B;
			// Если тип расширения соответствует record_size_limit
			case static_cast <uint8_t> (awh::tls::extension_type_t::RECORD_SIZE_LIMIT):
				// Возвращаем TLS wire-код для record_size_limit
				return 0x001C;
			// Если тип расширения соответствует delegated_credential
			case static_cast <uint8_t> (awh::tls::extension_type_t::DELEGATED_CREDENTIAL):
				// Возвращаем TLS wire-код для delegated_credential
				return 0x0022;
			// Если тип расширения соответствует session_ticket
			case static_cast <uint8_t> (awh::tls::extension_type_t::SESSION_TICKET):
				// Возвращаем TLS wire-код для session_ticket
				return 0x0023;
			// Если тип расширения соответствует pre_shared_key
			case static_cast <uint8_t> (awh::tls::extension_type_t::PRE_SHARED_KEY):
				// Возвращаем TLS wire-код для pre_shared_key
				return 0x0029;
			// Если тип расширения соответствует early_data
			case static_cast <uint8_t> (awh::tls::extension_type_t::EARLY_DATA):
				// Возвращаем TLS wire-код для early_data
				return 0x002A;
			// Если тип расширения соответствует supported_versions
			case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_VERSIONS):
				// Возвращаем TLS wire-код для supported_versions
				return 0x002B;
			// Если тип расширения соответствует cookie
			case static_cast <uint8_t> (awh::tls::extension_type_t::COOKIE):
				// Возвращаем TLS wire-код для cookie
				return 0x002C;
			// Если тип расширения соответствует psk_key_exchange_modes
			case static_cast <uint8_t> (awh::tls::extension_type_t::PSK_KEY_EXCHANGE_MODES):
				// Возвращаем TLS wire-код для psk_key_exchange_modes
				return 0x002D;
			// Если тип расширения соответствует certificate_authorities
			case static_cast <uint8_t> (awh::tls::extension_type_t::CERTIFICATE_AUTHORITIES):
				// Возвращаем TLS wire-код для certificate_authorities
				return 0x002F;
			// Если тип расширения соответствует post_handshake_auth
			case static_cast <uint8_t> (awh::tls::extension_type_t::POST_HANDSHAKE_AUTH):
				// Возвращаем TLS wire-код для post_handshake_auth
				return 0x0031;
			// Если тип расширения соответствует signature_algorithms_cert
			case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS_CERT):
				// Возвращаем TLS wire-код для signature_algorithms_cert
				return 0x0032;
			// Если тип расширения соответствует key_share
			case static_cast <uint8_t> (awh::tls::extension_type_t::KEY_SHARE):
				// Возвращаем TLS wire-код для key_share
				return 0x0033;
			// Если тип расширения соответствует quic_transport_parameters
			case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS):
				// Возвращаем TLS wire-код для quic_transport_parameters
				return 0x0039;
			// Если тип расширения соответствует tls_flags
			case static_cast <uint8_t> (awh::tls::extension_type_t::TLS_FLAGS):
				// Возвращаем TLS wire-код для tls_flags
				return 0x003E;
			// Если тип расширения соответствует next_protocol_negotiation
			case static_cast <uint8_t> (awh::tls::extension_type_t::NEXT_PROTO_NEG):
				// Возвращаем TLS wire-код для next_protocol_negotiation
				return 0x3374;
			// Если тип расширения соответствует application_settings_old (устаревшее)
			case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS_OLD):
				// Возвращаем TLS wire-код для application_settings_old (устаревшее)
				return 0x4469;
			// Если тип расширения соответствует application_settings
			case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS):
				// Возвращаем TLS wire-код для application_settings
				return 0x44CD;
			// Если тип расширения соответствует channel_id
			case static_cast <uint8_t> (awh::tls::extension_type_t::CHANNEL_ID):
				// Возвращаем TLS wire-код для channel_id
				return 0x7550;
			// Если тип расширения соответствует trust_anchors
			case static_cast <uint8_t> (awh::tls::extension_type_t::TRUST_ANCHORS):
				// Возвращаем TLS wire-код для trust_anchors
				return 0xCA34;
			// Если тип расширения соответствует ech_outer_extensions
			case static_cast <uint8_t> (awh::tls::extension_type_t::ECH_OUTER_EXTENSIONS):
				// Возвращаем TLS wire-код для ech_outer_extensions
				return 0xFD00;
			// Если тип расширения соответствует encrypted_client_hello
			case static_cast <uint8_t> (awh::tls::extension_type_t::ENCRYPTED_CLIENT_HELLO):
				// Возвращаем TLS wire-код для encrypted_client_hello
				return 0xFE0D;
			// Если тип расширения соответствует renegotiation_info
			case static_cast <uint8_t> (awh::tls::extension_type_t::RENEGOTIATION_INFO):
				// Возвращаем TLS wire-код для renegotiation_info
				return 0xFF01;
			// Если тип расширения соответствует quic_transport_parameters_legacy
			case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS_LEGACY):
				// Возвращаем TLS wire-код для quic_transport_parameters_legacy
				return 0xFFA5;
			// Если тип расширения не определён, возвращаем неизвестный код
			default: return 0xFFFF;
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения wire-кода для формата точки эллиптической кривой
	 *
	 * @param format формат точки
	 * @return       wire-код (0xFF = неизвестный)
	 */
	static inline uint8_t ecPointWire(const awh::tls::ec_point_format_t format) noexcept {
		/**
		 * Возвращаем wire-код для формата точки эллиптической кривой
		 */
		switch(static_cast <uint8_t> (format)){
			// Если формат соответствует uncompressed
			case static_cast <uint8_t> (awh::tls::ec_point_format_t::UNCOMPRESSED):
				// Возвращаем wire-код для uncompressed
				return 0x00;
			// Если формат соответствует ANSIX962
			case static_cast <uint8_t> (awh::tls::ec_point_format_t::ANSIX962):
				// Возвращаем wire-код для ANSIX962
				return 0x01;
			// Если формат соответствует ANSIX962_2
			case static_cast <uint8_t> (awh::tls::ec_point_format_t::ANSIX962_2):
				// Возвращаем wire-код для ANSIX962_2
				return 0x02;
			// Если формат не определён, возвращаем неизвестный код
			default: return 0xFF;
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения wire-кода для метода компрессии (legacy_compression_methods)
	 *
	 * @param compressor метод компрессии
	 * @return           wire-код (0xFF = нестандартный)
	 */
	static inline uint8_t compressorWire(const awh::tls::compressor_t compressor) noexcept {
		/**
		 * Возвращаем wire-код для метода компрессии (legacy_compression_methods)
		 */
		switch(static_cast <uint8_t> (compressor)){
			// Если метод компрессии соответствует NONE
			case static_cast <uint8_t> (awh::tls::compressor_t::NONE):
				// Возвращаем wire-код для NONE
				return 0x00;
			// Если метод компрессии соответствует ZLIB
			case static_cast <uint8_t> (awh::tls::compressor_t::ZLIB):
				// Возвращаем wire-код для ZLIB
				return 0x01;
			// Если метод компрессии не определён, возвращаем неизвестный код
			default: return 0xFF;
		}
	}
};

/**
 * Инкапсулируем статические объекты в пространство имён основных функций парсинга
 */
namespace fingerprint {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Вспомогательная функция чтения QUIC variable-length integer (RFC 9000 §16)
	 *
	 * @param buffer бинарный буфер данных
	 * @param size   размер буфера
	 * @param offset текущее смещение (увеличивается на количество прочитанных байт)
	 * @return       прочитанное значение, или 0 если данных недостаточно
	 */
	static inline uint64_t readQUICVarint(const uint8_t * buffer, const size_t size, size_t & offset) noexcept {
		// Если смещение за пределами буфера — возвращаем 0
		if(offset >= size)
			// Данных недостаточно для чтения
			return 0;
		// Первый байт определяет тип длины (2 старших бита)
		const uint8_t first = buffer[offset];
		// Получаем тип длины из старших 2 битов первого байта
		const uint8_t type = (first >> 6);
		// 1 байт: биты 00xxxxxx → значение 0..63
		if(type == 0)
			// Читаем 6 бит из первого байта и возвращаем значение
			return (buffer[offset++] & 0x3F);
		// 2 байта: биты 01xxxxxx xxxxxxxx → значение 0..16383
		else if(type == 1) {
			// Если данных недостаточно для чтения 2 байт, возвращаем 0
			if((offset + 2) > size){
				// Смещаемся в конец буфера, так как данных недостаточно для чтения 2 байт
				offset = size;
				// Данных недостаточно для чтения 2 байт
				return 0;
			}
			// Читаем 14 бит из 2 байт и возвращаем значение
			const uint64_t val = ((static_cast <uint64_t> (buffer[offset] & 0x3F) << 8) | buffer[offset + 1]);
			// Увеличиваем смещение на 2 байта
			offset += 2;
			// Возвращаем прочитанное значение
			return val;
		// 4 байта: биты 10xxxxxx × 3 → значение 0..1073741823
		} else if(type == 2) {
			// Если данных недостаточно для чтения 4 байт, возвращаем 0
			if((offset + 4) > size){
				// Смещаемся в конец буфера, так как данных недостаточно для чтения 4 байт
				offset = size;
				// Данных недостаточно для чтения 4 байт
				return 0;
			}
			// Читаем 30 бит из 4 байт и возвращаем значение
			const uint64_t val = (
				(static_cast <uint64_t> (buffer[offset]     & 0x3F) << 24) |
				(static_cast <uint64_t> (buffer[offset + 1])        << 16) |
				(static_cast <uint64_t> (buffer[offset + 2])        <<  8) |
				 static_cast <uint64_t> (buffer[offset + 3])
			);
			// Увеличиваем смещение на 4 байта
			offset += 4;
			// Возвращаем прочитанное значение
			return val;
		// 8 байт: биты 11xxxxxx × 7 → значение 0..4611686018427387903
		} else {
			// Если данных недостаточно для чтения 8 байт, возвращаем 0
			if((offset + 8) > size){
				// Смещаемся в конец буфера, так как данных недостаточно для чтения 8 байт
				offset = size;
				// Данных недостаточно для чтения 8 байт
				return 0;
			}
			// Читаем 62 бит из 8 байт и возвращаем значение
			const uint64_t val = (
				(static_cast <uint64_t> (buffer[offset]     & 0x3F) << 56) |
				(static_cast <uint64_t> (buffer[offset + 1])        << 48) |
				(static_cast <uint64_t> (buffer[offset + 2])        << 40) |
				(static_cast <uint64_t> (buffer[offset + 3])        << 32) |
				(static_cast <uint64_t> (buffer[offset + 4])        << 24) |
				(static_cast <uint64_t> (buffer[offset + 5])        << 16) |
				(static_cast <uint64_t> (buffer[offset + 6])        <<  8) |
				 static_cast <uint64_t> (buffer[offset + 7])
			);
			// Увеличиваем смещение на 8 байт
			offset += 8;
			// Возвращаем прочитанное значение
			return val;
		}
	}

	/**
	 * @brief Внутренняя функция парсинга QUIC Transport Parameters (RFC 9000 §19, RFC 9001)
	 * Заполняет map<type_id, value>. Значение читается как QUIC varint если помещается (≤8 байт), иначе 0.
	 *
	 * @param buffer бинарный буфер данных расширения
	 * @param size   размер буфера
	 * @param params карта для сохранения результатов
	 */
	static void parseQUICTransportParamsInternal(const uint8_t * buffer, const size_t size, unordered_map <uint64_t, uint64_t> & params) noexcept {
		// Инициализируем смещение
		size_t offset = 0;
		/**
		 * Каждый параметр: type(varint) + length(varint) + value(length байт)
		 */
		while(offset < size){
			// Сохраняем начальное смещение для проверки прогресса
			const size_t saved = offset;
			// Читаем идентификатор типа параметра
			const uint64_t type = readQUICVarint(buffer, size, offset);
			// Если смещение не изменилось — данные повреждены
			if(offset == saved || offset > size)
				// Прерываем разбор
				break;
			// Читаем длину назначения параметра
			const size_t lenStart = offset;
			// Читаем длину значения параметра
			const uint64_t lenValue = readQUICVarint(buffer, size, offset);
			// Проверяем прогресс и границы
			if((offset == lenStart) || ((offset + static_cast <size_t> (lenValue)) > size))
				// Прерываем разбор
				break;
			// Пытаемся прочитать значение как varint (только если помещается в 8 байт)
			uint64_t value = 0;
			// Если длина значения параметра больше 0 и не превышает 8 байт
			if((lenValue > 0) && (lenValue <= 8)){
				// Сохраняем текущее смещение для чтения значения
				size_t valOffset = offset;
				// Читаем значение параметра как QUIC varint
				value = readQUICVarint(buffer, size, valOffset);
			}
			// Сохраняем параметр в карту
			params.emplace(type, value);
			// Переходим к следующему параметру
			offset += static_cast <size_t> (lenValue);
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения GREASE (RFC 8701)
	 *
	 * @param buffer  бинарный буфер с данными расширения GREASE
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseGrease(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение GREASE в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_grease_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения encrypt_then_mac (RFC 7366)
	 *
	 * @param buffer  бинарный буфер с данными расширения encrypt_then_mac
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseEncryptThenMAC(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение encrypt_then_mac в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_encrypt_then_mac_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения signed_certificate_timestamp (RFC 6962)
	 *
	 * @param buffer  бинарный буфер с данными расширения signed_certificate_timestamp
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseCertificateTimestamp(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение signed_certificate_timestamp в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_signed_certificate_timestamp_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения extended_master_secret (RFC 7627)
	 *
	 * @param buffer  бинарный буфер с данными расширения extended_master_secret
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseExtendedMasterSecret(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение extended_master_secret в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_extended_master_secret_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения post_handshake_auth (RFC 8446 §4.2.9, пустое)
	 *
	 * @param buffer  бинарный буфер с данными расширения post_handshake_auth
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void postHandshakeAuth(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение post_handshake_auth в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_post_handshake_auth_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения record_size_limit (RFC 8449)
	 *
	 * @param buffer  бинарный буфер с данными расширения record_size_limit
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseRecordSizeLimit(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение record_size_limit в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_record_size_limit_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Устанавливаем значение ограничения размера TLS-записи в объект расширения record_size_limit
		awh_cast <awh::tls::fgp_t::extension_record_size_limit_t *> (browser.extensions.back().get())->data = ::local::u16(buffer);
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения padding (RFC 7685)
	 *
	 * @param buffer  бинарный буфер с данными расширения padding
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parsePadding(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение padding в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_padding_t> ());
		// Устанавливаем размер данных расширения padding в объект расширения padding
		awh_cast <awh::tls::fgp_t::extension_padding_t *> (browser.extensions.back().get())->size = size;
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения session_ticket (RFC 5077)
	 *
	 * @param buffer  бинарный буфер с данными расширения session_ticket
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseSessionTicket(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Определяем размер данных расширения session_ticket (не более 32 байта)
		const size_t bytes = ::min <size_t> (32, size);
		// Добавляем расширение session_ticket в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_session_ticket_t> ());
		// Забиваем данные расширения нулями, чтобы гарантировать наличие данных в случае, если расширение session_ticket пустое
		awh_cast <awh::tls::fgp_t::extension_session_ticket_t *> (browser.extensions.back().get())->data.resize(bytes, 0);
		// Копируем данные расширения session_ticket из буфера в блок данных расширения session_ticket
		::memcpy(&awh_cast <awh::tls::fgp_t::extension_session_ticket_t *> (browser.extensions.back().get())->data[0], buffer, bytes);
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения early_data (RFC 8446 §4.2.10)
	 *
	 * @param buffer  бинарный буфер с данными расширения early_data
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseEarlyData(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение early_data в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_early_data_t> ());
		// Если размер данных в буфере меньше 4 байт, то данных недостаточно для парсинга
		if(size < 4)
			// Выходим из функции
			return;
		// Устанавливаем значение максимального размера данных для ранних данных (early data) в объект расширения early_data
		awh_cast <awh::tls::fgp_t::extension_early_data_t *> (browser.extensions.back().get())->maxSize = (
			(static_cast <uint32_t> (buffer[0]) << 24) |
			(static_cast <uint32_t> (buffer[1]) << 16) |
			(static_cast <uint32_t> (buffer[2]) << 8) | buffer[3]
		);
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения cookie (RFC 8446 §4.2.2)
	 *
	 * @param buffer  бинарный буфер с данными расширения cookie
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseCookie(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение cookie в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_cookie_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Определяем размер данных расширения cookie
		const uint16_t length = ::local::u16(buffer);
		// Если количество данных расширения cookie достаточно для парсинга
		if((length > 0) && (size >= (2 + length))){
			// Устанавливаем размер данных расширения cookie в объект расширения cookie
			awh_cast <awh::tls::fgp_t::extension_cookie_t *> (browser.extensions.back().get())->data.resize(length);
			// Копируем данные расширения cookie из буфера в блок данных расширения cookie
			::memcpy(&awh_cast <awh::tls::fgp_t::extension_cookie_t *> (browser.extensions.back().get())->data[0], buffer + 2, length);
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения certificate_authorities (RFC 8446 §4.2.4)
	 *
	 * @param buffer  бинарный буфер с данными расширения certificate_authorities
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseCertificateAuthorities(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение certificate_authorities в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_certificate_authorities_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Читаем байтовую длину всего списка DistinguishedName
		const uint16_t count = ::local::u16(buffer);
		// Проверяем, что список помещается в буфер
		if(size < static_cast <size_t> (2 + count))
			// Выходим из функции
			return;
		// Инициализируем смещение: пропускаем поле длины списка (2 байта)
		size_t offset = 2;
		// Конец списка
		const size_t end = static_cast <size_t> (2 + count);
		/**
		 * Перебираем все записи DistinguishedName в списке.
		 * Каждая запись: dn_length(2 байта) + dn_data(dn_length байт)
		 */
		while((offset + 2) <= end){
			// Читаем длину текущего DER-кодированного DistinguishedName
			const uint16_t length = ::local::u16(buffer + offset);
			// Сдвигаем смещение за поле длины
			offset += 2;
			// Проверяем, что DN помещается в буфер
			if((offset + length) > end)
				// Если данных недостаточно, прекращаем парсинг
				break;
			// Копируем DER-байты текущего DistinguishedName в вектор
			awh_cast <awh::tls::fgp_t::extension_certificate_authorities_t *> (browser.extensions.back().get())->authorities.emplace_back(buffer + offset, buffer + offset + length);
			// Сдвигаем смещение за данные DN
			offset += length;
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения key_share (RFC 8446 §4.2.7)
	 *
	 * @param buffer  бинарный буфер с данными расширения key_share
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseKeyShare(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение key_share в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_key_share_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		/**
		 * RFC 8446 §4.2.8: поле client_shares_length — это байтовая длина всего списка KeyShareEntry, 
		 * а не количество записей. Каждая запись: group(2) + key_exchange_length(2) + key_exchange_data.
		 */
		const uint16_t count = ::local::u16(buffer);
		// Если список пуст или данных в буфере недостаточно для его размещения — выходим
		if((count == 0) || (size < static_cast <size_t> (2 + count)))
			// Выходим из функции
			return;
		// Конец списка client_shares в буфере
		const size_t end = static_cast <size_t> (2 + count);
		// Инициализируем смещение: пропускаем поле байтовой длины списка (2 байта)
		size_t offset = 2;
		/**
		 * Перебираем все записи KeyShareEntry в списке client_shares.
		 * Каждая запись: group(2 байта) + key_exchange_length(2 байта) + key_exchange_data(key_exchange_length байт).
		 */
		while((offset + 4) <= end){
			// Получаем идентификатор группы обмена ключами (NamedGroup) из первых 2 байт текущей записи
			const uint16_t gid = ::local::u16(buffer + offset);
			// Получаем байтовую длину данных ключа для текущей записи
			const uint16_t length = ::local::u16(buffer + (offset + 2));
			// Увеличиваем смещение на 4 байта (group:2 + key_exchange_length:2)
			offset += 4;
			// Проверяем, что данные ключа помещаются в список
			if((offset + static_cast <size_t> (length)) > end)
				// Если данных недостаточно — прекращаем разбор
				break;
			// Если код группы является GREASE
			if(::local::isGrease(gid))
				// Добавляем код GREASE в список поддерживаемых версий браузера
				awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::GREASE, vector <uint8_t> {});
			// Если код группы является одной из стандартных версий из RFC 8446 §4.2.7
			else {
				/**
				 * Определяем код шифра
				 */
				switch(gid){
					// Если элиптическая кривая соответствует P-256 (secp256r1)
					case 0x0017:
						// Добавляем код группы эллиптической кривой P-256 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::P_256, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует P-384 (secp384r1)
					case 0x0018:
						// Добавляем код группы эллиптической кривой P-384 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::P_384, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует P-521 (secp521r1)
					case 0x0019:
						// Добавляем код группы эллиптической кривой P-521 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::P_521, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует X25519
					case 0x001D:
						// Добавляем код группы эллиптической кривой X25519 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::X25519, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует X448
					case 0x001E:
						// Добавляем код группы эллиптической кривой X448 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::X448, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует secp256k1
					case 0x001C:
						// Добавляем код группы эллиптической кривой secp256k1 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::SECP256K1, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует FFDHE 2048
					case 0x0100:
						// Добавляем код группы FFDHE 2048 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::FFDHE2048, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует FFDHE 3072
					case 0x0101:
						// Добавляем код группы FFDHE 3072 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::FFDHE3072, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует FFDHE 4096
					case 0x0102:
						// Добавляем код группы FFDHE 4096 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::FFDHE4096, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует FFDHE 6144
					case 0x0103:
						// Добавляем код группы FFDHE 6144 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::FFDHE6144, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует FFDHE 8192
					case 0x0104:
						// Добавляем код группы FFDHE 8192 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::FFDHE8192, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует MLKEM 1024
					case 0x0202:
						// Добавляем код группы MLKEM 1024 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::MLKEM1024, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует X25519Kyber768Draft00
					case 0x6399:
						// Добавляем код группы X25519Kyber768Draft00 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::X25519_KYBER768_DRAFT00, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая соответствует X25519MLKEM768
					case 0x11EC:
						// Добавляем код группы X25519MLKEM768 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::X25519_MLKEM768, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если элиптическая кривая не соответствует ни одной из известных, добавляем код UNKNOWN в список поддерживаемых групп эллиптических кривых браузера
					default: awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->keyShares.emplace(awh::tls::group_t::UNKNOWN, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
				}
			}
			// Увеличиваем смещение на длину ключа для текущей группы обмена ключами
			offset += static_cast <size_t> (length);
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения psk_key_exchange_modes (RFC 8446 §4.2.9)
	 *
	 * @param buffer  бинарный буфер с данными расширения psk_key_exchange_modes
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parsePSKKeyExchangeModes(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение psk_key_exchange_modes в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_psk_key_exchange_t> ());
		// Если размер данных в буфере меньше 1 байта, то данных недостаточно для парсинга
		if(size < 1)
			// Выходим из функции
			return;
		// Получаем количество поддерживаемых режимов обмена ключами PSK из первого байта данных расширения
		const uint8_t count = buffer[0];
		// Если количество поддерживаемых режимов обмена ключами PSK равно нулю или данных в буфере недостаточно для парсинга, то выходим из функции
		if((count == 0) || (size < (1 + count)))
			// Выходим из функции
			return;
		// Выполняем парсинг поддерживаемых режимов обмена ключами PSK из данных расширения
		for(uint8_t i = 0; ((i < count) && (static_cast <size_t> (1 + i) < size)); ++i){
			/**
			 * Проверяем значение режима обмена ключами PSK
			 */
			switch(buffer[1 + i]){
				// Если режим обмена ключами PSK является PSK-only
				case 0x00:
					// Добавляем значение PSK_ONLY в список поддерживаемых режимов обмена ключами PSK браузера
					awh_cast <awh::tls::fgp_t::extension_psk_key_exchange_t *> (browser.extensions.back().get())->modes.push_back(awh::tls::psk_key_t::PSK_ONLY);
				break;
				// Если режим обмена ключами PSK является PSK with (EC)DHE
				case 0x01:
					// Добавляем значение PSK_DHE в список поддерживаемых режимов обмена ключами PSK браузера
					awh_cast <awh::tls::fgp_t::extension_psk_key_exchange_t *> (browser.extensions.back().get())->modes.push_back(awh::tls::psk_key_t::PSK_DHE);
				break;
				// Если режим обмена ключами PSK является неизвестным или нераспознанным, то добавляем значение UNKNOWN в список поддерживаемых режимов обмена ключами PSK браузера
				default: awh_cast <awh::tls::fgp_t::extension_psk_key_exchange_t *> (browser.extensions.back().get())->modes.push_back(awh::tls::psk_key_t::UNKNOWN);
			}
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения supported_versions (RFC 8446 §4.2.1)
	 *
	 * @param buffer  бинарный буфер с данными расширения supported_versions
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseSupportedVersions(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение supported_versions в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_supported_versions_t> ());
		// Если размер данных в буфере меньше 1 байта, то данных недостаточно для парсинга
		if(size < 1)
			// Выходим из функции
			return;
		// Получаем количество поддерживаемых версий из первого байта данных расширения
		const uint8_t count = buffer[0];
		/**
		 * count — байтовая длина списка версий (RFC 8446 §4.2.1: versions<2..254> = 1-байт prefixed byte count).
		 * Каждая ProtocolVersion = 2 байта, поэтому count должен быть чётным.
		 * Необходимый размер буфера: 1 (length byte) + count (list bytes).
		 */
		if((count == 0) || (size < static_cast <size_t> (1 + count)) || ((count % 2) != 0))
			// Выходим из функции
			return;
		// Выполняем парсинг поддерживаемых версий из данных расширения
		for(size_t i = 1; ((i < static_cast <size_t> (1 + count)) && ((i + 1) < size)); i += 2){
			// Получаем 16-битное значение версии из данных расширения
			const uint16_t ver = ::local::u16(buffer + i);
			// Если код версии является GREASE
			if(::local::isGrease(ver))
				// Добавляем код GREASE в список поддерживаемых версий браузера
				awh_cast <awh::tls::fgp_t::extension_supported_versions_t *> (browser.extensions.back().get())->versions.push_back(awh::tls::version_t::GREASE);
			// Если код версии является одной из стандартных версий из RFC 8446
			else {
				/**
				 * Определяем код версии
				 */
				switch(ver){
					// Если версия является SSLv3
					case 0x0300:
						// Добавляем код версии SSLv3 в список поддерживаемых версий браузера
						awh_cast <awh::tls::fgp_t::extension_supported_versions_t *> (browser.extensions.back().get())->versions.push_back(awh::tls::version_t::SSL_V3);
					break;
					// Если версия является TLS 1.0
					case 0x0301:
						// Добавляем код версии TLS 1.0 в список поддерживаемых версий браузера
						awh_cast <awh::tls::fgp_t::extension_supported_versions_t *> (browser.extensions.back().get())->versions.push_back(awh::tls::version_t::TLS_1_0);
					break;
					// Если версия является TLS 1.1
					case 0x0302:
						// Добавляем код версии TLS 1.1 в список поддерживаемых версий браузера
						awh_cast <awh::tls::fgp_t::extension_supported_versions_t *> (browser.extensions.back().get())->versions.push_back(awh::tls::version_t::TLS_1_1);
					break;
					// Если версия является TLS 1.2
					case 0x0303:
						// Добавляем код версии TLS 1.2 в список поддерживаемых версий браузера
						awh_cast <awh::tls::fgp_t::extension_supported_versions_t *> (browser.extensions.back().get())->versions.push_back(awh::tls::version_t::TLS_1_2);
					break;
					// Если версия является TLS 1.3
					case 0x0304:
						// Добавляем код версии TLS 1.3 в список поддерживаемых версий браузера
						awh_cast <awh::tls::fgp_t::extension_supported_versions_t *> (browser.extensions.back().get())->versions.push_back(awh::tls::version_t::TLS_1_3);
					break;
					// Если версия является DTLS 1.0
					case 0xFEFF:
						// Добавляем код версии DTLS 1.0 в список поддерживаемых версий браузера
						awh_cast <awh::tls::fgp_t::extension_supported_versions_t *> (browser.extensions.back().get())->versions.push_back(awh::tls::version_t::DTLS_1_0);
					break;
					// Если версия является DTLS 1.2
					case 0xFEFD:
						// Добавляем код версии DTLS 1.2 в список поддерживаемых версий браузера
						awh_cast <awh::tls::fgp_t::extension_supported_versions_t *> (browser.extensions.back().get())->versions.push_back(awh::tls::version_t::DTLS_1_2);
					break;
					// Если версия является неизвестной или нераспознанной
					default:
						// Добавляем код неизвестной версии в список поддерживаемых версий браузера
						awh_cast <awh::tls::fgp_t::extension_supported_versions_t *> (browser.extensions.back().get())->versions.push_back(awh::tls::version_t::UNKNOWN);
				}
			}
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения pre_shared_key (RFC 8446 §4.2.11)
	 *
	 * @param buffer  бинарный буфер с данными расширения pre_shared_key
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parsePreSharedKey(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение pre_shared_key в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_pre_shared_key_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Получаем количество данных расширения pre_shared_key из первых 2 байт данных расширения
		const uint16_t count = ::local::u16(buffer);
		// Если количество данных расширения pre_shared_key больше размера данных в буфере, то данных недостаточно для парсинга
		if(count > (size - 2))
			// Выходим из функции
			return;
		// Если количество данных расширения pre_shared_key больше 0, то выполняем парсинг идентификаторов предварительно совместных ключей
		if(count > 0){
			// Устанавливаем начальное значение смещения
			size_t offset = 2;
			/**
			 * Перебираем идентификаторы предварительно совместных ключей в данных расширения
			 */
			while(((offset + 2) <= static_cast <size_t> (2 + count)) && ((offset + 2) <= size)){
				// Получаем размер идентификатора предварительно совместного ключа из буфера
				const uint16_t length = ::local::u16(buffer + offset);
				// Увеличиваем смещение на размер поля с размером идентификатора предварительно совместного ключа (2 байта)
				offset += 2;
				// Если смещение с учетом размера идентификатора предварительно совместного ключа и поля с размером идентификатора превышает размер данных в буфере, то данных недостаточно для парсинга
				if((offset + static_cast <size_t> (length + 4)) > size)
					// Выходим из цикла
					break;
				// Добавляем новый идентификатор предварительно совместного ключа в список идентификаторов расширения pre_shared_key
				awh_cast <awh::tls::fgp_t::extension_pre_shared_key_t *> (browser.extensions.back().get())->identities.push_back(awh::tls::fgp_t::extension_pre_shared_key_t::Identity{});
				// Забиваем буфер идентификатора предварительно совместного ключа нулями, чтобы гарантировать наличие данных в случае, если идентификатор предварительно совместного ключа пустой
				awh_cast <awh::tls::fgp_t::extension_pre_shared_key_t *> (browser.extensions.back().get())->identities.back().data.resize(length, 0);
				// Копируем данные идентификатора предварительно совместного ключа из буфера в блок данных расширения pre_shared_key
				::memcpy(&awh_cast <awh::tls::fgp_t::extension_pre_shared_key_t *> (browser.extensions.back().get())->identities.back().data[0], buffer + offset, length);
				// Устанавливаем значение обфусцированного времени жизни билета (Obfuscated Ticket Age) для текущего идентификатора предварительно совместного ключа из буфера данных расширения pre_shared_key
				awh_cast <awh::tls::fgp_t::extension_pre_shared_key_t *> (browser.extensions.back().get())->identities.back().ticketAge = (
					(static_cast <uint32_t> (buffer[offset + length])     << 24) |
					(static_cast <uint32_t> (buffer[offset + length + 1]) << 16) |
					(static_cast <uint32_t> (buffer[offset + length + 2]) <<  8) |
					static_cast <uint32_t> (buffer[offset + length + 3])
				);
				// Продвигаем смещение за данные идентификатора (length байт) и поле ticketAge (4 байта)
				offset += (static_cast <size_t> (length) + 4);
			}
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения delegated_credential (RFC 9345 §4.2)
	 *
	 * @param buffer  бинарный буфер с данными расширения delegated_credential
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseDelegatedCredential(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение delegated_credential в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_delegated_credential_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Получаем количество поддерживаемых алгоритмов подписи из первых 2 байт данных расширения
		const uint16_t count = ::local::u16(buffer);
		// Если количество поддерживаемых алгоритмов подписи больше размера данных в буфере, то данных недостаточно для парсинга
		if(count > (size - 2))
			// Выходим из функции
			return;
		// Если количество поддерживаемых алгоритмов подписи больше 0
		if(count > 0){
			// Перебираем поддерживаемые алгоритмы подписи в данных расширения
			for(size_t i = 2; ((i + 1) < static_cast <size_t> (2 + count)) && ((i + 1) < size); i += 2){
				// Получаем код алгоритма подписи из буфера
				const uint16_t alg = ::local::u16(buffer + i);
				// Если код алгоритма подписи является GREASE
				if(::local::isGrease(alg))
					// Добавляем код GREASE в список поддерживаемых алгоритмов подписи браузера
					awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::GREASE);
				// Если код алгоритма подписи является одним из стандартных кодов из RFC 8446
				else {
					/**
					 * Определяем код алгоритма подписи
					 */
					switch(alg){
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-1 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0201:
							// Добавляем код алгоритма RSA-PKCS1-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA1);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-256 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0401:
							// Добавляем код алгоритма RSA-PKCS1-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA256);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-384 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0501:
							// Добавляем код алгоритма RSA-PKCS1-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA384);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-512 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0601:
							// Добавляем код алгоритма RSA-PKCS1-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA512);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-1.
						case 0x0203:
							// Добавляем код алгоритма ECDSA-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SHA1);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-256 и кривой secp256r1.
						case 0x0403:
							// Добавляем код алгоритма ECDSA-SECP256R1-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SECP256R1_SHA256);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-384 и кривой secp384r1.
						case 0x0503:
							// Добавляем код алгоритма ECDSA-SECP384R1-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SECP384R1_SHA384);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-512 и кривой secp521r1.
						case 0x0603:
							// Добавляем код алгоритма ECDSA-SECP521R1-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SECP521R1_SHA512);
						break;
						// Если алгоритм подписи использует RSA-PSS в сочетании с хеш-функцией SHA-256.
						case 0x0804:
							// Добавляем код алгоритма RSA-PSS-RSAE-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_RSAE_SHA256);
						break;
						// Если алгоритм подписи использует RSA-PSS в сочетании с хеш-функцией SHA-384.
						case 0x0805:
							// Добавляем код алгоритма RSA-PSS-RSAE-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_RSAE_SHA384);
						break;
						// Если алгоритм подписи использует RSA-PSS в сочетании с хеш-функцией SHA-512.
						case 0x0806:
							// Добавляем код алгоритма RSA-PSS-RSAE-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_RSAE_SHA512);
						break;
						// Если алгоритм подписи использует ED25519
						case 0x0807:
							// Добавляем код алгоритма ED25519 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ED25519);
						break;
						// Если алгоритм подписи использует ED448
						case 0x0808:
							// Добавляем код алгоритма ED448 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ED448);
						break;
						// Если алгоритм подписи использует RSA-PSS-PSS-SHA256 (выделенный PSS-сертификат, RFC 8446)
						case 0x0809:
							// Добавляем код алгоритма RSA-PSS-PSS-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_PSS_SHA256);
						break;
						// Если алгоритм подписи использует RSA-PSS-PSS-SHA384 (выделенный PSS-сертификат, RFC 8446)
						case 0x080A:
							// Добавляем код алгоритма RSA-PSS-PSS-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_PSS_SHA384);
						break;
						// Если алгоритм подписи использует RSA-PSS-PSS-SHA512 (выделенный PSS-сертификат, RFC 8446)
						case 0x080B:
							// Добавляем код алгоритма RSA-PSS-PSS-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_PSS_SHA512);
						break;
						// Если алгоритм подписи использует DSA в сочетании с хеш-функцией SHA-1.
						case 0x0202:
							// Добавляем код алгоритма DSA-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::DSA_SHA1);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией MD5 и схемой заполнения PKCS#1 версии 1.5 (не рекомендуется к использованию из-за слабой криптографической стойкости).
						case 0xFF01:
							// Добавляем код алгоритма RSA-PKCS1-MD5-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_MD5_SHA1);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-256 и схемой заполнения PKCS#1 версии 1.5, но является устаревшим и не рекомендуется к использованию (не входит в RFC 8446, но может быть поддержан некоторыми реализациями TLS для обратной совместимости).
						case 0x0420:
							// Добавляем код алгоритма RSA-PKCS1-SHA256-LEGACY в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA256_LEGACY);
						break;
						// Если алгоритм подписи не распознан, добавляем код UNKNOWN в список поддерживаемых алгоритмов подписи браузера
						default: awh_cast <awh::tls::fgp_t::extension_delegated_credential_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::UNKNOWN);
					}
				}
			}
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения compress_certificate (RFC 8879)
	 *
	 * @param buffer  бинарный буфер с данными расширения compress_certificate
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseCompressCertificate(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение compress_certificate в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_compress_certificate_t> ());
		// Если размер данных в буфере меньше 1 байта, то данных недостаточно для парсинга
		if(size < 1)
			// Выходим из функции
			return;
		// Получаем количество байт в списке алгоритмов сжатия из первого байта данных расширения
		const uint8_t count = buffer[0];
		// Если количество байт в списке алгоритмов сжатия больше размера данных в буфере, то данных недостаточно для парсинга
		if(count > (size - 1))
			// Выходим из функции
			return;
		// Если количество байт в списке алгоритмов сжатия больше 0
		if(count > 0){
			// Перебираем алгоритмы сжатия в данных расширения
			for(size_t i = 0; (((i + 2) <= static_cast <size_t> (count)) && ((1 + i + 2) <= size)); i += 2){
				// Извлекаем идентификатор алгоритма сжатия из текущей позиции в буфере данных расширения
				const uint16_t id = ::local::u16(buffer + (i + 1));
				/**
				 * Определяем название алгоритма сжатия на основе его идентификатора и выводим его в лог
				 * Известные идентификаторы алгоритмов сжатия:
				 * 1 - zlib
				 * 2 - brotli
				 * 3 - zstd
				 * Другие значения считаются неизвестными алгоритмами сжатия
				 */
				switch(id){
					// Если идентификатор алгоритма сжатия соответствует zlib
					case 0x01:
						// Устанавливаем флаг алгоритма сжатия zlib для расширения compress_certificate в списке расширений браузера
						awh_cast <awh::tls::fgp_t::extension_compress_certificate_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::compressor_t::ZLIB);
					break;
					// Если идентификатор алгоритма сжатия соответствует brotli
					case 0x02:
						// Устанавливаем флаг алгоритма сжатия brotli для расширения compress_certificate в списке расширений браузера
						awh_cast <awh::tls::fgp_t::extension_compress_certificate_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::compressor_t::BROTLI);
					break;
					// Если идентификатор алгоритма сжатия соответствует zstd
					case 0x03:
						// Устанавливаем флаг алгоритма сжатия zstd для расширения compress_certificate в списке расширений браузера
						awh_cast <awh::tls::fgp_t::extension_compress_certificate_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::compressor_t::ZSTD);
					break;
					// Если идентификатор алгоритма сжатия не соответствует известным алгоритмам сжатия
					default:
						// Устанавливаем флаг неизвестного алгоритма сжатия для расширения compress_certificate в списке расширений браузера
						awh_cast <awh::tls::fgp_t::extension_compress_certificate_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::compressor_t::UNKNOWN);
				}
			}
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения application_layer_protocol_negotiation / ALPN (RFC 7301)
	 *
	 * @param buffer  бинарный буфер с данными расширения ALPN
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseALPN(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение ALPN в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_alpn_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Получаем количество поддерживаемых ALPN-протоколов из первых 2 байт данных расширения
		const uint16_t count = ::local::u16(buffer);
		// Если количество поддерживаемых ALPN-протоколов больше размера данных в буфере, то данных недостаточно для парсинга
		if(count > (size - 2))
			// Выходим из функции
			return;
		// Если количество поддерживаемых ALPN-протоколов больше 0
		if(count > 0){
			// Текущая позиция в буфере данных расширения
			size_t pos = 2;
			/**
			 * Перебираем поддерживаемые ALPN-протоколы в данных расширения
			 */
			while((pos < static_cast <size_t> (2 + count)) && (pos < size)){
				// Получаем длину названия ALPN-протокола из текущей позиции в буфере
				const uint8_t length = buffer[pos++];
				// Если данных недостаточно для чтения названия протокола, прекращаем парсинг
				if((pos + static_cast <size_t> (length)) > size)
					// Прерываем перебор протоколов
					break;
				// Добавляем название ALPN-протокола в список поддерживаемых ALPN-протоколов браузера
				awh_cast <awh::tls::fgp_t::extension_alpn_t *> (browser.extensions.back().get())->protocols.push_back(string(reinterpret_cast <const char *> (buffer + pos), length));
				// Смещаем позицию в буфере на длину названия ALPN-протокола
				pos += static_cast <size_t> (length);
			}
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения heartbeat (RFC 6520)
	 *
	 * @param buffer  бинарный буфер с данными расширения heartbeat
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseHeartbeat(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение heartbeat в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_heartbeat_t> ());
		// Если размер данных в буфере меньше 1 байта, то данных недостаточно для парсинга
		if(size < 1)
			// Выходим из функции
			return;
		/**
		 * Определяем режим работы расширения heartbeat на основе значения в первом байте данных расширения
		 */
		switch(buffer[0]){
			// Если значение соответствует режиму peer_allowed_to_send
			case 0x01:
				// Устанавливаем флаг режима peer_allowed_to_send для расширения heartbeat в списке расширений браузера
				awh_cast <awh::tls::fgp_t::extension_heartbeat_t *> (browser.extensions.back().get())->mode = awh::tls::heartbeat_t::PEER_ALLOWED_TO_SEND;
			break;
			// Если значение соответствует режиму peer_not_allowed_to_send
			case 0x02:
				// Устанавливаем флаг режима peer_not_allowed_to_send для расширения heartbeat в списке расширений браузера
				awh_cast <awh::tls::fgp_t::extension_heartbeat_t *> (browser.extensions.back().get())->mode = awh::tls::heartbeat_t::PEER_NOT_ALLOWED_TO_SEND;
			break;
			// Если значение не соответствует известным режимам, то устанавливаем режим UNKNOWN для расширения heartbeat в списке расширений браузера
			default:
				// Устанавливаем режим UNKNOWN для расширения heartbeat в списке расширений браузера
				awh_cast <awh::tls::fgp_t::extension_heartbeat_t *> (browser.extensions.back().get())->mode = awh::tls::heartbeat_t::UNKNOWN;
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения use_srtp (RFC 5764)
	 *
	 * @param buffer  бинарный буфер с данными расширения use_srtp
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseUseSRTP(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение use_srtp в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_use_srtp_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Получаем количество поддерживаемых профилей SRTP из первых 2 байт данных расширения
		const uint16_t count = ::local::u16(buffer);
		// Если количество поддерживаемых профилей SRTP больше размера данных в буфере, то данных недостаточно для парсинга
		if(count > (size - 2))
			// Выходим из функции
			return;
		// Если количество поддерживаемых профилей SRTP больше 0
		if(count > 0){
			// Перебираем поддерживаемые профили SRTP в данных расширения
			for(size_t i = 2; (((i + 1) < static_cast <size_t> (2 + count)) && ((i + 1) < size)); i += 2){
				// Извлекаем код профиля SRTP из буфера
				const uint16_t profile = ::local::u16(buffer + i);
				// Если код профиля SRTP является GREASE
				if(::local::isGrease(profile))
					// Добавляем код GREASE в список поддерживаемых профилей SRTP браузера
					awh_cast <awh::tls::fgp_t::extension_use_srtp_t *> (browser.extensions.back().get())->profiles.push_back(awh::tls::srtp_t::GREASE);
				// Если код алгоритма подписи является одним из стандартных кодов из RFC 8446
				else {
					/**
					 * Определяем код профиля SRTP
					 */
					switch(profile){
						// Если код профиля SRTP соответствует SRTP_AES128_CM_HMAC_SHA1_80 из RFC 5764
						case 0x0001:
							// Добавляем код профиля SRTP SRTP_AES128_CM_HMAC_SHA1_80 в список поддерживаемых профилей SRTP браузера
							awh_cast <awh::tls::fgp_t::extension_use_srtp_t *> (browser.extensions.back().get())->profiles.push_back(awh::tls::srtp_t::AES128_CM_HMAC_SHA1_80);
						break;
						// Если код профиля SRTP соответствует SRTP_AES128_CM_HMAC_SHA1_32 из RFC 5764
						case 0x0002:
							// Добавляем код профиля SRTP SRTP_AES128_CM_HMAC_SHA1_32 в список поддерживаемых профилей SRTP браузера
							awh_cast <awh::tls::fgp_t::extension_use_srtp_t *> (browser.extensions.back().get())->profiles.push_back(awh::tls::srtp_t::AES128_CM_HMAC_SHA1_32);
						break;
						// Если код профиля SRTP соответствует SRTP_AES128_F8_HMAC_SHA1_80 из RFC 5764
						case 0x0005:
							// Добавляем код профиля SRTP SRTP_AES128_F8_HMAC_SHA1_80 в список поддерживаемых профилей SRTP браузера
							awh_cast <awh::tls::fgp_t::extension_use_srtp_t *> (browser.extensions.back().get())->profiles.push_back(awh::tls::srtp_t::AES128_F8_HMAC_SHA1_80);
						break;
						// Если код профиля SRTP соответствует SRTP_NULL_HMAC_SHA1_80 из RFC 5764
						case 0x0007:
							// Добавляем код профиля SRTP SRTP_NULL_HMAC_SHA1_80 в список поддерживаемых профилей SRTP браузера
							awh_cast <awh::tls::fgp_t::extension_use_srtp_t *> (browser.extensions.back().get())->profiles.push_back(awh::tls::srtp_t::NULL_HMAC_SHA1_80);
						break;
						// Если код профиля SRTP соответствует SRTP_NULL_HMAC_SHA1_32 из RFC 5764
						case 0x0008:
							// Добавляем код профиля SRTP SRTP_NULL_HMAC_SHA1_32 в список поддерживаемых профилей SRTP браузера
							awh_cast <awh::tls::fgp_t::extension_use_srtp_t *> (browser.extensions.back().get())->profiles.push_back(awh::tls::srtp_t::NULL_HMAC_SHA1_32);
						break;
						// Если код профиля SRTP соответствует SRTP_AEAD_AES_128_GCM из RFC 5764
						case 0x0009:
							// Добавляем код профиля SRTP SRTP_AEAD_AES_128_GCM в список поддерживаемых профилей SRTP браузера
							awh_cast <awh::tls::fgp_t::extension_use_srtp_t *> (browser.extensions.back().get())->profiles.push_back(awh::tls::srtp_t::AEAD_AES_128_GCM);
						break;
						// Если код профиля SRTP соответствует SRTP_AEAD_AES_256_GCM из RFC 5764
						case 0x000A:
							// Добавляем код профиля SRTP SRTP_AEAD_AES_256_GCM в список поддерживаемых профилей SRTP браузера
							awh_cast <awh::tls::fgp_t::extension_use_srtp_t *> (browser.extensions.back().get())->profiles.push_back(awh::tls::srtp_t::AEAD_AES_256_GCM);
						break;
						// Если код профиля SRTP не соответствует ни одному из известных кодов профилей SRTP
						default:
							// Добавляем код профиля SRTP UNKNOWN в список поддерживаемых профилей SRTP браузера
							awh_cast <awh::tls::fgp_t::extension_use_srtp_t *> (browser.extensions.back().get())->profiles.push_back(awh::tls::srtp_t::UNKNOWN);
					}
				}
			}
			// Получаем смещение для чтения Master Key Identifier (MKI) из данных расширения
			const size_t offset = static_cast <size_t> (2 + count);
			// Если смещение для чтения MKI меньше размера данных расширения
			if(offset < size)
				// Устанавливаем длину MKI в объект расширения use_srtp браузера
				awh_cast <awh::tls::fgp_t::extension_use_srtp_t *> (browser.extensions.back().get())->mkiLength = buffer[offset];
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения signature_algorithms TLS (RFC 8446 §4.2.3)
	 *
	 * @param buffer  бинарный буфер с данными расширения signature_algorithms
	 * @param size    размер данных расширения signature_algorithms
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseSignatureAlgorithms(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение signature_algorithms в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_signature_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Получаем количество поддерживаемых алгоритмов подписи из первых 2 байт данных расширения
		const uint16_t count = ::local::u16(buffer);
		// Если количество поддерживаемых алгоритмов подписи больше размера данных в буфере, то данных недостаточно для парсинга
		if(count > (size - 2))
			// Выходим из функции
			return;
		// Если количество поддерживаемых алгоритмов подписи больше 0
		if(count > 0){
			// Перебираем поддерживаемые алгоритмы подписи в данных расширения
			for(size_t i = 2; (((i + 1) < static_cast <size_t> (2 + count)) && ((i + 1) < size)); i += 2){
				// Получаем код алгоритма подписи из буфера
				const uint16_t alg = ::local::u16(buffer + i);
				// Если код алгоритма подписи является GREASE
				if(::local::isGrease(alg))
					// Добавляем код GREASE в список поддерживаемых алгоритмов подписи браузера
					awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::GREASE);
				// Если код алгоритма подписи является одним из стандартных кодов из RFC 8446
				else {
					/**
					 * Определяем код алгоритма подписи
					 */
					switch(alg){
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-1 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0201:
							// Добавляем код алгоритма RSA-PKCS1-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA1);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-256 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0401:
							// Добавляем код алгоритма RSA-PKCS1-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA256);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-384 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0501:
							// Добавляем код алгоритма RSA-PKCS1-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA384);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-512 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0601:
							// Добавляем код алгоритма RSA-PKCS1-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA512);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-1.
						case 0x0203:
							// Добавляем код алгоритма ECDSA-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SHA1);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-256 и кривой secp256r1.
						case 0x0403:
							// Добавляем код алгоритма ECDSA-SECP256R1-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SECP256R1_SHA256);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-384 и кривой secp384r1.
						case 0x0503:
							// Добавляем код алгоритма ECDSA-SECP384R1-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SECP384R1_SHA384);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-512 и кривой secp521r1.
						case 0x0603:
							// Добавляем код алгоритма ECDSA-SECP521R1-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SECP521R1_SHA512);
						break;
						// Если алгоритм подписи использует RSA-PSS в сочетании с хеш-функцией SHA-256.
						case 0x0804:
							// Добавляем код алгоритма RSA-PSS-RSAE-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_RSAE_SHA256);
						break;
						// Если алгоритм подписи использует RSA-PSS в сочетании с хеш-функцией SHA-384.
						case 0x0805:
							// Добавляем код алгоритма RSA-PSS-RSAE-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_RSAE_SHA384);
						break;
						// Если алгоритм подписи использует RSA-PSS в сочетании с хеш-функцией SHA-512.
						case 0x0806:
							// Добавляем код алгоритма RSA-PSS-RSAE-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_RSAE_SHA512);
						break;
						// Если алгоритм подписи использует ED25519
						case 0x0807:
							// Добавляем код алгоритма ED25519 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ED25519);
						break;
						// Если алгоритм подписи использует ED448
						case 0x0808:
							// Добавляем код алгоритма ED448 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ED448);
						break;
						// Если алгоритм подписи использует RSA-PSS-PSS-SHA256 (выделенный PSS-сертификат, RFC 8446)
						case 0x0809:
							// Добавляем код алгоритма RSA-PSS-PSS-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_PSS_SHA256);
						break;
						// Если алгоритм подписи использует RSA-PSS-PSS-SHA384 (выделенный PSS-сертификат, RFC 8446)
						case 0x080A:
							// Добавляем код алгоритма RSA-PSS-PSS-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_PSS_SHA384);
						break;
						// Если алгоритм подписи использует RSA-PSS-PSS-SHA512 (выделенный PSS-сертификат, RFC 8446)
						case 0x080B:
							// Добавляем код алгоритма RSA-PSS-PSS-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_PSS_SHA512);
						break;
						// Если алгоритм подписи использует DSA в сочетании с хеш-функцией SHA-1.
						case 0x0202:
							// Добавляем код алгоритма DSA-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::DSA_SHA1);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией MD5 и схемой заполнения PKCS#1 версии 1.5 (не рекомендуется к использованию из-за слабой криптографической стойкости).
						case 0xFF01:
							// Добавляем код алгоритма RSA-PKCS1-MD5-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_MD5_SHA1);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-256 и схемой заполнения PKCS#1 версии 1.5, но является устаревшим и не рекомендуется к использованию (не входит в RFC 8446, но может быть поддержан некоторыми реализациями TLS для обратной совместимости).
						case 0x0420:
							// Добавляем код алгоритма RSA-PKCS1-SHA256-LEGACY в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA256_LEGACY);
						break;
						// Если алгоритм подписи не распознан, добавляем код UNKNOWN в список поддерживаемых алгоритмов подписи браузера
						default: awh_cast <awh::tls::fgp_t::extension_signature_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::UNKNOWN);
					}
				}
			}
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения signature_algorithms_cert (RFC 8446 §4.2.3)
	 *
	 * @param buffer  бинарный буфер с данными расширения signature_algorithms_cert
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseSignatureAlgorithmsCert(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение signature_algorithms_cert в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_signature_algorithms_cert_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Получаем количество поддерживаемых алгоритмов подписи из первых 2 байт данных расширения
		const uint16_t count = ::local::u16(buffer);
		// Если количество поддерживаемых алгоритмов подписи больше размера данных в буфере, то данных недостаточно для парсинга
		if(count > (size - 2))
			// Выходим из функции
			return;
		// Если количество поддерживаемых алгоритмов подписи больше 0
		if(count > 0){
			// Перебираем поддерживаемые алгоритмы подписи в данных расширения
			for(size_t i = 2; ((i + 1) < static_cast <size_t> (2 + count)) && ((i + 1) < size); i += 2){
				// Получаем код алгоритма подписи из буфера
				const uint16_t alg = ::local::u16(buffer + i);
				// Если код алгоритма подписи является GREASE
				if(::local::isGrease(alg))
					// Добавляем код GREASE в список поддерживаемых алгоритмов подписи браузера
					awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::GREASE);
				// Если код алгоритма подписи является одним из стандартных кодов из RFC 8446
				else {
					/**
					 * Определяем код алгоритма подписи
					 */
					switch(alg){
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-1 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0201:
							// Добавляем код алгоритма RSA-PKCS1-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA1);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-256 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0401:
							// Добавляем код алгоритма RSA-PKCS1-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA256);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-384 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0501:
							// Добавляем код алгоритма RSA-PKCS1-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA384);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-512 и схемой заполнения PKCS#1 версии 1.5.
						case 0x0601:
							// Добавляем код алгоритма RSA-PKCS1-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA512);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-1.
						case 0x0203:
							// Добавляем код алгоритма ECDSA-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SHA1);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-256 и кривой secp256r1.
						case 0x0403:
							// Добавляем код алгоритма ECDSA-SECP256R1-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SECP256R1_SHA256);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-384 и кривой secp384r1.
						case 0x0503:
							// Добавляем код алгоритма ECDSA-SECP384R1-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SECP384R1_SHA384);
						break;
						// Если алгоритм подписи использует ECDSA в сочетании с хеш-функцией SHA-512 и кривой secp521r1.
						case 0x0603:
							// Добавляем код алгоритма ECDSA-SECP521R1-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ECDSA_SECP521R1_SHA512);
						break;
						// Если алгоритм подписи использует RSA-PSS в сочетании с хеш-функцией SHA-256.
						case 0x0804:
							// Добавляем код алгоритма RSA-PSS-RSAE-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_RSAE_SHA256);
						break;
						// Если алгоритм подписи использует RSA-PSS в сочетании с хеш-функцией SHA-384.
						case 0x0805:
							// Добавляем код алгоритма RSA-PSS-RSAE-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_RSAE_SHA384);
						break;
						// Если алгоритм подписи использует RSA-PSS в сочетании с хеш-функцией SHA-512.
						case 0x0806:
							// Добавляем код алгоритма RSA-PSS-RSAE-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_RSAE_SHA512);
						break;
						// Если алгоритм подписи использует ED25519
						case 0x0807:
							// Добавляем код алгоритма ED25519 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ED25519);
						break;
						// Если алгоритм подписи использует ED448
						case 0x0808:
							// Добавляем код алгоритма ED448 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::ED448);
						break;
						// Если алгоритм подписи использует RSA-PSS-PSS-SHA256 (выделенный PSS-сертификат, RFC 8446)
						case 0x0809:
							// Добавляем код алгоритма RSA-PSS-PSS-SHA256 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_PSS_SHA256);
						break;
						// Если алгоритм подписи использует RSA-PSS-PSS-SHA384 (выделенный PSS-сертификат, RFC 8446)
						case 0x080A:
							// Добавляем код алгоритма RSA-PSS-PSS-SHA384 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_PSS_SHA384);
						break;
						// Если алгоритм подписи использует RSA-PSS-PSS-SHA512 (выделенный PSS-сертификат, RFC 8446)
						case 0x080B:
							// Добавляем код алгоритма RSA-PSS-PSS-SHA512 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PSS_PSS_SHA512);
						break;
						// Если алгоритм подписи использует DSA в сочетании с хеш-функцией SHA-1.
						case 0x0202:
							// Добавляем код алгоритма DSA-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::DSA_SHA1);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией MD5 и схемой заполнения PKCS#1 версии 1.5 (не рекомендуется к использованию из-за слабой криптографической стойкости).
						case 0xFF01:
							// Добавляем код алгоритма RSA-PKCS1-MD5-SHA1 в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_MD5_SHA1);
						break;
						// Если алгоритм подписи использует RSA в сочетании с хеш-функцией SHA-256 и схемой заполнения PKCS#1 версии 1.5, но является устаревшим и не рекомендуется к использованию (не входит в RFC 8446, но может быть поддержан некоторыми реализациями TLS для обратной совместимости).
						case 0x0420:
							// Добавляем код алгоритма RSA-PKCS1-SHA256-LEGACY в список поддерживаемых алгоритмов подписи браузера
							awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::RSA_PKCS1_SHA256_LEGACY);
						break;
						// Если алгоритм подписи не распознан, добавляем код UNKNOWN в список поддерживаемых алгоритмов подписи браузера
						default: awh_cast <awh::tls::fgp_t::extension_signature_algorithms_cert_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::signature_t::UNKNOWN);
					}
				}
			}
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения ec_point_formats из данных ClientHello TLS (RFC 8422 §5.1)
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseECPointFormats(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение ec_point_formats в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_ec_point_t> ());
		// Если размер данных в буфере меньше 1 байта, то данных недостаточно для парсинга
		if(size < 1)
			// Выходим из функции
			return;
		// Получаем количество поддерживаемых форматов точек эллиптической кривой из первого байта данных расширения
		const uint8_t count = buffer[0];
		// Если количество поддерживаемых форматов точек эллиптической кривой больше размера данных в буфере, то данных недостаточно для парсинга
		if(count > (size - 1))
			// Выходим из функции
			return;
		// Если количество поддерживаемых форматов точек эллиптической кривой больше 0
		if(count > 0){
			// Перебираем поддерживаемые форматы точек эллиптической кривой в данных расширения
			for(uint8_t i = 0; ((i < count) && (static_cast <size_t> (i + 1) < size)); ++i){
				// Извлекаем код формата точек из буфера
				const uint8_t format = buffer[1 + i];
				/**
				 * Определяем код формата точек
				 */
				switch(format){
					// Если формат точек соответствует uncompressed
					case 0x00:
						// Добавляем код формата точек uncompressed в список поддерживаемых форматов точек эллиптической кривой браузера
						awh_cast <awh::tls::fgp_t::extension_ec_point_t *> (browser.extensions.back().get())->formats.push_back(awh::tls::ec_point_format_t::UNCOMPRESSED);
					break;
					// Если формат точек соответствует ANSI X9.62
					case 0x01:
						// Добавляем код формата точек ANSI X9.62 в список поддерживаемых форматов точек эллиптической кривой браузера
						awh_cast <awh::tls::fgp_t::extension_ec_point_t *> (browser.extensions.back().get())->formats.push_back(awh::tls::ec_point_format_t::ANSIX962);
					break;
					// Если формат точек соответствует ANSI X9.62 с использованием битовой маски
					case 0x02:
						// Добавляем код формата точек ANSI X9.62 с использованием битовой маски в список поддерживаемых форматов точек эллиптической кривой браузера
						awh_cast <awh::tls::fgp_t::extension_ec_point_t *> (browser.extensions.back().get())->formats.push_back(awh::tls::ec_point_format_t::ANSIX962_2);
					break;
					// ec_point_format — 1-байтовое поле; GREASE — 16-битная концепция, здесь неприменима
					default:
						// Добавляем код UNKNOWN в список поддерживаемых форматов точек эллиптической кривой браузера
						awh_cast <awh::tls::fgp_t::extension_ec_point_t *> (browser.extensions.back().get())->formats.push_back(awh::tls::ec_point_format_t::UNKNOWN);
				}
			}
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения supported_groups из данных ClientHello TLS (RFC 8422 §5.1, RFC 7919 §2.3)
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseSupportedGroups(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение supported_groups в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_supported_groups_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Получаем количество поддерживаемых групп эллиптических кривых из первых 2 байт данных расширения
		const uint16_t count = ::local::u16(buffer);
		// Если количество поддерживаемых групп эллиптических кривых больше размера данных в буфере, то данных недостаточно для парсинга
		if(count > (size - 2))
			// Выходим из функции
			return;
		// Если количество поддерживаемых групп эллиптических кривых больше 0
		if(count > 0){
			/**
			 * Перебираем поддерживаемые группы эллиптических кривых в данных расширения
			 */
			for(size_t i = 2; ((i + 1) < static_cast <size_t> (2 + count)) && ((i + 1) < size); i += 2){
				// Извлекаем код группы эллиптической кривой из буфера
				const uint16_t gid = ::local::u16(buffer + i);
				// Если код группы является GREASE
				if(::local::isGrease(gid))
					// Добавляем код GREASE в список поддерживаемых групп эллиптических кривых браузера
					awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::GREASE);
				// Если код шифра является одним из стандартных кодов из RFC 8446
				else {
					/**
					 * Определяем код шифра
					 */
					switch(gid){
						// Если элиптическая кривая соответствует P-256 (secp256r1)
						case 0x0017:
							// Добавляем код группы эллиптической кривой P-256 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::P_256);
						break;
						// Если элиптическая кривая соответствует P-384 (secp384r1)
						case 0x0018:
							// Добавляем код группы эллиптической кривой P-384 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::P_384);
						break;
						// Если элиптическая кривая соответствует P-521 (secp521r1)
						case 0x0019:
							// Добавляем код группы эллиптической кривой P-521 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::P_521);
						break;
						// Если элиптическая кривая соответствует X25519
						case 0x001D:
							// Добавляем код группы эллиптической кривой X25519 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::X25519);
						break;
						// Если элиптическая кривая соответствует X448
						case 0x001E:
							// Добавляем код группы эллиптической кривой X448 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::X448);
						break;
						// Если элиптическая кривая соответствует secp256k1
						case 0x001C:
							// Добавляем код группы эллиптической кривой secp256k1 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::SECP256K1);
						break;
						// Если элиптическая кривая соответствует FFDHE 2048
						case 0x0100:
							// Добавляем код группы FFDHE 2048 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::FFDHE2048);
						break;
						// Если элиптическая кривая соответствует FFDHE 3072
						case 0x0101:
							// Добавляем код группы FFDHE 3072 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::FFDHE3072);
						break;
						// Если элиптическая кривая соответствует FFDHE 4096
						case 0x0102:
							// Добавляем код группы FFDHE 4096 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::FFDHE4096);
						break;
						// Если элиптическая кривая соответствует FFDHE 6144
						case 0x0103:
							// Добавляем код группы FFDHE 6144 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::FFDHE6144);
						break;
						// Если элиптическая кривая соответствует FFDHE 8192
						case 0x0104:
							// Добавляем код группы FFDHE 8192 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::FFDHE8192);
						break;
						// Если элиптическая кривая соответствует MLKEM 1024
						case 0x0202:
							// Добавляем код группы MLKEM 1024 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::MLKEM1024);
						break;
						// Если элиптическая кривая соответствует X25519Kyber768Draft00
						case 0x6399:
							// Добавляем код группы X25519Kyber768Draft00 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::X25519_KYBER768_DRAFT00);
						break;
						// Если элиптическая кривая соответствует X25519MLKEM768
						case 0x11EC:
							// Добавляем код группы X25519MLKEM768 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::X25519_MLKEM768);
						break;
						// Если элиптическая кривая не соответствует ни одной из известных, добавляем код UNKNOWN в список поддерживаемых групп эллиптических кривых браузера
						default: awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::UNKNOWN);
					}
				}
			}
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения status_request из ClientHello (RFC 6066 §8)
	 *
	 * @param buffer  бинарный буфер с данными handshake-сообщения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseStatusRequest(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение status_request в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_status_request_t> ());
		// Если данных в буфере достаточно для парсинга
		if(size >= 1){
			// Устанавливаем тип статуса сертификата на основе значения в первом байте данных расширения
			awh_cast <awh::tls::fgp_t::extension_status_request_t *> (browser.extensions.back().get())->certificateStatusType = ((buffer[0] == 0x01) ? "OCSP" : "UNKNOWN");
			// Если данных в буфере достаточно для чтения responder_id_list_length
			if(size >= 3){
				// Читаем байтовую длину списка идентификаторов ответчиков OCSP
				const uint16_t length = ::local::u16(buffer + 1);
				// Устанавливаем длину списка идентификаторов ответчиков OCSP
				awh_cast <awh::tls::fgp_t::extension_status_request_t *> (browser.extensions.back().get())->responderIdListLength = length;
				/**
				 * RFC 6066 §8: request_extensions_length следует за списком responder_id_list.
				 * Смещение: status_type(1) + responder_id_list_length(2) + responder_id_list(length)
				 */
				const size_t offset = static_cast <size_t> (length + 3);
				// Если данных в буфере достаточно для чтения request_extensions_length
				if(size >= (offset + 2))
					// Устанавливаем длину списка расширений запроса OCSP
					awh_cast <awh::tls::fgp_t::extension_status_request_t *> (browser.extensions.back().get())->requestExtensionsLength = ::local::u16(buffer + offset);
			}
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения max_fragment_length из ClientHello (RFC 6066 §4)
	 *
	 * @param buffer  бинарный буфер с данными handshake-сообщения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseMaxFragmentLength(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение max_fragment_length в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_max_fragment_length_t> ());
		// Если размер данных в буфере меньше 1 байта, то данных недостаточно для парсинга
		if(size < 1)
			// Выходим из функции
			return;
		/**
		 * Определяем максимальную длину фрагмента TLS на основе значения в первом байте данных расширения
		 */
		switch(buffer[0]){
			// Если значение соответствует размеру 512 байт
			case 0x01:
				// Устанавливаем длину фрагмента TLS на 512 байт
				awh_cast <awh::tls::fgp_t::extension_max_fragment_length_t *> (browser.extensions.back().get())->length = 512;
			break;
			// Если значение соответствует размеру 1024 байта
			case 0x02:
				// Устанавливаем длину фрагмента TLS на 1024 байта
				awh_cast <awh::tls::fgp_t::extension_max_fragment_length_t *> (browser.extensions.back().get())->length = 1024;
			break;
			// Если значение соответствует размеру 2048 байт
			case 0x03:
				// Устанавливаем длину фрагмента TLS на 2048 байт
				awh_cast <awh::tls::fgp_t::extension_max_fragment_length_t *> (browser.extensions.back().get())->length = 2048;
			break;
			// Если значение соответствует размеру 4096 байт
			case 0x04:
				// Устанавливаем длину фрагмента TLS на 4096 байт
				awh_cast <awh::tls::fgp_t::extension_max_fragment_length_t *> (browser.extensions.back().get())->length = 4096;
			break;
			// Если значение не соответствует ни одному из стандартных размеров, устанавливаем размер фрагмента в 0 (неизвестный размер)
			default: awh_cast <awh::tls::fgp_t::extension_max_fragment_length_t *> (browser.extensions.back().get())->length = 0;
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения Server Name Indication (SNI) из ClientHello (RFC 6066 §3)
	 *
	 * @param buffer  бинарный буфер с данными handshake-сообщения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseServerName(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение SNI в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_server_name_t> ());
		// Минимум: list_length(2) + name_type(1) + name_length(2) + name(1) = 6 байт
		if(size < 6)
			// Выходим из функции
			return;
		// Читаем байтовую длину всего списка (не количество имён!)
		const uint16_t length = ::local::u16(buffer);
		// Проверяем что список помещается в буфер
		if(size < static_cast <size_t> (2 + length))
			// Выходим из функции
			return;
		// Инициализируем смещение: пропускаем list_length (2 байта)
		size_t offset = 2;
		// Конец списка
		const size_t end = (2 + length);
		/**
		 * Итерируем по байтам списка (на практике всегда один элемент)
		 */
		while((offset + 3) <= end){
			// Читаем тип имени (1 байт): 0x00 = host_name
			const uint8_t type = buffer[offset++];
			// Читаем длину имени (2 байта)
			const uint16_t length = ::local::u16(buffer + offset);
			// Сдвигаем смещение на 2 байта (длина имени)
			offset += 2;
			// Проверяем что имя помещается в буфер
			if((offset + length) > end)
				// Если данных недостаточно, прекращаем парсинг
				break;
			// Если тип имени — host_name и длина имени больше 0
			if((type == 0x00) && (length > 0))
				// Устанавливаем имя сервера в расширение SNI
				awh_cast <awh::tls::fgp_t::extension_server_name_t *> (browser.extensions.back().get())->names.push_back(string(reinterpret_cast <const char *> (buffer + offset), length));
			// Сдвигаем смещение на длину имени
			offset += static_cast <size_t> (length);
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения quic_transport_parameters (RFC 9001, 0x0039)
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseQUICTransportParams(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение quic_transport_parameters в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_quic_transport_params_t> ());
		// Если данных нет — пустое расширение
		if(size == 0)
			// Выходим из функции
			return;
		// Разбираем параметры QUIC
		parseQUICTransportParamsInternal(
			buffer, size,
			awh_cast <awh::tls::fgp_t::extension_quic_transport_params_t *> (browser.extensions.back().get())->params
		);
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения quic_transport_parameters_legacy (BoringSSL, 0xFFA5)
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseQUICTransportParamsLegacy(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение quic_transport_parameters_legacy в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_quic_transport_params_legacy_t> ());
		// Если данных нет — пустое расширение
		if(size == 0)
			// Выходим из функции
			return;
		// Разбираем параметры QUIC (формат идентичен стандартному)
		parseQUICTransportParamsInternal(
			buffer, size,
			awh_cast <awh::tls::fgp_t::extension_quic_transport_params_legacy_t *> (browser.extensions.back().get())->params
		);
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения tls_flags (draft-ietf-tls-tlsflags, 0x003E)
	 * Данные — битовое поле флагов: байт i содержит флаги с номерами i*8 .. i*8+7.
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseTLSFlags(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение tls_flags в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_tls_flags_t> ());
		// Если данных нет — пустое расширение (все флаги = 0)
		if(size == 0)
			// Выходим из функции
			return;
		// Копируем байты флагов
		awh_cast <awh::tls::fgp_t::extension_tls_flags_t *> (browser.extensions.back().get())->flags.assign(buffer, buffer + size);
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения next_proto_neg / NPN (0x3374)
	 * В ClientHello NPN обычно пустое (сигнализирует поддержку). Если данные присутствуют,
	 * разбираем как список протоколов: 1-байтовая длина + имя (без внешнего поля длины).
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseNPN(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение NPN в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_next_proto_neg_t> ());
		// Разбираем список протоколов
		size_t offset = 0;
		/**
		 * Итерируем по данным расширения, читая 1-байтовую длину имени протокола и само имя, пока не достигнем конца данных.
		 */
		while(offset < size){
			// Читаем 1-байтовую длину имени протокола
			const uint8_t length = buffer[offset++];
			// Проверяем границы
			if((length == 0) || ((offset + length) > size))
				// Если данных недостаточно для чтения имени протокола, прекращаем парсинг
				break;
			// Добавляем имя протокола
			awh_cast <awh::tls::fgp_t::extension_next_proto_neg_t *> (browser.extensions.back().get())->protocols.push_back(string(reinterpret_cast <const char *> (buffer + offset), length));
			// Сдвигаем смещение на длину имени протокола
			offset += static_cast <size_t> (length);
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения application_settings / ALPS new (0x44CD)
	 * Формат (draft-vvv-tls-alps): 2-байтовая длина списка + записи (1-байтовая длина + имя).
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseApplicationSettings(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение application_settings в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_application_settings_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Читаем байтовую длину списка протоколов
		const uint16_t count = ::local::u16(buffer);
		// Проверяем, что список помещается в буфер
		if(count > (size - 2))
			// Выходим из функции
			return;
		// Смещение для чтения записей: пропускаем 2 байта длины списка
		size_t offset = 2;
		// Конец списка
		const size_t end = static_cast <size_t> (2 + count);
		/**
		 * Перебираем протоколы: каждый — 1-байтовая длина + имя
		 */
		while(offset < end){
			// Извлекаем размер имени протокола из текущей позиции в буфере
			const uint8_t length = buffer[offset++];
			// Если длина имени протокола равна 0 или если имя протокола не помещается в оставшихся данных расширения
			if((length == 0) || ((offset + length) > end))
				// Прекращаем парсинг, так как данных недостаточно для чтения имени протокола
				break;
			// Добавляем имя протокола в список поддерживаемых протоколов расширения application_settings браузера
			awh_cast <awh::tls::fgp_t::extension_application_settings_t *> (browser.extensions.back().get())->protocols.push_back(string(reinterpret_cast <const char *> (buffer + offset), length));
			// Сдвигаем смещение на длину имени протокола
			offset += static_cast <size_t> (length);
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения application_settings_old / ALPS legacy (0x4469)
	 * Формат идентичен application_settings (0x44CD).
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseApplicationSettingsOld(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение application_settings_old в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_application_settings_old_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Читаем байтовую длину списка протоколов
		const uint16_t count = ::local::u16(buffer);
		// Проверяем, что список помещается в буфер
		if(count > (size - 2))
			// Выходим из функции
			return;
		// Смещение для чтения записей: пропускаем 2 байта длины списка
		size_t offset = 2;
		// Конец списка
		const size_t end = static_cast <size_t> (2 + count);
		/**
		 * Перебираем протоколы: каждый — 1-байтовая длина + имя
		 */
		while(offset < end){
			// Извлекаем размер имени протокола из текущей позиции в буфере
			const uint8_t length = buffer[offset++];
			// Если длина имени протокола равна 0 или если имя протокола не помещается в оставшихся данных расширения
			if((length == 0) || ((offset + length) > end))
				// Прекращаем парсинг, так как данных недостаточно для чтения имени протокола
				break;
			// Добавляем имя протокола в список поддерживаемых протоколов расширения application_settings_old браузера
			awh_cast <awh::tls::fgp_t::extension_application_settings_old_t *> (browser.extensions.back().get())->protocols.push_back(string(reinterpret_cast <const char *> (buffer + offset), length));
			// Сдвигаем смещение на длину имени протокола
			offset += static_cast <size_t> (length);
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения channel_id (BoringSSL/Chrome, 0x7550)
	 * В ClientHello всегда пустое — сигнализирует поддержку Channel ID.
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseChannelID(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение channel_id в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_channel_id_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения trust_anchors (BoringSSL draft, 0xCA34)
	 * В ClientHello присутствие расширения сигнализирует поддержку.
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseTrustAnchors(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение trust_anchors в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_trust_anchors_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения ech_outer_extensions (ECH draft, 0xFD00)
	 * Формат: 1-байтовый счётчик байт + список 2-байтовых ExtensionType (счётчик должен быть чётным).
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseECHOuterExtensions(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение ech_outer_extensions в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_ech_outer_extensions_t> ());
		// Минимум: 1 байт длины + 2 байта типа
		if(size < 3)
			// Выходим из функции, так как данных недостаточно для парсинга
			return;
		// Читаем байтовую длину списка
		const uint8_t count = buffer[0];
		// Длина должна быть чётной (каждый тип = 2 байта) и помещаться в буфер
		if(((count % 2) != 0) || (size < static_cast <size_t> (1 + count)))
			// Выходим из функции, так как данные некорректные для парсинга
			return;
		// Получаем ссылку на список расширений
		auto * ext = awh_cast <awh::tls::fgp_t::extension_ech_outer_extensions_t *> (browser.extensions.back().get());
		// Перебираем 2-байтовые коды типов расширений
		for(size_t i = 1; (i + 1) <= static_cast <size_t> (1 + count); i += 2)
			// Сохраняем как UNKNOWN (маппинг wire-кодов → extension_type_t не требуется для отпечатка)
			ext->extensions.push_back(awh::tls::extension_type_t::UNKNOWN);
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения Encrypted Client Hello / ECH (0xFE0D)
	 * Данные — непрозрачный двоичный blob (opaque).
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseEncryptedClientHello(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение ECH в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_encryption_client_hello_t> ());
		// Если данных нет — пустое расширение
		if(size == 0)
			// Выходим из функции, так как данных недостаточно для парсинга
			return;
		// Копируем весь blob целиком
		awh_cast <awh::tls::fgp_t::extension_encryption_client_hello_t *> (browser.extensions.back().get())->data.assign(buffer, buffer + size);
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения renegotiation_info (RFC 5746, 0xFF01)
	 * Формат: 1-байтовая длина + renegotiated_connection. В начальном ClientHello длина = 0.
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 */
	static void parseRenegotiationInfo(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение renegotiation_info в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_renegotiation_info_t> ());
		// Если данных нет — пустое расширение
		if(size < 1)
			// Выходим из функции, так как данных недостаточно для парсинга
			return;
		// Читаем 1-байтовую длину renegotiated_connection
		const uint8_t length = buffer[0];
		// В начальном ClientHello length = 0 (нет предыдущего verify_data)
		if((length == 0) || (size < static_cast <size_t> (1 + length)))
			// Выходим из функции, так как данных недостаточно для чтения renegotiated_connection
			return;
		// Копируем renegotiated_connection bytes
		awh_cast <awh::tls::fgp_t::extension_renegotiation_info_t *> (browser.extensions.back().get())->data.assign(buffer + 1, buffer + (1 + length));
	}

	

	
};







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

static void parse_pre_shared_key(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    uint16_t ids_len = read_u16(data);
    printf(", \"identities\": [");
    bool first = true;
    size_t off = 2;
    while (off + 2 <= static_cast<size_t>(2 + ids_len) && off + 2 <= len) {
        uint16_t id_len = read_u16(data + off);
        off += 2;
        if (off + id_len + 4 > len) break;
        if (!first) printf(", ");
        first = false;
        // identity в hex (первые 16 байт)
        printf("{\"identity\": \"");
        for (size_t i = 0; i < std::min<size_t>(16, id_len); ++i)
            printf("%02X", data[off + i]);
        if (id_len > 16) printf("...");
        // obfuscated_ticket_age (4 байта после identity)
        uint32_t age = (static_cast<uint32_t>(data[off + id_len])     << 24) |
                       (static_cast<uint32_t>(data[off + id_len + 1]) << 16) |
                       (static_cast<uint32_t>(data[off + id_len + 2]) <<  8) |
                        static_cast<uint32_t>(data[off + id_len + 3]);
        printf("\", \"obfuscated_ticket_age\": %u}", age);
        off += id_len + 4;
    }
    printf("]\n");
}

// 0x002F: certificate_authorities (RFC 8446 §4.2.4)
static void parse_certificate_authorities(const uint8_t* data, size_t len) {
    if (len < 2) { printf("\n"); return; }
    const uint16_t list_len = read_u16(data);
    if (len < static_cast<size_t>(2 + list_len)) { printf("\n"); return; }
    const size_t end = static_cast<size_t>(2 + list_len);
    size_t off = 2;
    printf(", \"authorities\": [");
    bool first = true;
    while (off + 2 <= end) {
        const uint16_t dn_len = read_u16(data + off);
        off += 2;
        if (off + dn_len > end) break;
        if (!first) printf(", ");
        first = false;
        printf("\"");
        for (uint16_t i = 0; i < dn_len; ++i)
            printf("%02X", data[off + i]);
        printf("\"");
        off += dn_len;
    }
    printf("]\n");
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


bool parse(const uint8_t * buffer, const size_t size) noexcept {
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
	if(off + sess_len > size){ printf("[ERR] Truncated at sessionId\n"); return false; }

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
	printf("  tlsVersionRecord     = \"%u\"\n",  static_cast<unsigned>(record_version));
	printf("  tlsVersionNegotiated = \"%u\"\n",  static_cast<unsigned>(neg_ver));

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
		printf("  ja3Hash               = \"%s\"\n", md5_hex(ja3).c_str());
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
		printf("  ja4r                  = \"%s_%s_%s_%s\"\n",
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
		printf("  peetprintHash         = \"%s\"\n", md5_hex(peet).c_str());
	}

	// --- clientRandom / sessionId ---
	printf("  clientRandom          = \"%s\"\n", fp_random_hex.c_str());
	printf("  sessionId             = \"%s\"\n", fp_session_id_hex.c_str());

	return true;
}

/**
 * @brief Метод вычисления цифровых отпечатков на основе распарсенного ClientHello
 *
 * @param browser объект с распарсенными данными ClientHello
 * @param result  объект для хранения всех вычисленных отпечатков
 * @return        результат вычисления цифровых отпечатков
 */
bool awh::tls::Fingerprint::imprint(const browser_t & browser, imprint_t & result) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Сбрасываем результат на всякий случай
		result = imprint_t{};
		// --- tlsVersionRecord: wire-код версии record layer ---
		result.tls.record = ::to_string(::local::versionWire(browser.record.version));
		// --- sessionId: произвольная длина в hex ---
		result.sessionId = ::local::tohex(browser.session.data(), browser.session.size());
		// --- clientRandom: 32 байта в hex ---
		result.clientRandom = ::local::tohex(browser.clientHello.random.data(), browser.clientHello.random.size());
		// --- tlsVersionNegotiated ---
		/**
		 * Берём наибольшую не-GREASE версию из расширения supported_versions.
		 * Если расширение отсутствует, используем clientHello.legacy_version.
		 */
		{
			// Получаем wire-код версии из clientHello.legacy_version в качестве начального значения
			uint16_t best = ::local::versionWire(browser.clientHello.version);
			/**
			 * Проходим по расширениям в порядке их объявления и ищем supported_versions.
			 */
			for(const auto & ext : browser.extensions){
				// Если это расширение supported_versions, проходим по его версиям и ищем наибольшую не-GREASE
				if(ext->type == extension_type_t::SUPPORTED_VERSIONS){
					// Приводим объект нужного нам расширения
					const auto * sv = awh_cast <const extension_supported_versions_t *> (ext.get());
					// Проходим по версиям в расширении и ищем наибольшую не-GREASE
					for(const auto version : sv->versions){
						// Если версия — GREASE, пропускаем её
						if(version == version_t::GREASE)
							// (хотя GREASE в supported_versions — это странно, но кто знает, может быть и такое)
							continue;
						// Получаем wire-код версии и сравниваем с лучшей найденной
						const uint16_t wire = ::local::versionWire(version);
						// Если wire-код версии больше текущего лучшего, обновляем лучший
						if(wire > best)
							// Устанавливаем эту версию как лучшую
							best = wire;
					}
					// Завершаем поиск, так как supported_versions — единственное расширение, влияющее на tlsVersionNegotiated
					break;
				}
			}
			// Записываем результат в строковом виде (десятичный) в результат
			result.tls.negotiated = ::to_string(best);
		}
		// --- Собираем объекты нужных нам расширений (однократный проход) ---
		const extension_supported_groups_t   * extensionGroups  = nullptr;
		const extension_ec_point_t           * extensionEcPoint = nullptr;
		const extension_signature_t          * extensionSigAlgs = nullptr;
		const extension_alpn_t               * extensionAlpn    = nullptr;
		const extension_server_name_t        * extensionSni     = nullptr;
		const extension_supported_versions_t * extensionSv      = nullptr;
		const extension_psk_key_exchange_t   * extensionPsk     = nullptr;
		/**
		 * Проходим по расширениям в порядке их объявления и сохраняем объекты нужных нам расширений
		 */
		for(const auto & ext : browser.extensions){
			/**
			 * Определяем тип расширения
			 */
			switch(static_cast <uint8_t> (ext->type)){
				// Если мы получили расширение supported_groups, сохраняем его объект в extensionGroups
				case static_cast <uint8_t> (extension_type_t::SUPPORTED_GROUPS):
					// Приводим объект расширения к нужному типу и сохраняем в extensionGroups
					extensionGroups = awh_cast <const extension_supported_groups_t *> (ext.get());
				break;
				// Если мы получили расширение ec_point_formats, сохраняем его объект в extensionEcPoint
				case static_cast <uint8_t> (extension_type_t::EC_POINT_FORMATS):
					// Приводим объект расширения к нужному типу и сохраняем в extensionEcPoint
					extensionEcPoint = awh_cast <const extension_ec_point_t *> (ext.get());
				break;
				// Если мы получили расширение signature_algorithms, сохраняем его объект в extensionSigAlgs
				case static_cast <uint8_t> (extension_type_t::SIGNATURE_ALGORITHMS):
					// Приводим объект расширения к нужному типу и сохраняем в extensionSigAlgs
					extensionSigAlgs = awh_cast <const extension_signature_t *> (ext.get());
				break;
				// Если мы получили расширение ALPN, сохраняем его объект в extensionAlpn
				case static_cast <uint8_t> (extension_type_t::ALPN):
					// Приводим объект расширения к нужному типу и сохраняем в extensionAlpn
					extensionAlpn = awh_cast <const extension_alpn_t *> (ext.get());
				break;
				// Если мы получили расширение server_name, сохраняем его объект в extensionSni
				case static_cast <uint8_t> (extension_type_t::SERVER_NAME):
					// Приводим объект расширения к нужному типу и сохраняем в extensionSni
					extensionSni = awh_cast <const extension_server_name_t *> (ext.get());
				break;
				// Если мы получили расширение supported_versions, сохраняем его объект в extensionSv
				case static_cast <uint8_t> (extension_type_t::SUPPORTED_VERSIONS):
					// Приводим объект расширения к нужному типу и сохраняем в extensionSv
					extensionSv = awh_cast <const extension_supported_versions_t *> (ext.get());
				break;
				// Если мы получили расширение psk_key_exchange_modes, сохраняем его объект в extensionPsk
				case static_cast <uint8_t> (extension_type_t::PSK_KEY_EXCHANGE_MODES):
					// Приводим объект расширения к нужному типу и сохраняем в extensionPsk
					extensionPsk = awh_cast <const extension_psk_key_exchange_t *> (ext.get());
				break;
			}
		}
		// ======================== JA3 ========================
		/**
		 * Формат: "{sslVersion},{ciphers},{extensions},{groups},{ec_points}"
		 * sslVersion = ClientHello.legacy_version wire-код (десятично)
		 * Списки: оригинальный порядок, без GREASE, разделитель '-'
		 */
		{
			// Список временных переменных для формирования JA3
			string ciphers = "", exts = "", groups = "", ecpts = "";
			/**
			 * Проходим по шифрам в порядке их объявления и формируем строку для JA3, пропуская GREASE и шифры без wire-кода
			 */
			for(const auto cipher : browser.ciphers){
				// Если шифр — GREASE, пропускаем его
				if(cipher == cipher_t::GREASE)
					// Просто пропускаем этот шифр и не включаем его в строку JA3
					continue;
				// Получаем wire-код шифра
				const uint16_t wire = ::local::cipherWire(cipher);
				// Если wire-код шифра равен 0, пропускаем его (неизвестный шифр без wire-кода)
				if(wire == 0)
					// Просто пропускаем этот шифр и не включаем его в строку JA3
					continue;
				// Если это не первый шифр, добавляем разделитель '-'
				if(!ciphers.empty())
					// Добавляем разделитель '-' перед добавлением следующего шифра
					ciphers.append(1, '-');
				// Добавляем wire-код шифра в строку ciphers для JA3
				ciphers.append(::to_string(wire));
			}
			/**
			 * Проходим по расширениям в порядке их объявления и формируем строку для JA3, пропуская GREASE и расширения без wire-кода
			 */
			for(const auto & ext : browser.extensions){
				// Если расширение — GREASE, пропускаем его
				if(ext->type == extension_type_t::GREASE)
					// Просто пропускаем это расширение и не включаем его в строку JA3
					continue;
				// Получаем wire-код расширения
				const uint16_t wire = ::local::extensionWire(ext->type);
				// Если wire-код расширения равен 0xFFFF, пропускаем его (неизвестное расширение без wire-кода)
				if(wire == 0xFFFF)
					// Просто пропускаем это расширение и не включаем его в строку JA3
					continue;
				// Если это не первое расширение, добавляем разделитель '-'
				if(!exts.empty())
					// Добавляем разделитель '-' перед добавлением следующего расширения
					exts.append(1, '-');
				// Добавляем wire-код расширения в строку exts для JA3
				exts.append(::to_string(wire));
			}
			// Если расширение supported_groups присутствует
			if(extensionGroups != nullptr){
				/**
				 * Проходим по группам в порядке их объявления и формируем строку для JA3, пропуская GREASE и группы без wire-кода
				 */
				for(const auto gid : extensionGroups->supportedGroups){
					// Если группа — GREASE, пропускаем её
					if(gid == group_t::GREASE)
						// Просто пропускаем эту группу и не включаем её в строку JA3
						continue;
					// Получаем wire-код группы
					if(!groups.empty())
						// Если это не первая группа, добавляем разделитель '-'
						groups.append(1, '-');
					// Добавляем wire-код группы в строку groups для JA3
					groups.append(::to_string(::local::groupWire(gid)));
				}
			}
			// Если расширение ec_point_formats присутствует
			if(extensionEcPoint != nullptr){
				/**
				 * Проходим по форматам точек в порядке их объявления и формируем строку для JA3, пропуская форматы без wire-кода
				 */
				for(const auto format : extensionEcPoint->formats){
					// Получаем wire-код формата точки
					const uint8_t wire = ::local::ecPointWire(format);
					// Если wire-код формата точки равен 0xFF, пропускаем его (неизвестный формат без wire-кода)
					if(wire == 0xFF)
						// Просто пропускаем этот формат точки и не включаем его в строку JA3
						continue;
					// Если это не первый формат точки, добавляем разделитель '-'
					if(!ecpts.empty())
						// Добавляем разделитель '-' перед добавлением следующего формата точки
						ecpts.append(1, '-');
					// Добавляем wire-код формата точки в строку ecpts для JA3
					ecpts.append(::to_string(wire));
				}
			}
			// Формируем строку JA3 в формате "{sslVersion},{ciphers},{extensions},{groups},{ec_points}"
			result.ja3 = (
				::to_string(::local::versionWire(browser.clientHello.version)) + ',' +
				(ciphers + ',' + exts + ',' + groups + ',' + ecpts)
			);
			// Вычисляем ja3Hash как MD5 от строки ja3
			result.ja3Hash = ::local::md5(result.ja3);
		}
		// ======================== JA4 ========================
		/**
		 * Спецификация: https://github.com/FoxIO-LLC/ja4
		 * Формат JA4:   {t|q}{ver2}{d|i}{cc:02d}{ec:02d}{alpn2}_{md5(sorted_ciphers)[:12]}_{md5(sorted_exts+"_"+sigalgs)[:12]}
		 * Формат JA4_r: {t|q}{ver2}{d|i}{cc:02d}{ec:02d}{alpn2}_{sorted_ciphers_hex}_{sorted_exts_hex}_{sigalgs_hex_original_order}
		 */
		{
			// Протокол: 'q' при наличии QUIC transport params, иначе 't'
			bool hasQUIC = false;
			/**
			 * Проходим по расширениям в порядке их объявления и проверяем наличие расширения QUIC transport parameters (0x0039) или его устаревшего аналога (0xFFA5)
			 */
			for(const auto & ext : browser.extensions){
				// Если расширение — QUIC transport parameters (0x0039) или его устаревший аналог (0xFFA5), устанавливаем флаг hasQUIC в true и завершаем поиск
				if((hasQUIC = ((ext->type == extension_type_t::QUIC_TRANSPORT_PARAMETERS) ||
				   (ext->type == extension_type_t::QUIC_TRANSPORT_PARAMETERS_LEGACY))))
				    // Завершаем поиск, так как наличие расширения QUIC transport parameters определяет протокол в JA4
					break;
			}
			// Согласованная версия → 2-символьная строка
			string ver2 = "";
			{
				// Получаем согласованную версию так же, как для tlsVersionNegotiated
				const uint16_t wire = static_cast <uint16_t> (::stoul(result.tls.negotiated));
				/**
				 * Преобразуем wire-код согласованной версии в 2-символьную строку по формату JA4:
				 */
				switch(wire){
					// Если версия — 0x0300, это может означать SSL 3.0 (s3)
					case 0x0300: ver2 = "s3"; break;
					// Если версия — 0x0301, это может означать TLS 1.0 (10)
					case 0x0301: ver2 = "10"; break;
					// Если версия — 0x0302, это может означать TLS 1.1 (11)
					case 0x0302: ver2 = "11"; break;
					// Если версия — 0x0303, это может означать TLS 1.2 (12)
					case 0x0303: ver2 = "12"; break;
					// Если версия — 0x0304, это может означать TLS 1.3 (13)
					case 0x0304: ver2 = "13"; break;
					// Если версия — 0xFEFF, это может означать DTLS 1.0 (d1)
					case 0xFEFF: ver2 = "d1"; break;
					// Если версия — 0xFEFD, это может означать DTLS 1.2 (d2)
					case 0xFEFD: ver2 = "d2"; break;
					// Иначе — неизвестная версия, используем "00"
					default: ver2 = "00";
				}
			}
			// Флаг SNI: 'd' если SNI присутствует, иначе 'i'
			const char sniFlag = (((extensionSni != nullptr) && !extensionSni->names.empty()) ? 'd' : 'i');
			// Шифры: все без GREASE и без SCSV(0x00FF), отсортированные
			vector <uint16_t> ciphersSorted;
			/**
			 * Проходим по шифрам в порядке их объявления и формируем список для JA4, пропуская GREASE и шифры без wire-кода или SCSV(0x00FF)
			 */
			for(const auto cipher : browser.ciphers){
				// Если шифр — GREASE, пропускаем его
				if(cipher == cipher_t::GREASE)
					// Просто пропускаем этот шифр и не включаем его в список для JA4
					continue;
				// Получаем wire-код шифра
				const uint16_t wire = ::local::cipherWire(cipher);
				// Если wire-код шифра равен 0 или 0x00FF (SCSV), пропускаем его (неизвестный шифр без wire-кода или SCSV)
				if((wire == 0) || (wire == 0x00FF))
					// Просто пропускаем этот шифр и не включаем его в список для JA4
					continue;
				// Добавляем wire-код шифра в список для JA4
				ciphersSorted.push_back(wire);
			}
			// Сортируем список шифров для JA4
			::sort(ciphersSorted.begin(), ciphersSorted.end());
			// Счётчик расширений: все кроме GREASE (включая SNI и ALPN)
			size_t extCount = 0;
			/**
			 * Проходим по расширениям в порядке их объявления и считаем количество расширений для JA4, пропуская GREASE
			 */
			for(const auto & ext : browser.extensions)
				// Если расширение соответствует стандартному расширению
				if(ext->type != extension_type_t::GREASE)
					// Увеличиваем счётчик расширений для JA4 на 1
					++extCount;
			// Расширения для hash/raw: без GREASE, SNI(0x0000) и ALPN(0x0010), отсортированные
			vector <uint16_t> extsSorted;
			/**
			 * Проходим по расширениям в порядке их объявления и формируем список для JA4, пропуская GREASE, SNI(0x0000) и ALPN(0x0010), а также расширения без wire-кода
			 */
			for(const auto & ext : browser.extensions){
				// Если расширение — GREASE, пропускаем его
				if(ext->type == extension_type_t::GREASE)
					// Просто пропускаем это расширение и не включаем его в список для JA4
					continue;
				// Если расширение — SNI(0x0000), пропускаем его
				if(ext->type == extension_type_t::SERVER_NAME)
					// Просто пропускаем это расширение и не включаем его в список для JA4
					continue;
				// Если расширение — ALPN(0x0010), пропускаем его
				if(ext->type == extension_type_t::ALPN)
					// Просто пропускаем это расширение и не включаем его в список для JA4
					continue;
				// Получаем wire-код расширения
				const uint16_t wire = ::local::extensionWire(ext->type);
				// Если wire-код расширения равен 0xFFFF, пропускаем его (неизвестное расширение без wire-кода)
				if(wire == 0xFFFF)
					// Просто пропускаем это расширение и не включаем его в список для JA4
					continue;
				// Добавляем wire-код расширения в список для JA4
				extsSorted.push_back(wire);
			}
			// Сортируем список расширений для JA4
			::sort(extsSorted.begin(), extsSorted.end());
			// Алгоритмы подписи: без GREASE, в исходном порядке ClientHello (не сортируются — требование спецификации JA4)
			vector <uint16_t> sigalgsOrdered;
			// Если расширение signature_algorithms присутствует
			if(extensionSigAlgs != nullptr){
				/**
				 * Проходим по алгоритмам подписи в порядке их объявления и формируем список для JA4, пропуская GREASE и алгоритмы без wire-кода
				 */
				for(const auto sig : extensionSigAlgs->algorithms){
					// Если алгоритм подписи — GREASE, пропускаем его
					if(sig == signature_t::GREASE)
						// Просто пропускаем этот алгоритм подписи и не включаем его в список для JA4
						continue;
					// Получаем wire-код алгоритма подписи
					const uint16_t wire = ::local::signatureWire(sig);
					// Если wire-код алгоритма подписи равен 0, пропускаем его (неизвестный алгоритм подписи без wire-кода)
					if(wire == 0)
						// Просто пропускаем этот алгоритм подписи и не включаем его в список для JA4
						continue;
					// Добавляем wire-код алгоритма подписи в список для JA4 (порядок сохраняется)
					sigalgsOrdered.push_back(wire);
				}
			}
			// Первые 2 символа первого ALPN-протокола
			string alpn2 = "00";
			// Если расширение ALPN присутствует и содержит хотя бы один протокол
			if((extensionAlpn != nullptr) && !extensionAlpn->protocols.empty()){
				// Берём первый протокол из расширения ALPN
				const string & p0 = extensionAlpn->protocols[0];
				// Если длина первого протокола ALPN больше или равна 2, используем первые 2 символа, иначе дополняем до 2 символов нулями
				if(p0.size() >= 2)
					// Используем первые 2 символа первого протокола ALPN для alpn2 в JA4
					alpn2 = ::move(p0.substr(0, 2));
				// Если длина первого протокола ALPN равна 1, используем этот символ и дополняем до 2 символов нулями
				else if(p0.size() == 1)
					// Используем первый символ первого протокола ALPN и дополняем до 2 символов нулями для alpn2 в JA4
					alpn2 = (p0 + '0');
			}
			// Формируем префикс
			char prefixBuf[16];
			// Формат префикса: {t|q}{ver2}{d|i}{cc:02d}{ec:02d}{alpn2}
			::snprintf(prefixBuf, sizeof(prefixBuf), "%c%s%c%02d%02d%s",
				(hasQUIC ? 'q' : 't'),
				ver2.c_str(),
				sniFlag,
				static_cast <int> (ciphersSorted.size()),
				static_cast <int> (extCount),
				alpn2.c_str()
			);
			// Сохраняем префикс в строке
			const string prefix(prefixBuf);
			/**
			 * @brief Функция для конвертации списка uint16_t в строку hex формата "xxxx,xxxx,..."
			 * 
			 * @param v вектор uint16_t для конвертации
			 * @return  строка hex формата "xxxx,xxxx,..."
			 */
			const auto makeHexList = [](const vector <uint16_t> & v) -> string {
				// Результат работы функции
				string result = "";
				// Буфер для конвертации одного uint16 в 4-символьный hex
				char buffer[5] = {0}; // 4 символа + null-терминатор
				/**
				 * Проходим по каждому элементу вектора и конвертируем его в 4-символьный hex
				 */
				for(const uint16_t w : v){
					// Если это не первый элемент, добавляем разделитель ','
					if(!result.empty())
						// Добавляем разделитель ',' перед добавлением следующего элемента
						result.append(1, ',');
					// Конвертируем uint16_t в 4-символьный hex и добавляем к результату
					::snprintf(buffer, sizeof(buffer), "%04x", w);
					// Добавляем конвертированный элемент к результату
					result.append(buffer);
				}
				// Возвращаем результат
				return result;
			};
			// Формируем строку расширений в hex формате (используется в JA4 и JA4_r)
			const string extsHex = makeHexList(extsSorted);
			// Формируем строку шифров в hex формате (используется в JA4 и JA4_r)
			const string ciphersHex = makeHexList(ciphersSorted);
			// Формируем строку алгоритмов подписи в hex формате в исходном порядке ClientHello (используется в JA4 и JA4_r)
			const string sigalgsHex = makeHexList(sigalgsOrdered);
			// Формируем строку JA4_r в формате {t|q}{ver2}{d|i}{cc:02d}{ec:02d}{alpn2}_{ciphersHex}_{extsHex}_{sigalgsHex}
			result.ja4r = (prefix + '_' + ciphersHex + '_' + extsHex + '_' + sigalgsHex);
			// Формируем строку JA4: компонент b = md5(sorted_ciphers)[:12], компонент c = md5(sorted_exts+"_"+sigalgs)[:12]
			result.ja4 = (
				prefix + '_' +
				::local::md5(ciphersHex).substr(0, 12) + '_' +
				::local::md5(extsHex + '_' + sigalgsHex).substr(0, 12)
			);
		}
		// ======================== PeetPrint ========================
		/**
		 * Формат: 8 секций, разделённых '|'
		 * 1: supported_versions (с GREASE, decimal)
		 * 2: legacy_compression_methods raw bytes: "{total}-{len}.{m1}.{m2}..."
		 * 3: supported_groups (с GREASE, decimal)
		 * 4: signature_algorithms (с GREASE, decimal)
		 * 5: кол-во EC point formats
		 * 6: кол-во PSK key exchange modes
		 * 7: cipher suites (с GREASE, decimal, исходный порядок)
		 * 8: extensions (с GREASE-маркерами, decimal, исходный порядок)
		 */
		{
			string peet = "";
			// Секция 1: supported_versions
			{
				// Формируем временную строку для секции supported_versions
				string sec = "";
				// Если расширение supported_versions присутствует
				if(extensionSv != nullptr){
					/**
					 * Проходим по версиям в порядке их объявления и формируем строку для секции supported_versions, включая GREASE
					 */
					for(const auto v : extensionSv->versions){
						// Если это не первая версия, добавляем разделитель '-'
						if(!sec.empty())
							// Добавляем разделитель '-' перед добавлением следующей версии
							sec.append(1, '-');
						// Добавляем версию в строку секции supported_versions, используя "GREASE" для GREASE-версий и десятичный код для остальных
						sec.append(
							(v == version_t::GREASE)
						    ? string("GREASE")
						    : ::to_string(::local::versionWire(v))
						);
					}
				}
				// Добавляем сформированную строку для секции supported_versions к общему peetprint, добавляя разделитель '|'
				peet.append(sec + '|');
			}
			/**
			 * Секция 2: legacy_compression_methods raw bytes
			 * Поле в TLS: <1-байтовая длина><байты методов>
			 * Формат: "{1+n}-{n}.{m1}.{m2}..."
			 */
			{
				// Количество методов компрессии (1-байтовое поле длины не входит в count)
				const size_t count = browser.compressors.size();
				// Формируем строку для секции legacy_compression_methods, начиная с "{1+n}-{n}"
				string sec = (::to_string(1 + count) + '-' + ::to_string(count));
				/**
				 * Проходим по методам сжатия в порядке их объявления и добавляем их к строке секции legacy_compression_methods, включая GREASE
				 */
				for(const auto compressor : browser.compressors){
					// Добавляем разделитель '.' перед добавлением следующего метода сжатия
					sec.append(1, '.');
					// Добавляем метод сжатия в строку секции legacy_compression_methods, используя "GREASE" для GREASE-методов и десятичный код для остальных
					sec.append(::to_string(::local::compressorWire(compressor)));
				}
				// Добавляем сформированную строку для секции legacy_compression_methods к общему peetprint, добавляя разделитель '|'
				peet.append(sec + '|');
			}
			// Секция 3: supported_groups
			{
				// Формируем временную строку для секции supported_groups
				string sec = "";
				// Если расширение supported_groups присутствует
				if(extensionGroups != nullptr){
					/**
					 * Проходим по группам в порядке их объявления и формируем строку для секции supported_groups, включая GREASE
					 */
					for(const auto gid : extensionGroups->supportedGroups){
						// Если это не первая группа, добавляем разделитель '-'
						if(!sec.empty())
							// Добавляем разделитель '-' перед добавлением следующей группы
							sec.append(1, '-');
						// Добавляем группу в строку секции supported_groups, используя "GREASE" для GREASE-групп и десятичный код для остальных
						sec.append(
							(gid == group_t::GREASE)
							? string("GREASE")
							: ::to_string(::local::groupWire(gid))
						);
					}
				}
				// Добавляем сформированную строку для секции supported_groups к общему peetprint, добавляя разделитель '|'
				peet.append(sec + '|');
			}
			// Секция 4: signature_algorithms
			{
				// Формируем временную строку для секции signature_algorithms
				string sec = "";
				// Если расширение signature_algorithms присутствует
				if(extensionSigAlgs != nullptr){
					/**
					 * Проходим по алгоритмам подписи в порядке их объявления и формируем строку для секции signature_algorithms, включая GREASE
					 */
					for(const auto sig : extensionSigAlgs->algorithms){
						// Если это не первый алгоритм подписи, добавляем разделитель '-'
						if(!sec.empty())
							// Добавляем разделитель '-' перед добавлением следующего алгоритма подписи
							sec.append(1, '-');
						// Добавляем алгоритм подписи в строку секции signature_algorithms, используя "GREASE" для GREASE-алгоритмов и десятичный код для остальных
						sec.append(
							(sig == signature_t::GREASE)
							? string("GREASE")
							: ::to_string(::local::signatureWire(sig))
						);
					}
				}
				// Добавляем сформированную строку для секции signature_algorithms к общему peetprint, добавляя разделитель '|'
				peet.append(sec + '|');
			}
			// Секция 5: кол-во EC point formats
			peet.append(::to_string(extensionEcPoint ? extensionEcPoint->formats.size() : static_cast <size_t> (0)) + '|');
			// Секция 6: кол-во PSK key exchange modes
			peet.append(::to_string(extensionPsk ? extensionPsk->modes.size() : static_cast <size_t> (0)) + '|');
			// Секция 7: cipher suites (с GREASE)
			{
				// Формируем временную строку для секции cipher suites
				string sec = "";
				/**
				 * Проходим по шифрам в порядке их объявления и формируем строку для секции cipher suites, включая GREASE и шифры без wire-кода
				 */
				for(const auto cipher : browser.ciphers){
					// Если это не первый шифр, добавляем разделитель '-'
					if(!sec.empty())
						// Добавляем разделитель '-' перед добавлением следующего шифра
						sec.append(1, '-');
					// Добавляем шифр в строку секции cipher suites, используя "GREASE" для GREASE-шифров и десятичный код для остальных, включая шифры без wire-кода (которые будут отображаться как 0)
					sec.append(
						(cipher == cipher_t::GREASE)
						? string("GREASE")
						: ::to_string(::local::cipherWire(cipher))
					);
				}
				// Добавляем сформированную строку для секции cipher suites к общему peetprint, добавляя разделитель '|'
				peet.append(sec + '|');
			}
			// Секция 8: extensions (с GREASE-маркерами)
			{
				// Формируем временную строку для секции extensions
				string sec = "";
				/**
				 * Проходим по расширениям в порядке их объявления и формируем строку для секции extensions, включая GREASE и расширения без wire-кода
				 */
				for(const auto & ext : browser.extensions){
					// Если это не первое расширение, добавляем разделитель '-'
					if(!sec.empty())
						// Добавляем разделитель '-' перед добавлением следующего расширения
						sec.append(1, '-');
					// Если расширение — GREASE, добавляем "GREASE"
					if(ext->type == extension_type_t::GREASE)
						// Добавляем "GREASE" для GREASE-расширений в строку секции extensions
						sec.append("GREASE");
					// Если расширение не GREASE, добавляем его в строку секции extensions, используя десятичный код для расширений с wire-кодом и 0 для расширений без wire-кода
					else {
						// Извлекаем wire-код расширения, используя десятичный код для расширений с wire-кодом и 0 для расширений без wire-кода
						const uint16_t wire = ::local::extensionWire(ext->type);
						// Добавляем расширение в строку секции extensions, используя десятичный код для расширений с wire-кодом и 0 для расширений без wire-кода (которые будут отображаться как 0)
						sec.append((wire == 0xFFFF) ? ::to_string(0u) : ::to_string(wire));
					}
				}
				// Добавляем сформированную строку для секции extensions к общему peetprint, без добавления разделителя '|' в конце, так как это последняя секция
				peet.append(sec);
			}
			// Вычисляем peetprintHash как MD5 от сформированного peetprint
			result.peetprintHash = ::local::md5(peet);
			// Устанавливаем сформированный peetprint
			result.peetprint = ::move(peet);
			// Выводим результат парсинга PeetPrint
			return !result.peetprintHash.empty();
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат по умолчанию
	return false;
}
/**
 * @brief Метод парсинга данных цифрового отпечатка
 *
 * @param buffer  бинарный буфер данных цифрового отпечатка
 * @param size    размер бинарного буфера данных цифрового отпечатка
 * @param browser объект для хранения распарсенных данных цифрового отпечатка
 * @return        результат парсинга данных цифрового отпечатка
 */
bool awh::tls::Fingerprint::parse(const uint8_t * buffer, const size_t size, browser_t & browser) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Сбрасываем данные браузера в дефолтное состояние (все поля UNKNOWN или false)
		browser = browser_t{};
		// Если размер данных меньше 11 байт, то это не может быть валидным TLS/DTLS ClientHello
		if(size < 11){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("Fingerprint buffer too short: %zu bytes (need >= 11)", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, size);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("Fingerprint buffer too short: %zu bytes (need >= 11)", log_t::flag_t::WARNING, size);
			#endif
			// Выводим результат по умолчанию
			return result;
		}
		// Проверяем, что первый байт — это 0x16 (handshake record)
		if(buffer[0] == 0x16){
			/**
			 * Определяем версию из заголовка record (offset 1 для TLS, offset 11 для DTLS)
			 */
			switch(::local::u16(buffer + 1)){
				// Если версия записи соответствует DTLSv1.0
				case 0xFEFF:
					// Устанавливаем версию записи в объекте browser как DTLS 1.0
					browser.record.version = version_t::DTLS_1_0;
				break;
				// Если версия записи соответствует DTLSv1.2
				case 0xFEFD:
					// Устанавливаем версию записи в объекте browser как DTLS 1.2
					browser.record.version = version_t::DTLS_1_2;
				break;
				// Если версия записи соответствует SSLv3
				case 0x0300:
					// Устанавливаем версию записи в объекте browser как SSL 3.0
					browser.record.version = version_t::SSL_V3;
				break;
				// Если версия записи соответствует TLSv1.0
				case 0x0301:
					// Устанавливаем версию записи в объекте browser как TLS 1.0
					browser.record.version = version_t::TLS_1_0;
				break;
				// Если версия записи соответствует TLSv1.1
				case 0x0302:
					// Устанавливаем версию записи в объекте browser как TLS 1.1
					browser.record.version = version_t::TLS_1_1;
				break;
				// Если версия записи соответствует TLSv1.2
				case 0x0303:
					// Устанавливаем версию записи в объекте browser как TLS 1.2
					browser.record.version = version_t::TLS_1_2;
				break;
				// Если версия записи соответствует TLSv1.3
				case 0x0304:
					// Устанавливаем версию записи в объекте browser как TLS 1.3
					browser.record.version = version_t::TLS_1_3;
				break;
			}
			/**
			 * Если версия записи не распознана, то она остаётся UNKNOWN, и мы выводим предупреждение, что версия записи не поддерживается.
			 * В этом случае мы не можем продолжать парсинг, так как многие смещения зависят от протокола (TLS vs DTLS), и возвращаем результат по умолчанию.
			 */
			if(browser.record.version == version_t::UNKNOWN){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Unsupported record version: 0x%04X", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, ::local::u16(buffer + 1));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Unsupported record version: 0x%04X", log_t::flag_t::WARNING, ::local::u16(buffer + 1));
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Определяем, является ли это DTLS (версии 0xFEFF для DTLS 1.0 и 0xFEFD для DTLS 1.2)
			const bool isDTLS = ((browser.record.version == version_t::DTLS_1_0) || (browser.record.version == version_t::DTLS_1_2));
			/**
			 * Размеры заголовков зависят от протокола:
			 * - TLS:  record header  =  5 байт (type:1 + version:2 + length:2)
			 * - DTLS: record header  = 13 байт (type:1 + version:2 + epoch:2 + seq:6 + length:2)
			 * - TLS:  handshake hdr  =  4 байта (type:1 + length:3)
			 * - DTLS: handshake hdr  = 12 байт  (type:1 + length:3 + msg_seq:2 + frag_off:3 + frag_len:3)
			 */
			// Получаем размер заголовка record в зависимости от протокола
			const size_t recordSize = (isDTLS ? 13u : 5u);
			// Получаем размер заголовка handshake в зависимости от протокола
			const size_t handshakeSize = (isDTLS ? 12u : 4u);
			// Если размер данных меньше суммы заголовков record и handshake, а также 2 байт для версии и 32 байт для random
			if(size < (recordSize + handshakeSize + 2u + 32u)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Fingerprint buffer too short for %s headers: %zu bytes", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, (isDTLS ? "DTLS" : "TLS"), size);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Fingerprint buffer too short for %s headers: %zu bytes", log_t::flag_t::WARNING, (isDTLS ? "DTLS" : "TLS"), size);
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Длина полезной нагрузки record: в TLS — offset 3, в DTLS — offset 11
			browser.record.length = (isDTLS ? ::local::u16(buffer + 11) : ::local::u16(buffer + 3));
			// Если это DTLS, то извлекаем epoch и sequence number из соответствующих полей заголовка record
			if(isDTLS){
				// Устанавливаем эпоху записи рукопожатия
				browser.record.epoch = ::local::u16(buffer + 3);
				// Устанавливаем sequence number записи рукопожатия, объединяя 6 байт в 48-битное число
				browser.record.sequence = (
					(static_cast <uint64_t> (buffer[5]) << 40) |
					(static_cast <uint64_t> (buffer[6]) << 32) |
					(static_cast <uint64_t> (buffer[7]) << 24) |
					(static_cast <uint64_t> (buffer[8]) << 16) |
					(static_cast <uint64_t> (buffer[9]) << 8) |
					static_cast <uint64_t> (buffer[10])
				);
			}
			// Если запись рукопожатия не соответствует ClientHello
			if(buffer[recordSize] != 0x01){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Handshake entry does not match the ClientHello", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Handshake entry does not match the ClientHello", log_t::flag_t::WARNING);
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			/**
			 * Определяем версию из заголовка handshake
			 */
			switch(::local::u16(buffer + (recordSize + handshakeSize))){
				// Если версия записи соответствует DTLSv1.0
				case 0xFEFF:
					// Устанавливаем версию записи в объекте browser как DTLS 1.0
					browser.clientHello.version = version_t::DTLS_1_0;
				break;
				// Если версия записи соответствует DTLSv1.2
				case 0xFEFD:
					// Устанавливаем версию записи в объекте browser как DTLS 1.2
					browser.clientHello.version = version_t::DTLS_1_2;
				break;
				// Если версия записи соответствует SSLv3
				case 0x0300:
					// Устанавливаем версию записи в объекте browser как SSL 3.0
					browser.clientHello.version = version_t::SSL_V3;
				break;
				// Если версия записи соответствует TLSv1.0
				case 0x0301:
					// Устанавливаем версию записи в объекте browser как TLS 1.0
					browser.clientHello.version = version_t::TLS_1_0;
				break;
				// Если версия записи соответствует TLSv1.1
				case 0x0302:
					// Устанавливаем версию записи в объекте browser как TLS 1.1
					browser.clientHello.version = version_t::TLS_1_1;
				break;
				// Если версия записи соответствует TLSv1.2
				case 0x0303:
					// Устанавливаем версию записи в объекте browser как TLS 1.2
					browser.clientHello.version = version_t::TLS_1_2;
				break;
				// Если версия записи соответствует TLSv1.3
				case 0x0304:
					// Устанавливаем версию записи в объекте browser как TLS 1.3
					browser.clientHello.version = version_t::TLS_1_3;
				break;
			}
			/**
			 * Если версия рукопожатия не распознана, то она остаётся UNKNOWN, и мы выводим предупреждение, что версия рукопожатия не поддерживается.
			 * В этом случае мы не можем продолжать парсинг, так как многие смещения зависят от протокола (TLS vs DTLS), и возвращаем результат по умолчанию.
			 */
			if(browser.clientHello.version == version_t::UNKNOWN){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("Unsupported handshake version: 0x%04X", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, ::local::u16(buffer + 1));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("Unsupported handshake version: 0x%04X", log_t::flag_t::WARNING, ::local::u16(buffer + 1));
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Устанавливаем размер записи рукопожатия
			browser.handshake.length = (
				(static_cast <uint32_t> (buffer[recordSize + 1]) << 16) |
				(static_cast <uint32_t> (buffer[recordSize + 2]) << 8)  |
				static_cast <uint32_t> (buffer[recordSize + 3])
			);
			// Если это DTLS, то извлекаем sequence number и фрагментацию из соответствующих полей заголовка handshake
			if(isDTLS){
				// Устанавливаем sequence number рукопожатия
				browser.handshake.sequence = ::local::u16(buffer + (recordSize + 4));
				// Устанавливаем смещение фрагмента рукопожатия, объединяя 3 байта в 24-битное число
				browser.handshake.fragment.offset = (
					(static_cast <uint32_t> (buffer[recordSize + 6]) << 16) |
					(static_cast <uint32_t> (buffer[recordSize + 7]) << 8)  |
					static_cast <uint32_t> (buffer[recordSize + 8])
				);
				// Устанавливаем длину фрагмента рукопожатия, объединяя 3 байта в 24-битное число
				browser.handshake.fragment.length = (
					(static_cast <uint32_t> (buffer[recordSize + 9]) << 16) |
					(static_cast <uint32_t> (buffer[recordSize + 10]) << 8)  |
					static_cast <uint32_t> (buffer[recordSize + 11])
				);
			}
			// Random: 32 байта после client_version
			size_t offset = ((recordSize + handshakeSize) + 2);
			/**
			 * Перебираем все 32 байта random записи рукопожатия
			 */
			for(size_t i = 0; i < 32; ++i)
				// Устанавливаем все байты в буфер random объекта clientHello
				browser.clientHello.random[i] = buffer[offset + i];
			/**
			 * offset = начало переменных полей (session_id_len)
			 * TLS:  rec(5)  + hs(4)  + version(2) + random(32) = 43
			 * DTLS: rec(13) + hs(12) + version(2) + random(32) = 59
			 */
			offset += 32;
			// Если размер данных меньше указанного смещения
			if(size < (offset + 1)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("ClientHello truncated at session_id_len", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("ClientHello truncated at session_id_len", log_t::flag_t::WARNING);
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Получаем длину session_id
			size_t length = static_cast <size_t> (buffer[offset++]);
			// Если длина session_id больше 32 байт, то это не соответствует стандарту TLS
			if(length > 32){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("ClientHello session_id_len > 32 (%zu)", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, length);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("ClientHello session_id_len > 32 (%zu)", log_t::flag_t::WARNING, length);
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Если размер данных меньше смещения + длины session_id, то это означает, что данные обрезаны
			if((offset + length) > size){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("ClientHello truncated at session_id (offset=%zu, length=%zu, size=%zu)", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, offset, length, size);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("ClientHello truncated at session_id (offset=%zu, length=%zu, size=%zu)", log_t::flag_t::WARNING, offset, length, size);
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Выделяем память для идентификатора сессии
			browser.session.resize(length, 0);
			// Копируем данные идентификатора сессии из буфера
			::memcpy(&browser.session[0], buffer + offset, length);
			// Увеличиваем смещение на длину session_id
			offset += length;
			// Если DTLS: cookie (только для DTLS ClientHello, RFC 6347 §4.2.1)
			if(isDTLS){
				// Если размер данных меньше смещения + 1 байт для cookie_len, то это означает, что данные обрезаны
				if(offset >= size){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("ClientHello truncated at cookie_len", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("ClientHello truncated at cookie_len", log_t::flag_t::WARNING);
					#endif
					// Выводим результат по умолчанию
					return result;
				}
				// Получаем длину cookie
				length = static_cast <size_t> (buffer[offset++]);
				// Если размер данных меньше смещения + длины cookie, то это означает, что данные обрезаны
				if((offset + length) > size){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("ClientHello truncated at cookie data (offset=%zu, length=%zu, size=%zu)", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, offset, length, size);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Выводим сообщение об ошибке
						this->_log->print("ClientHello truncated at cookie data (offset=%zu, length=%zu, size=%zu)", log_t::flag_t::WARNING, offset, length, size);
					#endif
					// Выводим результат по умолчанию
					return result;
				}
				// Выделяем память для cookie
				browser.cookie.resize(length, 0);
				// Копируем данные cookie из буфера
				::memcpy(&browser.cookie[0], buffer + offset, length);
				// Увеличиваем смещение на длину cookie
				offset += length;
			}
			// Если размер данных меньше смещения + 2 байт для cipher_suites_len, то это означает, что данные обрезаны
			if((offset + 2) > size){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("ClientHello truncated at cipher_suites_len", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("ClientHello truncated at cipher_suites_len", log_t::flag_t::WARNING);
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Получаем длину списка cipher_suites
			length = static_cast <size_t> (::local::u16(buffer + offset));
			// Увеличиваем смещение на 2 байта для длины cipher_suites
			offset += 2;
			// Если длина cipher_suites не кратна 2 или если размер данных меньше смещения + длины cipher_suites, то это означает, что данные обрезаны или некорректны
			if(((offset + length) > size) || ((length % 2) != 0)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("ClientHello invalid cipher_suites length (%zu)", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, length);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("ClientHello invalid cipher_suites length (%zu)", log_t::flag_t::WARNING, length);
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Значение шифра из ClientHello
			uint16_t cipher = 0;
			/**
			 * Перебираем весь список доступных шифров
			 */
			for(size_t i = 0; i < length; i += 2){
				// Извлекаем код шифра из буфера
				cipher = ::local::u16(buffer + (offset + i));
				// Если код шифра является GREASE
				if(::local::isGrease(cipher)){
					// Устанавливаем флаг grease в объекте browser
					browser.grease = true;
					// Получаем код шифра
					browser.ciphers.push_back(cipher_t::GREASE);
				// Если код шифра является одним из стандартных кодов из RFC 8446
				} else {
					/**
					 * Определяем код шифра
					 */
					switch(cipher){
						// Если код шифра соответствует AES128-SHA
						case 0x002F:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::AES128_SHA);
						break;
						// Если код шифра соответствует AES256-SHA
						case 0x0035:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::AES256_SHA);
						break;
						// Если код шифра соответствует AES128-GCM-SHA256
						case 0x009C:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::AES128_GCM_SHA256);
						break;
						// Если код шифра соответствует AES256-GCM-SHA384
						case 0x009D:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::AES256_GCM_SHA384);
						break;
						// Если код шифра соответствует PSK-AES128-CBC-SHA
						case 0x008C:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::PSK_AES128_CBC_SHA);
						break;
						// Если код шифра соответствует PSK-AES256-CBC-SHA
						case 0x008D:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::PSK_AES256_CBC_SHA);
						break;
						// Если код шифра соответствует ECDHE-RSA-AES128-SHA
						case 0xC013:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_AES128_SHA);
						break;
						// Если код шифра соответствует ECDHE-RSA-AES256-SHA
						case 0xC014:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_AES256_SHA);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-AES128-SHA
						case 0xC009:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_AES128_SHA);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-AES256-SHA
						case 0xC00A:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_AES256_SHA);
						break;
						// Если код шифра соответствует ECDHE-RSA-AES128-SHA256
						case 0xC027:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_AES128_SHA256);
						break;
						// Если код шифра соответствует ECDHE-PSK-AES128-CBC-SHA
						case 0xC035:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_PSK_AES128_CBC_SHA);
						break;
						// Если код шифра соответствует ECDHE-PSK-AES256-CBC-SHA
						case 0xC036:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_PSK_AES256_CBC_SHA);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-AES128-SHA256
						case 0xC023:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_AES128_SHA256);
						break;
						// Если код шифра соответствует ECDHE-RSA-AES128-GCM-SHA256
						case 0xC02F:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_AES128_GCM_SHA256);
						break;
						// Если код шифра соответствует ECDHE-RSA-AES256-GCM-SHA384
						case 0xC030:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_AES256_GCM_SHA384);
						break;
						// Если код шифра соответствует ECDHE-RSA-CHACHA20-POLY1305
						case 0xCCA8:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_CHACHA20_POLY1305);
						break;
						// Если код шифра соответствует ECDHE-PSK-CHACHA20-POLY1305
						case 0xCCAC:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_PSK_CHACHA20_POLY1305);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-AES128-GCM-SHA256
						case 0xC02B:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_AES128_GCM_SHA256);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-AES256-GCM-SHA384
						case 0xC02C:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_AES256_GCM_SHA384);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-CHACHA20-POLY1305
						case 0xCCA9:
							// Получаем код шифра
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_CHACHA20_POLY1305);
						break;
						// Если код шифра соответствует TLS_AES_128_GCM_SHA256
						case 0x1301:
							// Получаем код шифра
							browser.ciphers.push_back(tls::cipher_t::TLS_AES_128_GCM_SHA256);
						break;
						// Если код шифра соответствует TLS_AES_256_GCM_SHA384
						case 0x1302:
							// Получаем код шифра
							browser.ciphers.push_back(tls::cipher_t::TLS_AES_256_GCM_SHA384);
						break;
						// Если код шифра соответствует TLS_CHACHA20_POLY1305_SHA256
						case 0x1303:
							// Получаем код шифра
							browser.ciphers.push_back(tls::cipher_t::TLS_CHACHA20_POLY1305_SHA256);
						break;
						// Если код шифра не соответствует ни одному из известных
						default: browser.ciphers.push_back(cipher_t::UNKNOWN);
					}
				}
			}
			// Увеличиваем смещение на длину cipher_suites
			offset += length;
			// Если размер данных меньше смещения + 1 байт для compression_methods_len, то это означает, что данные обрезаны
			if((offset + 1) > size){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("ClientHello truncated at compression_methods_len", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("ClientHello truncated at compression_methods_len", log_t::flag_t::WARNING);
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Получаем длину списка compression_methods
			length = static_cast <size_t> (buffer[offset++]);
			// Если размер данных меньше смещения + длины compression_methods, то это означает, что данные обрезаны
			if((offset + length) > size){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("ClientHello truncated at compression_methods", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("ClientHello truncated at compression_methods", log_t::flag_t::WARNING);
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Если длина compression_methods не равна 1 или если первый байт compression_methods не равен 0x00 (null), то это не соответствует стандарту TLS
			if((length != 1) || (buffer[offset] != 0x00)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("ClientHello non-standard compression_methods (length=%zu, value=0x%02X)", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING, length, buffer[offset]);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("ClientHello non-standard compression_methods (length=%zu, value=0x%02X)", log_t::flag_t::WARNING, length, buffer[offset]);
				#endif
			}
			/**
			 * Перебираем весь список доступных методов компрессии.
			 * legacy_compression_methods: каждый элемент занимает 1 байт (uint8).
			 * Допустимые значения: 0x00 (null) и 0x01 (DEFLATE/zlib, устарело).
			 * Brotli/zstd здесь не существуют — они принадлежат расширению compress_certificate (0x001B).
			 */
			for(size_t i = 0; i < length; i += 1){
				/**
				 * Определяем код метода компрессии
				 */
				switch(buffer[offset + i]){
					// Если метод компрессии не задан (null)
					case 0x00:
						// Добавляем установленный метод компрессии в список поддерживаемых методов
						browser.compressors.push_back(compressor_t::NONE);
					break;
					// Если метод компрессии соответствует DEFLATE/zlib (RFC 3749, устарело, запрещено в TLS 1.3)
					case 0x01:
						// Добавляем установленный метод компрессии в список поддерживаемых методов
						browser.compressors.push_back(compressor_t::ZLIB);
					break;
					// Если метод компрессии не соответствует ни одному из известных
					default: browser.compressors.push_back(compressor_t::UNKNOWN);
				}
			}
			// Увеличиваем смещение на длину cipher_suites
			offset += length;
			// Если размер данных не хватает для извлечения списка расширений
			if((offset + 2) > size){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("ClientHello truncated at extensions_length", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("ClientHello truncated at extensions_length", log_t::flag_t::WARNING);
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Получаем длину списка расширений
			length = static_cast <size_t> (::local::u16(buffer + offset));
			// Увеличиваем смещение на 2 байта для длины расширений
			offset += 2;
			// Если размер данных меньше смещения + длины расширений, то это означает, что данные обрезаны
			if((offset + length) > size){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("ClientHello truncated inside extensions", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					this->_log->print("ClientHello truncated inside extensions", log_t::flag_t::WARNING);
				#endif
				// Выводим результат по умолчанию
				return result;
			}
			// Определяем конечное смещение для расширений, чтобы не выходить за пределы данных
			const size_t end = (offset + length);
			/**
			 * Перебираем все расширения, пока не достигнем конца данных расширений
			 * Каждое расширение состоит из 4 байт заголовка (2 байта для типа и 2 байта для длины) и данных указанной длины.
			 * Мы будем извлекать тип и длину каждого расширения, а затем обрабатывать данные в зависимости от типа расширения.
			 * Если мы обнаружим, что данные обрезаны внутри расширений, то мы выведем предупреждение и вернем результат по умолчанию.
			 * Для GREASE и неизвестных расширений мы просто пропустим данные и закроем объект без обработки.
			 * Для известных расширений мы вызовем соответствующую функцию парсинга, которая будет извлекать и отображать информацию о расширении.
			 * После обработки каждого расширения мы будем увеличивать смещение на длину данных расширения, чтобы перейти к следующему расширению.
			 * Этот процесс будет продолжаться до тех пор, пока мы не достигнем конечного смещения для расширений.
			 */
			while(offset < end){
				// Если размер данных меньше смещения + 4 байт для заголовка расширения, то это означает, что данные обрезаны внутри расширений
				if((offset + 4) > end)
					// Выходим из цикла обработки расширений
					break;
				// Извлекаем тип расширения из первых 2 байт заголовка расширения
				const uint16_t type = ::local::u16(buffer + offset);
				// Извлекаем размер данных расширения из следующих 2 байт заголовка расширения
				const uint16_t size = ::local::u16(buffer + (offset + 2));
				// Увеличиваем смещение на 4 байта для заголовка расширения
				offset += 4;
				// Если размер данных меньше смещения + размера данных расширения, то это означает, что данные обрезаны внутри расширений
				if((offset + size) > end)
					// Выходим из цикла обработки расширений
					break;
				// Если код алгоритма подписи является GREASE
				if(::local::isGrease(type))
					// Выполняем парсинг расширения GREASE, который просто пропускает данные и закрывает объект без обработки
					::fingerprint::parseGrease(buffer + offset, size, browser);
				/**
				 * Если код алгоритма подписи не является GREASE, то мы проверяем,
				 * соответствует ли он одному из известных типов расширений и вызываем соответствующую функцию парсинга для извлечения и отображения информации о расширении.
				 */
				else {
					/**
					 * Диспетчер по типам расширений:
					 * для каждого известного типа расширения мы вызываем соответствующую функцию парсинга,
					 * которая будет извлекать и отображать информацию о расширении.
					 * Для GREASE и неизвестных типов расширений мы просто пропускаем данные и закрываем объект без обработки.
					 */
					switch(type){
						// Если тип расширения соответствует server_name (RFC 6066 §3)
						case 0x0000:
							// Выполняем парсинг расширения server_name
							::fingerprint::parseServerName(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует max_fragment_length (RFC 6066 §4)
						case 0x0001:
							// Выполняем парсинг расширения max_fragment_length
							::fingerprint::parseMaxFragmentLength(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует status_request (OCSP, RFC 6066 §8)
						case 0x0005:
							// Выполняем парсинг расширения status_request
							::fingerprint::parseStatusRequest(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует supported_groups (RFC 8422, RFC 7919)
						case 0x000A:
							// Выполняем парсинг расширения supported_groups
							::fingerprint::parseSupportedGroups(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует ec_point_formats (RFC 8422 §5.1)
						case 0x000B:
							// Выполняем парсинг расширения ec_point_formats
							::fingerprint::parseECPointFormats(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует signature_algorithms (RFC 8446 §4.2.3)
						case 0x000D:
							// Выполняем парсинг расширения signature_algorithms
							::fingerprint::parseSignatureAlgorithms(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует use_srtp (RFC 5764 §4.2, DTLS)
						case 0x000E:
							// Выполняем парсинг расширения use_srtp
							::fingerprint::parseUseSRTP(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует heartbeat (RFC 6520)
						case 0x000F:
							// Выполняем парсинг расширения heartbeat
							::fingerprint::parseHeartbeat(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует application_layer_protocol_negotiation / ALPN (RFC 7301)
						case 0x0010:
							// Выполняем парсинг расширения application_layer_protocol_negotiation / ALPN
							::fingerprint::parseALPN(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует signed_certificate_timestamp / SCT (RFC 6962)
						case 0x0012:
							// Выполняем парсинг расширения signed_certificate_timestamp / SCT
							::fingerprint::parseCertificateTimestamp(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует padding (RFC 7685)
						case 0x0015:
							// Выполняем парсинг расширения padding
							::fingerprint::parsePadding(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует encrypt_then_mac (RFC 7366)
						case 0x0016:
							// Выполняем парсинг расширения encrypt_then_mac
							::fingerprint::parseEncryptThenMAC(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует extended_master_secret (RFC 7627)
						case 0x0017:
							// Выполняем парсинг расширения extended_master_secret
							::fingerprint::parseExtendedMasterSecret(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует compress_certificate (RFC 8879)
						case 0x001B:
							// Выполняем парсинг расширения compress_certificate
							::fingerprint::parseCompressCertificate(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует record_size_limit (RFC 8449)
						case 0x001C:
							// Выполняем парсинг расширения record_size_limit
							::fingerprint::parseRecordSizeLimit(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует delegated_credential (RFC 9345)
						case 0x0022:
							// Выполняем парсинг расширения delegated_credential
							::fingerprint::parseDelegatedCredential(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует session_ticket (RFC 5077)
						case 0x0023:
							// Выполняем парсинг расширения session_ticket
							::fingerprint::parseSessionTicket(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует pre_shared_key (RFC 8446 §4.2.11)
						case 0x0029:
							// Выполняем парсинг расширения pre_shared_key
							::fingerprint::parsePreSharedKey(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует early_data (RFC 8446 §4.2.10)
						case 0x002A:
							// Выполняем парсинг расширения early_data
							::fingerprint::parseEarlyData(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует supported_versions (RFC 8446 §4.2.1)
						case 0x002B:
							// Выполняем парсинг расширения supported_versions
							::fingerprint::parseSupportedVersions(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует cookie (RFC 8446 §4.2.2)
						case 0x002C:
							// Выполняем парсинг расширения cookie
							::fingerprint::parseCookie(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует psk_key_exchange_modes (RFC 8446 §4.2.8)
						case 0x002D:
							// Выполняем парсинг расширения psk_key_exchange_modes
							::fingerprint::parsePSKKeyExchangeModes(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует certificate_authorities (RFC 8446 §4.2.4)
						case 0x002F:
							// Выполняем парсинг расширения certificate_authorities
							::fingerprint::parseCertificateAuthorities(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует post_handshake_auth (RFC 8446 §4.2.9, пустое)
						case 0x0031:
							// Выполняем парсинг расширения post_handshake_auth
							::fingerprint::postHandshakeAuth(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует signature_algorithms_cert (RFC 8446 §4.2.3)
						case 0x0032:
							// Выполняем парсинг расширения signature_algorithms_cert
							::fingerprint::parseSignatureAlgorithmsCert(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует key_share (RFC 8446 §4.2.8)
						case 0x0033:
							// Выполняем парсинг расширения key_share
							::fingerprint::parseKeyShare(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует quic_transport_parameters (RFC 9001)
						case 0x0039:
							// Выполняем парсинг расширения quic_transport_parameters
							::fingerprint::parseQUICTransportParams(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует tls_flags (draft-ietf-tls-tlsflags)
						case 0x003E:
							// Выполняем парсинг расширения tls_flags
							::fingerprint::parseTLSFlags(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует next_proto_neg / NPN (Google, устарело, заменено на ALPN)
						case 0x3374:
							// Выполняем парсинг расширения next_proto_neg / NPN
							::fingerprint::parseNPN(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует application_settings_old (Chrome legacy ALPS, 0x4469)
						case 0x4469:
							// Выполняем парсинг расширения application_settings_old (ALPS legacy)
							::fingerprint::parseApplicationSettingsOld(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует application_settings / ALPS новый стандарт (0x44CD)
						case 0x44CD:
							// Выполняем парсинг расширения application_settings (ALPS)
							::fingerprint::parseApplicationSettings(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует channel_id (BoringSSL/Chrome, устарело, пустое в ClientHello)
						case 0x7550:
							// Выполняем парсинг расширения channel_id
							::fingerprint::parseChannelID(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует trust_anchors (BoringSSL draft)
						case 0xCA34:
							// Выполняем парсинг расширения trust_anchors
							::fingerprint::parseTrustAnchors(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует ech_outer_extensions (ECH draft)
						case 0xFD00:
							// Выполняем парсинг расширения ech_outer_extensions
							::fingerprint::parseECHOuterExtensions(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует extensionEncryptedClientHello / ECH (draft-ietf-tls-esni)
						case 0xFE0D:
							// Выполняем парсинг расширения Encrypted Client Hello / ECH
							::fingerprint::parseEncryptedClientHello(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует extensionRenegotiationInfo (RFC 5746)
						case 0xFF01:
							// Выполняем парсинг расширения renegotiation_info
							::fingerprint::parseRenegotiationInfo(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует quic_transport_parameters_legacy (BoringSSL legacy QUIC)
						case 0xFFA5:
							// Выполняем парсинг расширения quic_transport_parameters_legacy
							::fingerprint::parseQUICTransportParamsLegacy(buffer + offset, size, browser);
						break;
					}
				}
				// Увеличиваем смещение на размер данных расширения, чтобы перейти к следующему расширению
				offset += static_cast <size_t> (size);
			}


			cout << " =================== COUNT EXTENSIONS: " << browser.extensions.size() << " ===================" << endl;


			/*

			// 8. Extensions
			printf("\n[Next] extensions_length begins at offset %zu\n", off);
			fp_ext_start = off;
			parse_extensions_chrome_style(buffer, size, off);
			*/




	
			return ::parse(buffer, size);
		
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::tls::Fingerprint::Fingerprint(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {}
/**
 * @brief Деструктор
 *
 */
awh::tls::Fingerprint::~Fingerprint() noexcept {}
