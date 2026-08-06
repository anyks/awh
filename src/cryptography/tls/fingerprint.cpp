/**
 * @file: fingerprint.cpp
 * @date: 2026-04-28
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация модуля цифровых отпечатков TLS — формирование и разбор расширений ClientHello (SNI, ALPN,
 *        supported_groups, GREASE, Channel ID, OCSP,
 *        SCT и других) для эмуляции отпечатка клиента и анализа входящих рукопожатий
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Как это использует ТСПУ:
 *
 * ТСПУ (Технические Средства Противодействия Угрозам) — это DPI-системы ТСПУ Роскомнадзора. Они перехватывают TLS ClientHello до установки сессии и анализируют отпечатки по нескольким уровням:
 *
 * - 1. Блокировка по ja3Hash:
 *   Стандартный Chrome/Firefox имеет известный ja3Hash.
 *   Если хеш совпадает со списком "запрещённых" клиентов (например, Tor Browser, некоторые VPN-клиенты) — соединение сбрасывается через TCP RST.
 *
 * - 2. Детектирование подмены отпечатка:
 *   Инструменты типа GoodByeDPI и Zapret меняют поле tls.record или вставляют фрагментацию — ТСПУ сравнивает tls.record vs tls.negotiated.
 *   Если record=769 (TLS 1.0) а negotiated=772 (TLS 1.3) — аномалия, это признак обхода.
 *
 * - 3. Корреляция JA3 + SNI:
 *   JA4 включает флаг d/i (есть ли SNI). Если SNI отсутствует (i) — трафик подозрителен даже без блокировки по хешу.
 *
 * - 4. PeetPrint как дополнительный слой:
 *   PeetPrint фиксирует точное сочетание версий + компрессоров + групп + шифров с GREASE-маркерами.
 *   Это практически уникально для каждой версии браузерного движка, поэтому используется для:
 *     - идентификации конкретного браузера/библиотеки (например, отличить Chrome 117 от 118)
 *     - детектирования curl/golang клиентов, замаскированных под Chrome
 *
 * Пример:
 *  // Сравнить с известным эталонным отпечатком Chrome 120
 *  const string CHROME120_JA3H = "66918128f1b9b03303d77a6f...";
 *  if(imp.ja3Hash == CHROME120_JA3H)
 *      printf("Chrome 120\n");
 *
 * Базы эталонных отпечатков:
 * - JA3: https://ja3.zone — база известных ja3Hash → имя клиента
 * - JA4: https://github.com/FoxIO-LLC/ja4/tree/main/technical_details — спецификация и базы
 * - PeetPrint: https://github.com/nicowillis/PeetPrint — база браузерных отпечатков
 * - API fingerprint browser: https://tls.peet.ws/api/all
 *
 * P.S. Суть работы с ТСПУ: этот код даёт те же ja3Hash/ja4/peetprintHash что и коммерческие DPI — можно воспроизвести их логику детектирования на своей стороне.
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <cstdint>
#include <atomic>
#include <limits>
#include <cstring>
#include <sstream>
#include <iostream>
#include <algorithm>

/**
 * Заголовочные файлы OpenSSL
 */
#include <openssl/md5.h>
#include <openssl/sha.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <cryptography/tls/fingerprint.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические объекты в пространство имён локальных параметров
 *
 */
namespace local {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Тип блокировки хранилища отпечатков браузеров для совместного доступа
	 *
	 */
	using fgp_shared_lock_t = locker_t <std::shared_mutex>;
	/**
	 * @brief Тип блокировки хранилища отпечатков браузеров для эксклюзивного доступа
	 *
	 */
	using fgp_exclusive_lock_t = locker_t <std::shared_mutex>;

	/**
	 * @brief Режим совместного доступа к хранилищу отпечатков браузеров
	 *
	 */
	constexpr auto fgp_shared = fgp_shared_lock_t::mode_t::SHARED;
	/**
	 * @brief Режим эксклюзивной блокировки хранилища отпечатков браузеров
	 *
	 */
	constexpr auto fgp_exclusive = fgp_exclusive_lock_t::mode_t::EXCLUSIVE;
};

/**
 * @brief Инкапсулируем статические объекты в пространство имён локальных вспомогательных функций
 *
 */
namespace local {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Вспомогательная функция проверки GREASE-значений
	 *
	 * @param value 16-битное значение для проверки
	 * @return      true, если значение является GREASE-значением, иначе false
	 *
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
	 *
	 */
	static inline uint16_t u16(const uint8_t * buffer) noexcept {
		// Читаем 16-битное значение из буфера в формате big-endian
		return ((static_cast <uint16_t> (buffer[0]) << 8) | buffer[1]);
	}

	/**
	 * @brief Вычисляет полный размер TLS/DTLS record layer
	 *
	 * @param buffer буфер с данными record layer
	 * @param size   размер буфера в байтах
	 * @return       полный размер record layer или 0, если данных недостаточно
	 *
	 */
	static inline size_t recordLayerSize(const uint8_t * buffer, const size_t size) noexcept {
		// Если буфер пустой или слишком короткий
		if((buffer == nullptr) || (size < 3u))
			// Возвращаем 0 — размер определить нельзя
			return 0;
		// Получаем версию протокола из заголовка record layer
		const uint16_t version = u16(buffer + 1);
		// Если это DTLS record layer
		if((version == 0xFEFFu) || (version == 0xFEFDu)){
			// Если данных недостаточно для DTLS record header
			if(size < 13u)
				// Возвращаем 0 — размер определить нельзя
				return 0;
			// Возвращаем полный размер DTLS record layer
			return (13u + static_cast <size_t> (u16(buffer + 11)));
		}
		// Если данных недостаточно для TLS record header
		if(size < 5u)
			// Возвращаем 0 — размер определить нельзя
			return 0;
		// Возвращаем полный размер TLS record layer
		return (5u + static_cast <size_t> (u16(buffer + 3)));
	}

	/**
	 * @brief Вспомогательная функция вычисления MD5 строки
	 *
	 * @param input входная строка
	 * @return      MD5 hex lowercase
	 *
	 */
	static string md5(const string & input) noexcept {
		// Переменная результата
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
	 *
	 */
	static string tohex(const uint8_t * buffer, const size_t size) noexcept {
		// Переменная результата
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
	 *
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
	 *
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
	 *
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
	 *
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
	 *
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
			// Если тип расширения соответствует client_certificate_type
			case static_cast <uint8_t> (awh::tls::extension_type_t::CLIENT_CERTIFICATE_TYPE):
				// Возвращаем TLS wire-код для client_certificate_type
				return 0x0013;
			// Если тип расширения соответствует server_certificate_type
			case static_cast <uint8_t> (awh::tls::extension_type_t::SERVER_CERTIFICATE_TYPE):
				// Возвращаем TLS wire-код для server_certificate_type
				return 0x0014;
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
			// Если тип расширения соответствует oid_filters
			case static_cast <uint8_t> (awh::tls::extension_type_t::OID_FILTERS):
				// Возвращаем TLS wire-код для oid_filters
				return 0x0030;
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
			// Если тип расширения соответствует transparency_info
			case static_cast <uint8_t> (awh::tls::extension_type_t::TRANSPARENCY_INFO):
				// Возвращаем TLS wire-код для transparency_info
				return 0x0035;
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
	 *
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
	 *
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
			// Если метод компрессии соответствует Brotli
			case static_cast <uint8_t> (awh::tls::compressor_t::BROTLI):
				// Возвращаем wire-код для Brotli
				return 0x02;
			// Если метод компрессии соответствует ZStandard (Zstd)
			case static_cast <uint8_t> (awh::tls::compressor_t::ZSTD):
				// Возвращаем wire-код для ZStandard (Zstd)
				return 0x03;
			// Если метод компрессии не определён, возвращаем неизвестный код
			default: return 0xFF;
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения имени для кода метода компрессии (legacy_compression_methods)
	 *
	 * @param id код метода компрессии
	 * @return   имя метода компрессии
	 *
	 */
	static const char * compressALGName(const uint8_t id) noexcept {
		/**
		 * Определяем имя для кода метода компрессии (legacy_compression_methods)
		 */
		switch(id){
			// Если код метода компрессии соответствует Zlib
			case 0x01:
				// Возвращаем строку "Zlib" для метода компрессии
				return "Zlib";
			// Если код метода компрессии соответствует Brotli
			case 0x02:
				// Возвращаем строку "Brotli" для метода компрессии
				return "Brotli";
			// Если код метода компрессии соответствует ZStandard (Zstd)
			case 0x03:
				// Возвращаем строку "Zstandard (ZSTD)" для метода компрессии
				return "Zstandard (ZSTD)";
			// Если код метода компрессии не распознан, возвращаем "UNKNOWN"
			default: return "UNKNOWN";
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения имени для кода версии TLS
	 *
	 * @param version код версии TLS
	 * @return        имя версии TLS
	 *
	 */
	static const char * tlsVersionName(const uint16_t version) noexcept {
		// Если версия является GREASE, возвращаем "GREASE"
		if(::local::isGrease(version))
			// Возвращаем строку "GREASE" для GREASE версий
			return "[GREASE]";
		/**
		 * Определяем имя для кода версии TLS
		 */
		switch(version){
			// Если код версии соответствует SSL 3.0, возвращаем "SSLv3"
			case 0x0300: return "SSLv3";
			// Если код версии соответствует TLS 1.0, возвращаем "TLSv1.0"
			case 0x0301: return "TLSv1.0";
			// Если код версии соответствует TLS 1.1, возвращаем "TLSv1.1"
			case 0x0302: return "TLSv1.1";
			// Если код версии соответствует TLS 1.2, возвращаем "TLSv1.2"
			case 0x0303: return "TLSv1.2";
			// Если код версии соответствует TLS 1.3, возвращаем "TLSv1.3"
			case 0x0304: return "TLSv1.3";
			// Если код версии соответствует DTLS 1.0, возвращаем "DTLSv1.0"
			case 0xFEFF: return "DTLSv1.0";
			// Если код версии соответствует DTLS 1.2, возвращаем "DTLSv1.2"
			case 0xFEFD: return "DTLSv1.2";
			// Если код версии не распознан, возвращаем "UNKNOWN"
			default: return "UNKNOWN";
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения имени для кода группы эллиптических кривых
	 *
	 * @param gid код группы эллиптических кривых
	 * @return    имя группы эллиптических кривых
	 *
	 */
	static const char * groupName(const uint16_t gid) noexcept {
		// Если код группы является GREASE, возвращаем "GREASE"
		if(::local::isGrease(gid))
			// Возвращаем строку "GREASE" для GREASE кодов групп эллиптических кривых
			return "[GREASE]";
		/**
		 * Определяем код группы
		 */
		switch(gid){
			// Если эллиптическая кривая соответствует P-256 (secp256r1)
			case 0x0017:
				// Возвращаем строку "P-256 (secp256r1)" для группы эллиптических кривых
				return "P-256 (secp256r1)";
			break;
			// Если эллиптическая кривая соответствует P-384 (secp384r1)
			case 0x0018:
				// Возвращаем строку "P-384 (secp384r1)" для группы эллиптических кривых
				return "P-384 (secp384r1)";
			break;
			// Если эллиптическая кривая соответствует P-521 (secp521r1)
			case 0x0019:
				// Возвращаем строку "P-521 (secp521r1)" для группы эллиптических кривых
				return "P-521 (secp521r1)";
			break;
			// Если эллиптическая кривая соответствует X25519
			case 0x001D:
				// Возвращаем строку "X25519" для группы эллиптических кривых
				return "X25519";
			break;
			// Если эллиптическая кривая соответствует X448
			case 0x001E:
				// Возвращаем строку "X448" для группы эллиптических кривых
				return "X448";
			break;
			// Если эллиптическая кривая соответствует secp256k1
			case 0x001C:
				// Возвращаем строку "secp256k1" для группы эллиптических кривых
				return "secp256k1";
			break;
			// Если эллиптическая кривая соответствует FFDHE 2048
			case 0x0100:
				// Возвращаем строку "ffdhe2048" для группы эллиптических кривых
				return "ffdhe2048";
			break;
			// Если эллиптическая кривая соответствует FFDHE 3072
			case 0x0101:
				// Возвращаем строку "ffdhe3072" для группы эллиптических кривых
				return "ffdhe3072";
			break;
			// Если эллиптическая кривая соответствует FFDHE 4096
			case 0x0102:
				// Возвращаем строку "ffdhe4096" для группы эллиптических кривых
				return "ffdhe4096";
			break;
			// Если эллиптическая кривая соответствует FFDHE 6144
			case 0x0103:
				// Возвращаем строку "ffdhe6144" для группы эллиптических кривых
				return "ffdhe6144";
			break;
			// Если эллиптическая кривая соответствует FFDHE 8192
			case 0x0104:
				// Возвращаем строку "ffdhe8192" для группы эллиптических кривых
				return "ffdhe8192";
			break;
			// Если эллиптическая кривая соответствует MLKEM 1024
			case 0x0202:
				// Возвращаем строку "mlkem1024" для группы эллиптических кривых
				return "mlkem1024";
			break;
			// Если эллиптическая кривая соответствует X25519Kyber768Draft00
			case 0x6399:
				// Возвращаем строку "X25519Kyber768Draft00" для группы эллиптических кривых
				return "X25519Kyber768Draft00";
			break;
			// Если эллиптическая кривая соответствует X25519MLKEM768
			case 0x11EC:
				// Возвращаем строку "X25519MLKEM768" для группы эллиптических кривых
				return "X25519MLKEM768";
			break;
			// Если эллиптическая кривая не соответствует ни одной из известных, добавляем код UNKNOWN в список поддерживаемых групп эллиптических кривых браузера
			default: return "UNKNOWN";
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения имени для кода алгоритма подписи
	 *
	 * @param id код алгоритма подписи
	 * @return   имя алгоритма подписи
	 *
	 */
	static const char * signatureName(const uint16_t id) noexcept {
		// Если код алгоритма подписи является GREASE, возвращаем "GREASE"
		if(::local::isGrease(id))
			// Возвращаем строку "GREASE" для GREASE кодов алгоритмов подписи
			return "[GREASE]";
		/**
		 * Определяем имя для кода алгоритма подписи
		 */
		switch(id){
			// Если код алгоритма подписи соответствует RSA_PKCS1_SHA1
			case 0x0201:
				// Возвращаем строку "rsa_pkcs1_sha1" для алгоритма подписи
				return "rsa_pkcs1_sha1";
			// Если код алгоритма подписи соответствует RSA_PKCS1_SHA256
			case 0x0401:
				// Возвращаем строку "rsa_pkcs1_sha256" для алгоритма подписи
				return "rsa_pkcs1_sha256";
			// Если код алгоритма подписи соответствует RSA_PKCS1_SHA384
			case 0x0501:
				// Возвращаем строку "rsa_pkcs1_sha384" для алгоритма подписи
				return "rsa_pkcs1_sha384";
			// Если код алгоритма подписи соответствует RSA_PKCS1_SHA512
			case 0x0601:
				// Возвращаем строку "rsa_pkcs1_sha512" для алгоритма подписи
				return "rsa_pkcs1_sha512";
			// Если код алгоритма подписи соответствует ECDSA_SHA1
			case 0x0203:
				// Возвращаем строку "ecdsa_sha1" для алгоритма подписи
				return "ecdsa_sha1";
			// Если код алгоритма подписи соответствует ECDSA_SECP256R1_SHA256
			case 0x0403:
				// Возвращаем строку "ecdsa_secp256r1_sha256" для алгоритма подписи
				return "ecdsa_secp256r1_sha256";
			// Если код алгоритма подписи соответствует ECDSA_SECP384R1_SHA384
			case 0x0503:
				// Возвращаем строку "ecdsa_secp384r1_sha384" для алгоритма подписи
				return "ecdsa_secp384r1_sha384";
			// Если код алгоритма подписи соответствует ECDSA_SECP521R1_SHA512
			case 0x0603:
				// Возвращаем строку "ecdsa_secp521r1_sha512" для алгоритма подписи
				return "ecdsa_secp521r1_sha512";
			// Если код алгоритма подписи соответствует RSA_PSS_RSAE_SHA256
			case 0x0804:
				// Возвращаем строку "rsa_pss_rsae_sha256" для алгоритма подписи
				return "rsa_pss_rsae_sha256";
			// Если код алгоритма подписи соответствует RSA_PSS_RSAE_SHA384
			case 0x0805:
				// Возвращаем строку "rsa_pss_rsae_sha384" для алгоритма подписи
				return "rsa_pss_rsae_sha384";
			// Если код алгоритма подписи соответствует RSA_PSS_RSAE_SHA512
			case 0x0806:
				// Возвращаем строку "rsa_pss_rsae_sha512" для алгоритма подписи
				return "rsa_pss_rsae_sha512";
			// Если код алгоритма подписи соответствует EdDSA-25519
			case 0x0807:
				// Возвращаем строку "ed25519" для алгоритма подписи
				return "ed25519";
			// Если код алгоритма подписи соответствует EdDSA-448
			case 0x0808:
				// Возвращаем строку "ed448" для алгоритма подписи
				return "ed448";
			// Если код алгоритма подписи соответствует RSA_PSS_PSS_SHA256
			case 0x0809:
				// Возвращаем строку "rsa_pss_pss_sha256" для алгоритма подписи
				return "rsa_pss_pss_sha256";
			// Если код алгоритма подписи соответствует RSA_PSS_PSS_SHA384
			case 0x080A:
				// Возвращаем строку "rsa_pss_pss_sha384" для алгоритма подписи
				return "rsa_pss_pss_sha384";
			// Если код алгоритма подписи соответствует RSA_PSS_PSS_SHA512
			case 0x080B:
				// Возвращаем строку "rsa_pss_pss_sha512" для алгоритма подписи
				return "rsa_pss_pss_sha512";
			// Если код алгоритма подписи соответствует DSA_SHA1
			case 0x0202:
				// Возвращаем строку "dsa_sha1" для алгоритма подписи
				return "dsa_sha1";
			// Если код алгоритма подписи соответствует RSA_PKCS1_MD5_SHA1
			case 0xFF01:
				// Возвращаем строку "rsa_pkcs1_md5_sha1" для алгоритма подписи
				return "rsa_pkcs1_md5_sha1";
			// Если код алгоритма подписи соответствует RSA_PKCS1_SHA256_LEGACY
			case 0x0420:
				// Возвращаем строку "rsa_pkcs1_sha256_legacy" для алгоритма подписи
				return "rsa_pkcs1_sha256_legacy";
			// Если код алгоритма подписи не распознан, возвращаем "UNKNOWN"
			default: return "UNKNOWN";
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения имени для кода шифра TLS
	 *
	 * @param id код шифра TLS
	 * @return   имя шифра TLS
	 *
	 */
	static const char * cipherName(const uint16_t id) noexcept {
		// Если код шифра является GREASE, возвращаем "GREASE"
		if(::local::isGrease(id))
			// Возвращаем строку "GREASE" для GREASE кодов шифров
			return "[GREASE]";
		/**
		 * Определяем код шифра
		 */
		switch(id){
			// Если код шифра соответствует AES128-SHA
			case 0x002F:
				// Возвращаем имя шифра
				return "AES128-SHA";
			break;
			// Если код шифра соответствует AES256-SHA
			case 0x0035:
				// Возвращаем имя шифра
				return "AES256-SHA";
			break;
			// Если код шифра соответствует AES128-GCM-SHA256
			case 0x009C:
				// Возвращаем имя шифра
				return "AES128-GCM-SHA256";
			break;
			// Если код шифра соответствует AES256-GCM-SHA384
			case 0x009D:
				// Возвращаем имя шифра
				return "AES256-GCM-SHA384";
			break;
			// Если код шифра соответствует PSK-AES128-CBC-SHA
			case 0x008C:
				// Возвращаем имя шифра
				return "PSK-AES128-CBC-SHA";
			break;
			// Если код шифра соответствует PSK-AES256-CBC-SHA
			case 0x008D:
				// Возвращаем имя шифра
				return "PSK-AES256-CBC-SHA";
			break;
			// Если код шифра соответствует ECDHE-RSA-AES128-SHA
			case 0xC013:
				// Возвращаем имя шифра
				return "ECDHE-RSA-AES128-SHA";
			break;
			// Если код шифра соответствует ECDHE-RSA-AES256-SHA
			case 0xC014:
				// Возвращаем имя шифра
				return "ECDHE-RSA-AES256-SHA";
			break;
			// Если код шифра соответствует ECDHE-ECDSA-AES128-SHA
			case 0xC009:
				// Возвращаем имя шифра
				return "ECDHE-ECDSA-AES128-SHA";
			break;
			// Если код шифра соответствует ECDHE-ECDSA-AES256-SHA
			case 0xC00A:
				// Возвращаем имя шифра
				return "ECDHE-ECDSA-AES256-SHA";
			break;
			// Если код шифра соответствует ECDHE-RSA-AES128-SHA256
			case 0xC027:
				// Возвращаем имя шифра
				return "ECDHE-RSA-AES128-SHA256";
			break;
			// Если код шифра соответствует ECDHE-PSK-AES128-CBC-SHA
			case 0xC035:
				// Возвращаем имя шифра
				return "ECDHE-PSK-AES128-CBC-SHA";
			break;
			// Если код шифра соответствует ECDHE-PSK-AES256-CBC-SHA
			case 0xC036:
				// Возвращаем имя шифра
				return "ECDHE-PSK-AES256-CBC-SHA";
			break;
			// Если код шифра соответствует ECDHE-ECDSA-AES128-SHA256
			case 0xC023:
				// Возвращаем имя шифра
				return "ECDHE-ECDSA-AES128-SHA256";
			break;
			// Если код шифра соответствует ECDHE-RSA-AES128-GCM-SHA256
			case 0xC02F:
				// Возвращаем имя шифра
				return "ECDHE-RSA-AES128-GCM-SHA256";
			break;
			// Если код шифра соответствует ECDHE-RSA-AES256-GCM-SHA384
			case 0xC030:
				// Возвращаем имя шифра
				return "ECDHE-RSA-AES256-GCM-SHA384";
			break;
			// Если код шифра соответствует ECDHE-RSA-CHACHA20-POLY1305
			case 0xCCA8:
				// Возвращаем имя шифра
				return "ECDHE-RSA-CHACHA20-POLY1305";
			break;
			// Если код шифра соответствует ECDHE-PSK-CHACHA20-POLY1305
			case 0xCCAC:
				// Возвращаем имя шифра
				return "ECDHE-PSK-CHACHA20-POLY1305";
			break;
			// Если код шифра соответствует ECDHE-ECDSA-AES128-GCM-SHA256
			case 0xC02B:
				// Возвращаем имя шифра
				return "ECDHE-ECDSA-AES128-GCM-SHA256";
			break;
			// Если код шифра соответствует ECDHE-ECDSA-AES256-GCM-SHA384
			case 0xC02C:
				// Возвращаем имя шифра
				return "ECDHE-ECDSA-AES256-GCM-SHA384";
			break;
			// Если код шифра соответствует ECDHE-ECDSA-CHACHA20-POLY1305
			case 0xCCA9:
				// Возвращаем имя шифра
				return "ECDHE-ECDSA-CHACHA20-POLY1305";
			break;
			// Если код шифра соответствует TLS_AES_128_GCM_SHA256
			case 0x1301:
				// Возвращаем имя шифра
				return "TLS_AES_128_GCM_SHA256";
			break;
			// Если код шифра соответствует TLS_AES_256_GCM_SHA384
			case 0x1302:
				// Возвращаем имя шифра
				return "TLS_AES_256_GCM_SHA384";
			break;
			// Если код шифра соответствует TLS_CHACHA20_POLY1305_SHA256
			case 0x1303:
				// Возвращаем имя шифра
				return "TLS_CHACHA20_POLY1305_SHA256";
			break;
			// Если код шифра не соответствует ни одному из известных
			default: return "UNKNOWN";
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения имени для кода расширения TLS
	 *
	 * @param type код расширения TLS
	 * @return     имя расширения TLS
	 *
	 */
	static const char * extensionName(const uint16_t type) noexcept {
		// Если код расширения является GREASE, возвращаем "GREASE"
		if(::local::isGrease(type))
			// Возвращаем строку "GREASE" для GREASE кодов расширений
			return "[GREASE]";
		/**
		 * Определяем имя для кода расширения TLS
		 */
		switch(type){
			// Если код расширения соответствует server_name, возвращаем "server_name" (RFC 6066)
			case 0x0000:
				// Возвращаем строку "server_name" для расширения
				return "server_name";
			// Если код расширения соответствует max_fragment_length (RFC 6066)
			case 0x0001:
				// Возвращаем строку "max_fragment_length" для расширения
				return "max_fragment_length";
			// Если код расширения соответствует status_request (RFC 6066) (OCSP)
			case 0x0005:
				// Возвращаем строку "status_request" для расширения
				return "status_request";
			// Если код расширения соответствует supported_groups (RFC 8422)
			case 0x000A:
				// Возвращаем строку "supported_groups" для расширения
				return "supported_groups";
			// Если код расширения соответствует ec_point_formats (RFC 8422)
			case 0x000B:
				// Возвращаем строку "ec_point_formats" для расширения
				return "ec_point_formats";
			// Если код расширения соответствует signature_algorithms (RFC 8446)
			case 0x000D:
				// Возвращаем строку "signature_algorithms" для расширения
				return "signature_algorithms";
			// Если код расширения соответствует use_srtp (RFC 5764)
			case 0x000E:
				// Возвращаем строку "use_srtp" для расширения
				return "use_srtp";
			// Если код расширения соответствует heartbeat (RFC 6520)
			case 0x000F:
				// Возвращаем строку "heartbeat" для расширения
				return "heartbeat";
			// Если код расширения соответствует application_layer_protocol_negotiation (RFC 7301)
			case 0x0010:
				// Возвращаем строку "application_layer_protocol_negotiation" для расширения
				return "application_layer_protocol_negotiation";
			// Если код расширения соответствует signed_certificate_timestamp (RFC 6962)
			case 0x0012:
				// Возвращаем строку "signed_certificate_timestamp" для расширения
				return "signed_certificate_timestamp";
			// Если код расширения соответствует client_certificate_type (RFC 7250)
			case 0x0013:
				// Возвращаем строку "client_certificate_type" для расширения
				return "client_certificate_type";
			// Если код расширения соответствует server_certificate_type (RFC 7250)
			case 0x0014:
				// Возвращаем строку "server_certificate_type" для расширения
				return "server_certificate_type";
			// Если код расширения соответствует padding (RFC 7685)
			case 0x0015:
				// Возвращаем строку "padding" для расширения
				return "padding";
			// Если код расширения соответствует encrypt_then_mac (RFC 7366)
			case 0x0016:
				// Возвращаем строку "encrypt_then_mac" для расширения
				return "encrypt_then_mac";
			// Если код расширения соответствует extended_master_secret (RFC 7627)
			case 0x0017:
				// Возвращаем строку "extended_master_secret" для расширения
				return "extended_master_secret";
			// Если код расширения соответствует compress_certificate (RFC 8879)
			case 0x001B:
				// Возвращаем строку "compress_certificate" для расширения
				return "compress_certificate";
			// Если код расширения соответствует record_size_limit (RFC 8449)
			case 0x001C:
				// Возвращаем строку "record_size_limit" для расширения
				return "record_size_limit";
			// Если код расширения соответствует delegated_credential (RFC 9345)
			case 0x0022:
				// Возвращаем строку "delegated_credential" для расширения
				return "delegated_credential";
			// Если код расширения соответствует session_ticket (RFC 5077)
			case 0x0023:
				// Возвращаем строку "session_ticket" для расширения
				return "session_ticket";
			// Если код расширения соответствует pre_shared_key (RFC 8446)
			case 0x0029:
				// Возвращаем строку "pre_shared_key" для расширения
				return "pre_shared_key";
			// Если код расширения соответствует early_data (RFC 8446)
			case 0x002A:
				// Возвращаем строку "early_data" для расширения
				return "early_data";
			// Если код расширения соответствует supported_versions (RFC 8446)
			case 0x002B:
				// Возвращаем строку "supported_versions" для расширения
				return "supported_versions";
			// Если код расширения соответствует cookie (RFC 8446)
			case 0x002C:
				// Возвращаем строку "cookie" для расширения
				return "cookie";
			// Если код расширения соответствует psk_key_exchange_modes (RFC 8446)
			case 0x002D:
				// Возвращаем строку "psk_key_exchange_modes" для расширения
				return "psk_key_exchange_modes";
			// Если код расширения соответствует certificate_authorities (RFC 8446)
			case 0x002F:
				// Возвращаем строку "certificate_authorities" для расширения
				return "certificate_authorities";
			// Если код расширения соответствует oid_filters (RFC 8446)
			case 0x0030:
				// Возвращаем строку "oid_filters" для расширения
				return "oid_filters";
			// Если код расширения соответствует post_handshake_auth (RFC 8446)
			case 0x0031:
				// Возвращаем строку "post_handshake_auth" для расширения
				return "post_handshake_auth";
			// Если код расширения соответствует signature_algorithms_cert (RFC 8446)
			case 0x0032:
				// Возвращаем строку "signature_algorithms_cert" для расширения
				return "signature_algorithms_cert";
			// Если код расширения соответствует key_share (RFC 8446)
			case 0x0033:
				// Возвращаем строку "key_share" для расширения
				return "key_share";
			// Если код расширения соответствует transparency_info (редко)
			case 0x0035:
				// Возвращаем строку "transparency_info" для расширения
				return "transparency_info";
			// Если код расширения соответствует quic_transport_parameters (RFC 9001)
			case 0x0039:
				// Возвращаем строку "quic_transport_parameters" для расширения
				return "quic_transport_parameters";
			// Если код расширения соответствует tls_flags (draft)
			case 0x003E:
				// Возвращаем строку "tls_flags" для расширения
				return "tls_flags";
			// Если код расширения соответствует next_proto_neg (NPN, предшественник ALPN)
			case 0x3374:
				// Возвращаем строку "next_proto_neg" для расширения
				return "next_proto_neg";
			// Если код расширения соответствует application_settings_old (Chrome legacy ALPS)
			case 0x4469:
				// Возвращаем строку "application_settings_old" для расширения
				return "application_settings_old";
			// Если код расширения соответствует application_settings (ALPS новый стандарт)
			case 0x44CD:
				// Возвращаем строку "application_settings" для расширения
				return "application_settings";
			// Если код расширения соответствует channel_id (BoringSSL)
			case 0x7550:
				// Возвращаем строку "channel_id" для расширения
				return "channel_id";
			// Если код расширения соответствует trust_anchors (BoringSSL draft)
			case 0xCA34:
				// Возвращаем строку "trust_anchors" для расширения
				return "trust_anchors";
			// Если код расширения соответствует ech_outer_extensions (ECH outer)
			case 0xFD00:
				// Возвращаем строку "ech_outer_extensions" для расширения
				return "ech_outer_extensions";
			// Если код расширения соответствует extensionEncryptedClientHello (ECH / GREASE)
			case 0xFE0D:
				// Возвращаем строку "extensionEncryptedClientHello" для расширения
				return "extensionEncryptedClientHello";
			// Если код расширения соответствует extensionRenegotiationInfo (RFC 5746)
			case 0xFF01:
				// Возвращаем строку "extensionRenegotiationInfo" для расширения
				return "extensionRenegotiationInfo";
			// Если код расширения соответствует quic_transport_parameters_legacy (BoringSSL legacy QUIC)
			case 0xFFA5:
				// Возвращаем строку "quic_transport_parameters_legacy" для расширения
				return "quic_transport_parameters_legacy";
			// Если код расширения не распознан, возвращаем "UNKNOWN"
			default: return "UNKNOWN";
		}
	}
};

/**
 * @brief Инкапсулируем статические объекты в пространство имён основных функций парсинга
 *
 */
namespace fingerprint {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Вспомогательная функция чтения QUIC variable-length integer (RFC 9000 §16)
	 *
	 * @param buffer бинарный буфер данных
	 * @param size   размер буфера
	 * @param offset текущее смещение (увеличивается на количество прочитанных байт)
	 * @return       прочитанное значение, или 0 если данных недостаточно
	 *
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
	 *
	 * @note Заполняет map<type_id, value>. Значение читается как QUIC varint если помещается (≤8 байт), иначе 0.
	 *
	 * @param buffer бинарный буфер данных расширения
	 * @param size   размер буфера
	 * @param params карта для сохранения результатов
	 *
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
	 *
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
	 *
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
	 *
	 */
	static void parseCertificateTimestamp(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение signed_certificate_timestamp в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_signed_certificate_timestamp_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения client_certificate_type (RFC 7250)
	 *
	 * @param buffer  бинарный буфер с данными расширения client_certificate_type
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
	 */
	static void parseClientCertificateType(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение client_certificate_type в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_client_certificate_type_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения server_certificate_type (RFC 7250)
	 *
	 * @param buffer  бинарный буфер с данными расширения server_certificate_type
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
	 */
	static void parseServerCertificateType(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение server_certificate_type в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_server_certificate_type_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения extended_master_secret (RFC 7627)
	 *
	 * @param buffer  бинарный буфер с данными расширения extended_master_secret
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
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
	 *
	 */
	static void postHandshakeAuth(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение post_handshake_auth в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_post_handshake_auth_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения oid_filters (RFC 8446 §4.2.5, пустое)
	 *
	 * @param buffer  бинарный буфер с данными расширения oid_filters
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
	 */
	static void parseOIDFilters(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение oid_filters в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_oid_filters_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения transparency_info (RFC 6962, пустое)
	 *
	 * @param buffer  бинарный буфер с данными расширения transparency_info
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
	 */
	static void parseTransparencyInfo(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение transparency_info в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_transparency_info_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения record_size_limit (RFC 8449)
	 *
	 * @param buffer  бинарный буфер с данными расширения record_size_limit
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
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
	 *
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
	 *
	 */
	static void parseSessionTicket(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение session_ticket в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_session_ticket_t> ());
		// Определяем размер данных расширения session_ticket (не более 32 байта)
		const size_t bytes = ::min <size_t> (32, size);
		// Если размер данных расширения session_ticket больше 0, то продолжаем парсинг
		if(bytes > 0){
			// Забиваем данные расширения нулями, чтобы гарантировать наличие данных в случае, если расширение session_ticket пустое
			awh_cast <awh::tls::fgp_t::extension_session_ticket_t *> (browser.extensions.back().get())->data.resize(bytes, 0);
			// Копируем данные расширения session_ticket из буфера в блок данных расширения session_ticket
			::memcpy(&awh_cast <awh::tls::fgp_t::extension_session_ticket_t *> (browser.extensions.back().get())->data[0], buffer, bytes);
		}
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения early_data (RFC 8446 §4.2.10)
	 *
	 * @param buffer  бинарный буфер с данными расширения early_data
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
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
	 *
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
	 *
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
	 *
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
				// Добавляем GREASE-запись в список обмена ключами браузера (сохраняем ключевой материал для корректного воспроизведения в apply)
				awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::GREASE, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
			// Если код группы является одной из стандартных версий из RFC 8446 §4.2.7
			else {
				/**
				 * Определяем код группы
				 */
				switch(gid){
					// Если эллиптическая кривая соответствует P-256 (secp256r1)
					case 0x0017:
						// Добавляем код группы эллиптической кривой P-256 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::P_256, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует P-384 (secp384r1)
					case 0x0018:
						// Добавляем код группы эллиптической кривой P-384 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::P_384, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует P-521 (secp521r1)
					case 0x0019:
						// Добавляем код группы эллиптической кривой P-521 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::P_521, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует X25519
					case 0x001D:
						// Добавляем код группы эллиптической кривой X25519 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::X25519, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует X448
					case 0x001E:
						// Добавляем код группы эллиптической кривой X448 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::X448, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует secp256k1
					case 0x001C:
						// Добавляем код группы эллиптической кривой secp256k1 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::SECP256K1, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует FFDHE 2048
					case 0x0100:
						// Добавляем код группы FFDHE 2048 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::FFDHE2048, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует FFDHE 3072
					case 0x0101:
						// Добавляем код группы FFDHE 3072 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::FFDHE3072, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует FFDHE 4096
					case 0x0102:
						// Добавляем код группы FFDHE 4096 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::FFDHE4096, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует FFDHE 6144
					case 0x0103:
						// Добавляем код группы FFDHE 6144 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::FFDHE6144, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует FFDHE 8192
					case 0x0104:
						// Добавляем код группы FFDHE 8192 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::FFDHE8192, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует MLKEM 1024
					case 0x0202:
						// Добавляем код группы MLKEM 1024 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::MLKEM1024, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует X25519Kyber768Draft00
					case 0x6399:
						// Добавляем код группы X25519Kyber768Draft00 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::X25519_KYBER768_DRAFT00, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая соответствует X25519MLKEM768
					case 0x11EC:
						// Добавляем код группы X25519MLKEM768 в список поддерживаемых групп эллиптических кривых браузера
						awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::X25519_MLKEM768, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
					break;
					// Если эллиптическая кривая не соответствует ни одной из известных, добавляем код UNKNOWN в список поддерживаемых групп эллиптических кривых браузера
					default: awh_cast <awh::tls::fgp_t::extension_key_share_t *> (browser.extensions.back().get())->shares.emplace_back(awh::tls::group_t::UNKNOWN, vector <uint8_t> (buffer + offset, buffer + (offset + length)));
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
	 *
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
		/**
		 * Выполняем парсинг поддерживаемых режимов обмена ключами PSK из данных расширения
		 */
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
	 *
	 */
	static void parseSupportedVersions(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение supported_versions в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_supported_versions_t> ());
		// Если размер данных в буфере меньше 1 байта, то данных недостаточно для парсинга
		if(size < 1)
			// Выходим из функции
			return;
		// Получаем байтовую длину списка поддерживаемых версий из первого байта данных расширения
		const uint8_t count = buffer[0];
		/**
		 * count — байтовая длина списка версий (RFC 8446 §4.2.1: versions<2..254> = 1-байт prefixed byte count).
		 * Каждая ProtocolVersion = 2 байта, поэтому count должен быть чётным.
		 * Необходимый размер буфера: 1 (length byte) + count (list bytes).
		 */
		if((count == 0) || (size < static_cast <size_t> (1 + count)) || ((count % 2) != 0))
			// Выходим из функции
			return;
		/**
		 * Выполняем парсинг поддерживаемых версий из данных расширения
		 */
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
	 *
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
	 *
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
			/**
			 * Перебираем поддерживаемые алгоритмы подписи в данных расширения
			 */
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
	 *
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
			/**
			 * Перебираем алгоритмы сжатия в данных расширения
			 */
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
					// Если идентификатор алгоритма сжатия соответствует ZLib
					case 0x01:
						// Устанавливаем флаг алгоритма сжатия ZLib для расширения compress_certificate в списке расширений браузера
						awh_cast <awh::tls::fgp_t::extension_compress_certificate_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::compressor_t::ZLIB);
					break;
					// Если идентификатор алгоритма сжатия соответствует Brotli
					case 0x02:
						// Устанавливаем флаг алгоритма сжатия Brotli для расширения compress_certificate в списке расширений браузера
						awh_cast <awh::tls::fgp_t::extension_compress_certificate_t *> (browser.extensions.back().get())->algorithms.push_back(awh::tls::compressor_t::BROTLI);
					break;
					// Если идентификатор алгоритма сжатия соответствует ZStandard (Zstd)
					case 0x03:
						// Устанавливаем флаг алгоритма сжатия ZStandard (Zstd) для расширения compress_certificate в списке расширений браузера
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
	 *
	 */
	static void parseALPN(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение ALPN в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_alpn_t> ());
		// Если размер данных в буфере меньше 2 байт, то данных недостаточно для парсинга
		if(size < 2)
			// Выходим из функции
			return;
		// Получаем байтовую длину списка ALPN-протоколов из первых 2 байт данных расширения
		const uint16_t count = ::local::u16(buffer);
		// Если байтовая длина списка ALPN-протоколов больше размера данных в буфере, то данных недостаточно для парсинга
		if(count > (size - 2))
			// Выходим из функции
			return;
		// Если байтовая длина списка ALPN-протоколов больше 0
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
	 *
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
	 *
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
			/**
			 * Перебираем поддерживаемые профили SRTP в данных расширения
			 */
			for(size_t i = 2; (((i + 1) < static_cast <size_t> (2 + count)) && ((i + 1) < size)); i += 2){
				// Извлекаем код профиля SRTP из буфера
				const uint16_t profile = ::local::u16(buffer + i);
				// Если код профиля SRTP является GREASE
				if(::local::isGrease(profile))
					// Добавляем код GREASE в список поддерживаемых профилей SRTP браузера
					awh_cast <awh::tls::fgp_t::extension_use_srtp_t *> (browser.extensions.back().get())->profiles.push_back(awh::tls::srtp_t::GREASE);
				// Если код профиля SRTP является одним из стандартных кодов из RFC 5764
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
	 *
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
			/**
			 * Перебираем поддерживаемые алгоритмы подписи в данных расширения
			 */
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
	 *
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
			/**
			 * Перебираем поддерживаемые алгоритмы подписи в данных расширения
			 */
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
	 *
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
			/**
			 * Перебираем поддерживаемые форматы точек эллиптической кривой в данных расширения
			 */
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
	 *
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
				// Если код группы является одним из стандартных кодов из RFC 8446
				else {
					/**
					 * Определяем код группы
					 */
					switch(gid){
						// Если эллиптическая кривая соответствует P-256 (secp256r1)
						case 0x0017:
							// Добавляем код группы эллиптической кривой P-256 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::P_256);
						break;
						// Если эллиптическая кривая соответствует P-384 (secp384r1)
						case 0x0018:
							// Добавляем код группы эллиптической кривой P-384 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::P_384);
						break;
						// Если эллиптическая кривая соответствует P-521 (secp521r1)
						case 0x0019:
							// Добавляем код группы эллиптической кривой P-521 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::P_521);
						break;
						// Если эллиптическая кривая соответствует X25519
						case 0x001D:
							// Добавляем код группы эллиптической кривой X25519 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::X25519);
						break;
						// Если эллиптическая кривая соответствует X448
						case 0x001E:
							// Добавляем код группы эллиптической кривой X448 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::X448);
						break;
						// Если эллиптическая кривая соответствует secp256k1
						case 0x001C:
							// Добавляем код группы эллиптической кривой secp256k1 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::SECP256K1);
						break;
						// Если эллиптическая кривая соответствует FFDHE 2048
						case 0x0100:
							// Добавляем код группы FFDHE 2048 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::FFDHE2048);
						break;
						// Если эллиптическая кривая соответствует FFDHE 3072
						case 0x0101:
							// Добавляем код группы FFDHE 3072 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::FFDHE3072);
						break;
						// Если эллиптическая кривая соответствует FFDHE 4096
						case 0x0102:
							// Добавляем код группы FFDHE 4096 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::FFDHE4096);
						break;
						// Если эллиптическая кривая соответствует FFDHE 6144
						case 0x0103:
							// Добавляем код группы FFDHE 6144 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::FFDHE6144);
						break;
						// Если эллиптическая кривая соответствует FFDHE 8192
						case 0x0104:
							// Добавляем код группы FFDHE 8192 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::FFDHE8192);
						break;
						// Если эллиптическая кривая соответствует MLKEM 1024
						case 0x0202:
							// Добавляем код группы MLKEM 1024 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::MLKEM1024);
						break;
						// Если эллиптическая кривая соответствует X25519Kyber768Draft00
						case 0x6399:
							// Добавляем код группы X25519Kyber768Draft00 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::X25519_KYBER768_DRAFT00);
						break;
						// Если эллиптическая кривая соответствует X25519MLKEM768
						case 0x11EC:
							// Добавляем код группы X25519MLKEM768 в список поддерживаемых групп эллиптических кривых браузера
							awh_cast <awh::tls::fgp_t::extension_supported_groups_t *> (browser.extensions.back().get())->supportedGroups.push_back(awh::tls::group_t::X25519_MLKEM768);
						break;
						// Если эллиптическая кривая не соответствует ни одной из известных, добавляем код UNKNOWN в список поддерживаемых групп эллиптических кривых браузера
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
	 *
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
	 *
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
	 *
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
	 *
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
	 *
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
	 *
	 * @note Данные — битовое поле флагов: байт i содержит флаги с номерами i*8 .. i*8+7.
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
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
	 *
	 * @details В ClientHello NPN обычно пустое (сигнализирует поддержку). Если данные присутствуют,
	 *          разбираем как список протоколов: 1-байтовая длина + имя (без внешнего поля длины).
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
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
	 *
	 * @note Формат (draft-vvv-tls-alps): 2-байтовая длина списка + записи (1-байтовая длина + имя).
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
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
	 *
	 * @note Формат идентичен application_settings (0x44CD).
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
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
	 *
	 * @note В ClientHello всегда пустое — сигнализирует поддержку Channel ID.
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
	 */
	static void parseChannelID(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение channel_id в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_channel_id_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения trust_anchors (BoringSSL draft, 0xCA34)
	 *
	 * @note В ClientHello присутствие расширения сигнализирует поддержку.
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
	 */
	static void parseTrustAnchors(const uint8_t * buffer, const size_t size, awh::tls::fgp_t::browser_t & browser) noexcept {
		// Добавляем расширение trust_anchors в список расширений браузера
		browser.extensions.push_back(make_unique <awh::tls::fgp_t::extension_trust_anchors_t> ());
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения ech_outer_extensions (ECH draft, 0xFD00)
	 *
	 * @note Формат: 1-байтовый счётчик байт + список 2-байтовых ExtensionType (счётчик должен быть чётным).
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
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
		/**
		 * Перебираем 2-байтовые коды типов расширений
		 */
		for(size_t i = 1; (i + 1) <= static_cast <size_t> (1 + count); i += 2)
			// Сохраняем как UNKNOWN (маппинг wire-кодов → extension_type_t не требуется для отпечатка)
			ext->extensions.push_back(awh::tls::extension_type_t::UNKNOWN);
	}

	/**
	 * @brief Вспомогательная функция для парсинга расширения Encrypted Client Hello / ECH (0xFE0D)
	 *
	 * @note Данные — непрозрачный двоичный blob (opaque).
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
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
	 *
	 * @note Формат: 1-байтовая длина + renegotiated_connection. В начальном ClientHello длина = 0.
	 *
	 * @param buffer  бинарный буфер с данными расширения
	 * @param size    размер данных в буфере
	 * @param browser объект для хранения распарсенных данных цифрового отпечатка браузера
	 *
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

/**
 * Инкапсулируем статические объекты в пространство имён работы с протоколом HTTP/2
 */
namespace http2 {
	/**
	 * @brief Вспомогательная функция возвращения односимвольных сокращений псевдо-заголовка по индексу HPACK статической таблицы
	 *
	 * @details Согласно RFC 7541 Appendix A, псевдо-заголовки занимают индексы 1-7:
	 *           1 = :authority → 'a'
	 *           2 = :method GET / 3 = :method POST → 'm'
	 *           4 = :path / / 5 = :path /index.html → 'p'
	 *           6 = :scheme http / 7 = :scheme https → 's'
	 *
	 * @param index индекс в статической таблице HPACK
	 * @return      'a'/'m'/'p'/'s' или '\0' если не псевдо-заголовок / индекс вне диапазона
	 *
	 */
	static char hpackPseudoChar(const uint32_t index) noexcept {
		/**
		 * Определяем псевдо-заголовок по индексу в статической таблице HPACK
		 */
		switch(index){
			case 1:         return 'a'; // :authority
			case 2: case 3: return 'm'; // :method
			case 4: case 5: return 'p'; // :path
			case 6: case 7: return 's'; // :scheme
			default:        return '\0';
		}
	}

	/**
	 * @brief Вспомогательная функция возвращения односимвольных сокращений псевдо-заголовка по его имени
	 *
	 * @param name имя заголовка из HPACK-блока
	 * @return     'a'/'m'/'p'/'s' или '\0' если не псевдо-заголовок
	 *
	 */
	static char hpackPseudoFromName(const string & name) noexcept {
		// Если имя соответствует :authority, возвращаем 'a'
		if(name == ":authority")
			// Возвращаем 'a' для :authority
			return 'a';
		// Если имя соответствует :method, возвращаем 'm'
		if(name == ":method")
			// Возвращаем 'm' для :method
			return 'm';
		// Если имя соответствует :path, возвращаем 'p'
		if(name == ":path")
			// Возвращаем 'p' для :path
			return 'p';
		// Если имя соответствует :scheme, возвращаем 's'
		if(name == ":scheme")
			// Возвращаем 's' для :scheme
			return 's';
		// Если имя не соответствует ни одному из псевдо-заголовков, возвращаем '\0'
		return '\0';
	}

	/**
	 * @brief Вспомогательная функция декодирования HPACK-целого числа с N-битным префиксом (RFC 7541 §5.1)
	 *
	 * @param buffer буфер с HPACK-блоком
	 * @param size   размер буфера
	 * @param offset текущая позиция (изменяется при чтении)
	 * @param width  ширина префикса в битах (1-8)
	 * @return       декодированное целое число; 0 при выходе за границу буфера
	 *
	 */
	static uint32_t hpackDecodeInt(const uint8_t * buffer, const size_t size, size_t & offset, const uint8_t width) noexcept {
		// Если позиция вышла за конец буфера, возвращаем 0
		if(offset >= size)
			// Позиция вышла за конец буфера
			return 0;
		// Маска для N-битного префикса
		const uint32_t max = (1u << width) - 1u;
		// Читаем N-битное значение из первого байта
		uint32_t result = (static_cast <uint32_t> (buffer[offset++]) & max);
		// Если значение меньше max, оно умещается в N бит — возвращаем сразу
		if(result < max)
			// Значение умещается в N бит
			return result;
		// Иначе читаем дополнительные байты с продолжением (MSB = флаг продолжения)
		uint32_t shift = 0;
		/**
		 * Читаем байты продолжения
		 */
		while(offset < size){
			// Читаем байт продолжения
			const uint8_t byte = buffer[offset++];
			// Добавляем 7 бит данных
			result += (static_cast <uint32_t> (byte & 0x7F) << shift);
			// Сдвигаем позицию на 7 бит
			shift += 7;
			// Если MSB = 0, это последний байт продолжения
			if(!(byte & 0x80) || (shift >= 28))
				// Завершаем декодирование
				break;
		}
		// Возвращаем декодированное целое число
		return result;
	}

	/**
	 * @brief Вспомогательная функция декодирования HPACK строки из буфера (RFC 7541 §5.2)
	 *
	 * @details Поддерживает только raw (не Huffman) кодирование. Для Huffman возвращает пустую строку
	 *          и продвигает offset за строку — это нормально: псевдо-заголовки в реальных браузерах
	 *          передаются через indexed header fields из статической таблицы, без Huffman-кодирования имён.
	 *
	 * @param buffer буфер с HPACK-блоком
	 * @param size   размер буфера
	 * @param offset текущее смещение (изменяется при чтении)
	 * @return       декодированная строка; пустая при Huffman-кодировании или ошибке
	 *
	 */
	static string hpackDecodeStr(const uint8_t * buffer, const size_t size, size_t & offset) noexcept {
		// Если позиция вышла за конец буфера, возвращаем пустую строку
		if(offset >= size)
			// Позиция вышла за конец буфера
			return "";
		// Флаг Huffman-кодирования (бит 7)
		const bool isHuffman = ((buffer[offset] & 0x80) != 0);
		// Длина строки (7-битный префикс)
		const size_t length = static_cast <size_t> (hpackDecodeInt(buffer, size, offset, 7));
		// Проверяем что хватает данных
		if((offset + length) > size){
			// Позиция вышла за конец буфера
			offset = size;
			// Если данных недостаточно для чтения строки, возвращаем пустую строку
			return "";
		}
		// Результат декодирования
		string result = "";
		// Если строка не Huffman-кодирована — читаем как есть
		if(!isHuffman)
			// Читаем строку как есть
			result.assign(reinterpret_cast <const char *> (buffer + offset), length);
		// Продвигаем позицию
		offset += length;
		// Возвращаем декодированную строку (пустую если Huffman)
		return result;
	}
};

/**
 * @brief Конструктор
 *
 * @param type Тип расширения
 *
 */
awh::tls::Fingerprint::Extension::Extension(const extension_type_t type) noexcept : type(type) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Grease::Extension_Grease() noexcept :
 extension_t(extension_type_t::GREASE) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Channel_ID::Extension_Channel_ID() noexcept :
 extension_t(extension_type_t::CHANNEL_ID) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_OID_Filters::Extension_OID_Filters() noexcept :
 extension_t(extension_type_t::OID_FILTERS) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Trust_Anchors::Extension_Trust_Anchors() noexcept :
 extension_t(extension_type_t::TRUST_ANCHORS) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Encrypt_Then_MAC::Extension_Encrypt_Then_MAC() noexcept :
 extension_t(extension_type_t::ENCRYPT_THEN_MAC) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Transparency_Info::Extension_Transparency_Info() noexcept :
 extension_t(extension_type_t::TRANSPARENCY_INFO) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Post_Handshake_Auth::Extension_Post_Handshake_Auth() noexcept :
 extension_t(extension_type_t::POST_HANDSHAKE_AUTH) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Client_Certificate_Type::Extension_Client_Certificate_Type() noexcept :
 extension_t(extension_type_t::CLIENT_CERTIFICATE_TYPE) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Server_Certificate_Type::Extension_Server_Certificate_Type() noexcept :
 extension_t(extension_type_t::SERVER_CERTIFICATE_TYPE) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Server_Name::Extension_Server_Name() noexcept :
 extension_t(extension_type_t::SERVER_NAME) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Status_Request::Extension_Status_Request() noexcept :
 extension_t(extension_type_t::STATUS_REQUEST),
 certificateStatusType{""},
 responderIdListLength(0),
 requestExtensionsLength(0) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Supported_Groups::Extension_Supported_Groups() noexcept :
 extension_t(extension_type_t::SUPPORTED_GROUPS) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_EC_Point::Extension_EC_Point() noexcept :
 extension_t(extension_type_t::EC_POINT_FORMATS) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_ALPN::Extension_ALPN() noexcept :
 extension_t(extension_type_t::ALPN) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Application_Settings::Extension_Application_Settings() noexcept :
 extension_t(extension_type_t::APPLICATION_SETTINGS) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Application_Settings_Old::Extension_Application_Settings_Old() noexcept :
 extension_t(extension_type_t::APPLICATION_SETTINGS_OLD) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Next_Proto_Neg::Extension_Next_Proto_Neg() noexcept :
 extension_t(extension_type_t::NEXT_PROTO_NEG) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Signed_Certificate_Timestamp::Extension_Signed_Certificate_Timestamp() noexcept :
 extension_t(extension_type_t::SIGNED_CERTIFICATE_TIMESTAMP) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Padding::Extension_Padding() noexcept :
 extension_t(extension_type_t::PADDING), size(0) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Extended_Master_Secret::Extension_Extended_Master_Secret() noexcept :
 extension_t(extension_type_t::EXTENDED_MASTER_SECRET),
 masterSecretData{""}, extendedMasterSecretData{""} {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Compress_Certificate::Extension_Compress_Certificate() noexcept :
 extension_t(extension_type_t::COMPRESS_CERTIFICATE) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Session_Ticket::Extension_Session_Ticket() noexcept :
 extension_t(extension_type_t::SESSION_TICKET) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Supported_Versions::Extension_Supported_Versions() noexcept :
 extension_t(extension_type_t::SUPPORTED_VERSIONS) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_PSK_Key_Exchange::Extension_PSK_Key_Exchange() noexcept :
 extension_t(extension_type_t::PSK_KEY_EXCHANGE_MODES) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Early_Data::Extension_Early_Data() noexcept :
 extension_t(extension_type_t::EARLY_DATA), maxSize(0) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Key_Share::Extension_Key_Share() noexcept :
 extension_t(extension_type_t::KEY_SHARE) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Encryption_Client_Hello::Extension_Encryption_Client_Hello() noexcept :
 extension_t(extension_type_t::ENCRYPTED_CLIENT_HELLO) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Renegotiation_Info::Extension_Renegotiation_Info() noexcept :
 extension_t(extension_type_t::RENEGOTIATION_INFO) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Record_Size_Limit::Extension_Record_Size_Limit() noexcept :
 extension_t(extension_type_t::RECORD_SIZE_LIMIT), data(0) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Cookie::Extension_Cookie() noexcept :
 extension_t(extension_type_t::COOKIE) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Pre_Shared_Key::Identity::Identity() noexcept : ticketAge(0) {}
/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Pre_Shared_Key::Extension_Pre_Shared_Key() noexcept :
 extension_t(extension_type_t::PRE_SHARED_KEY) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Certificate_Authorities::Extension_Certificate_Authorities() noexcept :
 extension_t(extension_type_t::CERTIFICATE_AUTHORITIES) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Max_Fragment_Length::Extension_Max_Fragment_Length() noexcept :
 extension_t(extension_type_t::MAX_FRAGMENT_LENGTH), length(0) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Use_SRTP::Extension_Use_SRTP() noexcept :
 extension_t(extension_type_t::USE_SRTP), mkiLength(0) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Heartbeat::Extension_Heartbeat() noexcept :
 extension_t(extension_type_t::HEARTBEAT), mode(heartbeat_t::UNKNOWN) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Signature::Extension_Signature() noexcept :
 extension_t(extension_type_t::SIGNATURE_ALGORITHMS) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Delegated_Credential::Extension_Delegated_Credential() noexcept :
 extension_t(extension_type_t::DELEGATED_CREDENTIAL) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Signature_Algorithms_Cert::Extension_Signature_Algorithms_Cert() noexcept :
 extension_t(extension_type_t::SIGNATURE_ALGORITHMS_CERT) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_TLS_Flags::Extension_TLS_Flags() noexcept :
 extension_t(extension_type_t::TLS_FLAGS) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Quic_Transport_Params::Extension_Quic_Transport_Params() noexcept :
 extension_t(extension_type_t::QUIC_TRANSPORT_PARAMETERS) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_Quic_Transport_Params_Legacy::Extension_Quic_Transport_Params_Legacy() noexcept :
 extension_t(extension_type_t::QUIC_TRANSPORT_PARAMETERS_LEGACY) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Extension_ECH_Outer_Extensions::Extension_ECH_Outer_Extensions() noexcept :
 extension_t(extension_type_t::ECH_OUTER_EXTENSIONS) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::VersionTLS::VersionTLS() noexcept :
 record{""}, negotiated{""} {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Imprint::Imprint() noexcept :
 ja3{""}, ja4{""}, ja4r{""},
 ja3Hash{""}, sessionId{""},
 peetprint{""}, peetprintHash{""},
 clientRandom{""}, akamai{""} {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Record::Record() noexcept :
 epoch(0), length(0), sequence(0),
 version(version_t::UNKNOWN) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Fragment::Fragment() noexcept : offset(0), length(0) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Handshake::Handshake() noexcept : length(0), sequence(0) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::ClientHello::ClientHello() noexcept : version(version_t::UNKNOWN) {}
	
/**
 * @brief Оператор сравнения двух отпечатков браузеров
 *
 * @param browser объект цифрового отпечатка браузера для сравнения
 * @return        результат сравнения
 *
 */
bool awh::tls::Fingerprint::Browser::operator == (const Browser & browser) const noexcept {
	// Если флаги поддержки GREASE не совпадают, объекты не равны
	if(this->grease != browser.grease)
		// Возвращаем результат сравнения: объекты не равны
		return false;
	// Если объекты записи TLS рукопожатия не совпадают побайтно, объекты не равны
	else if(::memcmp(&this->record, &browser.record, sizeof(this->record)) != 0)
		// Возвращаем результат сравнения: объекты не равны
		return false;
	// Если объекты рукопожатия TLS не совпадают побайтно, объекты не равны
	else if(::memcmp(&this->handshake, &browser.handshake, sizeof(this->handshake)) != 0)
		// Возвращаем результат сравнения: объекты не равны
		return false;
	// Если объекты ClientHello TLS не совпадают побайтно, объекты не равны
	else if(::memcmp(&this->clientHello, &browser.clientHello, sizeof(this->clientHello)) != 0)
		// Возвращаем результат сравнения: объекты не равны
		return false;
	// Если размеры cookie рукопожатия DTLS не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
	else if((this->cookie.size() != browser.cookie.size()) || (!this->cookie.empty() && (::memcmp(&this->cookie[0], &browser.cookie[0], this->cookie.size()) != 0)))
		// Возвращаем результат сравнения: объекты не равны
		return false;
	// Если размеры идентификатора сессии TLS не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
	else if((this->session.size() != browser.session.size()) || (!this->session.empty() && (::memcmp(&this->session[0], &browser.session[0], this->session.size()) != 0)))
		// Возвращаем результат сравнения: объекты не равны
		return false;
	// Если размеры списка поддерживаемых шифров не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
	else if((this->ciphers.size() != browser.ciphers.size()) || (!this->ciphers.empty() && (::memcmp(&this->ciphers[0], &browser.ciphers[0], this->ciphers.size()) != 0)))
		// Возвращаем результат сравнения: объекты не равны
		return false;
	// Если размеры списка поддерживаемых методов сжатия TLS не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
	else if((this->compressors.size() != browser.compressors.size()) || (!this->compressors.empty() && (::memcmp(&this->compressors[0], &browser.compressors[0], this->compressors.size()) != 0)))
		// Возвращаем результат сравнения: объекты не равны
		return false;
	// Если размеры списка поддерживаемых расширений TLS не совпадают, объекты не равны
	else if(this->extensions.size() != browser.extensions.size())
		// Возвращаем результат сравнения: объекты не равны
		return false;
	// Если объекты расширений совпадают по размерам и не пустые
	else if(!this->extensions.empty() && !browser.extensions.empty()) {
		/**
		 * Итерируем по расширениям и сравниваем их типы
		 */
		for(size_t i = 0; i < this->extensions.size(); ++i){
			// Если типы расширений не совпадают, объекты не равны
			if(this->extensions[i]->type != browser.extensions[i]->type)
				// Возвращаем результат сравнения: объекты не равны
				return false;
			// Если типы расширений совпадают, сравниваем их содержимое в зависимости от типа
			else {
				/**
				 * Восстанавливаем объект расширения в зависимости от типа
				 */
				switch(static_cast <uint8_t> (browser.extensions[i]->type)){
					// Если тип расширения соответствует server_name
					case static_cast <uint8_t> (awh::tls::extension_type_t::SERVER_NAME): {
						// Если размеры списков имён в расширении server_name не совпадают, объекты не равны
						if(awh_cast <extension_server_name_t *> (this->extensions[i].get())->names.size() != awh_cast <extension_server_name_t *> (browser.extensions[i].get())->names.size())
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если объекты расширений совпадают по размерам и не пустые
						else if(!awh_cast <extension_server_name_t *> (this->extensions[i].get())->names.empty() && !awh_cast <extension_server_name_t *> (browser.extensions[i].get())->names.empty()) {
							/**
							 * Итерируем по расширениям и сравниваем их типы
							 */
							for(size_t j = 0; j < awh_cast <extension_server_name_t *> (this->extensions[i].get())->names.size(); ++j){
								// Если имена в расширении server_name не совпадают, объекты не равны
								if(awh_cast <extension_server_name_t *> (this->extensions[i].get())->names[j] != awh_cast <extension_server_name_t *> (browser.extensions[i].get())->names[j])
									// Возвращаем результат сравнения: объекты не равны
									return false;
							}
						}
					} break;
					// Если тип расширения соответствует max_fragment_length
					case static_cast <uint8_t> (awh::tls::extension_type_t::MAX_FRAGMENT_LENGTH): {
						// Если значение длины максимального фрагмента не совпадает, объекты не равны
						if(awh_cast <extension_max_fragment_length_t *> (this->extensions[i].get())->length != awh_cast <extension_max_fragment_length_t *> (browser.extensions[i].get())->length)
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует status_request
					case static_cast <uint8_t> (awh::tls::extension_type_t::STATUS_REQUEST): {
						// Если типы запрашиваемого статуса в расширении status_request не совпадают, объекты не равны
						if(awh_cast <extension_status_request_t *> (this->extensions[i].get())->certificateStatusType != awh_cast <extension_status_request_t *> (browser.extensions[i].get())->certificateStatusType)
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если длины списков идентификаторов респондентов в расширении status_request не совпадают, объекты не равны
						else if(awh_cast <extension_status_request_t *> (this->extensions[i].get())->responderIdListLength != awh_cast <extension_status_request_t *> (browser.extensions[i].get())->responderIdListLength)
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если длины расширений запроса статуса сертификата не совпадают, объекты не равны
						else if(awh_cast <extension_status_request_t *> (this->extensions[i].get())->requestExtensionsLength != awh_cast <extension_status_request_t *> (browser.extensions[i].get())->requestExtensionsLength)
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует supported_groups
					case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_GROUPS): {
						// Если размеры списков поддерживаемых групп не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_supported_groups_t *> (this->extensions[i].get())->supportedGroups.size() != awh_cast <extension_supported_groups_t *> (browser.extensions[i].get())->supportedGroups.size()) ||
						  (!awh_cast <extension_supported_groups_t *> (this->extensions[i].get())->supportedGroups.empty() && (::memcmp(&awh_cast <extension_supported_groups_t *> (this->extensions[i].get())->supportedGroups[0], &awh_cast <extension_supported_groups_t *> (browser.extensions[i].get())->supportedGroups[0], awh_cast <extension_supported_groups_t *> (this->extensions[i].get())->supportedGroups.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует ec_point_formats
					case static_cast <uint8_t> (awh::tls::extension_type_t::EC_POINT_FORMATS): {
						// Если размеры списков форматов точек в расширении ec_point_formats не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_ec_point_t *> (this->extensions[i].get())->formats.size() != awh_cast <extension_ec_point_t *> (browser.extensions[i].get())->formats.size()) ||
						  (!awh_cast <extension_ec_point_t *> (this->extensions[i].get())->formats.empty() && (::memcmp(&awh_cast <extension_ec_point_t *> (this->extensions[i].get())->formats[0], &awh_cast <extension_ec_point_t *> (browser.extensions[i].get())->formats[0], awh_cast <extension_ec_point_t *> (this->extensions[i].get())->formats.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует signature_algorithms
					case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS): {
						// Если размеры списков алгоритмов подписи в расширении signature_algorithms не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_signature_t *> (this->extensions[i].get())->algorithms.size() != awh_cast <extension_signature_t *> (browser.extensions[i].get())->algorithms.size()) ||
						  (!awh_cast <extension_signature_t *> (this->extensions[i].get())->algorithms.empty() && (::memcmp(&awh_cast <extension_signature_t *> (this->extensions[i].get())->algorithms[0], &awh_cast <extension_signature_t *> (browser.extensions[i].get())->algorithms[0], awh_cast <extension_signature_t *> (this->extensions[i].get())->algorithms.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует use_srtp
					case static_cast <uint8_t> (awh::tls::extension_type_t::USE_SRTP): {
						// Если значения длины MKI в расширении use_srtp не совпадают, объекты не равны
						if(awh_cast <extension_use_srtp_t *> (this->extensions[i].get())->mkiLength != awh_cast <extension_use_srtp_t *> (browser.extensions[i].get())->mkiLength)
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если размеры списков профилей SRTP в расширении use_srtp не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						else if((awh_cast <extension_use_srtp_t *> (this->extensions[i].get())->profiles.size() != awh_cast <extension_use_srtp_t *> (browser.extensions[i].get())->profiles.size()) ||
							   (!awh_cast <extension_use_srtp_t *> (this->extensions[i].get())->profiles.empty() && (::memcmp(&awh_cast <extension_use_srtp_t *> (this->extensions[i].get())->profiles[0], &awh_cast <extension_use_srtp_t *> (browser.extensions[i].get())->profiles[0], awh_cast <extension_use_srtp_t *> (this->extensions[i].get())->profiles.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует heartbeat
					case static_cast <uint8_t> (awh::tls::extension_type_t::HEARTBEAT): {
						// Если значения режима heartbeat в расширении heartbeat не совпадают, объекты не равны
						if(awh_cast <extension_heartbeat_t *> (this->extensions[i].get())->mode != awh_cast <extension_heartbeat_t *> (browser.extensions[i].get())->mode)
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует alpn
					case static_cast <uint8_t> (awh::tls::extension_type_t::ALPN): {
						// Получаем ссылки на списки протоколов ALPN обоих объектов
						const auto & lhsProtos = awh_cast <extension_alpn_t *> (this->extensions[i].get())->protocols;
						const auto & rhsProtos = awh_cast <extension_alpn_t *> (browser.extensions[i].get())->protocols;
						// Если размеры списков протоколов в расширении alpn не совпадают, объекты не равны
						if(lhsProtos.size() != rhsProtos.size())
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если списки протоколов ALPN не пустые, сравниваем их поэлементно
						else if(!lhsProtos.empty()) {
							/**
							 * Итерируем по протоколам ALPN и сравниваем их
							 */
							for(size_t j = 0; j < lhsProtos.size(); ++j){
								// Если протоколы ALPN не совпадают, объекты не равны
								if(lhsProtos[j] != rhsProtos[j])
									// Возвращаем результат сравнения: объекты не равны
									return false;
							}
						}
					} break;
					// Если тип расширения соответствует padding
					case static_cast <uint8_t> (awh::tls::extension_type_t::PADDING): {
						// Если значения размера паддинга в расширении padding не совпадают, объекты не равны
						if(awh_cast <extension_padding_t *> (this->extensions[i].get())->size != awh_cast <extension_padding_t *> (browser.extensions[i].get())->size)
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует extended_master_secret
					case static_cast <uint8_t> (awh::tls::extension_type_t::EXTENDED_MASTER_SECRET): {
						// Если данные master_secret в расширении extended_master_secret не совпадают, объекты не равны
						if(awh_cast <extension_extended_master_secret_t *> (this->extensions[i].get())->masterSecretData != awh_cast <extension_extended_master_secret_t *> (browser.extensions[i].get())->masterSecretData)
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если данные extended_master_secret в расширении extended_master_secret не совпадают, объекты не равны
						else if(awh_cast <extension_extended_master_secret_t *> (this->extensions[i].get())->extendedMasterSecretData != awh_cast <extension_extended_master_secret_t *> (browser.extensions[i].get())->extendedMasterSecretData)
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует compress_certificate
					case static_cast <uint8_t> (awh::tls::extension_type_t::COMPRESS_CERTIFICATE): {
						// Если размеры списков алгоритмов сжатия сертификата в расширении compress_certificate не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_compress_certificate_t *> (this->extensions[i].get())->algorithms.size() != awh_cast <extension_compress_certificate_t *> (browser.extensions[i].get())->algorithms.size()) ||
						  (!awh_cast <extension_compress_certificate_t *> (this->extensions[i].get())->algorithms.empty() && (::memcmp(&awh_cast <extension_compress_certificate_t *> (this->extensions[i].get())->algorithms[0], &awh_cast <extension_compress_certificate_t *> (browser.extensions[i].get())->algorithms[0], awh_cast <extension_compress_certificate_t *> (this->extensions[i].get())->algorithms.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует record_size_limit
					case static_cast <uint8_t> (awh::tls::extension_type_t::RECORD_SIZE_LIMIT): {
						// Если значения record_size_limit в расширении record_size_limit не совпадают, объекты не равны
						if(awh_cast <extension_record_size_limit_t *> (this->extensions[i].get())->data != awh_cast <extension_record_size_limit_t *> (browser.extensions[i].get())->data)
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует delegated_credential
					case static_cast <uint8_t> (awh::tls::extension_type_t::DELEGATED_CREDENTIAL): {
						// Если размеры списков алгоритмов делегированных учётных данных в расширении delegated_credential не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_delegated_credential_t *> (this->extensions[i].get())->algorithms.size() != awh_cast <extension_delegated_credential_t *> (browser.extensions[i].get())->algorithms.size()) ||
						  (!awh_cast <extension_delegated_credential_t *> (this->extensions[i].get())->algorithms.empty() && (::memcmp(&awh_cast <extension_delegated_credential_t *> (this->extensions[i].get())->algorithms[0], &awh_cast <extension_delegated_credential_t *> (browser.extensions[i].get())->algorithms[0], awh_cast <extension_delegated_credential_t *> (this->extensions[i].get())->algorithms.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует session_ticket
					case static_cast <uint8_t> (awh::tls::extension_type_t::SESSION_TICKET): {
						// Если размеры данных session_ticket в расширении session_ticket не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_session_ticket_t *> (this->extensions[i].get())->data.size() != awh_cast <extension_session_ticket_t *> (browser.extensions[i].get())->data.size()) ||
						  (!awh_cast <extension_session_ticket_t *> (this->extensions[i].get())->data.empty() && (::memcmp(&awh_cast <extension_session_ticket_t *> (this->extensions[i].get())->data[0], &awh_cast <extension_session_ticket_t *> (browser.extensions[i].get())->data[0], awh_cast <extension_session_ticket_t *> (this->extensions[i].get())->data.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует pre_shared_key
					case static_cast <uint8_t> (awh::tls::extension_type_t::PRE_SHARED_KEY): {
						// Получаем ссылки на списки идентификаторов PSK обоих объектов
						const auto & lhsIds = awh_cast <extension_pre_shared_key_t *> (this->extensions[i].get())->identities;
						const auto & rhsIds = awh_cast <extension_pre_shared_key_t *> (browser.extensions[i].get())->identities;
						// Если размеры списков идентификаторов PSK в расширении pre_shared_key не совпадают, объекты не равны
						if(lhsIds.size() != rhsIds.size())
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если списки идентификаторов PSK не пустые, сравниваем их поэлементно
						else if(!lhsIds.empty()) {
							/**
							 * Итерируем по идентификаторам PSK и сравниваем их
							 */
							for(size_t j = 0; j < lhsIds.size(); ++j){
								// Если значения ticket_age не совпадают, объекты не равны
								if(lhsIds[j].ticketAge != rhsIds[j].ticketAge)
									// Возвращаем результат сравнения: объекты не равны
									return false;
								// Если размеры данных идентификатора PSK не совпадают или если они не пустые и не совпадают побайтно, объекты не равны
								else if((lhsIds[j].data.size() != rhsIds[j].data.size()) ||
								       (!lhsIds[j].data.empty() && (::memcmp(lhsIds[j].data.data(), rhsIds[j].data.data(), lhsIds[j].data.size()) != 0)))
									// Возвращаем результат сравнения: объекты не равны
									return false;
							}
						}
					} break;
					// Если тип расширения соответствует early_data
					case static_cast <uint8_t> (awh::tls::extension_type_t::EARLY_DATA): {
						// Если значения max_early_data_size в расширении early_data не совпадают, объекты не равны
						if(awh_cast <extension_early_data_t *> (this->extensions[i].get())->maxSize != awh_cast <extension_early_data_t *> (browser.extensions[i].get())->maxSize)
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует supported_versions
					case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_VERSIONS): {
						// Если размеры списков поддерживаемых версий TLS не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_supported_versions_t *> (this->extensions[i].get())->versions.size() != awh_cast <extension_supported_versions_t *> (browser.extensions[i].get())->versions.size()) ||
						  (!awh_cast <extension_supported_versions_t *> (this->extensions[i].get())->versions.empty() && (::memcmp(awh_cast <extension_supported_versions_t *> (this->extensions[i].get())->versions.data(), awh_cast <extension_supported_versions_t *> (browser.extensions[i].get())->versions.data(), awh_cast <extension_supported_versions_t *> (this->extensions[i].get())->versions.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует cookie
					case static_cast <uint8_t> (awh::tls::extension_type_t::COOKIE): {
						// Если размеры данных расширения cookie не совпадают или если данные не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_cookie_t *> (this->extensions[i].get())->data.size() != awh_cast <extension_cookie_t *> (browser.extensions[i].get())->data.size()) ||
						  (!awh_cast <extension_cookie_t *> (this->extensions[i].get())->data.empty() && (::memcmp(awh_cast <extension_cookie_t *> (this->extensions[i].get())->data.data(), awh_cast <extension_cookie_t *> (browser.extensions[i].get())->data.data(), awh_cast <extension_cookie_t *> (this->extensions[i].get())->data.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует psk_key_exchange_modes
					case static_cast <uint8_t> (awh::tls::extension_type_t::PSK_KEY_EXCHANGE_MODES): {
						// Если размеры списков режимов обмена ключами PSK не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_psk_key_exchange_t *> (this->extensions[i].get())->modes.size() != awh_cast <extension_psk_key_exchange_t *> (browser.extensions[i].get())->modes.size()) ||
						  (!awh_cast <extension_psk_key_exchange_t *> (this->extensions[i].get())->modes.empty() && (::memcmp(awh_cast <extension_psk_key_exchange_t *> (this->extensions[i].get())->modes.data(), awh_cast <extension_psk_key_exchange_t *> (browser.extensions[i].get())->modes.data(), awh_cast <extension_psk_key_exchange_t *> (this->extensions[i].get())->modes.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует certificate_authorities
					case static_cast <uint8_t> (awh::tls::extension_type_t::CERTIFICATE_AUTHORITIES): {
						// Получаем ссылки на списки авторитетов сертификатов обоих объектов
						const auto & lhsAuths = awh_cast <extension_certificate_authorities_t *> (this->extensions[i].get())->authorities;
						const auto & rhsAuths = awh_cast <extension_certificate_authorities_t *> (browser.extensions[i].get())->authorities;
						// Если размеры списков авторитетов сертификатов не совпадают, объекты не равны
						if(lhsAuths.size() != rhsAuths.size())
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если списки авторитетов сертификатов не пустые, сравниваем их поэлементно
						else if(!lhsAuths.empty()) {
							/**
							 * Итерируем по авторитетам сертификатов и сравниваем их
							 */
							for(size_t j = 0; j < lhsAuths.size(); ++j){
								// Если размеры данных авторитета сертификата не совпадают или если они не пустые и не совпадают побайтно, объекты не равны
								if((lhsAuths[j].size() != rhsAuths[j].size()) ||
								   (!lhsAuths[j].empty() && (::memcmp(lhsAuths[j].data(), rhsAuths[j].data(), lhsAuths[j].size()) != 0)))
									// Возвращаем результат сравнения: объекты не равны
									return false;
							}
						}
					} break;
					// Если тип расширения соответствует signature_algorithms_cert
					case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS_CERT): {
						// Если размеры списков алгоритмов подписи сертификатов не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_signature_algorithms_cert_t *> (this->extensions[i].get())->algorithms.size() != awh_cast <extension_signature_algorithms_cert_t *> (browser.extensions[i].get())->algorithms.size()) ||
						  (!awh_cast <extension_signature_algorithms_cert_t *> (this->extensions[i].get())->algorithms.empty() && (::memcmp(awh_cast <extension_signature_algorithms_cert_t *> (this->extensions[i].get())->algorithms.data(), awh_cast <extension_signature_algorithms_cert_t *> (browser.extensions[i].get())->algorithms.data(), awh_cast <extension_signature_algorithms_cert_t *> (this->extensions[i].get())->algorithms.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует key_share
					case static_cast <uint8_t> (awh::tls::extension_type_t::KEY_SHARE): {
						// Получаем ссылки на списки ключей для обмена обоих объектов
						const auto & lhsShares = awh_cast <extension_key_share_t *> (this->extensions[i].get())->shares;
						const auto & rhsShares = awh_cast <extension_key_share_t *> (browser.extensions[i].get())->shares;
						// Если размеры списков ключей для обмена не совпадают, объекты не равны
						if(lhsShares.size() != rhsShares.size())
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если списки ключей для обмена не пустые, сравниваем их поэлементно
						else if(!lhsShares.empty()) {
							/**
							 * Итерируем по ключам для обмена и сравниваем их
							 */
							for(size_t j = 0; j < lhsShares.size(); ++j){
								// Если группы ключа для обмена не совпадают, объекты не равны
								if(lhsShares[j].first != rhsShares[j].first)
									// Возвращаем результат сравнения: объекты не равны
									return false;
								// Если размеры данных ключа для обмена не совпадают или если они не пустые и не совпадают побайтно, объекты не равны
								else if((lhsShares[j].second.size() != rhsShares[j].second.size()) ||
								        (!lhsShares[j].second.empty() && (::memcmp(lhsShares[j].second.data(), rhsShares[j].second.data(), lhsShares[j].second.size()) != 0)))
									// Возвращаем результат сравнения: объекты не равны
									return false;
							}
						}
					} break;
					// Если тип расширения соответствует quic_transport_parameters
					case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS): {
						// Если параметры транспортного уровня QUIC не совпадают, объекты не равны
						if(awh_cast <extension_quic_transport_params_t *> (this->extensions[i].get())->params != awh_cast <extension_quic_transport_params_t *> (browser.extensions[i].get())->params)
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует tls_flags
					case static_cast <uint8_t> (awh::tls::extension_type_t::TLS_FLAGS): {
						// Если размеры списков флагов TLS не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_tls_flags_t *> (this->extensions[i].get())->flags.size() != awh_cast <extension_tls_flags_t *> (browser.extensions[i].get())->flags.size()) ||
						  (!awh_cast <extension_tls_flags_t *> (this->extensions[i].get())->flags.empty() && (::memcmp(awh_cast <extension_tls_flags_t *> (this->extensions[i].get())->flags.data(), awh_cast <extension_tls_flags_t *> (browser.extensions[i].get())->flags.data(), awh_cast <extension_tls_flags_t *> (this->extensions[i].get())->flags.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует next_protocol_negotiation
					case static_cast <uint8_t> (awh::tls::extension_type_t::NEXT_PROTO_NEG): {
						// Получаем ссылки на списки протоколов NPN обоих объектов
						const auto & lhsNPN = awh_cast <extension_next_proto_neg_t *> (this->extensions[i].get())->protocols;
						const auto & rhsNPN = awh_cast <extension_next_proto_neg_t *> (browser.extensions[i].get())->protocols;
						// Если размеры списков протоколов NPN не совпадают, объекты не равны
						if(lhsNPN.size() != rhsNPN.size())
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если списки протоколов NPN не пустые, сравниваем их поэлементно
						else if(!lhsNPN.empty()) {
							/**
							 * Итерируем по протоколам NPN и сравниваем их
							 */
							for(size_t j = 0; j < lhsNPN.size(); ++j){
								// Если протоколы NPN не совпадают, объекты не равны
								if(lhsNPN[j] != rhsNPN[j])
									// Возвращаем результат сравнения: объекты не равны
									return false;
							}
						}
					} break;
					// Если тип расширения соответствует application_settings_old (устаревшее)
					case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS_OLD): {
						// Получаем ссылки на списки протоколов расширения application_settings_old обоих объектов
						const auto & lhsASO = awh_cast <extension_application_settings_old_t *> (this->extensions[i].get())->protocols;
						const auto & rhsASO = awh_cast <extension_application_settings_old_t *> (browser.extensions[i].get())->protocols;
						// Если размеры списков протоколов в расширении application_settings_old не совпадают, объекты не равны
						if(lhsASO.size() != rhsASO.size())
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если списки протоколов не пустые, сравниваем их поэлементно
						else if(!lhsASO.empty()) {
							/**
							 * Итерируем по протоколам и сравниваем их
							 */
							for(size_t j = 0; j < lhsASO.size(); ++j){
								// Если протоколы не совпадают, объекты не равны
								if(lhsASO[j] != rhsASO[j])
									// Возвращаем результат сравнения: объекты не равны
									return false;
							}
						}
					} break;
					// Если тип расширения соответствует application_settings
					case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS): {
						// Получаем ссылки на списки протоколов расширения application_settings обоих объектов
						const auto & lhsAS = awh_cast <extension_application_settings_t *> (this->extensions[i].get())->protocols;
						const auto & rhsAS = awh_cast <extension_application_settings_t *> (browser.extensions[i].get())->protocols;
						// Если размеры списков протоколов в расширении application_settings не совпадают, объекты не равны
						if(lhsAS.size() != rhsAS.size())
							// Возвращаем результат сравнения: объекты не равны
							return false;
						// Если списки протоколов не пустые, сравниваем их поэлементно
						else if(!lhsAS.empty()) {
							/**
							 * Итерируем по протоколам и сравниваем их
							 */
							for(size_t j = 0; j < lhsAS.size(); ++j){
								// Если протоколы не совпадают, объекты не равны
								if(lhsAS[j] != rhsAS[j])
									// Возвращаем результат сравнения: объекты не равны
									return false;
							}
						}
					} break;
					// Если тип расширения соответствует ech_outer_extensions
					case static_cast <uint8_t> (awh::tls::extension_type_t::ECH_OUTER_EXTENSIONS): {
						// Если размеры списков расширений в расширении ech_outer_extensions не совпадают или если объекты не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_ech_outer_extensions_t *> (this->extensions[i].get())->extensions.size() != awh_cast <extension_ech_outer_extensions_t *> (browser.extensions[i].get())->extensions.size()) ||
						  (!awh_cast <extension_ech_outer_extensions_t *> (this->extensions[i].get())->extensions.empty() && (::memcmp(awh_cast <extension_ech_outer_extensions_t *> (this->extensions[i].get())->extensions.data(), awh_cast <extension_ech_outer_extensions_t *> (browser.extensions[i].get())->extensions.data(), awh_cast <extension_ech_outer_extensions_t *> (this->extensions[i].get())->extensions.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует encrypted_client_hello
					case static_cast <uint8_t> (awh::tls::extension_type_t::ENCRYPTED_CLIENT_HELLO): {
						// Если размеры данных расширения encrypted_client_hello не совпадают или если данные не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_encryption_client_hello_t *> (this->extensions[i].get())->data.size() != awh_cast <extension_encryption_client_hello_t *> (browser.extensions[i].get())->data.size()) ||
						  (!awh_cast <extension_encryption_client_hello_t *> (this->extensions[i].get())->data.empty() && (::memcmp(awh_cast <extension_encryption_client_hello_t *> (this->extensions[i].get())->data.data(), awh_cast <extension_encryption_client_hello_t *> (browser.extensions[i].get())->data.data(), awh_cast <extension_encryption_client_hello_t *> (this->extensions[i].get())->data.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует renegotiation_info
					case static_cast <uint8_t> (awh::tls::extension_type_t::RENEGOTIATION_INFO): {
						// Если размеры данных расширения renegotiation_info не совпадают или если данные не пустые и не совпадают побайтно, объекты не равны
						if((awh_cast <extension_renegotiation_info_t *> (this->extensions[i].get())->data.size() != awh_cast <extension_renegotiation_info_t *> (browser.extensions[i].get())->data.size()) ||
						  (!awh_cast <extension_renegotiation_info_t *> (this->extensions[i].get())->data.empty() && (::memcmp(awh_cast <extension_renegotiation_info_t *> (this->extensions[i].get())->data.data(), awh_cast <extension_renegotiation_info_t *> (browser.extensions[i].get())->data.data(), awh_cast <extension_renegotiation_info_t *> (this->extensions[i].get())->data.size()) != 0)))
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
					// Если тип расширения соответствует quic_transport_parameters_legacy
					case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS_LEGACY): {
						// Если параметры транспортного уровня QUIC (устаревшее расширение) не совпадают, объекты не равны
						if(awh_cast <extension_quic_transport_params_legacy_t *> (this->extensions[i].get())->params != awh_cast <extension_quic_transport_params_legacy_t *> (browser.extensions[i].get())->params)
							// Возвращаем результат сравнения: объекты не равны
							return false;
					} break;
				}
			}
		}
	}
	// Если все проверки пройдены, объекты считаются равными
	return true;
}
/**
 * @brief Оператор перемещения
 *
 * @param browser объект цифрового отпечатка браузера для перемещения
 * @return        текущий объект после перемещения
 *
 */
awh::tls::Fingerprint::Browser & awh::tls::Fingerprint::Browser::operator = (Browser && browser) noexcept {
	// Копируем флаг поддержки GREASE
	this->grease = browser.grease;
	// Сбрасываем флаг в исходном объекте (необязательно, но подчёркивает что данные перемещены)
	browser.grease = false;
	// Перемещаем объект записи TLS рукопожатия
	this->record = ::move(browser.record);
	// Перемещаем объект рукопожатия TLS
	this->handshake = ::move(browser.handshake);
	// Перемещаем объект ClientHello
	this->clientHello = ::move(browser.clientHello);
	// Перемещаем объект cookie рукопожатия DTLS
	this->cookie = ::move(browser.cookie);
	// Перемещаем объект идентификатора сессии TLS
	this->session = ::move(browser.session);
	// Перемещаем список поддерживаемых шифров
	this->ciphers = ::move(browser.ciphers);
	// Перемещаем список поддерживаемых методов сжатия TLS
	this->compressors = ::move(browser.compressors);
	// Перемещаем список поддерживаемых расширений TLS
	this->extensions = ::move(browser.extensions);
	// Возвращаем текущий объект после перемещения
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param browser объект цифрового отпечатка браузера для копирования
 * @return        текущий объект после копирования
 *
 */
awh::tls::Fingerprint::Browser & awh::tls::Fingerprint::Browser::operator = (const Browser & browser) noexcept {
	// Копируем флаг поддержки GREASE
	this->grease = browser.grease;
	// Копируем объект записи TLS рукопожатия
	this->record = browser.record;
	// Копируем объект рукопожатия TLS
	this->handshake = browser.handshake;
	// Копируем объект ClientHello
	this->clientHello = browser.clientHello;
	// Копируем объект cookie рукопожатия DTLS
	this->cookie = browser.cookie;
	// Копируем объект идентификатора сессии TLS
	this->session = browser.session;
	// Копируем список поддерживаемых шифров
	this->ciphers = browser.ciphers;
	// Копируем список поддерживаемых методов сжатия TLS
	this->compressors = browser.compressors;
	// Очищаем список расширений в текущем объекте, чтобы подготовиться к копированию расширений из исходного объекта
	this->extensions.clear();
	/**
	 * Перебираем расширения в исходном объекте и восстанавливаем их в текущем объекте в зависимости от типа
	 */
	for(auto & extension : browser.extensions){
		/**
		 * Восстанавливаем объект расширения в зависимости от типа
		 */
		switch(static_cast <uint8_t> (extension->type)){
			// Если тип расширения соответствует grease
			case static_cast <uint8_t> (awh::tls::extension_type_t::GREASE):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_grease_t> ());
			break;
			// Если тип расширения соответствует channel_id
			case static_cast <uint8_t> (awh::tls::extension_type_t::CHANNEL_ID):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_channel_id_t> ());
			break;
			// Если тип расширения соответствует oid_filters
			case static_cast <uint8_t> (awh::tls::extension_type_t::OID_FILTERS):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_oid_filters_t> ());
			break;
			// Если тип расширения соответствует trust_anchors
			case static_cast <uint8_t> (awh::tls::extension_type_t::TRUST_ANCHORS):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_trust_anchors_t> ());
			break;
			// Если тип расширения соответствует encrypt_then_mac
			case static_cast <uint8_t> (awh::tls::extension_type_t::ENCRYPT_THEN_MAC):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_encrypt_then_mac_t> ());
			break;
			// Если тип расширения соответствует transparency_info
			case static_cast <uint8_t> (awh::tls::extension_type_t::TRANSPARENCY_INFO):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_transparency_info_t> ());
			break;
			// Если тип расширения соответствует post_handshake_auth
			case static_cast <uint8_t> (awh::tls::extension_type_t::POST_HANDSHAKE_AUTH):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_post_handshake_auth_t> ());
			break;
			// Если тип расширения соответствует client_certificate_type
			case static_cast <uint8_t> (awh::tls::extension_type_t::CLIENT_CERTIFICATE_TYPE):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_client_certificate_type_t> ());
			break;
			// Если тип расширения соответствует server_certificate_type
			case static_cast <uint8_t> (awh::tls::extension_type_t::SERVER_CERTIFICATE_TYPE):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_server_certificate_type_t> ());
			break;
			// Если тип расширения соответствует signed_certificate_timestamp
			case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNED_CERTIFICATE_TIMESTAMP):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_signed_certificate_timestamp_t> ());
			break;
			// Если тип расширения соответствует server_name
			case static_cast <uint8_t> (awh::tls::extension_type_t::SERVER_NAME): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_server_name_t> ());
				// Копируем список имён из исходного расширения
				awh_cast <extension_server_name_t *> (this->extensions.back().get())->names = awh_cast <extension_server_name_t *> (extension.get())->names;
			} break;
			// Если тип расширения соответствует max_fragment_length
			case static_cast <uint8_t> (awh::tls::extension_type_t::MAX_FRAGMENT_LENGTH): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_max_fragment_length_t> ());
				// Копируем значение длины из исходного расширения
				awh_cast <extension_max_fragment_length_t *> (this->extensions.back().get())->length = awh_cast <extension_max_fragment_length_t *> (extension.get())->length;
			} break;
			// Если тип расширения соответствует status_request
			case static_cast <uint8_t> (awh::tls::extension_type_t::STATUS_REQUEST): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_status_request_t> ());
				// Копируем значение типа запроса статуса из исходного расширения
				awh_cast <extension_status_request_t *> (this->extensions.back().get())->certificateStatusType = awh_cast <extension_status_request_t *> (extension.get())->certificateStatusType;
				// Копируем значение длины списка идентификаторов респондентов из исходного расширения
				awh_cast <extension_status_request_t *> (this->extensions.back().get())->responderIdListLength = awh_cast <extension_status_request_t *> (extension.get())->responderIdListLength;
				// Копируем список идентификаторов респондентов из исходного расширения
				awh_cast <extension_status_request_t *> (this->extensions.back().get())->requestExtensionsLength = awh_cast <extension_status_request_t *> (extension.get())->requestExtensionsLength;
			} break;
			// Если тип расширения соответствует supported_groups
			case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_GROUPS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_supported_groups_t> ());
				// Копируем список поддерживаемых групп из исходного расширения
				awh_cast <extension_supported_groups_t *> (this->extensions.back().get())->supportedGroups = awh_cast <extension_supported_groups_t *> (extension.get())->supportedGroups;
			} break;
			// Если тип расширения соответствует ec_point_formats
			case static_cast <uint8_t> (awh::tls::extension_type_t::EC_POINT_FORMATS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_ec_point_t> ());
				// Копируем список форматов точек из исходного расширения
				awh_cast <extension_ec_point_t *> (this->extensions.back().get())->formats = awh_cast <extension_ec_point_t *> (extension.get())->formats;
			} break;
			// Если тип расширения соответствует signature_algorithms
			case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_signature_t> ());
				// Копируем список алгоритмов подписи из исходного расширения
				awh_cast <extension_signature_t *> (this->extensions.back().get())->algorithms = awh_cast <extension_signature_t *> (extension.get())->algorithms;
			} break;
			// Если тип расширения соответствует use_srtp
			case static_cast <uint8_t> (awh::tls::extension_type_t::USE_SRTP): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_use_srtp_t> ());
				// Копируем список профилей SRTP из исходного расширения
				awh_cast <extension_use_srtp_t *> (this->extensions.back().get())->profiles = awh_cast <extension_use_srtp_t *> (extension.get())->profiles;
				// Копируем значение mki_length из исходного расширения
				awh_cast <extension_use_srtp_t *> (this->extensions.back().get())->mkiLength = awh_cast <extension_use_srtp_t *> (extension.get())->mkiLength;
			} break;
			// Если тип расширения соответствует heartbeat
			case static_cast <uint8_t> (awh::tls::extension_type_t::HEARTBEAT): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_heartbeat_t> ());
				// Копируем значение режима heartbeat из исходного расширения
				awh_cast <extension_heartbeat_t *> (this->extensions.back().get())->mode = awh_cast <extension_heartbeat_t *> (extension.get())->mode;
			} break;
			// Если тип расширения соответствует alpn
			case static_cast <uint8_t> (awh::tls::extension_type_t::ALPN): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_alpn_t> ());
				// Копируем список протоколов ALPN из исходного расширения
				awh_cast <extension_alpn_t *> (this->extensions.back().get())->protocols = awh_cast <extension_alpn_t *> (extension.get())->protocols;
			} break;
			// Если тип расширения соответствует padding
			case static_cast <uint8_t> (awh::tls::extension_type_t::PADDING): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_padding_t> ());
				// Копируем значение размера паддинга из исходного расширения
				awh_cast <extension_padding_t *> (this->extensions.back().get())->size = awh_cast <extension_padding_t *> (extension.get())->size;
			} break;
			// Если тип расширения соответствует extended_master_secret
			case static_cast <uint8_t> (awh::tls::extension_type_t::EXTENDED_MASTER_SECRET): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_extended_master_secret_t> ());
				// Копируем данные master_secret из исходного расширения
				awh_cast <extension_extended_master_secret_t *> (this->extensions.back().get())->masterSecretData = awh_cast <extension_extended_master_secret_t *> (extension.get())->masterSecretData;
				// Копируем данные extended_master_secret из исходного расширения
				awh_cast <extension_extended_master_secret_t *> (this->extensions.back().get())->extendedMasterSecretData = awh_cast <extension_extended_master_secret_t *> (extension.get())->extendedMasterSecretData;
			} break;
			// Если тип расширения соответствует compress_certificate
			case static_cast <uint8_t> (awh::tls::extension_type_t::COMPRESS_CERTIFICATE): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_compress_certificate_t> ());
				// Копируем список алгоритмов сжатия сертификата из исходного расширения
				awh_cast <extension_compress_certificate_t *> (this->extensions.back().get())->algorithms = awh_cast <extension_compress_certificate_t *> (extension.get())->algorithms;
			} break;
			// Если тип расширения соответствует record_size_limit
			case static_cast <uint8_t> (awh::tls::extension_type_t::RECORD_SIZE_LIMIT): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_record_size_limit_t> ());
				// Копируем значение record_size_limit из исходного расширения
				awh_cast <extension_record_size_limit_t *> (this->extensions.back().get())->data = awh_cast <extension_record_size_limit_t *> (extension.get())->data;
			} break;
			// Если тип расширения соответствует delegated_credential
			case static_cast <uint8_t> (awh::tls::extension_type_t::DELEGATED_CREDENTIAL): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_delegated_credential_t> ());
				// Копируем список алгоритмов делегированных учётных данных из исходного расширения
				awh_cast <extension_delegated_credential_t *> (this->extensions.back().get())->algorithms = awh_cast <extension_delegated_credential_t *> (extension.get())->algorithms;
			} break;
			// Если тип расширения соответствует session_ticket
			case static_cast <uint8_t> (awh::tls::extension_type_t::SESSION_TICKET): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_session_ticket_t> ());
				// Копируем данные session_ticket из исходного расширения
				awh_cast <extension_session_ticket_t *> (this->extensions.back().get())->data = awh_cast <extension_session_ticket_t *> (extension.get())->data;
			} break;
			// Если тип расширения соответствует pre_shared_key
			case static_cast <uint8_t> (awh::tls::extension_type_t::PRE_SHARED_KEY): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_pre_shared_key_t> ());
				// Копируем список идентификаторов PSK из исходного расширения
				awh_cast <extension_pre_shared_key_t *> (this->extensions.back().get())->identities = awh_cast <extension_pre_shared_key_t *> (extension.get())->identities;
			} break;
			// Если тип расширения соответствует early_data
			case static_cast <uint8_t> (awh::tls::extension_type_t::EARLY_DATA): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_early_data_t> ());
				// Копируем значение maxSize из исходного расширения
				awh_cast <extension_early_data_t *> (this->extensions.back().get())->maxSize = awh_cast <extension_early_data_t *> (extension.get())->maxSize;
			} break;
			// Если тип расширения соответствует supported_versions
			case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_VERSIONS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_supported_versions_t> ());
				// Копируем список поддерживаемых версий TLS из исходного расширения
				awh_cast <extension_supported_versions_t *> (this->extensions.back().get())->versions = awh_cast <extension_supported_versions_t *> (extension.get())->versions;
			} break;
			// Если тип расширения соответствует cookie
			case static_cast <uint8_t> (awh::tls::extension_type_t::COOKIE): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_cookie_t> ());
				// Копируем данные расширения cookie из исходного расширения
				awh_cast <extension_cookie_t *> (this->extensions.back().get())->data = awh_cast <extension_cookie_t *> (extension.get())->data;
			} break;
			// Если тип расширения соответствует psk_key_exchange_modes
			case static_cast <uint8_t> (awh::tls::extension_type_t::PSK_KEY_EXCHANGE_MODES): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_psk_key_exchange_t> ());
				// Копируем список режимов обмена ключами PSK из исходного расширения
				awh_cast <extension_psk_key_exchange_t *> (this->extensions.back().get())->modes = awh_cast <extension_psk_key_exchange_t *> (extension.get())->modes;
			} break;
			// Если тип расширения соответствует certificate_authorities
			case static_cast <uint8_t> (awh::tls::extension_type_t::CERTIFICATE_AUTHORITIES): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_certificate_authorities_t> ());
				// Копируем список авторитетов сертификатов из исходного расширения
				awh_cast <extension_certificate_authorities_t *> (this->extensions.back().get())->authorities = awh_cast <extension_certificate_authorities_t *> (extension.get())->authorities;
			} break;
			// Если тип расширения соответствует signature_algorithms_cert
			case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS_CERT): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_signature_algorithms_cert_t> ());
				// Копируем список алгоритмов подписи сертификатов из исходного расширения
				awh_cast <extension_signature_algorithms_cert_t *> (this->extensions.back().get())->algorithms = awh_cast <extension_signature_algorithms_cert_t *> (extension.get())->algorithms;
			} break;
			// Если тип расширения соответствует key_share
			case static_cast <uint8_t> (awh::tls::extension_type_t::KEY_SHARE): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_key_share_t> ());
				// Копируем список ключей для обмена из исходного расширения
				awh_cast <extension_key_share_t *> (this->extensions.back().get())->shares = awh_cast <extension_key_share_t *> (extension.get())->shares;
			} break;
			// Если тип расширения соответствует quic_transport_parameters
			case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_quic_transport_params_t> ());
				// Копируем список параметров транспортного уровня QUIC из исходного расширения
				awh_cast <extension_quic_transport_params_t *> (this->extensions.back().get())->params = awh_cast <extension_quic_transport_params_t *> (extension.get())->params;
			} break;
			// Если тип расширения соответствует tls_flags
			case static_cast <uint8_t> (awh::tls::extension_type_t::TLS_FLAGS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_tls_flags_t> ());
				// Копируем список флагов TLS из исходного расширения
				awh_cast <extension_tls_flags_t *> (this->extensions.back().get())->flags = awh_cast <extension_tls_flags_t *> (extension.get())->flags;
			} break;
			// Если тип расширения соответствует next_protocol_negotiation
			case static_cast <uint8_t> (awh::tls::extension_type_t::NEXT_PROTO_NEG): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_next_proto_neg_t> ());
				// Копируем список протоколов NPN из исходного расширения
				awh_cast <extension_next_proto_neg_t *> (this->extensions.back().get())->protocols = awh_cast <extension_next_proto_neg_t *> (extension.get())->protocols;
			} break;
			// Если тип расширения соответствует application_settings_old (устаревшее)
			case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS_OLD): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_application_settings_old_t> ());
				// Копируем список протоколов из исходного расширения
				awh_cast <extension_application_settings_old_t *> (this->extensions.back().get())->protocols = awh_cast <extension_application_settings_old_t *> (extension.get())->protocols;
			} break;
			// Если тип расширения соответствует application_settings
			case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_application_settings_t> ());
				// Копируем список протоколов из исходного расширения
				awh_cast <extension_application_settings_t *> (this->extensions.back().get())->protocols = awh_cast <extension_application_settings_t *> (extension.get())->protocols;
			} break;
			// Если тип расширения соответствует ech_outer_extensions
			case static_cast <uint8_t> (awh::tls::extension_type_t::ECH_OUTER_EXTENSIONS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_ech_outer_extensions_t> ());
				// Копируем список расширений из исходного расширения
				awh_cast <extension_ech_outer_extensions_t *> (this->extensions.back().get())->extensions = awh_cast <extension_ech_outer_extensions_t *> (extension.get())->extensions;
			} break;
			// Если тип расширения соответствует encrypted_client_hello
			case static_cast <uint8_t> (awh::tls::extension_type_t::ENCRYPTED_CLIENT_HELLO): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_encryption_client_hello_t> ());
				// Копируем данные расширения encrypted_client_hello из исходного расширения
				awh_cast <extension_encryption_client_hello_t *> (this->extensions.back().get())->data = awh_cast <extension_encryption_client_hello_t *> (extension.get())->data;
			} break;
			// Если тип расширения соответствует renegotiation_info
			case static_cast <uint8_t> (awh::tls::extension_type_t::RENEGOTIATION_INFO): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_renegotiation_info_t> ());
				// Копируем данные расширения renegotiation_info из исходного расширения
				awh_cast <extension_renegotiation_info_t *> (this->extensions.back().get())->data = awh_cast <extension_renegotiation_info_t *> (extension.get())->data;
			} break;
			// Если тип расширения соответствует quic_transport_parameters_legacy
			case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS_LEGACY): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_quic_transport_params_legacy_t> ());
				// Копируем список параметров транспортного уровня QUIC из исходного расширения
				awh_cast <extension_quic_transport_params_legacy_t *> (this->extensions.back().get())->params = awh_cast <extension_quic_transport_params_legacy_t *> (extension.get())->params;
			} break;
		}
	}
	// Возвращаем текущий объект после перемещения
	return (* this);
}
/**
 * @brief Конструктор перемещения
 *
 * @param browser объект цифрового отпечатка браузера для перемещения
 *
 */
awh::tls::Fingerprint::Browser::Browser(browser_t && browser) noexcept {
	// Копируем флаг поддержки GREASE
	this->grease = browser.grease;
	// Сбрасываем флаг в исходном объекте (необязательно, но подчёркивает что данные перемещены)
	browser.grease = false;
	// Перемещаем объект записи TLS рукопожатия
	this->record = ::move(browser.record);
	// Перемещаем объект рукопожатия TLS
	this->handshake = ::move(browser.handshake);
	// Перемещаем объект ClientHello
	this->clientHello = ::move(browser.clientHello);
	// Перемещаем объект cookie рукопожатия DTLS
	this->cookie = ::move(browser.cookie);
	// Перемещаем объект идентификатора сессии TLS
	this->session = ::move(browser.session);
	// Перемещаем список поддерживаемых шифров
	this->ciphers = ::move(browser.ciphers);
	// Перемещаем список поддерживаемых методов сжатия TLS
	this->compressors = ::move(browser.compressors);
	// Перемещаем список поддерживаемых расширений TLS
	this->extensions = ::move(browser.extensions);
}
/**
 * @brief Конструктор копирования
 *
 * @param browser объект цифрового отпечатка браузера для копирования
 *
 */
awh::tls::Fingerprint::Browser::Browser(const browser_t & browser) noexcept {
	// Копируем флаг поддержки GREASE
	this->grease = browser.grease;
	// Копируем объект записи TLS рукопожатия
	this->record = browser.record;
	// Копируем объект рукопожатия TLS
	this->handshake = browser.handshake;
	// Копируем объект ClientHello
	this->clientHello = browser.clientHello;
	// Копируем объект cookie рукопожатия DTLS
	this->cookie = browser.cookie;
	// Копируем объект идентификатора сессии TLS
	this->session = browser.session;
	// Копируем список поддерживаемых шифров
	this->ciphers = browser.ciphers;
	// Копируем список поддерживаемых методов сжатия TLS
	this->compressors = browser.compressors;
	// Очищаем список расширений в текущем объекте, чтобы подготовиться к копированию расширений из исходного объекта
	this->extensions.clear();
	/**
	 * Перебираем расширения в исходном объекте и восстанавливаем их в текущем объекте в зависимости от типа
	 */
	for(auto & extension : browser.extensions){
		/**
		 * Восстанавливаем объект расширения в зависимости от типа
		 */
		switch(static_cast <uint8_t> (extension->type)){
			// Если тип расширения соответствует grease
			case static_cast <uint8_t> (awh::tls::extension_type_t::GREASE):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_grease_t> ());
			break;
			// Если тип расширения соответствует channel_id
			case static_cast <uint8_t> (awh::tls::extension_type_t::CHANNEL_ID):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_channel_id_t> ());
			break;
			// Если тип расширения соответствует oid_filters
			case static_cast <uint8_t> (awh::tls::extension_type_t::OID_FILTERS):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_oid_filters_t> ());
			break;
			// Если тип расширения соответствует trust_anchors
			case static_cast <uint8_t> (awh::tls::extension_type_t::TRUST_ANCHORS):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_trust_anchors_t> ());
			break;
			// Если тип расширения соответствует encrypt_then_mac
			case static_cast <uint8_t> (awh::tls::extension_type_t::ENCRYPT_THEN_MAC):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_encrypt_then_mac_t> ());
			break;
			// Если тип расширения соответствует transparency_info
			case static_cast <uint8_t> (awh::tls::extension_type_t::TRANSPARENCY_INFO):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_transparency_info_t> ());
			break;
			// Если тип расширения соответствует post_handshake_auth
			case static_cast <uint8_t> (awh::tls::extension_type_t::POST_HANDSHAKE_AUTH):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_post_handshake_auth_t> ());
			break;
			// Если тип расширения соответствует client_certificate_type
			case static_cast <uint8_t> (awh::tls::extension_type_t::CLIENT_CERTIFICATE_TYPE):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_client_certificate_type_t> ());
			break;
			// Если тип расширения соответствует server_certificate_type
			case static_cast <uint8_t> (awh::tls::extension_type_t::SERVER_CERTIFICATE_TYPE):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_server_certificate_type_t> ());
			break;
			// Если тип расширения соответствует signed_certificate_timestamp
			case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNED_CERTIFICATE_TIMESTAMP):
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_signed_certificate_timestamp_t> ());
			break;
			// Если тип расширения соответствует server_name
			case static_cast <uint8_t> (awh::tls::extension_type_t::SERVER_NAME): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_server_name_t> ());
				// Копируем список имён из исходного расширения
				awh_cast <extension_server_name_t *> (this->extensions.back().get())->names = awh_cast <extension_server_name_t *> (extension.get())->names;
			} break;
			// Если тип расширения соответствует max_fragment_length
			case static_cast <uint8_t> (awh::tls::extension_type_t::MAX_FRAGMENT_LENGTH): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_max_fragment_length_t> ());
				// Копируем значение длины из исходного расширения
				awh_cast <extension_max_fragment_length_t *> (this->extensions.back().get())->length = awh_cast <extension_max_fragment_length_t *> (extension.get())->length;
			} break;
			// Если тип расширения соответствует status_request
			case static_cast <uint8_t> (awh::tls::extension_type_t::STATUS_REQUEST): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_status_request_t> ());
				// Копируем значение типа запроса статуса из исходного расширения
				awh_cast <extension_status_request_t *> (this->extensions.back().get())->certificateStatusType = awh_cast <extension_status_request_t *> (extension.get())->certificateStatusType;
				// Копируем значение длины списка идентификаторов респондентов из исходного расширения
				awh_cast <extension_status_request_t *> (this->extensions.back().get())->responderIdListLength = awh_cast <extension_status_request_t *> (extension.get())->responderIdListLength;
				// Копируем список идентификаторов респондентов из исходного расширения
				awh_cast <extension_status_request_t *> (this->extensions.back().get())->requestExtensionsLength = awh_cast <extension_status_request_t *> (extension.get())->requestExtensionsLength;
			} break;
			// Если тип расширения соответствует supported_groups
			case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_GROUPS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_supported_groups_t> ());
				// Копируем список поддерживаемых групп из исходного расширения
				awh_cast <extension_supported_groups_t *> (this->extensions.back().get())->supportedGroups = awh_cast <extension_supported_groups_t *> (extension.get())->supportedGroups;
			} break;
			// Если тип расширения соответствует ec_point_formats
			case static_cast <uint8_t> (awh::tls::extension_type_t::EC_POINT_FORMATS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_ec_point_t> ());
				// Копируем список форматов точек из исходного расширения
				awh_cast <extension_ec_point_t *> (this->extensions.back().get())->formats = awh_cast <extension_ec_point_t *> (extension.get())->formats;
			} break;
			// Если тип расширения соответствует signature_algorithms
			case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_signature_t> ());
				// Копируем список алгоритмов подписи из исходного расширения
				awh_cast <extension_signature_t *> (this->extensions.back().get())->algorithms = awh_cast <extension_signature_t *> (extension.get())->algorithms;
			} break;
			// Если тип расширения соответствует use_srtp
			case static_cast <uint8_t> (awh::tls::extension_type_t::USE_SRTP): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_use_srtp_t> ());
				// Копируем список профилей SRTP из исходного расширения
				awh_cast <extension_use_srtp_t *> (this->extensions.back().get())->profiles = awh_cast <extension_use_srtp_t *> (extension.get())->profiles;
				// Копируем значение mki_length из исходного расширения
				awh_cast <extension_use_srtp_t *> (this->extensions.back().get())->mkiLength = awh_cast <extension_use_srtp_t *> (extension.get())->mkiLength;
			} break;
			// Если тип расширения соответствует heartbeat
			case static_cast <uint8_t> (awh::tls::extension_type_t::HEARTBEAT): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_heartbeat_t> ());
				// Копируем значение режима heartbeat из исходного расширения
				awh_cast <extension_heartbeat_t *> (this->extensions.back().get())->mode = awh_cast <extension_heartbeat_t *> (extension.get())->mode;
			} break;
			// Если тип расширения соответствует alpn
			case static_cast <uint8_t> (awh::tls::extension_type_t::ALPN): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_alpn_t> ());
				// Копируем список протоколов ALPN из исходного расширения
				awh_cast <extension_alpn_t *> (this->extensions.back().get())->protocols = awh_cast <extension_alpn_t *> (extension.get())->protocols;
			} break;
			// Если тип расширения соответствует padding
			case static_cast <uint8_t> (awh::tls::extension_type_t::PADDING): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_padding_t> ());
				// Копируем значение размера паддинга из исходного расширения
				awh_cast <extension_padding_t *> (this->extensions.back().get())->size = awh_cast <extension_padding_t *> (extension.get())->size;
			} break;
			// Если тип расширения соответствует extended_master_secret
			case static_cast <uint8_t> (awh::tls::extension_type_t::EXTENDED_MASTER_SECRET): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_extended_master_secret_t> ());
				// Копируем данные master_secret из исходного расширения
				awh_cast <extension_extended_master_secret_t *> (this->extensions.back().get())->masterSecretData = awh_cast <extension_extended_master_secret_t *> (extension.get())->masterSecretData;
				// Копируем данные extended_master_secret из исходного расширения
				awh_cast <extension_extended_master_secret_t *> (this->extensions.back().get())->extendedMasterSecretData = awh_cast <extension_extended_master_secret_t *> (extension.get())->extendedMasterSecretData;
			} break;
			// Если тип расширения соответствует compress_certificate
			case static_cast <uint8_t> (awh::tls::extension_type_t::COMPRESS_CERTIFICATE): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_compress_certificate_t> ());
				// Копируем список алгоритмов сжатия сертификата из исходного расширения
				awh_cast <extension_compress_certificate_t *> (this->extensions.back().get())->algorithms = awh_cast <extension_compress_certificate_t *> (extension.get())->algorithms;
			} break;
			// Если тип расширения соответствует record_size_limit
			case static_cast <uint8_t> (awh::tls::extension_type_t::RECORD_SIZE_LIMIT): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_record_size_limit_t> ());
				// Копируем значение record_size_limit из исходного расширения
				awh_cast <extension_record_size_limit_t *> (this->extensions.back().get())->data = awh_cast <extension_record_size_limit_t *> (extension.get())->data;
			} break;
			// Если тип расширения соответствует delegated_credential
			case static_cast <uint8_t> (awh::tls::extension_type_t::DELEGATED_CREDENTIAL): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_delegated_credential_t> ());
				// Копируем список алгоритмов делегированных учётных данных из исходного расширения
				awh_cast <extension_delegated_credential_t *> (this->extensions.back().get())->algorithms = awh_cast <extension_delegated_credential_t *> (extension.get())->algorithms;
			} break;
			// Если тип расширения соответствует session_ticket
			case static_cast <uint8_t> (awh::tls::extension_type_t::SESSION_TICKET): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_session_ticket_t> ());
				// Копируем данные session_ticket из исходного расширения
				awh_cast <extension_session_ticket_t *> (this->extensions.back().get())->data = awh_cast <extension_session_ticket_t *> (extension.get())->data;
			} break;
			// Если тип расширения соответствует pre_shared_key
			case static_cast <uint8_t> (awh::tls::extension_type_t::PRE_SHARED_KEY): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_pre_shared_key_t> ());
				// Копируем список идентификаторов PSK из исходного расширения
				awh_cast <extension_pre_shared_key_t *> (this->extensions.back().get())->identities = awh_cast <extension_pre_shared_key_t *> (extension.get())->identities;
			} break;
			// Если тип расширения соответствует early_data
			case static_cast <uint8_t> (awh::tls::extension_type_t::EARLY_DATA): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_early_data_t> ());
				// Копируем значение maxSize из исходного расширения
				awh_cast <extension_early_data_t *> (this->extensions.back().get())->maxSize = awh_cast <extension_early_data_t *> (extension.get())->maxSize;
			} break;
			// Если тип расширения соответствует supported_versions
			case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_VERSIONS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_supported_versions_t> ());
				// Копируем список поддерживаемых версий TLS из исходного расширения
				awh_cast <extension_supported_versions_t *> (this->extensions.back().get())->versions = awh_cast <extension_supported_versions_t *> (extension.get())->versions;
			} break;
			// Если тип расширения соответствует cookie
			case static_cast <uint8_t> (awh::tls::extension_type_t::COOKIE): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_cookie_t> ());
				// Копируем данные расширения cookie из исходного расширения
				awh_cast <extension_cookie_t *> (this->extensions.back().get())->data = awh_cast <extension_cookie_t *> (extension.get())->data;
			} break;
			// Если тип расширения соответствует psk_key_exchange_modes
			case static_cast <uint8_t> (awh::tls::extension_type_t::PSK_KEY_EXCHANGE_MODES): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_psk_key_exchange_t> ());
				// Копируем список режимов обмена ключами PSK из исходного расширения
				awh_cast <extension_psk_key_exchange_t *> (this->extensions.back().get())->modes = awh_cast <extension_psk_key_exchange_t *> (extension.get())->modes;
			} break;
			// Если тип расширения соответствует certificate_authorities
			case static_cast <uint8_t> (awh::tls::extension_type_t::CERTIFICATE_AUTHORITIES): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_certificate_authorities_t> ());
				// Копируем список авторитетов сертификатов из исходного расширения
				awh_cast <extension_certificate_authorities_t *> (this->extensions.back().get())->authorities = awh_cast <extension_certificate_authorities_t *> (extension.get())->authorities;
			} break;
			// Если тип расширения соответствует signature_algorithms_cert
			case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS_CERT): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_signature_algorithms_cert_t> ());
				// Копируем список алгоритмов подписи сертификатов из исходного расширения
				awh_cast <extension_signature_algorithms_cert_t *> (this->extensions.back().get())->algorithms = awh_cast <extension_signature_algorithms_cert_t *> (extension.get())->algorithms;
			} break;
			// Если тип расширения соответствует key_share
			case static_cast <uint8_t> (awh::tls::extension_type_t::KEY_SHARE): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_key_share_t> ());
				// Копируем список ключей для обмена из исходного расширения
				awh_cast <extension_key_share_t *> (this->extensions.back().get())->shares = awh_cast <extension_key_share_t *> (extension.get())->shares;
			} break;
			// Если тип расширения соответствует quic_transport_parameters
			case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_quic_transport_params_t> ());
				// Копируем список параметров транспортного уровня QUIC из исходного расширения
				awh_cast <extension_quic_transport_params_t *> (this->extensions.back().get())->params = awh_cast <extension_quic_transport_params_t *> (extension.get())->params;
			} break;
			// Если тип расширения соответствует tls_flags
			case static_cast <uint8_t> (awh::tls::extension_type_t::TLS_FLAGS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_tls_flags_t> ());
				// Копируем список флагов TLS из исходного расширения
				awh_cast <extension_tls_flags_t *> (this->extensions.back().get())->flags = awh_cast <extension_tls_flags_t *> (extension.get())->flags;
			} break;
			// Если тип расширения соответствует next_protocol_negotiation
			case static_cast <uint8_t> (awh::tls::extension_type_t::NEXT_PROTO_NEG): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_next_proto_neg_t> ());
				// Копируем список протоколов NPN из исходного расширения
				awh_cast <extension_next_proto_neg_t *> (this->extensions.back().get())->protocols = awh_cast <extension_next_proto_neg_t *> (extension.get())->protocols;
			} break;
			// Если тип расширения соответствует application_settings_old (устаревшее)
			case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS_OLD): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_application_settings_old_t> ());
				// Копируем список протоколов из исходного расширения
				awh_cast <extension_application_settings_old_t *> (this->extensions.back().get())->protocols = awh_cast <extension_application_settings_old_t *> (extension.get())->protocols;
			} break;
			// Если тип расширения соответствует application_settings
			case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_application_settings_t> ());
				// Копируем список протоколов из исходного расширения
				awh_cast <extension_application_settings_t *> (this->extensions.back().get())->protocols = awh_cast <extension_application_settings_t *> (extension.get())->protocols;
			} break;
			// Если тип расширения соответствует ech_outer_extensions
			case static_cast <uint8_t> (awh::tls::extension_type_t::ECH_OUTER_EXTENSIONS): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_ech_outer_extensions_t> ());
				// Копируем список расширений из исходного расширения
				awh_cast <extension_ech_outer_extensions_t *> (this->extensions.back().get())->extensions = awh_cast <extension_ech_outer_extensions_t *> (extension.get())->extensions;
			} break;
			// Если тип расширения соответствует encrypted_client_hello
			case static_cast <uint8_t> (awh::tls::extension_type_t::ENCRYPTED_CLIENT_HELLO): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_encryption_client_hello_t> ());
				// Копируем данные расширения encrypted_client_hello из исходного расширения
				awh_cast <extension_encryption_client_hello_t *> (this->extensions.back().get())->data = awh_cast <extension_encryption_client_hello_t *> (extension.get())->data;
			} break;
			// Если тип расширения соответствует renegotiation_info
			case static_cast <uint8_t> (awh::tls::extension_type_t::RENEGOTIATION_INFO): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_renegotiation_info_t> ());
				// Копируем данные расширения renegotiation_info из исходного расширения
				awh_cast <extension_renegotiation_info_t *> (this->extensions.back().get())->data = awh_cast <extension_renegotiation_info_t *> (extension.get())->data;
			} break;
			// Если тип расширения соответствует quic_transport_parameters_legacy
			case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS_LEGACY): {
				// Добавляем расширение в список
				this->extensions.push_back(make_unique <extension_quic_transport_params_legacy_t> ());
				// Копируем список параметров транспортного уровня QUIC из исходного расширения
				awh_cast <extension_quic_transport_params_legacy_t *> (this->extensions.back().get())->params = awh_cast <extension_quic_transport_params_legacy_t *> (extension.get())->params;
			} break;
		}
	}
}
/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::Browser::Browser() noexcept : grease(false) {}

/**
 * @brief Конструктор
 *
 * @param i идентификатор параметра
 * @param v значение параметра
 *
 */
awh::tls::Fingerprint::H2Setting::H2Setting(const uint16_t i, const uint32_t v) noexcept : id(i), value(v) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::H2Priority::H2Priority() noexcept :
 exclusive(false), weight(0), streamId(0),  dependency(0) {}

/**
 * @brief Конструктор
 *
 */
awh::tls::Fingerprint::H2Browser::H2Browser() noexcept : windowUpdate(0) {}

/**
 * @brief Оператор преобразования в сырой итератор
 *
 * @return iterator итератор для преобразования
 *
 */
awh::tls::Fingerprint::Iterator::operator awh::tls::Fingerprint::Iterator::iterator() noexcept {
	// Возвращаем текущее значение итератора
	return this->_it;
}
/**
 * @brief Оператор извлечения указателя заголовка
 *
 * @return указатель заголовка
 *
 */
awh::tls::Fingerprint::Iterator::pointer awh::tls::Fingerprint::Iterator::operator -> () noexcept {
	// Возвращаем результат
	return &this->_it->second;
}
/**
 * @brief Оператор разыменования заголовка
 *
 * @return значение заголовка
 *
 */
awh::tls::Fingerprint::Iterator::reference awh::tls::Fingerprint::Iterator::operator * () const noexcept {
	// Возвращаем результат
	return this->_it->second;
}
/**
 * @brief Оператор смещения вперед
 *
 * @return значение текущего итератора
 *
 */
awh::tls::Fingerprint::Iterator & awh::tls::Fingerprint::Iterator::operator ++ () noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем смещение итератора
		++this->_it;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return (* this);
}
/**
 * @brief Оператор сравнения соответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 *
 */
bool awh::tls::Fingerprint::Iterator::operator == (const Iterator & other) const noexcept {
	// Возвращаем результат
	return (this->_it == other._it);
}
/**
 * @brief Оператора сравнения несоответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 *
 */
bool awh::tls::Fingerprint::Iterator::operator != (const Iterator & other) const noexcept {
	// Возвращаем результат
	return (this->_it != other._it);
}
/**
 * @brief Конструктор
 *
 * @param it  итератор для установки
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::tls::Fingerprint::Iterator::Iterator(iterator it, const fmk_t * fmk, const log_t * log) noexcept : _it(it), _fmk(fmk), _log(log) {}

/**
 * @brief Метод форматированного вывода всех данных цифрового отпечатка браузера
 *
 * @details Распечатывает в читаемом текстовом виде все поля browser_t (Record Layer,
 *          Handshake, ClientHello, Cipher Suites, Compressors, Extensions), а также
 *          вычисляет и печатает все отпечатки imprint_t (JA3, JA4, JA4_r, PeetPrint).
 *
 * @param browser объект с распарсенными данными ClientHello
 * @return        форматированная строка с полным описанием отпечатка
 *
 */
string awh::tls::Fingerprint::print(const browser_t & browser) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Строковый поток для сборки результата
		ostringstream out;
		// Горизонтальная линия-разделитель (80 символов)
		const string LINE(80, '=');
		/**
		 * @brief Вспомогательная лямбда: форматирование uint16_t как "0x%04X"
		 *
		 * @param num число для форматирования
		 * @return    строка вида "0x%04X"
		 *
		 */
		const auto hex16 = [](const uint16_t num) -> string {
			// Переменная результата
			char result[8];
			// Форматируем число в виде "0x%04X"
			::snprintf(result, sizeof(result), "0x%04X", num);
			// Возвращаем результат
			return result;
		};
		/**
		 * @brief Вспомогательная лямбда: форматирование uint8_t как "0x%02X"
		 *
		 * @param num число для форматирования
		 * @return    строка вида "0x%02X"
		 *
		 */
		const auto hex8 = [](const uint8_t num) -> string {
			// Переменная результата
			char result[6];
			// Форматируем число в виде "0x%02X"
			::snprintf(result, sizeof(result), "0x%02X", num);
			// Возвращаем результат
			return result;
		};
		/**
		 * @brief Вспомогательная лямбда: секционный заголовок с количеством элементов
		 *
		 * @param title название секции
		 *
		 */
		const auto section = [&out](const string & title) -> void {
			// Печатаем заголовок в отладочный вывод секции с отступами и линиями-разделителями
			out << endl << "  [ " << title << " ]" << endl;
		};
		// ================================================================
		// Заголовок
		// ================================================================
		out << endl << LINE << endl;
		out << "                  TLS ClientHello  -  Browser Fingerprint" << endl;
		out << LINE << endl;
		// ================================================================
		// Record Layer
		// ================================================================
		/**
		 * Формируем секцию Record Layer
		 */
		section("Record Layer");
		{
			// Получаем wire-код версии record layer
			const uint16_t wire = ::local::versionWire(browser.record.version);
			// Возвращаем версию и её wire-код
			out << "    Version  : " << ::local::tlsVersionName(wire) << "  (" << hex16(wire) << ")" << endl;
			// Возвращаем длину записи Record Layer
			out << "    Length   : " << browser.record.length << endl;
			// Поля DTLS выводим только если они ненулевые
			if((browser.record.epoch != 0) || (browser.record.sequence != 0)){
				// Возвращаем поле эпохи для DTLS
				out << "    Epoch    : " << browser.record.epoch    << "  [DTLS]" << endl;
				// Возвращаем поле последовательности для DTLS, выводим как десятичное (можно было бы и hex, но десятичный формат привычнее для DTLS)
				out << "    Sequence : " << browser.record.sequence << "  [DTLS]" << endl;
			}
		}
		// ================================================================
		// Handshake Header
		// ================================================================
		/**
		 * Формируем секцию Handshake Header
		 */
		section("Handshake Header");
		// Возвращаем длину Handshake, которая может отличаться от длины Record Layer при фрагментации (обычно в DTLS)
		out << "    Length       : " << browser.handshake.length << endl;
		// Возвращаем последовательность Handshake, которая может отличаться от 0 при фрагментации (обычно в DTLS)
		out << "    Sequence     : " << browser.handshake.sequence << endl;
		// Возвращаем смещение фрагмента Handshake, которое может отличаться от 0 при фрагментации (обычно в DTLS)
		out << "    Frag. Offset : " << browser.handshake.fragment.offset << endl;
		// Возвращаем длину фрагмента Handshake, которая может отличаться от общей длины Handshake при фрагментации (обычно в DTLS)
		out << "    Frag. Length : " << browser.handshake.fragment.length << endl;
		// ================================================================
		// ClientHello
		// ================================================================
		/**
		 * Формируем секцию ClientHello
		 */
		section("ClientHello");
		{
			// Получаем wire-код версии legacy_version из ClientHello
			const uint16_t wire = ::local::versionWire(browser.clientHello.version);
			// Возвращаем legacy_version и его wire-код
			out << "    Legacy Version : " << ::local::tlsVersionName(wire) << "  (" << hex16(wire) << ")" << endl;
			// Возвращаем произвольные байты в 16-ричном формате
			out << "    Random         : " << ::local::tohex(browser.clientHello.random.data(), browser.clientHello.random.size()) << endl;
			// Session ID: выводим hex или пометку "(empty)" если отсутствует
			if(!browser.session.empty())
				// Возвращаем Session ID в 16-ричном формате
				out << "    Session ID     : " << ::local::tohex(browser.session.data(), browser.session.size()) << endl;
			// Если Session ID отсутствует, выводим пометку "(empty)"
			else out << "    Session ID     : (empty)" << endl;
			// Cookie DTLS: выводим только если присутствует
			if(!browser.cookie.empty())
				// Возвращаем DTLS Cookie в 16-ричном формате
				out << "    DTLS Cookie    : " << ::local::tohex(browser.cookie.data(), browser.cookie.size()) << endl;
			// Возвращаем наличие GREASE в ClientHello (по наличию GREASE-значений в версиях, шифрах, группах и расширениях)
			out << "    GREASE Used    : " << (browser.grease ? "yes" : "no") << endl;
		}
		// ================================================================
		// Cipher Suites
		// ================================================================
		{
			// Буфер для заголовка секции с количеством шифров
			char title[64];
			// Формируем заголовок секции Cipher Suites с количеством шифров в скобках
			::snprintf(title, sizeof(title), "Cipher Suites (%zu)", browser.ciphers.size());
			/**
			 * Формируем секцию Cipher Suites с заголовком, включающим количество шифров
			 */
			section(title);
		}
		/**
		 * Перебираем все шифры из ClientHello и выводим их имена с выравниванием, а также их wire-коды в виде "0x%04X"
		 */
		for(size_t i = 0; i < browser.ciphers.size(); ++i){
			// Получаем wire-код шифра
			const uint16_t wire = ::local::cipherWire(browser.ciphers[i]);
			// Имя шифра: GREASE или реальное название
			const char * name = (browser.ciphers[i] == cipher_t::GREASE) ? "[GREASE]" : ::local::cipherName(wire);
			// Выравниваем имена по столбцу 40
			char index[8];
			// Формируем индекс шифра в виде "[ i ]"
			::snprintf(index, sizeof(index), "[%2zu]", i);
			// Печатаем индекс, имя шифра с выравниванием и его wire-код в виде "0x%04X"
			out << "    " << index << "  ";
			// Печатаем имя шифра с выравниванием по столбцу 40
			out << name;
			// Вычисляем количество пробелов для выравнивания по столбцу 40
			const int32_t pad = (36 - static_cast <int32_t> (::strlen(name)));
			/**
			 * Печатаем пробелы для выравнивания
			 */
			for(int32_t j = 0; j < pad; ++j)
				// Печатаем пробел для выравнивания
				out << ' ';
			// Печатаем wire-код шифра в виде "0x%04X"
			out << "(" << hex16(wire) << ")" << endl;
		}
		// ================================================================
		// Compression Methods
		// ================================================================
		{
			// Буфер для заголовка секции с количеством методов сжатия
			char title[64];
			// Формируем заголовок секции Compression Methods с количеством методов сжатия в скобках
			::snprintf(title, sizeof(title), "Compression Methods (%zu)", browser.compressors.size());
			/**
			 * Формируем секцию Compression Methods с заголовком, включающим количество методов сжатия
			 */
			section(title);
		}
		/**
		 * Перебираем все методы сжатия из ClientHello и выводим их имена с выравниванием, а также их wire-коды в виде "0x%02X"
		 */
		for(size_t i = 0; i < browser.compressors.size(); ++i){
			// Получаем wire-код метода сжатия
			const uint8_t wire = ::local::compressorWire(browser.compressors[i]);
			// Буфер для индекса метода сжатия
			char index[8];
			// Формируем индекс метода сжатия в виде "[ i ]"
			::snprintf(index, sizeof(index), "[%2zu]", i);
			// Печатаем индекс, имя метода сжатия и его wire-код в виде "0x%02X"
			out << "    " << index << "  " << ::local::compressALGName(wire) << "  (" << hex8(wire) << ")" << endl;
		}
		// ================================================================
		// Extensions
		// ================================================================
		{
			// Буфер для заголовка секции с количеством расширений
			char title[64];
			// Формируем заголовок секции Extensions с количеством расширений в скобках
			::snprintf(title, sizeof(title), "Extensions (%zu)", browser.extensions.size());
			/**
			 * Формируем секцию Extensions с заголовком, включающим количество расширений
			 */
			section(title);
		}
		/**
		 * Перебираем все расширения из ClientHello и выводим их имена с выравниванием, а также их wire-коды в виде "0x%04X"
		 */
		for(size_t i = 0; i < browser.extensions.size(); ++i){
			// Получаем объект расширения
			const auto & ext = browser.extensions[i];
			// Получаем wire-код расширения (0xFFFF — неизвестное)
			const uint16_t wire = ::local::extensionWire(ext->type);
			// Буфер для индекса расширения
			char index[8];
			// Формируем индекс расширения в виде "[ i ]"
			::snprintf(index, sizeof(index), "[%2zu]", i);
			// GREASE расширение: выводим специальный маркер и переходим к следующему
			if(ext->type == extension_type_t::GREASE){
				// Печатаем индекс расширения и пометку "[GREASE]"
				out << "    " << index << "  [GREASE]" << endl;
				// Переходим к следующему расширению без добавления деталей
				continue;
			}
			// Имя расширения с выравниванием по столбцу 50
			const char * ename = ::local::extensionName(wire);
			// Печатаем индекс расширения
			out << "    " << index << "  ";
			// Печатаем имя расширения с выравниванием по столбцу 50
			out << ename;
			// Вычисляем количество пробелов для выравнивания по столбцу 50
			const int32_t pad = (46 - static_cast <int32_t> (::strlen(ename)));
			/**
			 * Печатаем пробелы для выравнивания
			 */
			for(int32_t j = 0; j < pad; ++j)
				// Печатаем пробел для выравнивания
				out << ' ';
			// Печатаем wire-код расширения в виде "0x%04X"
			out << "(" << hex16(wire) << ")";
			/**
			 * Детали расширения в зависимости от его типа
			 */
			switch(static_cast <uint8_t> (ext->type)){
				// server_name: выводим имена серверов
				case static_cast <uint8_t> (extension_type_t::SERVER_NAME): {
					// Получаем объект расширения server_name
					const auto * p = awh_cast <const extension_server_name_t *> (ext.get());
					// Если список имён серверов не пустой, выводим их
					if(!p->names.empty()){
						// Возвращаем первый сервер с маркером "->"
						out << "  ->  \"" << p->names[0] << '"';
						/**
						 * Перебираем оставшиеся имена серверов (если есть)
						 */
						for(size_t j = 1; j < p->names.size(); ++j)
							// Возвращаем последующие серверы, разделяя запятой
							out << ", \"" << p->names[j] << '"';
					}
				} break;
				// supported_versions: выводим список версий с GREASE-маркерами
				case static_cast <uint8_t> (extension_type_t::SUPPORTED_VERSIONS): {
					// Получаем объект расширения supported_versions
					const auto * p = awh_cast <const extension_supported_versions_t *> (ext.get());
					// Печатаем маркер "->" перед списком версий
					out << "  ->  ";
					/**
					 * Перебираем версии и выводим их, разделяя запятой
					 */
					for(size_t j = 0; j < p->versions.size(); ++j){
						// Если это не первая версия
						if(j > 0)
							// Печатаем запятую для разделения версий
							out << ", ";
						// Если версия представляет из себя GREASE
						if(p->versions[j] == version_t::GREASE)
							// Печатаем специальный маркер для GREASE-версии
							out << "[GREASE]";
						// Печатаем имя версии по её wire-коду
						else out << ::local::tlsVersionName(::local::versionWire(p->versions[j]));
					}
				} break;
				// supported_groups: выводим список кривых и групп с GREASE-маркерами
				case static_cast <uint8_t> (extension_type_t::SUPPORTED_GROUPS): {
					// Получаем объект расширения supported_groups
					const auto * p = awh_cast <const extension_supported_groups_t *> (ext.get());
					// Печатаем маркер "->" перед списком групп
					out << "  ->  ";
					/**
					 * Перебираем группы и выводим их, разделяя запятой
					 */
					for(size_t j = 0; j < p->supportedGroups.size(); ++j){
						// Если это не первая группа
						if(j > 0)
							// Печатаем запятую для разделения групп
							out << ", ";
						// Если группа представляет из себя GREASE
						if(p->supportedGroups[j] == group_t::GREASE)
							// Печатаем специальный маркер для GREASE-группы
							out << "[GREASE]";
						// Печатаем имя группы по её wire-коду
						else out << ::local::groupName(::local::groupWire(p->supportedGroups[j]));
					}
				} break;
				// ec_point_formats: выводим форматы точек эллиптической кривой
				case static_cast <uint8_t> (extension_type_t::EC_POINT_FORMATS): {
					// Получаем объект расширения ec_point_formats
					const auto * p = awh_cast <const extension_ec_point_t *> (ext.get());
					// Печатаем маркер "->" перед списком форматов точек
					out << "  ->  ";
					/**
					 * Перебираем форматы точек и выводим их, разделяя запятой
					 */
					for(size_t j = 0; j < p->formats.size(); ++j){
						// Если это не первый формат точки
						if(j > 0)
							// Печатаем запятую для разделения форматов точек
							out << ", ";
						// Получаем wire-код формата точки
						const uint8_t fw = ::local::ecPointWire(p->formats[j]);
						// Если формат точки представляет из себя GREASE
						if(p->formats[j] == ec_point_format_t::GREASE)
							// Печатаем специальный маркер для GREASE-формата точки
							out << "[GREASE]";
						// Если формат точки — uncompressed (0x00), выводим его имя
						else if(fw == 0x00)
							// Печатаем имя формата точки "uncompressed" для wire-кода 0x00
							out << "uncompressed";
						// Если формат точки — ansiX962_compressed_prime (0x01), выводим его имя
						else if(fw == 0x01)
							// Печатаем имя формата точки "ansiX962_compressed_prime" для wire-кода 0x01
							out << "ansiX962_compressed_prime";
						// Если формат точки — ansiX962_compressed_char2 (0x02), выводим его имя
						else if(fw == 0x02)
							// Печатаем имя формата точки "ansiX962_compressed_char2" для wire-кода 0x02
							out << "ansiX962_compressed_char2";
						// Для остальных форматов точек выводим их wire-коды в виде "0x%02X"
						else out << hex8(fw);
					}
				} break;
				// signature_algorithms: каждый алгоритм на отдельной строке
				case static_cast <uint8_t> (extension_type_t::SIGNATURE_ALGORITHMS): {
					// Получаем объект расширения signature_algorithms
					const auto * p = awh_cast <const extension_signature_t *> (ext.get());
					// Печатаем перевод строки
					out << endl;
					/**
					 * Перебираем алгоритмы и выводим их на отдельных строках с отступами
					 */
					for(const auto sig : p->algorithms){
						// Получаем wire-код алгоритма подписи
						const uint16_t sw = ::local::signatureWire(sig);
						// Печатаем отступы для алгоритмов подписи
						out << "              ";
						// Если алгоритм подписи представляет из себя GREASE
						if(sig == signature_t::GREASE)
							// Печатаем специальный маркер для GREASE-алгоритма подписи
							out << "[GREASE]";
						// Печатаем имя алгоритма подписи по его wire-коду
						else out << ::local::signatureName(sw) << "  (" << hex16(sw) << ")";
						// Печатаем перевод строки после каждого алгоритма подписи
						out << endl;
					}
					// Переходим к следующему расширению без добавления финального переноса строки
					continue;
				}
				// alpn: выводим список согласованных протоколов
				case static_cast <uint8_t> (extension_type_t::ALPN): {
					// Получаем объект расширения alpn
					const auto * p = awh_cast <const extension_alpn_t *> (ext.get());
					// Печатаем маркер "->" перед списком протоколов
					out << "  ->  ";
					/**
					 * Перебираем протоколы и выводим их, разделяя запятой
					 */
					for(size_t j = 0; j < p->protocols.size(); ++j){
						// Если это не первый протокол
						if(j > 0)
							// Печатаем запятую для разделения протоколов
							out << ", ";
						// Печатаем имя протокола в кавычках
						out << '"' << p->protocols[j] << '"';
					}
				} break;
				// next_proto_neg (NPN): выводим список протоколов
				case static_cast <uint8_t> (extension_type_t::NEXT_PROTO_NEG): {
					// Получаем объект расширения next_proto_neg
					const auto * p = awh_cast <const extension_next_proto_neg_t *> (ext.get());
					// Печатаем маркер "->" перед списком протоколов
					out << "  ->  ";
					/**
					 * Перебираем протоколы и выводим их, разделяя запятой
					 */
					for(size_t j = 0; j < p->protocols.size(); ++j){
						// Если это не первый протокол
						if(j > 0)
							// Печатаем запятую для разделения протоколов
							out << ", ";
						// Печатаем имя протокола в кавычках
						out << '"' << p->protocols[j] << '"';
					}
				} break;
				// application_settings (ALPS новый): выводим список протоколов
				case static_cast <uint8_t> (extension_type_t::APPLICATION_SETTINGS): {
					// Получаем объект расширения application_settings
					const auto * p = awh_cast <const extension_application_settings_t *> (ext.get());
					// Печатаем маркер "->" перед списком протоколов
					out << "  ->  ";
					/**
					 * Перебираем протоколы и выводим их, разделяя запятой
					 */
					for(size_t j = 0; j < p->protocols.size(); ++j){
						// Если это не первый протокол
						if(j > 0)
							// Печатаем запятую для разделения протоколов
							out << ", ";
						// Печатаем имя протокола в кавычках
						out << '"' << p->protocols[j] << '"';
					}
				} break;
				// application_settings_old (Chrome legacy ALPS): выводим список протоколов
				case static_cast <uint8_t> (extension_type_t::APPLICATION_SETTINGS_OLD): {
					// Получаем объект расширения application_settings_old
					const auto * p = awh_cast <const extension_application_settings_old_t *> (ext.get());
					// Печатаем маркер "->" перед списком протоколов
					out << "  ->  ";
					/**
					 * Перебираем протоколы и выводим их, разделяя запятой
					 */
					for(size_t j = 0; j < p->protocols.size(); ++j){
						// Если это не первый протокол
						if(j > 0)
							// Печатаем запятую для разделения протоколов
							out << ", ";
						// Печатаем имя протокола в кавычках
						out << '"' << p->protocols[j] << '"';
					}
				} break;
				// key_share: выводим пары группа→размер ключа
				case static_cast <uint8_t> (extension_type_t::KEY_SHARE): {
					// Получаем объект расширения key_share
					const auto * p = awh_cast <const extension_key_share_t *> (ext.get());
					// Печатаем маркер "->" перед списком групп и размеров ключей
					out << "  ->  ";
					// Флаг для определения первой пары группа→размер ключа (для правильного разделения запятой)
					bool first = true;
					/**
					 * Перебираем пары группа→размер ключа и выводим их, разделяя запятой
					 */
					for(const auto & [grp, data] : p->shares){
						// Если это не первая пара группа→размер ключа
						if(!first)
							// Печатаем запятую для разделения пар группа→размер ключа
							out << ", ";
						// Убираем флаг первой пары группа→размер ключа после обработки первой пары
						first = false;
						// Если группа представляет из себя GREASE
						if(grp == group_t::GREASE)
							// Печатаем специальный маркер для GREASE-группы и размер данных в скобках
							out << "[GREASE](" << data.size() << "B)";
						// Печатаем имя группы по её wire-коду и размер данных в скобках
						else out << ::local::groupName(::local::groupWire(grp)) << "(" << data.size() << "B)";
					}
				} break;
				// psk_key_exchange_modes: выводим режимы обмена ключами PSK
				case static_cast <uint8_t> (extension_type_t::PSK_KEY_EXCHANGE_MODES): {
					// Получаем объект расширения psk_key_exchange_modes
					const auto * p = awh_cast <const extension_psk_key_exchange_t *> (ext.get());
					// Печатаем маркер "->" перед списком режимов обмена ключами PSK
					out << "  ->  ";
					/**
					 * Перебираем режимы обмена ключами PSK и выводим их, разделяя запятой
					 */
					for(size_t j = 0; j < p->modes.size(); ++j){
						// Если это не первый режим обмена ключами PSK
						if(j > 0)
							// Печатаем запятую для разделения режимов обмена ключами PSK
							out << ", ";
						/**
						 * Печатаем имя режима обмена ключами PSK по его типу
						 */
						switch(p->modes[j]){
							// Если режим обмена ключами PSK — psk_ke (0x00)
							case psk_key_t::PSK_ONLY:
								// Печатаем имя режима обмена ключами PSK "psk_ke" для типа 0x00
								out << "psk_ke(0x00)";
							break;
							// Если режим обмена ключами PSK — psk_dhe_ke (0x01)
							case psk_key_t::PSK_DHE:
								// Печатаем имя режима обмена ключами PSK "psk_dhe_ke" для типа 0x01
								out << "psk_dhe_ke(0x01)";
							break;
							// Для остальных режимов обмена ключами
							default:
								// Возвращаем неизвестный режим обмена ключами
								out << "UNKNOWN";
						}
					}
				} break;
				// pre_shared_key: выводим количество идентификаторов PSK
				case static_cast <uint8_t> (extension_type_t::PRE_SHARED_KEY): {
					// Получаем объект расширения pre_shared_key
					const auto * p = awh_cast <const extension_pre_shared_key_t *> (ext.get());
					// Печатаем маркер "->" перед количеством идентификаторов PSK и правильное склонение слова "identity"
					out << "  ->  " << p->identities.size()
					    << (p->identities.size() == 1 ? " identity" : " identities");
				} break;
				// session_ticket: выводим размер данных или пометку о запросе
				case static_cast <uint8_t> (extension_type_t::SESSION_TICKET): {
					// Получаем объект расширения session_ticket
					const auto * p = awh_cast <const extension_session_ticket_t *> (ext.get());
					// Если данные для session_ticket отсутствуют
					if(p->data.empty())
						// Возвращаем пометку о том, что данные отсутствуют и это может быть запросом на новый билет
						out << "  ->  (empty / request for new ticket)";
					// Если данные для session_ticket присутствуют, выводим их размер в байтах
					else out << "  ->  " << p->data.size() << " bytes";
				} break;
				// status_request (OCSP): выводим тип статуса
				case static_cast <uint8_t> (extension_type_t::STATUS_REQUEST): {
					// Получаем объект расширения status_request
					const auto * p = awh_cast <const extension_status_request_t *> (ext.get());
					// Если тип статуса для status_request не пустой, выводим его
					if(!p->certificateStatusType.empty())
						// Возвращаем тип статуса для status_request
						out << "  ->  type=" << p->certificateStatusType;
				} break;
				// padding: выводим размер блока заполнения
				case static_cast <uint8_t> (extension_type_t::PADDING): {
					// Получаем объект расширения padding
					const auto * p = awh_cast <const extension_padding_t *> (ext.get());
					// Печатаем маркер "->" перед размером блока заполнения в байтах
					out << "  ->  " << p->size << " bytes";
				} break;
				// record_size_limit: выводим максимальный размер записи
				case static_cast <uint8_t> (extension_type_t::RECORD_SIZE_LIMIT): {
					// Получаем объект расширения record_size_limit
					const auto * p = awh_cast <const extension_record_size_limit_t *> (ext.get());
					// Печатаем маркер "->" перед максимальным размером записи в байтах
					out << "  ->  " << p->data << " bytes";
				} break;
				// early_data: выводим максимальный размер ранних данных
				case static_cast <uint8_t> (extension_type_t::EARLY_DATA): {
					// Получаем объект расширения early_data
					const auto * p = awh_cast <const extension_early_data_t *> (ext.get());
					// Если максимальный размер ранних данных больше 0
					if(p->maxSize > 0)
						// Печатаем маркер "->" перед максимальным размером ранних данных в байтах
						out << "  ->  max=" << p->maxSize << " bytes";
				} break;
				// compress_certificate: выводим поддерживаемые алгоритмы сжатия сертификатов
				case static_cast <uint8_t> (extension_type_t::COMPRESS_CERTIFICATE): {
					// Получаем объект расширения compress_certificate
					const auto * p = awh_cast <const extension_compress_certificate_t *> (ext.get());
					// Печатаем маркер "->" перед списком поддерживаемых алгоритмов сжатия сертификатов
					out << "  ->  ";
					/**
					 * Перебираем поддерживаемые алгоритмы сжатия сертификатов и выводим их, разделяя запятой
					 */
					for(size_t j = 0; j < p->algorithms.size(); ++j){
						// Если это не первый алгоритм сжатия сертификатов
						if(j > 0)
							// Печатаем запятую для разделения алгоритмов сжатия сертификатов
							out << ", ";
						// Печатаем имя алгоритма сжатия сертификатов по его wire-коду
						out << ::local::compressALGName(::local::compressorWire(p->algorithms[j]));
					}
				} break;
				// heartbeat: выводим режим (allowed/not_allowed)
				case static_cast <uint8_t> (extension_type_t::HEARTBEAT): {
					// Получаем объект расширения heartbeat
					const auto * p = awh_cast <const extension_heartbeat_t *> (ext.get());
					/**
					 * Печатаем маркер "->" перед режимом heartbeat
					 */
					switch(static_cast <uint8_t> (p->mode)){
						// Если режим heartbeat — peer_allowed_to_send
						case static_cast <uint8_t> (heartbeat_t::PEER_ALLOWED_TO_SEND):
							// Печатаем имя режима heartbeat "peer_allowed_to_send"
							out << "  ->  peer_allowed_to_send";
						break;
						// Если режим heartbeat — peer_not_allowed_to_send
						case static_cast <uint8_t> (heartbeat_t::PEER_NOT_ALLOWED_TO_SEND):
							// Печатаем имя режима heartbeat "peer_not_allowed_to_send"
							out << "  ->  peer_not_allowed_to_send";
						break;
					}
				} break;
				// renegotiation_info: выводим размер данных переговоров
				case static_cast <uint8_t> (extension_type_t::RENEGOTIATION_INFO): {
					// Получаем объект расширения renegotiation_info
					const auto * p = awh_cast <const extension_renegotiation_info_t *> (ext.get());
					// Печатаем маркер "->" перед размером данных переговоров в байтах
					out << "  ->  " << p->data.size() << " bytes";
				} break;
				// encrypted_client_hello (ECH): выводим размер зашифрованных данных
				case static_cast <uint8_t> (extension_type_t::ENCRYPTED_CLIENT_HELLO): {
					// Получаем объект расширения encrypted_client_hello
					const auto * p = awh_cast <const extension_encryption_client_hello_t *> (ext.get());
					// Печатаем маркер "->" перед размером зашифрованных данных в байтах
					out << "  ->  " << p->data.size() << " bytes";
				} break;
				// ech_outer_extensions: выводим количество вложенных расширений
				case static_cast <uint8_t> (extension_type_t::ECH_OUTER_EXTENSIONS): {
					// Получаем объект расширения ech_outer_extensions
					const auto * p = awh_cast <const extension_ech_outer_extensions_t *> (ext.get());
					// Печатаем маркер "->" перед количеством вложенных расширений
					out << "  ->  " << p->extensions.size() << " extension(s)";
				} break;
				// certificate_authorities: выводим количество доверенных CA
				case static_cast <uint8_t> (extension_type_t::CERTIFICATE_AUTHORITIES): {
					// Получаем объект расширения certificate_authorities
					const auto * p = awh_cast <const extension_certificate_authorities_t *> (ext.get());
					// Печатаем маркер "->" перед количеством доверенных CA
					out << "  ->  " << p->authorities.size() << " CA(s)";
				} break;
				// max_fragment_length: выводим максимальную длину фрагмента
				case static_cast <uint8_t> (extension_type_t::MAX_FRAGMENT_LENGTH): {
					// Получаем объект расширения max_fragment_length
					const auto * p = awh_cast <const extension_max_fragment_length_t *> (ext.get());
					// Печатаем маркер "->" перед максимальной длиной фрагмента в байтах
					out << "  ->  " << p->length;
				} break;
				// tls_flags: выводим количество флагов
				case static_cast <uint8_t> (extension_type_t::TLS_FLAGS): {
					// Получаем объект расширения tls_flags
					const auto * p = awh_cast <const extension_tls_flags_t *> (ext.get());
					// Печатаем маркер "->" перед количеством флагов
					out << "  ->  " << p->flags.size() << " flag(s)";
				} break;
				// quic_transport_parameters: выводим количество параметров QUIC
				case static_cast <uint8_t> (extension_type_t::QUIC_TRANSPORT_PARAMETERS): {
					// Получаем объект расширения quic_transport_parameters
					const auto * p = awh_cast <const extension_quic_transport_params_t *> (ext.get());
					// Печатаем маркер "->" перед количеством параметров QUIC
					out << "  ->  " << p->params.size() << " param(s)";
				} break;
				// quic_transport_parameters_legacy: выводим количество параметров QUIC (устаревший)
				case static_cast <uint8_t> (extension_type_t::QUIC_TRANSPORT_PARAMETERS_LEGACY): {
					// Получаем объект расширения quic_transport_parameters_legacy
					const auto * p = awh_cast <const extension_quic_transport_params_legacy_t *> (ext.get());
					// Печатаем маркер "->" перед количеством параметров QUIC (устаревший)
					out << "  ->  " << p->params.size() << " param(s)";
				} break;
			}
			// Печатаем перевод строки после каждого расширения
			out << endl;
		}
		// ================================================================
		// Вычисленные отпечатки
		// ================================================================
		/**
		 * Объект для хранения вычисленных отпечатков браузера (JA3, JA4, PeetPrint и т.д.)
		 */
		imprint_t imp{};
		// Вычисляем отпечатки из переданного browser_t
		const bool hasImp = this->imprint(browser, imp);
		// Печатаем перевод строки перед секцией вычисленных отпечатков
		out << endl;
		// Если вычисление отпечатков не удалось (например, из-за отсутствия данных для JA3), выводим сообщение об ошибке в секции отпечатков
		if(!hasImp)
			// Вычисление отпечатков не удалось — сообщаем об ошибке
			out << "  [ Computed Fingerprints  (ERROR - computation failed) ]" << endl;
		// Если вычисление отпечатков прошло успешно, выводим их в секции Computed Fingerprints
		else {
			// Печатаем заголовок в отладочный вывод секции Computed Fingerprints
			out << "  [ Computed Fingerprints ]" << endl;
			// Wire-коды версий для получения их текстовых названий
			const uint16_t recV = static_cast <uint16_t> (::stoul(imp.tls.record));
			const uint16_t negV = static_cast <uint16_t> (::stoul(imp.tls.negotiated));
			// TLS-версии
			out << "    TLS Record Version     : " << imp.tls.record     << "  (" << hex16(recV) << ")  =  " << ::local::tlsVersionName(recV) << endl;
			out << "    TLS Negotiated Version : " << imp.tls.negotiated << "  (" << hex16(negV) << ")  =  " << ::local::tlsVersionName(negV) << endl;
			// Идентификаторы соединения
			out << "    Client Random          : " << imp.clientRandom << endl;
			out << "    Session ID             : " << (imp.sessionId.empty() ? "(empty)" : imp.sessionId) << endl;
			// JA3
			out << endl;
			out << "    JA3        : " << imp.ja3 << endl;
			out << "    JA3 Hash   : " << imp.ja3Hash << endl;
			// JA4
			out << endl;
			out << "    JA4        : " << imp.ja4 << endl;
			out << "    JA4_r      : " << imp.ja4r << endl;
			// PeetPrint
			out << endl;
			out << "    PeetPrint  : " << imp.peetprint << endl;
			out << "    PeetHash   : " << imp.peetprintHash << endl;
			// Печатаем перевод строки после секции отпечатков
			out << endl;
			// Итоговый вывод: похож ли отпечаток на реальный браузер
			out << "    Browser?   : " << (this->looksLikeBrowser(imp) ? "YES (looks like a real browser)" : "NO  (does not look like a real browser)") << endl;
		}
		// Финальная строка-разделитель
		out << endl << LINE << endl;
		// Возвращаем сформированную строку
		return out.str();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// В случае ошибки возвращаем пустую строку
	return "";
}
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
 *
 */
string awh::tls::Fingerprint::akamai(const h2_browser_t & h2) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если SETTINGS-фрейм не был разобран — fingerprint не имеет смысла
		if(h2.settings.empty())
			// Возвращаем пустую строку
			return "";
		// Строковый поток для формирования fingerprint
		ostringstream out;
		// ─── Секция 1: SETTINGS ────────────────────────────────────────────────────────
		// Формат: "{id}:{value}" пары через ';', в порядке wire
		{
			// Флаг для определения первой пары id:value (для правильного разделения ';')
			bool first = true;
			/**
			 * Перебираем SETTINGS в порядке их объявления и формируем пары id:value, разделяя их ';'
			 */
			for(const auto & s : h2.settings){
				// Если это не первая пара id:value
				if(!first)
					// Печатаем ';' для разделения пар id:value
					out << ';';
				// Возвращаем пару id:value в формате "{id}:{value}"
				out << s.id << ':' << s.value;
				// Убираем флаг первой пары id:value после обработки первой пары
				first = false;
			}
		}
		// Разделитель секций
		out << '|';
		// ─── Секция 2: WINDOW_UPDATE ───────────────────────────────────────────────────
		// Инкремент connection-level WINDOW_UPDATE (stream 0); 0 если фрейм не отправлялся
		out << h2.windowUpdate;
		// Разделитель секций
		out << '|';
		// ─── Секция 3: PRIORITY frames ─────────────────────────────────────────────────
		// Формат: "{streamId}:{exclusive}:{dependency}:{weight}" через ','
		// weight = raw_byte + 1 (фактический HTTP/2-вес 1-256)
		{
			// Флаг для определения первой PRIORITY-записи (для правильного разделения ',')
			bool first = true;
			/**
			 * Перебираем PRIORITY-фреймы в порядке их объявления и формируем строки streamId:exclusive:dependency:weight, разделяя их ','
			 */
			for(const auto & p : h2.priorities){
				// Если это не первая PRIORITY-запись
				if(!first)
					// Печатаем ',' для разделения PRIORITY-записей
					out << ',';
				// Возвращаем строку streamId:exclusive:dependency:weight для текущего PRIORITY-фрейма
				out << p.streamId << ':'
				    << (p.exclusive ? 1 : 0) << ':'
				    << p.dependency << ':'
				    << static_cast <uint32_t> (p.weight + 1);
				// Убираем флаг первой PRIORITY-записи после обработки первой записи
				first = false;
			}
		}
		// Разделитель секций
		out << '|';
		// ─── Секция 4: pseudo-headers ──────────────────────────────────────────────────
		// Формат: m/p/s/a через ','
		{
			// Флаг для определения первого pseudo-header (для правильного разделения ',')
			bool first = true;
			/**
			 * Перебираем pseudo-headers в порядке их объявления и выводим их, разделяя ','
			 */
			for(const auto & ph : h2.pseudoHeaders){
				// Если это не первый pseudo-header
				if(!first)
					// Печатаем ',' для разделения pseudo-headers
					out << ',';
				// Возвращаем pseudo-header (m/p/s/a)
				out << ph;
				// Убираем флаг первого pseudo-header после обработки первого pseudo-header
				first = false;
			}
		}
		// Возвращаем сформированный fingerprint
		return out.str();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем пустую строку при ошибке
	return "";
}
/**
 * @brief Метод проверки, соответствует ли цифровой отпечаток шаблону, характерному для браузера
 *
 * @param imp объект цифрового отпечатка для проверки
 * @return     результат проверки, принадлежит ли цифровой отпечаток реальному браузеру
 *
 */
bool awh::tls::Fingerprint::looksLikeBrowser(const imprint_t & imp) const noexcept {
	// Если TLS 1.3 не согласован
	if(!this->_fmk->compare("772", imp.tls.negotiated))
		// (772 — это десятичное представление 0x0304, т.е. TLS 1.3)
		return false;
	// SNI присутствует (4-й символ ja4 == 'd')
	if((imp.ja4.size() < 4) || (imp.ja4[3] != 'd'))
		/**
		 * Хотя отсутствие SNI не всегда означает, что это не браузер,
		 * но всё же большинство современных браузеров его отправляют,
		 * так что для простоты будем считать его обязательным.
		 */
		return false;
	// JA3 не пустой (есть хотя бы шифры и расширения)
	return !imp.ja3Hash.empty();
}
/**
 * @brief Метод вычисления цифровых отпечатков на основе распарсенного ClientHello
 *
 * @param browser объект с распарсенными данными ClientHello
 * @param result  объект для хранения всех вычисленных отпечатков
 * @return        результат вычисления цифровых отпечатков
 *
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
					/**
					 * Проходим по версиям в расширении и ищем наибольшую не-GREASE
					 */
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
				static_cast <int32_t> (ciphersSorted.size()),
				static_cast <int32_t> (extCount),
				alpn2.c_str()
			);
			// Сохраняем префикс в строке
			const string prefix(prefixBuf);
			/**
			 * @brief Функция для конвертации списка uint16_t в строку hex формата "xxxx,xxxx,..."
			 *
			 * @param v вектор uint16_t для конвертации
			 * @return  строка hex формата "xxxx,xxxx,..."
			 *
			 */
			const auto makeHexList = [](const vector <uint16_t> & v) -> string {
				// Переменная результата
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
			// Возвращаем результат парсинга PeetPrint
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
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод парсинга данных цифрового отпечатка
 *
 * @param buffer  бинарный буфер данных цифрового отпечатка
 * @param size    размер бинарного буфера данных цифрового отпечатка
 * @param browser объект для хранения распарсенных данных цифрового отпечатка
 * @return        результат парсинга данных цифрового отпечатка
 *
 */
bool awh::tls::Fingerprint::parse(const uint8_t * buffer, const size_t size, browser_t & browser) const noexcept {
	// Переменная результата
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
				// Записываем ошибку в лог
				this->_log->debug("Fingerprint buffer too short: %zu bytes (need >= 11)", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, size);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("Fingerprint buffer too short: %zu bytes (need >= 11)", log_t::flag_t::WARNING, size);
			#endif
			// Возвращаем значение по умолчанию
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
					// Записываем ошибку в лог
					this->_log->debug("Unsupported record version: 0x%04X", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, ::local::u16(buffer + 1));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Unsupported record version: 0x%04X", log_t::flag_t::WARNING, ::local::u16(buffer + 1));
				#endif
				// Возвращаем значение по умолчанию
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
					// Записываем ошибку в лог
					this->_log->debug("Fingerprint buffer too short for %s headers: %zu bytes", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, (isDTLS ? "DTLS" : "TLS"), size);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Fingerprint buffer too short for %s headers: %zu bytes", log_t::flag_t::WARNING, (isDTLS ? "DTLS" : "TLS"), size);
				#endif
				// Возвращаем значение по умолчанию
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
				 * Если это не DTLS, то выводим предупреждение,
				 * что запись рукопожатия не соответствует ClientHello,
				 * так как в DTLS могут быть фрагменты, и мы не можем однозначно определить тип сообщения по первому байту.
				 */
				if(!isDTLS){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("Handshake entry does not match the ClientHello", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("Handshake entry does not match the ClientHello", log_t::flag_t::WARNING);
					#endif
				}
				// Возвращаем значение по умолчанию
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
					// Записываем ошибку в лог
					this->_log->debug("Unsupported handshake version: 0x%04X", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, ::local::u16(buffer + (recordSize + handshakeSize)));
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("Unsupported handshake version: 0x%04X", log_t::flag_t::WARNING, ::local::u16(buffer + (recordSize + handshakeSize)));
				#endif
				// Возвращаем значение по умолчанию
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
					// Записываем ошибку в лог
					this->_log->debug("ClientHello truncated at session_id_len", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("ClientHello truncated at session_id_len", log_t::flag_t::WARNING);
				#endif
				// Возвращаем значение по умолчанию
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
					// Записываем ошибку в лог
					this->_log->debug("ClientHello session_id_len > 32 (%zu)", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, length);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("ClientHello session_id_len > 32 (%zu)", log_t::flag_t::WARNING, length);
				#endif
				// Возвращаем значение по умолчанию
				return result;
			}
			// Если размер данных меньше смещения + длины session_id, то это означает, что данные обрезаны
			if((offset + length) > size){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("ClientHello truncated at session_id (offset=%zu, length=%zu, size=%zu)", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, offset, length, size);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("ClientHello truncated at session_id (offset=%zu, length=%zu, size=%zu)", log_t::flag_t::WARNING, offset, length, size);
				#endif
				// Возвращаем значение по умолчанию
				return result;
			}
			// Если длина session_id больше 0, то извлекаем session_id
			if(length > 0){
				// Выделяем память для идентификатора сессии
				browser.session.resize(length, 0);
				// Копируем данные идентификатора сессии из буфера
				::memcpy(&browser.session[0], buffer + offset, length);
				// Увеличиваем смещение на длину session_id
				offset += length;
			}
			// Если DTLS: cookie (только для DTLS ClientHello, RFC 6347 §4.2.1)
			if(isDTLS){
				// Если размер данных меньше смещения + 1 байт для cookie_len, то это означает, что данные обрезаны
				if(offset >= size){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Записываем ошибку в лог
						this->_log->debug("ClientHello truncated at cookie_len", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("ClientHello truncated at cookie_len", log_t::flag_t::WARNING);
					#endif
					// Возвращаем значение по умолчанию
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
						// Записываем ошибку в лог
						this->_log->debug("ClientHello truncated at cookie data (offset=%zu, length=%zu, size=%zu)", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, offset, length, size);
					/**
					 * Если режим отладки не включён
					 */
					#else
						// Записываем ошибку в лог
						this->_log->print("ClientHello truncated at cookie data (offset=%zu, length=%zu, size=%zu)", log_t::flag_t::WARNING, offset, length, size);
					#endif
					// Возвращаем значение по умолчанию
					return result;
				}
				// Если длина cookie больше 0, то извлекаем cookie
				if(length > 0){
					// Выделяем память для cookie
					browser.cookie.resize(length, 0);
					// Копируем данные cookie из буфера
					::memcpy(&browser.cookie[0], buffer + offset, length);
					// Увеличиваем смещение на длину cookie
					offset += length;
				}
			}
			// Если размер данных меньше смещения + 2 байт для cipher_suites_len, то это означает, что данные обрезаны
			if((offset + 2) > size){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("ClientHello truncated at cipher_suites_len", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("ClientHello truncated at cipher_suites_len", log_t::flag_t::WARNING);
				#endif
				// Возвращаем значение по умолчанию
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
					// Записываем ошибку в лог
					this->_log->debug("ClientHello invalid cipher_suites length (%zu)", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, length);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("ClientHello invalid cipher_suites length (%zu)", log_t::flag_t::WARNING, length);
				#endif
				// Возвращаем значение по умолчанию
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
					// Добавляем шифр в список поддерживаемых шифров браузера
					browser.ciphers.push_back(cipher_t::GREASE);
				// Если код шифра является одним из стандартных кодов из RFC 8446
				} else {
					/**
					 * Определяем код шифра
					 */
					switch(cipher){
						// Если код шифра соответствует AES128-SHA
						case 0x002F:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::AES128_SHA);
						break;
						// Если код шифра соответствует AES256-SHA
						case 0x0035:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::AES256_SHA);
						break;
						// Если код шифра соответствует AES128-GCM-SHA256
						case 0x009C:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::AES128_GCM_SHA256);
						break;
						// Если код шифра соответствует AES256-GCM-SHA384
						case 0x009D:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::AES256_GCM_SHA384);
						break;
						// Если код шифра соответствует PSK-AES128-CBC-SHA
						case 0x008C:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::PSK_AES128_CBC_SHA);
						break;
						// Если код шифра соответствует PSK-AES256-CBC-SHA
						case 0x008D:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::PSK_AES256_CBC_SHA);
						break;
						// Если код шифра соответствует ECDHE-RSA-AES128-SHA
						case 0xC013:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_AES128_SHA);
						break;
						// Если код шифра соответствует ECDHE-RSA-AES256-SHA
						case 0xC014:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_AES256_SHA);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-AES128-SHA
						case 0xC009:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_AES128_SHA);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-AES256-SHA
						case 0xC00A:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_AES256_SHA);
						break;
						// Если код шифра соответствует ECDHE-RSA-AES128-SHA256
						case 0xC027:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_AES128_SHA256);
						break;
						// Если код шифра соответствует ECDHE-PSK-AES128-CBC-SHA
						case 0xC035:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_PSK_AES128_CBC_SHA);
						break;
						// Если код шифра соответствует ECDHE-PSK-AES256-CBC-SHA
						case 0xC036:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_PSK_AES256_CBC_SHA);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-AES128-SHA256
						case 0xC023:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_AES128_SHA256);
						break;
						// Если код шифра соответствует ECDHE-RSA-AES128-GCM-SHA256
						case 0xC02F:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_AES128_GCM_SHA256);
						break;
						// Если код шифра соответствует ECDHE-RSA-AES256-GCM-SHA384
						case 0xC030:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_AES256_GCM_SHA384);
						break;
						// Если код шифра соответствует ECDHE-RSA-CHACHA20-POLY1305
						case 0xCCA8:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_RSA_CHACHA20_POLY1305);
						break;
						// Если код шифра соответствует ECDHE-PSK-CHACHA20-POLY1305
						case 0xCCAC:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_PSK_CHACHA20_POLY1305);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-AES128-GCM-SHA256
						case 0xC02B:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_AES128_GCM_SHA256);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-AES256-GCM-SHA384
						case 0xC02C:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_AES256_GCM_SHA384);
						break;
						// Если код шифра соответствует ECDHE-ECDSA-CHACHA20-POLY1305
						case 0xCCA9:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(cipher_t::ECDHE_ECDSA_CHACHA20_POLY1305);
						break;
						// Если код шифра соответствует TLS_AES_128_GCM_SHA256
						case 0x1301:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(tls::cipher_t::TLS_AES_128_GCM_SHA256);
						break;
						// Если код шифра соответствует TLS_AES_256_GCM_SHA384
						case 0x1302:
							// Добавляем шифр в список поддерживаемых шифров браузера
							browser.ciphers.push_back(tls::cipher_t::TLS_AES_256_GCM_SHA384);
						break;
						// Если код шифра соответствует TLS_CHACHA20_POLY1305_SHA256
						case 0x1303:
							// Добавляем шифр в список поддерживаемых шифров браузера
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
					// Записываем ошибку в лог
					this->_log->debug("ClientHello truncated at compression_methods_len", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("ClientHello truncated at compression_methods_len", log_t::flag_t::WARNING);
				#endif
				// Возвращаем значение по умолчанию
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
					// Записываем ошибку в лог
					this->_log->debug("ClientHello truncated at compression_methods", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("ClientHello truncated at compression_methods", log_t::flag_t::WARNING);
				#endif
				// Возвращаем значение по умолчанию
				return result;
			}
			// Если длина compression_methods не равна 1 или если первый байт compression_methods не равен 0x00 (null), то это не соответствует стандарту TLS
			if((length != 1) || (buffer[offset] != 0x00)){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("ClientHello non-standard compression_methods (length=%zu, value=0x%02X)", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING, length, buffer[offset]);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
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
			// Увеличиваем смещение на длину compression_methods
			offset += length;
			// Если размер данных не хватает для извлечения списка расширений
			if((offset + 2) > size){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Записываем ошибку в лог
					this->_log->debug("ClientHello truncated at extensions_length", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("ClientHello truncated at extensions_length", log_t::flag_t::WARNING);
				#endif
				// Возвращаем значение по умолчанию
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
					// Записываем ошибку в лог
					this->_log->debug("ClientHello truncated inside extensions", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::WARNING);
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Записываем ошибку в лог
					this->_log->print("ClientHello truncated inside extensions", log_t::flag_t::WARNING);
				#endif
				// Возвращаем значение по умолчанию
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
				// Если тип расширения является GREASE
				if(::local::isGrease(type))
					// Выполняем парсинг расширения GREASE, который просто пропускает данные и закрывает объект без обработки
					::fingerprint::parseGrease(buffer + offset, size, browser);
				/**
				 * Если тип расширения не является GREASE, то мы проверяем,
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
						// Если тип расширения соответствует client_certificate_type (RFC 7250)
						case 0x0013:
							// Выполняем парсинг расширения client_certificate_type (RFC 7250)
							::fingerprint::parseClientCertificateType(buffer + offset, size, browser);
						break;
						// Если тип расширения соответствует server_certificate_type (RFC 7250)
						case 0x0014:
							// Выполняем парсинг расширения server_certificate_type (RFC 7250)
							::fingerprint::parseServerCertificateType(buffer + offset, size, browser);
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
						// Если тип расширения соответствует oid_filters (RFC 8446 §4.2.5)
						case 0x0030:
							// Выполняем парсинг расширения oid_filters (RFC 8446 §4.2.5)
							::fingerprint::parseOIDFilters(buffer + offset, size, browser);
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
						// Если тип расширения соответствует transparency_info (RFC 6962, пустое)
						case 0x0035:
							// Выполняем парсинг расширения transparency_info (RFC 6962, пустое)
							::fingerprint::parseTransparencyInfo(buffer + offset, size, browser);
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
			// Возвращаем результат
			return !browser.extensions.empty();
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
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
 *
 */
bool awh::tls::Fingerprint::parseH2(const uint8_t * buffer, const size_t size, h2_browser_t & h2) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		/**
		 * HTTP/2 client connection preface (RFC 7540 §3.5):
		 * 24 октета: "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
		 */
		static const uint8_t H2_MAGIC[24] = {
			'P','R','I',' ','*',' ','H','T','T','P','/','2','.','0',
			'\r','\n','\r','\n','S','M','\r','\n','\r','\n'
		};
		// Сбрасываем результат
		h2 = h2_browser_t{};
		// Минимальный размер: magic (24) + один заголовок фрейма (9)
		if(size < 33)
			// Буфер слишком короткий
			return false;
		// Проверяем HTTP/2 magic
		if(::memcmp(buffer, H2_MAGIC, 24) != 0)
			// Неверный connection preface
			return false;
		// Начальное смещение — после magic
		size_t offset = 24;
		// Флаги: нашли ли SETTINGS и HEADERS
		bool gotSettings = false;
		bool gotHeaders  = false;
		/**
		 * Разбираем фреймы HTTP/2 по порядку.
		 * Заголовок фрейма (RFC 7540 §4.1): 9 байт:
		 *   Length  [2:0]  — 3 байта, длина payload
		 *   Type    [3]    — 1 байт, тип фрейма
		 *   Flags   [4]    — 1 байт, флаги
		 *   StreamID[8:5]  — 4 байта (MSB зарезервирован), 31-битный stream ID
		 */
		while(offset + 9 <= size){
			// Читаем заголовок фрейма
			const uint32_t payLen = (
				(static_cast <uint32_t> (buffer[offset    ])   << 16)
				| (static_cast <uint32_t> (buffer[offset + 1]) <<  8)
				|  static_cast <uint32_t> (buffer[offset + 2])
			);
			// Тип фрейма
			const uint8_t fType = buffer[offset + 3];
			// Флаги фрейма
			const uint8_t fFlags = buffer[offset + 4];
			// Stream ID (31 бит, MSB зарезервирован)
			const uint32_t fSid = (
				(static_cast <uint32_t> (buffer[offset + 5] & 0x7F) << 24)
				| (static_cast <uint32_t> (buffer[offset + 6])      << 16)
				| (static_cast <uint32_t> (buffer[offset + 7])      <<  8)
				|  static_cast <uint32_t> (buffer[offset + 8])
			);
			// Перемещаемся за заголовок
			offset += 9;
			// Если данных для payload не хватает — прерываем
			if(offset + payLen > size)
				// Данных не хватает
				break;
			// Указатель на payload текущего фрейма
			const uint8_t * pay = buffer + offset;
			/**
			 * Разбираем тело фрейма согласно его типу
			 */
			switch(fType){
				// ─── SETTINGS (0x4) ─────────────────────────────────────────────────────────
				// RFC 7540 §6.5: payload — набор пар {id:uint16, value:uint32}, по 6 байт каждая
				case 0x4: {
					// Обрабатываем только не-ACK SETTINGS на stream 0
					if((fSid == 0) && !(fFlags & 0x1)){
						// Индекс внутри payload
						size_t index = 0;
						// Читаем все пары id:value
						while(index + 6 <= payLen){
							// Идентификатор параметра (2 байта big-endian)
							const uint16_t id = ((static_cast <uint16_t> (pay[index]) << 8) | pay[index + 1]);
							// Значение параметра (4 байта big-endian)
							const uint32_t val = (
								(static_cast <uint32_t> (pay[index + 2])   << 24)
								| (static_cast <uint32_t> (pay[index + 3]) << 16)
								| (static_cast <uint32_t> (pay[index + 4]) <<  8)
								|  static_cast <uint32_t> (pay[index + 5])
							);
							// Сохраняем пару в список (порядок важен)
							h2.settings.emplace_back(id, val);
							// Перемещаемся к следующей паре
							index += 6;
						}
						// Фиксируем что SETTINGS получен
						gotSettings = true;
					}
				} break;
				// ─── WINDOW_UPDATE (0x8) ────────────────────────────────────────────────────
				// RFC 7540 §6.9: payload — 4 байта (MSB резерв, 31-бит increment)
				case 0x8: {
					// Нас интересует только connection-level (stream 0)
					if((fSid == 0) && (payLen >= 4)){
						// Сохраняем значение window update (31-битный increment) для stream 0
						h2.windowUpdate = (
							(static_cast <uint32_t> (pay[0] & 0x7F) << 24)
							| (static_cast <uint32_t> (pay[1])      << 16)
							| (static_cast <uint32_t> (pay[2])      <<  8)
							|  static_cast <uint32_t> (pay[3])
						);
					}
				} break;
				// ─── PRIORITY (0x2) ─────────────────────────────────────────────────────────
				// RFC 7540 §6.3: payload — 4 байта stream dep + 1 байт weight
				case 0x2: {
					// Payload должен быть не менее 5 байт
					if(payLen >= 5){
						// Объект для хранения информации о приоритизации потока
						h2_priority_t prio{};
						// Stream ID приоритизируемого потока — из заголовка фрейма
						prio.streamId = fSid;
						// E-бит (MSB первого байта payload) — эксклюзивная зависимость
						prio.exclusive = ((pay[0] & 0x80) != 0);
						// 31-битный stream dependency ID
						prio.dependency = (
							(static_cast <uint32_t> (pay[0] & 0x7F) << 24)
							| (static_cast <uint32_t> (pay[1])      << 16)
							| (static_cast <uint32_t> (pay[2])      <<  8)
							|  static_cast <uint32_t> (pay[3])
						);
						// Raw weight (0-255); фактический вес = weight + 1
						prio.weight = pay[4];
						// Сохраняем в порядке получения
						h2.priorities.push_back(prio);
					}
				} break;
				// ─── HEADERS (0x1) ──────────────────────────────────────────────────────────
				// RFC 7540 §6.2: содержит HPACK-блок с заголовками
				case 0x1: {
					// Разбираем только первый HEADERS-фрейм (первый запрос клиента)
					if(!gotHeaders){
						// Фиксируем что HEADERS получен
						gotHeaders = true;
						// Позиция внутри payload
						size_t hpos = 0;
						// Размер padding-а (если есть)
						uint8_t padLen = 0;
						// Если установлен флаг PADDED (0x8) — первый байт payload = длина padding
						if(fFlags & 0x8)
							// Читаем длину padding
							padLen = pay[hpos++];
						// Если установлен флаг PRIORITY (0x20) — следующие 5 байт = stream dep + weight
						if((fFlags & 0x20) && (hpos + 5 <= payLen))
							// Пропускаем inline PRIORITY из HEADERS (не учитываем в fingerprint)
							hpos += 5;
						// HPACK-блок заканчивается до padding: [hpos, payLen - padLen)
						const size_t hpackEnd = (
							(payLen > static_cast <uint32_t> (padLen))
							? static_cast <size_t> (payLen - padLen)
							: static_cast <size_t> (payLen)
						);
						/**
						 * Декодируем HPACK-блок (RFC 7541) для извлечения порядка псевдо-заголовков.
						 * Прекращаем, как только встречаем не-псевдо-заголовок:
						 * по спецификации HTTP/2 псевдо-заголовки идут строго перед обычными.
						 *
						 * Поддерживаемые типы представлений:
						 *   1xxxxxxx — Indexed Header Field (§6.1)
						 *   01xxxxxx — Literal + Incremental Indexing (§6.2.1)
						 *   0000xxxx — Literal without Indexing (§6.2.2)
						 *   0001xxxx — Literal Never Indexed (§6.2.3)
						 *   001xxxxx — Dynamic Table Size Update (§6.3) — пропускаем
						 */
						while(hpos < hpackEnd){
							// Первый байт текущего заголовка
							const uint8_t b0 = pay[hpos];
							// Определяем сокращение псевдо-заголовка
							char pseudoCh = '\0';
							// Если это Indexed Header Field (1xxxxxxx)
							if(b0 & 0x80){
								// ── Indexed Header Field: 1xxxxxxx (RFC 7541 §6.1) ────────
								const uint32_t idx = ::http2::hpackDecodeInt(pay, hpackEnd, hpos, 7);
								// Проверяем индекс в статической таблице
								pseudoCh = ::http2::hpackPseudoChar(idx);
								// Если не псевдо-заголовок — завершаем разбор
								if(pseudoCh == '\0')
									// Достигли первого обычного заголовка
									goto endHpack;
							// Если это Literal + Incremental Indexing (01xxxxxx)
							} else if((b0 & 0xC0) == 0x40) {
								// ── Literal + Incremental Indexing: 01xxxxxx (RFC 7541 §6.2.1) ──
								const uint32_t nameIdx = ::http2::hpackDecodeInt(pay, hpackEnd, hpos, 6);
								// Если имя представлено индексом в статической таблице — получаем псевдо-символ, иначе декодируем имя как строку
								if(nameIdx > 0){
									// Имя из статической таблицы
									pseudoCh = ::http2::hpackPseudoChar(nameIdx);
								// Если имя не представлено индексом — декодируем как строку и пытаемся сопоставить псевдо-символу
								} else {
									// Имя — литеральная строка
									const string name = ::http2::hpackDecodeStr(pay, hpackEnd, hpos);
									// Пытаемся сопоставить псевдо-символу по имени
									pseudoCh = ::http2::hpackPseudoFromName(name);
								}
								// Пропускаем значение
								::http2::hpackDecodeStr(pay, hpackEnd, hpos);
								// Если не псевдо-заголовок — завершаем разбор
								if(pseudoCh == '\0')
									// Достигли первого обычного заголовка
									goto endHpack;
							// Если это Literal without Indexing (0000xxxx)
							} else if((b0 & 0xF0) == 0x00) {
								// ── Literal without Indexing: 0000xxxx (RFC 7541 §6.2.2) ────
								const uint32_t nameIdx = ::http2::hpackDecodeInt(pay, hpackEnd, hpos, 4);
								// Если имя представлено индексом в статической таблице — получаем псевдо-символ, иначе декодируем имя как строку
								if(nameIdx > 0)
									// Устанавливаем псевдо-символ по индексу из статической таблицы
									pseudoCh = ::http2::hpackPseudoChar(nameIdx);
								// Если имя не представлено индексом — декодируем как строку и пытаемся сопоставить псевдо-символу
								else {
									// Получаем имя как литеральную строку
									const string name = ::http2::hpackDecodeStr(pay, hpackEnd, hpos);
									// Пытаемся сопоставить псевдо-символу по имени
									pseudoCh = ::http2::hpackPseudoFromName(name);
								}
								// Пропускаем значение
								::http2::hpackDecodeStr(pay, hpackEnd, hpos);
								// Если не псевдо-заголовок — завершаем разбор
								if(pseudoCh == '\0')
									// Достигли первого обычного заголовка
									goto endHpack;
							// Если это Literal Never Indexed (0001xxxx)
							} else if((b0 & 0xF0) == 0x10) {
								// ── Literal Never Indexed: 0001xxxx (RFC 7541 §6.2.3) ───────
								const uint32_t nameIdx = ::http2::hpackDecodeInt(pay, hpackEnd, hpos, 4);
								// Если имя представлено индексом в статической таблице — получаем псевдо-символ, иначе декодируем имя как строку
								if(nameIdx > 0)
									// Устанавливаем псевдо-символ по индексу из статической таблицы
									pseudoCh = ::http2::hpackPseudoChar(nameIdx);
								// Если имя не представлено индексом — декодируем как строку и пытаемся сопоставить псевдо-символу
								else {
									// Получаем имя как литеральную строку
									const string name = ::http2::hpackDecodeStr(pay, hpackEnd, hpos);
									// Пытаемся сопоставить псевдо-символу по имени
									pseudoCh = ::http2::hpackPseudoFromName(name);
								}
								// Пропускаем значение
								::http2::hpackDecodeStr(pay, hpackEnd, hpos);
								// Если не псевдо-заголовок — завершаем разбор
								if(pseudoCh == '\0')
									// Достигли первого обычного заголовка
									goto endHpack;
							// Если это Dynamic Table Size Update (001xxxxx)
							} else if((b0 & 0xE0) == 0x20) {
								// ── Dynamic Table Size Update: 001xxxxx (RFC 7541 §6.3) ─────
								::http2::hpackDecodeInt(pay, hpackEnd, hpos, 5);
								// Не является заголовком, продолжаем
								continue;
							// Если это неизвестный тип представления — прерываем
							} else break;
							// Если нашли псевдо-заголовок — добавляем, избегая дублей
							if(pseudoCh != '\0'){
								// Получаем строку из псевдо-символа
								const string ph(1, pseudoCh);
								// Флаг найденного псевдо-заголовка
								bool found = false;
								/**
								 * Проверяем, не добавляли ли уже этот псевдо-заголовок (может встречаться несколько раз, но в fingerprint учитываем только порядок первых вхождений)
								 */
								for(const auto & s : h2.pseudoHeaders){
									// Если строка начинается с того же псевдо-символа — значит этот псевдо-заголовок уже добавлен
									if((found = (!s.empty() && (s[0] == pseudoCh))))
										// Выходим из цикла проверки
										break;
								}
								// Если этот псевдо-заголовок ещё не добавлен — сохраняем его
								if(!found)
									// Добавляем псевдо-заголовок в список (порядок важен)
									h2.pseudoHeaders.push_back(ph);
							}
						}
						// Завершаем разбор HPACK-блока
						endHpack:;
					}
				} break;
				// Все остальные типы фреймов пропускаем
				default: break;
			}
			// Перемещаемся к следующему фрейму
			offset += payLen;
			// Если первый HEADERS уже обработан — дальше разбирать не нужно
			if(gotHeaders)
				// Завершаем разбор
				break;
		}
		// Успешно если удалось разобрать SETTINGS
		return gotSettings;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем отрицательный результат
	return false;
}
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
 *
 */
vector <uint8_t> awh::tls::Fingerprint::apply(const uint8_t * buffer, const size_t size, const browser_t & browser) const noexcept {
	// Переменная результата
	vector <uint8_t> result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Проверяем входные параметры
		if((buffer == nullptr) || (size == 0) || browser.ciphers.empty())
			// Возвращаем пустой результат
			return result;
		// Получаем полный размер TLS/DTLS record layer
		const size_t recordLen = ::local::recordLayerSize(buffer, size);
		// Если record layer неполный — возвращаем пустой результат
		if((recordLen == 0) || (size < recordLen)){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем предупреждение в лог
				this->_log->debug("ClientHello record is incomplete", __PRETTY_FUNCTION__, make_tuple(size, recordLen), log_t::flag_t::WARNING);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем предупреждение в лог
				this->_log->print("ClientHello record is incomplete", log_t::flag_t::WARNING);
			#endif
			// Возвращаем пустой результат
			return result;
		}
		// Парсим исходный буфер для получения структурированных данных
		browser_t src{};
		// Если парсинг не удался — возвращаем пустой результат
		if(!this->parse(buffer, size, src))
			// Возвращаем пустой результат
			return result;
		// Определяем протокол (TLS или DTLS)
		const bool isDTLS = (
			(src.record.version == version_t::DTLS_1_0) ||
			(src.record.version == version_t::DTLS_1_2)
		);
		// Размер заголовка record (TLS: 5, DTLS: 13)
		const size_t recordSize    = (isDTLS ? 13u : 5u);
		// Размер заголовка handshake (TLS: 4, DTLS: 12)
		const size_t handshakeSize = (isDTLS ? 12u : 4u);
		/**
		 * ─── Пересканируем исходный буфер ───────────────────────────────────────────
		 * Цель: определить позиции полей и собрать данные, не представленные в browser_t:
		 *   - GREASE wire-коды шифров из cipher_suites
		 *   - сырые байты каждого расширения по его wire-типу
		 *   - GREASE wire-коды и payload типов расширений
		 */
		// Начальное смещение: пропускаем record header + handshake header + client_version(2) + random(32)
		size_t scanOff = (recordSize + handshakeSize + 2u + 32u);
		// Если буфер слишком короткий для минимальной структуры — возвращаем пустой результат
		if(scanOff >= size)
			// Возвращаем пустой результат
			return result;
		// Пропускаем session_id (1 байт длины + данные)
		const size_t sessLen = static_cast <size_t> (buffer[scanOff++]);
		// Если буфер обрезан на session_id — возвращаем пустой результат
		if((scanOff + sessLen) > size)
			// Возвращаем пустой результат
			return result;
		// Пропускаем данные session_id
		scanOff += sessLen;
		// Для DTLS пропускаем cookie (1 байт длины + данные)
		if(isDTLS){
			// Если буфер обрезан на cookie_len — возвращаем пустой результат
			if(scanOff >= size)
				// Возвращаем пустой результат
				return result;
			// Пропускаем cookie
			const size_t cookieLen = static_cast <size_t> (buffer[scanOff++]);
			// Если буфер обрезан на cookie — возвращаем пустой результат
			if((scanOff + cookieLen) > size)
				// Возвращаем пустой результат
				return result;
			// Пропускаем данные cookie
			scanOff += cookieLen;
		}
		// Запоминаем позицию начала поля cipher_suites_len в исходном буфере
		const size_t csStart = scanOff;
		// Если буфер слишком короткий для cipher_suites_len — возвращаем пустой результат
		if((csStart + 2u) > size)
			// Возвращаем пустой результат
			return result;
		// Получаем байтовую длину списка cipher_suites
		const size_t csLen = static_cast <size_t> (::local::u16(buffer + csStart));
		// Проверяем, что cipher_suites помещается в буфер
		if((csStart + 2u + csLen) > size)
			// Возвращаем пустой результат
			return result;
		// Собираем GREASE wire-коды шифров из cipher_suites (в порядке появления)
		vector <uint16_t> greaseCipherVals;
		/**
		 * Перебираем все cipher_suites из исходного буфера для сбора GREASE-значений
		 */
		for(size_t i = 0u; (i + 1u) < csLen; i += 2u){
			// Читаем 2-байтовый wire-код шифра
			const uint16_t w = ::local::u16(buffer + csStart + 2u + i);
			// Если это GREASE-значение — сохраняем его для последующего использования
			if(::local::isGrease(w))
				// Добавляем GREASE wire-код шифра в список
				greaseCipherVals.push_back(w);
		}
		// Перемещаем смещение за конец cipher_suites
		scanOff = (csStart + 2u + csLen);
		// Запоминаем позицию начала поля compression_methods_len в исходном буфере
		const size_t cmStart = scanOff;
		// Если буфер слишком короткий для compression_methods_len — возвращаем пустой результат
		if(cmStart >= size)
			// Возвращаем пустой результат
			return result;
		// Получаем байтовую длину списка compression_methods
		const size_t cmLen = static_cast <size_t> (buffer[scanOff++]);
		// Проверяем, что compression_methods помещаются в буфер
		if((scanOff + cmLen) > size)
			// Возвращаем пустой результат
			return result;
		// Перемещаем смещение за конец compression_methods (exclusive)
		scanOff += cmLen;
		// Запоминаем позицию конца compression_methods
		const size_t cmEnd = scanOff;
		// Карта: wire-тип расширения → сырые байты payload расширения (из исходного буфера)
		unordered_map <uint16_t, vector <uint8_t>> origExtRaw;
		// Wire-коды GREASE-расширений в порядке появления
		vector <uint16_t> greaseExtVals;
		// Payload GREASE-расширений по их wire-типу
		unordered_map <uint16_t, vector <uint8_t>> greaseExtRaw;
		// Читаем и разбираем расширения исходного буфера (если они есть)
		if((scanOff + 2u) <= size){
			// Байтовая длина всего списка расширений
			const size_t extTotalLen = static_cast <size_t> (::local::u16(buffer + scanOff));
			// Начало данных расширений
			size_t extOff = (scanOff + 2u);
			// Конец данных расширений
			const size_t extEnd = (extOff + extTotalLen);
			/**
			 * Перебираем все расширения исходного буфера
			 */
			while(((extOff + 4u) <= extEnd) && (extEnd <= size)){
				// Читаем wire-тип расширения
				const uint16_t extType = ::local::u16(buffer + extOff);
				// Читаем длину payload расширения
				const uint16_t extLen  = ::local::u16(buffer + extOff + 2u);
				// Перемещаемся за заголовок расширения
				extOff += 4u;
				// Если данные расширения выходят за пределы extensions — прерываем
				if((extOff + static_cast <size_t> (extLen)) > extEnd)
					// Прерываем цикл
					break;
				// Если это GREASE-расширение — сохраняем его wire-тип и payload отдельно
				if(::local::isGrease(extType)){
					// Добавляем GREASE wire-тип в список (порядок важен)
					greaseExtVals.push_back(extType);
					// Сохраняем payload GREASE-расширения по его wire-типу
					greaseExtRaw[extType] = vector <uint8_t>(buffer + extOff, buffer + extOff + extLen);
				// Иначе сохраняем payload обычного расширения по его wire-типу
				} else
					// Сохраняем сырые байты payload расширения
					origExtRaw[extType] = vector <uint8_t>(buffer + extOff, buffer + extOff + extLen);
				// Перемещаемся за payload расширения
				extOff += static_cast <size_t> (extLen);
			}
		}
		// ─── Строим отфильтрованный список cipher_suites ────────────────────────────
		/**
		 * @brief Вспомогательная функция: проверяет, присутствует ли шифр cipher в списке ciphers
		 *
		 * @details Правила:
		 *           - Порядок определяется шаблоном browser.ciphers
		 *           - Включаем шифр, если он присутствует в src.ciphers (пересечение)
		 *           - Для GREASE: используем wire-коды из исходного буфера в том же порядке
		 *
		 * @param ciphers список шифров для проверки
		 * @param cipher  шифр для поиска
		 * @return        результат проверки существования шифра в списке
		 *
		 */
		auto hasCipher = [](const vector <cipher_t> & ciphers, const cipher_t cipher) -> bool {
			// Ищем шифр в списке
			return (::find(ciphers.begin(), ciphers.end(), cipher) != ciphers.end());
		};
		// Индекс GREASE wire-кода шифра из исходного буфера
		size_t greaseCipherIndex = 0u;
		// Payload (байты) списка шифров для результирующего буфера
		vector <uint8_t> cipherPayload;
		/**
		 * Перебираем шифры шаблона в порядке шаблона
		 */
		for(const auto & cipher : browser.ciphers){
			// Если шифр отсутствует в исходном ClientHello — пропускаем
			if(!hasCipher(src.ciphers, cipher))
				// Пропускаем шифр
				continue;
			// Если это GREASE-шифр — используем wire-код из исходного буфера
			if(cipher == cipher_t::GREASE){
				// Если GREASE-коды кончились — используем значение по умолчанию
				const uint16_t gv = (
					(greaseCipherIndex < greaseCipherVals.size())
					? greaseCipherVals[greaseCipherIndex++]
					: 0x0A0Au
				);
				// Добавляем GREASE wire-код шифра в payload
				cipherPayload.push_back(static_cast <uint8_t> (gv >> 8u));
				// Добавляем второй байт GREASE wire-кода шифра в payload
				cipherPayload.push_back(static_cast <uint8_t> (gv & 0xFFu));
			// Иначе конвертируем enum в wire-код и добавляем в payload
			} else {
				// Получаем wire-код шифра
				const uint16_t wire = ::local::cipherWire(cipher);
				// Если wire-код ненулевой (известный шифр) — добавляем
				if(wire != 0u){
					// Добавляем старший байт wire-кода шифра в payload
					cipherPayload.push_back(static_cast <uint8_t> (wire >> 8u));
					// Добавляем младший байт wire-кода шифра в payload
					cipherPayload.push_back(static_cast <uint8_t> (wire & 0xFFu));
				}
			}
		}
		// Если после фильтрации список шифров пуст — возвращаем пустой результат
		if(cipherPayload.empty())
			// Возвращаем пустой результат
			return result;
		// ─── Строим отфильтрованный список расширений ───────────────────────────────
		/**
		 * @brief Вспомогательная функция: ищет расширение с типом type в src.extensions
		 *
		 * @details Правила:
		 *           - Порядок определяется шаблоном browser.extensions.
		 *           - Включаем расширение, если оно присутствует в src.extensions (пересечение).
		 *           - GREASE-расширения: используем wire-коды из исходного буфера в том же порядке.
		 *           - Для "filter-list" расширений (supported_groups, sig_algs и пр.):
		 *             пересекаем список шаблона со списком из src, порядок — из шаблона.
		 *           - Для "blob" расширений (SNI, session_ticket и пр.):
		 *             используем сырые байты из исходного буфера без изменений.
		 *
		 * @param type тип расширения для поиска
		 * @return     объект найденного расширения или nullptr, если не найдено
		 *
		 */
		auto findSrcExt = [&src](const extension_type_t type) -> const extension_t * {
			/**
			 * Перебираем расширения исходного ClientHello
			 */
			for(const auto & extension : src.extensions){
				// Если нашли нужный тип — возвращаем указатель
				if(extension->type == type)
					// Возвращаем указатель на расширение
					return extension.get();
			}
			// Расширение не найдено
			return nullptr;
		};
		/**
		 * @brief Вспомогательная функция: добавляет uint16_t big-endian в буфер
		 *
		 * @param buffer буфер для добавления байтов
		 * @param num    число для добавления (будет преобразовано в 2 байта big-endian)
		 *
		 */
		auto u16be = [](vector <uint8_t> & buffer, const uint16_t num) -> void {
			// Добавляем старший байт
			buffer.push_back(static_cast <uint8_t> (num >> 8u));
			// Добавляем младший байт
			buffer.push_back(static_cast <uint8_t> (num & 0xFFu));
		};
		/**
		 * @brief Вспомогательная функция: формирует и добавляет расширение (wire-тип + длина + payload) в buffer
		 *
		 * @param buffer буфер для добавления расширения
		 * @param type   wire-тип расширения для добавления
		 * @param data   байты payload расширения для добавления
		 *
		 */
		auto appendExt = [&u16be](vector <uint8_t> & buffer, const uint16_t type, const vector <uint8_t> & data) -> void {
			// Добавляем wire-тип расширения (2 байта big-endian)
			u16be(buffer, type);
			// Добавляем длину payload расширения (2 байта big-endian)
			u16be(buffer, static_cast <uint16_t> (data.size()));
			// Добавляем байты payload расширения
			buffer.insert(buffer.end(), data.begin(), data.end());
		};
		// Индекс GREASE wire-кода расширения из исходного буфера
		size_t greaseExtIndex = 0u;
		// Payload всех расширений для результирующего буфера
		vector <uint8_t> extPayload;
		/**
		 * Перебираем расширения шаблона в порядке шаблона
		 */
		for(const auto & tmplExt : browser.extensions){
			// Тип расширения из шаблона
			const extension_type_t etype = tmplExt->type;
			// Wire-тип расширения (будет вычислен ниже)
			uint16_t wireType = 0u;
			/**
			 * Для GREASE-расширения: используем wire-код из исходного буфера
			 * Для обычных расширений: проверяем наличие в src, получаем wire-код
			 */
			if(etype == extension_type_t::GREASE){
				// Если GREASE-расширения из исходного буфера кончились — пропускаем
				if(greaseExtIndex >= greaseExtVals.size())
					// Пропускаем расширение
					continue;
				// Используем следующий GREASE wire-тип из исходного буфера
				wireType = greaseExtVals[greaseExtIndex++];
			// Если это обычное расширение — проверяем его наличие в src и получаем wire-тип
			} else {
				// Проверяем, присутствует ли это расширение в исходном ClientHello
				if(findSrcExt(etype) == nullptr)
					// Расширение отсутствует в исходном ClientHello — пропускаем
					continue;
				// Получаем wire-тип расширения
				wireType = ::local::extensionWire(etype);
				// Если wire-тип неизвестен (extensionWire возвращает 0xFFFF для неподдерживаемых типов) — пропускаем
				if(wireType == 0xFFFFu)
					// Пропускаем расширение с неизвестным wire-типом
					continue;
			}
			// Флаг: является ли это "filter-list" расширением (пропустить если payload пуст после фильтрации)
			bool filterList = false;
			// Payload текущего расширения
			vector <uint8_t> payload;
			/**
			 * Строим payload в зависимости от типа расширения
			 */
			switch(static_cast <uint8_t> (etype)){
				/**
				 * GREASE-расширение: payload из исходного буфера
				 */
				case static_cast <uint8_t> (extension_type_t::GREASE): {
					// Ищем payload GREASE-расширения по его wire-типу в карте сырых байт расширений из исходного буфера
					auto i = greaseExtRaw.find(wireType);
					// Если payload GREASE-расширения есть в исходном буфере — используем его
					if(i != greaseExtRaw.end())
						// Берём payload из исходного буфера
						payload = i->second;
				} break;
				/**
				 * ─── Filter-list расширения ──────────────────────────────────────────
				 * Пересекаем список шаблона со списком из src; порядок из шаблона.
				 * Если пересечение пусто — расширение будет пропущено (filterList = true).
				 */
				case static_cast <uint8_t> (extension_type_t::SUPPORTED_GROUPS): {
					// Отмечаем как filter-list расширение
					filterList = true;
					// Буфер для байт списка групп
					vector <uint8_t> groupBytes;
					// Получаем указатели на списки групп: шаблон и исходный ClientHello
					const auto * tmpl = static_cast <const extension_supported_groups_t *> (tmplExt.get());
					const auto * orig = static_cast <const extension_supported_groups_t *> (findSrcExt(etype));
					/**
					 * Перебираем группы шаблона, включаем только те, что есть в исходном ClientHello
					 */
					for(const auto & gid : tmpl->supportedGroups){
						// Проверяем наличие группы в исходном ClientHello
						if(::find(orig->supportedGroups.begin(), orig->supportedGroups.end(), gid) == orig->supportedGroups.end())
							// Группы нет в исходном ClientHello — пропускаем
							continue;
						// Вычисляем wire-код группы (GREASE → 0x0A0A)
						const uint16_t gw = ((gid == group_t::GREASE) ? 0x0A0Au : ::local::groupWire(gid));
						// Пропускаем неизвестные группы
						if(gw == 0u)
							// Неизвестная группа — пропускаем
							continue;
						// Добавляем wire-код группы в буфер
						u16be(groupBytes, gw);
					}
					// Сериализуем: uint16_t byte_len + uint16_t[N] groups
					if(!groupBytes.empty()){
						// Добавляем 2-байтовую длину списка групп
						u16be(payload, static_cast <uint16_t> (groupBytes.size()));
						// Добавляем байты списка групп
						payload.insert(payload.end(), groupBytes.begin(), groupBytes.end());
					}
				} break;
				// Если тип расширения соответствует ec_point_formats, то обрабатываем его как filter-list расширение, пересекаем шаблон со списком из src, порядок из шаблона
				case static_cast <uint8_t> (extension_type_t::EC_POINT_FORMATS): {
					// Отмечаем как filter-list расширение
					filterList = true;
					// Буфер для байт списка форматов точек
					vector <uint8_t> fmtBytes;
					// Получаем указатели на списки форматов точек: шаблон и исходный ClientHello
					const auto * tmpl = static_cast <const extension_ec_point_t *> (tmplExt.get());
					const auto * orig = static_cast <const extension_ec_point_t *> (findSrcExt(etype));
					/**
					 * Перебираем форматы точек шаблона, включаем только те, что есть в исходном ClientHello
					 */
					for(const auto & format : tmpl->formats){
						// Проверяем наличие формата точек в исходном ClientHello
						if(::find(orig->formats.begin(), orig->formats.end(), format) == orig->formats.end())
							// Формата точек нет в исходном ClientHello — пропускаем
							continue;
						// Получаем wire-код формата точек
						const uint8_t fw = ::local::ecPointWire(format);
						// Пропускаем неизвестные форматы точек
						if(fw == 0xFFu)
							// Неизвестный формат точек — пропускаем
							continue;
						// Добавляем wire-код формата точек в буфер
						fmtBytes.push_back(fw);
					}
					// Сериализуем: uint8_t count + uint8_t[N] formats
					if(!fmtBytes.empty()){
						// Добавляем 1-байтовое количество форматов точек
						payload.push_back(static_cast <uint8_t> (fmtBytes.size()));
						// Добавляем байты форматов точек
						payload.insert(payload.end(), fmtBytes.begin(), fmtBytes.end());
					}
				} break;
				// Если тип расширения соответствует delegated_credential
				case static_cast <uint8_t> (extension_type_t::DELEGATED_CREDENTIAL):
				// Если тип расширения соответствует signature_algorithms
				case static_cast <uint8_t> (extension_type_t::SIGNATURE_ALGORITHMS):
				// Если тип расширения соответствует signature_algorithms_cert
				case static_cast <uint8_t> (extension_type_t::SIGNATURE_ALGORITHMS_CERT): {
					// Отмечаем как filter-list расширение
					filterList = true;
					// Буфер для байт списка алгоритмов подписи
					vector <uint8_t> algBytes;
					// Получаем список алгоритмов подписи из шаблона
					const vector <signature_t> * tmplAlgs = nullptr;
					// Получаем список алгоритмов подписи из исходного ClientHello
					const vector <signature_t> * origAlgs = nullptr;
					// Выбираем правильный тип расширения для приведения указателя
					if(etype == extension_type_t::SIGNATURE_ALGORITHMS){
						// Список алгоритмов подписи из шаблона
						tmplAlgs = &static_cast <const extension_signature_t *> (tmplExt.get())->algorithms;
						// Список алгоритмов подписи из исходного ClientHello
						origAlgs = &static_cast <const extension_signature_t *> (findSrcExt(etype))->algorithms;
					// Если тип расширения соответствует signature_algorithms_cert
					} else if(etype == extension_type_t::SIGNATURE_ALGORITHMS_CERT) {
						// Список алгоритмов подписи для сертификатов из шаблона
						tmplAlgs = &static_cast <const extension_signature_algorithms_cert_t *> (tmplExt.get())->algorithms;
						// Список алгоритмов подписи для сертификатов из исходного ClientHello
						origAlgs = &static_cast <const extension_signature_algorithms_cert_t *> (findSrcExt(etype))->algorithms;
					// Если тип расширения соответствует delegated_credential
					} else {
						// Список делегированных алгоритмов подписи из шаблона
						tmplAlgs = &static_cast <const extension_delegated_credential_t *> (tmplExt.get())->algorithms;
						// Список делегированных алгоритмов подписи из исходного ClientHello
						origAlgs = &static_cast <const extension_delegated_credential_t *> (findSrcExt(etype))->algorithms;
					}
					/**
					 * Перебираем алгоритмы подписи шаблона, включаем только те, что есть в исходном ClientHello
					 */
					for(const auto & a : * tmplAlgs){
						// Проверяем наличие алгоритма подписи в исходном ClientHello
						if(::find(origAlgs->begin(), origAlgs->end(), a) == origAlgs->end())
							// Алгоритма подписи нет в исходном ClientHello — пропускаем
							continue;
						// Вычисляем wire-код алгоритма подписи (GREASE → 0x0A0A)
						const uint16_t aw = ((a == signature_t::GREASE) ? 0x0A0Au : ::local::signatureWire(a));
						// Пропускаем неизвестные алгоритмы подписи
						if(aw == 0u)
							// Неизвестный алгоритм подписи — пропускаем
							continue;
						// Добавляем wire-код алгоритма подписи в буфер
						u16be(algBytes, aw);
					}
					// Сериализуем: uint16_t byte_len + uint16_t[N] algorithms
					if(!algBytes.empty()){
						// Добавляем 2-байтовую длину списка алгоритмов подписи
						u16be(payload, static_cast <uint16_t> (algBytes.size()));
						// Добавляем байты списка алгоритмов подписи
						payload.insert(payload.end(), algBytes.begin(), algBytes.end());
					}
				} break;
				// Если тип расширения соответствует use_srtp
				case static_cast <uint8_t> (extension_type_t::USE_SRTP): {
					// Отмечаем как filter-list расширение
					filterList = true;
					// Буфер для байт списка профилей SRTP
					vector <uint8_t> profBytes;
					// Получаем указатели на расширение use_srtp: шаблон и исходный ClientHello
					const auto * tmpl = static_cast <const extension_use_srtp_t *> (tmplExt.get());
					const auto * orig = static_cast <const extension_use_srtp_t *> (findSrcExt(etype));
					/**
					 * @brief Вспомогательная функция: enum srtp_t → wire-код uint16_t
					 *
					 * @param profile профиль SRTP
					 * @return        wire-код профиля SRTP в виде uint16_t (GREASE → 0x0A0A, неизвестные профили → 0) для использования в расширении use_srtp
					 *
					 */
					auto srtpWire = [](const srtp_t profile) -> uint16_t {
						/**
						 * Определяем wire-код профиля SRTP
						 */
						switch(static_cast <uint8_t> (profile)){
							// Если профиль SRTP соответствует AES128_CM_HMAC_SHA1_80 (RFC 5764)
							case static_cast <uint8_t> (srtp_t::AES128_CM_HMAC_SHA1_80):
								// Печатаем wire-код профиля SRTP для AES128_CM_HMAC_SHA1_80
								return 0x0001u;
							// Если профиль SRTP соответствует AES128_CM_HMAC_SHA1_32 (RFC 5764)
							case static_cast <uint8_t> (srtp_t::AES128_CM_HMAC_SHA1_32):
								// Печатаем wire-код профиля SRTP для AES128_CM_HMAC_SHA1_32
								return 0x0002u;
							// Если профиль SRTP соответствует AES128_F8_HMAC_SHA1_80 (RFC 5764)
							case static_cast <uint8_t> (srtp_t::AES128_F8_HMAC_SHA1_80):
								// Печатаем wire-код профиля SRTP для AES128_F8_HMAC_SHA1_80
								return 0x0005u;
							// Если профиль SRTP соответствует NULL_HMAC_SHA1_80 (RFC 5764)
							case static_cast <uint8_t> (srtp_t::NULL_HMAC_SHA1_80):
								// Печатаем wire-код профиля SRTP для NULL_HMAC_SHA1_80
								return 0x0007u;
							// Если профиль SRTP соответствует NULL_HMAC_SHA1_32 (RFC 5764)
							case static_cast <uint8_t> (srtp_t::NULL_HMAC_SHA1_32):
								// Печатаем wire-код профиля SRTP для NULL_HMAC_SHA1_32
								return 0x0008u;
							// Если профиль SRTP соответствует AEAD_AES_128_GCM (RFC 5764)
							case static_cast <uint8_t> (srtp_t::AEAD_AES_128_GCM):
								// Печатаем wire-код профиля SRTP для AEAD_AES_128_GCM
								return 0x0009u;
							// Если профиль SRTP соответствует AEAD_AES_256_GCM (RFC 5764)
							case static_cast <uint8_t> (srtp_t::AEAD_AES_256_GCM):
								// Печатаем wire-код профиля SRTP для AEAD_AES_256_GCM
								return 0x000Au;
							// Неизвестный профиль SRTP
							default: return 0u;
						}
					};
					/**
					 * Перебираем профили SRTP шаблона, включаем только те, что есть в исходном ClientHello
					 */
					for(const auto & profile : tmpl->profiles){
						// Проверяем наличие профиля SRTP в исходном ClientHello
						if(::find(orig->profiles.begin(), orig->profiles.end(), profile) == orig->profiles.end())
							// Профиля SRTP нет в исходном ClientHello — пропускаем
							continue;
						// Вычисляем wire-код профиля SRTP (GREASE → 0x0A0A)
						const uint16_t pw = ((profile == srtp_t::GREASE) ? 0x0A0Au : srtpWire(profile));
						// Пропускаем неизвестные профили SRTP
						if(pw == 0u)
							// Неизвестный профиль SRTP — пропускаем
							continue;
						// Добавляем wire-код профиля SRTP в буфер
						u16be(profBytes, pw);
					}
					// Сериализуем: uint16_t byte_len + uint16_t[N] profiles + uint8_t mki_len [+ mki]
					if(!profBytes.empty()){
						// Добавляем 2-байтовую длину списка профилей SRTP
						u16be(payload, static_cast <uint16_t> (profBytes.size()));
						// Добавляем байты списка профилей SRTP
						payload.insert(payload.end(), profBytes.begin(), profBytes.end());
						// Добавляем длину MKI из исходного расширения
						payload.push_back(orig->mkiLength);
						// Если MKI данные присутствуют — добавляем их из сырых байт исходного буфера
						if((orig->mkiLength > 0u) && origExtRaw.count(wireType)){
							// Получаем сырые байты расширения use_srtp из исходного буфера
							const auto & raw = origExtRaw.at(wireType);
							// Вычисляем смещение к MKI (после profiles_len(2) + profiles * 2)
							const size_t mkiOff = (2u + orig->profiles.size() * 2u);
							// Проверяем, что MKI данные есть в сырых байтах расширения
							if((mkiOff + 1u) <= raw.size()){
								// Читаем фактическую длину MKI из сырых байт
								const size_t mkiActual = raw[mkiOff];
								// Добавляем байты MKI если они помещаются в сырые байты расширения
								if((mkiOff + 1u + mkiActual) <= raw.size())
									// Добавляем MKI данные в payload
									payload.insert(payload.end(), raw.begin() + mkiOff + 1u, raw.begin() + mkiOff + 1u + mkiActual);
							}
						}
					}
				} break;
				// Если тип расширения соответствует heartbeat
				case static_cast <uint8_t> (extension_type_t::HEARTBEAT): {
					// heartbeat: один байт режима; используем значение из исходного ClientHello
					const auto * orig = static_cast <const extension_heartbeat_t *> (findSrcExt(etype));
					// Добавляем байт режима heartbeat
					payload.push_back(static_cast <uint8_t> (orig->mode));
				} break;
				// Если тип расширения соответствует ALPN
				case static_cast <uint8_t> (extension_type_t::ALPN):
				// Если тип расширения соответствует APPLICATION_SETTINGS
				case static_cast <uint8_t> (extension_type_t::APPLICATION_SETTINGS):
				// Если тип расширения соответствует APPLICATION_SETTINGS_OLD
				case static_cast <uint8_t> (extension_type_t::APPLICATION_SETTINGS_OLD): {
					// Отмечаем как filter-list расширение (для пропуска при пустом пересечении)
					filterList = true;
					// Буфер для байт списка протоколов
					vector <uint8_t> protoBytes;
					// Получаем список протоколов из шаблона
					const vector <string> * tmplProtos = nullptr;
					// Получаем список протоколов из исходного ClientHello
					const vector <string> * origProtos = nullptr;
					// Выбираем правильный тип расширения для приведения указателя
					if(etype == extension_type_t::ALPN){
						// Список протоколов ALPN из шаблона
						tmplProtos = &static_cast <const extension_alpn_t *> (tmplExt.get())->protocols;
						// Список протоколов ALPN из исходного ClientHello
						origProtos = &static_cast <const extension_alpn_t *> (findSrcExt(etype))->protocols;
					// Если тип расширения соответствует APPLICATION_SETTINGS
					} else if(etype == extension_type_t::APPLICATION_SETTINGS) {
						// Список протоколов ALPS из шаблона
						tmplProtos = &static_cast <const extension_application_settings_t *> (tmplExt.get())->protocols;
						// Список протоколов ALPS из исходного ClientHello
						origProtos = &static_cast <const extension_application_settings_t *> (findSrcExt(etype))->protocols;
					// Если тип расширения соответствует APPLICATION_SETTINGS_OLD
					} else {
						// Список протоколов ALPS (old) из шаблона
						tmplProtos = &static_cast <const extension_application_settings_old_t *> (tmplExt.get())->protocols;
						// Список протоколов ALPS (old) из исходного ClientHello
						origProtos = &static_cast <const extension_application_settings_old_t *> (findSrcExt(etype))->protocols;
					}
					/**
					 * Перебираем протоколы шаблона, включаем только те, что есть в исходном ClientHello
					 */
					for(const auto & proto : * tmplProtos){
						// Проверяем наличие протокола в исходном ClientHello
						if(::find(origProtos->begin(), origProtos->end(), proto) == origProtos->end())
							// Протокола нет в исходном ClientHello — пропускаем
							continue;
						// Добавляем 1-байтовую длину имени протокола
						protoBytes.push_back(static_cast <uint8_t> (proto.size()));
						// Добавляем байты имени протокола
						protoBytes.insert(protoBytes.end(), proto.begin(), proto.end());
					}
					// Если пересечение непусто — сериализуем: uint16_t total_len + (uint8_t len + bytes)*N
					if(!protoBytes.empty()){
						// Добавляем 2-байтовую длину списка протоколов
						u16be(payload, static_cast <uint16_t> (protoBytes.size()));
						// Добавляем байты списка протоколов
						payload.insert(payload.end(), protoBytes.begin(), protoBytes.end());
					}
				} break;
				// Если тип расширения соответствует compress_certificate
				case static_cast <uint8_t> (extension_type_t::COMPRESS_CERTIFICATE): {
					// Отмечаем как filter-list расширение
					filterList = true;
					// Буфер для байт списка алгоритмов сжатия
					vector <uint8_t> algBytes;
					// Получаем указатели на расширение compress_certificate: шаблон и исходный ClientHello
					const auto * tmpl = static_cast <const extension_compress_certificate_t *> (tmplExt.get());
					const auto * orig = static_cast <const extension_compress_certificate_t *> (findSrcExt(etype));
					/**
					 * Перебираем алгоритмы сжатия шаблона, включаем только те, что есть в исходном ClientHello
					 * RFC 8879: uint8_t byte_count + uint16_t[N] algorithms (wire = 0x0001..0x0003)
					 */
					for(const auto & algorithm : tmpl->algorithms){
						// Проверяем наличие алгоритма в исходном ClientHello
						if(::find(orig->algorithms.begin(), orig->algorithms.end(), algorithm) == orig->algorithms.end())
							// Алгоритма нет в исходном ClientHello — пропускаем
							continue;
						// Получаем wire-код (NONE и UNKNOWN пропускаем)
						const uint8_t cw = ::local::compressorWire(algorithm);
						// Пропускаем NONE и неизвестные алгоритмы
						if((cw == 0u) || (cw == 0xFFu))
							// Пропускаем неизвестный алгоритм
							continue;
						// Добавляем 2-байтовый wire-код алгоритма сжатия в буфер
						u16be(algBytes, static_cast <uint16_t> (cw));
					}
					// Сериализуем: uint8_t byte_count + uint16_t[N] algorithms
					if(!algBytes.empty()){
						// Добавляем 1-байтовое количество байт в списке алгоритмов
						payload.push_back(static_cast <uint8_t> (algBytes.size()));
						// Добавляем байты списка алгоритмов сжатия
						payload.insert(payload.end(), algBytes.begin(), algBytes.end());
					}
				} break;
				// Если тип расширения соответствует supported_versions
				case static_cast <uint8_t> (extension_type_t::SUPPORTED_VERSIONS): {
					// Отмечаем как filter-list расширение
					filterList = true;
					// Буфер для байт списка версий
					vector <uint8_t> verBytes;
					// Получаем указатели на расширение supported_versions: шаблон и исходный ClientHello
					const auto * tmpl = static_cast <const extension_supported_versions_t *> (tmplExt.get());
					const auto * orig = static_cast <const extension_supported_versions_t *> (findSrcExt(etype));
					/**
					 * Перебираем версии шаблона, включаем только те, что есть в исходном ClientHello
					 */
					for(const auto & version : tmpl->versions){
						// Проверяем наличие версии в исходном ClientHello
						if(::find(orig->versions.begin(), orig->versions.end(), version) == orig->versions.end())
							// Версии нет в исходном ClientHello — пропускаем
							continue;
						// Вычисляем wire-код версии (GREASE → 0x0A0A)
						const uint16_t vw = ((version == version_t::GREASE) ? 0x0A0Au : ::local::versionWire(version));
						// Пропускаем неизвестные версии
						if(vw == 0u)
							// Неизвестная версия — пропускаем
							continue;
						// Добавляем wire-код версии в буфер
						u16be(verBytes, vw);
					}
					// Сериализуем: uint8_t byte_len + uint16_t[N] versions
					if(!verBytes.empty()){
						// Добавляем 1-байтовую длину списка версий
						payload.push_back(static_cast <uint8_t> (verBytes.size()));
						// Добавляем байты списка версий
						payload.insert(payload.end(), verBytes.begin(), verBytes.end());
					}
				} break;
				// Если тип расширения соответствует psk_key_exchange_modes
				case static_cast <uint8_t> (extension_type_t::PSK_KEY_EXCHANGE_MODES): {
					// Отмечаем как filter-list расширение
					filterList = true;
					// Буфер для байт списка режимов
					vector <uint8_t> modeBytes;
					// Получаем указатели на расширение psk_key_exchange_modes: шаблон и исходный ClientHello
					const auto * tmpl = static_cast <const extension_psk_key_exchange_t *> (tmplExt.get());
					const auto * orig = static_cast <const extension_psk_key_exchange_t *> (findSrcExt(etype));
					/**
					 * Перебираем режимы PSK шаблона, включаем только те, что есть в исходном ClientHello
					 */
					for(const auto & mode : tmpl->modes){
						// Проверяем наличие режима PSK в исходном ClientHello
						if(::find(orig->modes.begin(), orig->modes.end(), mode) == orig->modes.end())
							// Режима PSK нет в исходном ClientHello — пропускаем
							continue;
						// Пропускаем UNKNOWN
						if(mode == psk_key_t::UNKNOWN)
							// UNKNOWN режим PSK — пропускаем
							continue;
						// Добавляем байт режима PSK (wire == enum value: PSK_ONLY=0, PSK_DHE=1)
						modeBytes.push_back(static_cast <uint8_t> (mode));
					}
					// Сериализуем: uint8_t byte_count + uint8_t[N] modes
					if(!modeBytes.empty()){
						// Добавляем 1-байтовое количество байт в списке режимов
						payload.push_back(static_cast <uint8_t> (modeBytes.size()));
						// Добавляем байты режимов PSK
						payload.insert(payload.end(), modeBytes.begin(), modeBytes.end());
					}
				} break;
				// Если тип расширения соответствует key_share
				case static_cast <uint8_t> (extension_type_t::KEY_SHARE): {
					// Отмечаем как filter-list расширение
					filterList = true;
					// Буфер для байт списка KeyShareEntry
					vector <uint8_t> ksBytes;
					// Получаем указатели на расширение key_share: шаблон и исходный ClientHello
					const auto * tmpl = static_cast <const extension_key_share_t *> (tmplExt.get());
					const auto * orig = static_cast <const extension_key_share_t *> (findSrcExt(etype));
					/**
					 * Перебираем группы шаблона, включаем только те, что есть в исходном ClientHello.
					 * Ключевой материал (key_exchange) берём из исходного ClientHello.
					 */
					for(const auto & ks : tmpl->shares){
						// Ищем эту группу в исходном ClientHello
						const auto i = ::find_if(
							orig->shares.begin(), orig->shares.end(),
							[&ks](const pair <group_t, vector <uint8_t>> & e){ return e.first == ks.first; }
						);
						// Если группы нет в исходном ClientHello — пропускаем
						if(i == orig->shares.end())
							// Группы нет в исходном ClientHello — пропускаем
							continue;
						// Вычисляем wire-код группы (GREASE → 0x0A0A)
						const uint16_t gw = ((ks.first == group_t::GREASE) ? 0x0A0Au : ::local::groupWire(ks.first));
						// Пропускаем неизвестные группы
						if(gw == 0u)
							// Неизвестная группа — пропускаем
							continue;
						// Добавляем wire-код группы (2 байта)
						u16be(ksBytes, gw);
						// Добавляем 2-байтовую длину ключевого материала
						u16be(ksBytes, static_cast <uint16_t> (i->second.size()));
						// Добавляем ключевой материал из исходного ClientHello
						ksBytes.insert(ksBytes.end(), i->second.begin(), i->second.end());
					}
					// Сериализуем: uint16_t total_byte_len + (group:2 + key_len:2 + key_data)*N
					if(!ksBytes.empty()){
						// Добавляем 2-байтовую суммарную длину KeyShareEntry
						u16be(payload, static_cast <uint16_t> (ksBytes.size()));
						// Добавляем байты всех KeyShareEntry
						payload.insert(payload.end(), ksBytes.begin(), ksBytes.end());
					}
				} break;
				// Если тип расширения соответствует record_size_limit
				case static_cast <uint8_t> (extension_type_t::RECORD_SIZE_LIMIT): {
					// record_size_limit: 2-байтовое значение из исходного ClientHello
					const auto * orig = static_cast <const extension_record_size_limit_t *> (findSrcExt(etype));
					// Добавляем wire-код значения record_size_limit
					u16be(payload, orig->data);
				} break;
				/**
				 * ─── Blob-расширения ─────────────────────────────────────────────────
				 * Используем сырые байты из исходного буфера без изменений.
				 * Это расширения с данными, специфичными для соединения
				 * (SNI, session_ticket, PSK, ECH и пр.) или пустые в ClientHello
				 * (encrypt_then_mac, extended_master_secret, post_handshake_auth и пр.).
				 */
				default: {
					// Ищем сырые байты расширения по его wire-типу в карте сырых байт расширений из исходного буфера
					auto i = origExtRaw.find(wireType);
					// Если сырые байты расширения есть в исходном буфере — используем их
					if(i != origExtRaw.end())
						// Берём payload из исходного буфера
						payload = i->second;
					// Иначе payload остаётся пустым (для расширений без данных в ClientHello)
				}
			}
			// Если это filter-list расширение и payload пуст после фильтрации — пропускаем расширение
			if(filterList && payload.empty())
				// Пропускаем расширение с пустым пересечением
				continue;
			// Добавляем расширение в общий payload расширений
			appendExt(extPayload, wireType, payload);
		}
		/**
		 * ─── Собираем результирующий буфер ──────────────────────────────────────────
		 *
		 * Структура:
		 *   1. Verbatim: все байты от начала буфера до cipher_suites_len (record + hs headers + version + random + session_id + cookie)
		 *   2. cipher_suites_len (2 байта) + cipher_suites (отфильтрованные)
		 *   3. Verbatim: compression_methods_len + compression_methods из исходного буфера
		 *   4. extensions_len (2 байта) + extensions (отфильтрованные и переупорядоченные)
		 */
		// 1. Копируем verbatim-часть (от начала до cipher_suites_len)
		result.insert(result.end(), buffer, buffer + csStart);
		// 2. Добавляем cipher_suites_len + cipher_suites
		u16be(result, static_cast <uint16_t> (cipherPayload.size()));
		// Добавляем байты списка шифров
		result.insert(result.end(), cipherPayload.begin(), cipherPayload.end());
		// 3. Копируем compression_methods из исходного буфера verbatim
		result.insert(result.end(), buffer + cmStart, buffer + cmEnd);
		// 4. Добавляем extensions_len + extensions (если есть)
		if(!extPayload.empty()){
			// Добавляем 2-байтовую суммарную длину расширений
			u16be(result, static_cast <uint16_t> (extPayload.size()));
			// Добавляем байты всех расширений
			result.insert(result.end(), extPayload.begin(), extPayload.end());
		}
		/**
		 * ─── Обновляем поля длины в заголовках ──────────────────────────────────────
		 *
		 * Handshake length (uint24 big-endian): байты [recordSize+1..recordSize+3]
		 *   = total - recordSize - handshakeSize  (длина тела ClientHello)
		 *
		 * Record payload length (uint16 big-endian):
		 *   TLS:  байты [3..4] = total - recordSize
		 *   DTLS: байты [11..12] = total - recordSize
		 *
		 * DTLS frag_length (uint24 big-endian): байты [recordSize+9..recordSize+11]
		 *   = то же значение, что и handshake length (фрагмент не разбит)
		 */
		// Вычисляем длину тела ClientHello (после заголовка рукопожатия)
		const size_t handshakeBodyLen = (result.size() - recordSize - handshakeSize);
		// Обновляем поле handshake length (bytes [recordSize+1..recordSize+3], uint24)
		result[recordSize + 1u] = static_cast <uint8_t> ((handshakeBodyLen >> 16u) & 0xFFu);
		// Обновляем второй байт поля handshake length
		result[recordSize + 2u] = static_cast <uint8_t> ((handshakeBodyLen >>  8u) & 0xFFu);
		// Обновляем третий байт поля handshake length
		result[recordSize + 3u] = static_cast <uint8_t> (handshakeBodyLen          & 0xFFu);
		// Для DTLS также обновляем frag_length (bytes [recordSize+9..recordSize+11], uint24)
		if(isDTLS){
			// Обновляем первый байт поля frag_length
			result[recordSize + 9u]  = static_cast <uint8_t> ((handshakeBodyLen >> 16u) & 0xFFu);
			// Обновляем второй байт поля frag_length
			result[recordSize + 10u] = static_cast <uint8_t> ((handshakeBodyLen >>  8u) & 0xFFu);
			// Обновляем третий байт поля frag_length
			result[recordSize + 11u] = static_cast <uint8_t> (handshakeBodyLen          & 0xFFu);
		}
		// Вычисляем длину payload записи TLS (после заголовка record)
		const size_t recordPayloadLen = (result.size() - recordSize);
		// Обновляем поле record payload length
		if(!isDTLS){
			// TLS: record length в байтах [3..4] (uint16 big-endian)
			result[3u] = static_cast <uint8_t> (recordPayloadLen >> 8u);
			// Обновляем второй байт поля record length
			result[4u] = static_cast <uint8_t> (recordPayloadLen & 0xFFu);
		// Если это DTLS, то поле длины записи находится в байтах [11..12]
		} else {
			// DTLS: record length в байтах [11..12] (uint16 big-endian)
			result[11u] = static_cast <uint8_t> (recordPayloadLen >> 8u);
			// Обновляем второй байт поля record length
			result[12u] = static_cast <uint8_t> (recordPayloadLen & 0xFFu);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод очистки всех цифровых отпечатков браузеров из хранилища
 *
 */
void awh::tls::Fingerprint::clear() noexcept {
	// Блокируем хранилище отпечатков для записи
	const local::fgp_exclusive_lock_t lock(this->_mtx, local::fgp_exclusive);
	// Очищаем хранилище цифровых отпечатков браузеров
	this->_browsers.clear();
}
/**
 * @brief Метод проверки, пусто ли хранилище цифровых отпечатков браузеров
 *
 * @return результат проверки
 *
 */
bool awh::tls::Fingerprint::empty() const noexcept {
	// Блокируем хранилище отпечатков для чтения
	const local::fgp_shared_lock_t lock(this->_mtx, local::fgp_shared);
	// Проверяем, что хранилище цифровых отпечатков браузеров пусто
	return this->_browsers.empty();
}
/**
 * @brief Метод получения количества цифровых отпечатков браузеров, хранящихся в хранилище
 *
 * @return количество цифровых отпечатков браузеров
 *
 */
size_t awh::tls::Fingerprint::size() const noexcept {
	// Блокируем хранилище отпечатков для чтения
	const local::fgp_shared_lock_t lock(this->_mtx, local::fgp_shared);
	// Возвращаем количество цифровых отпечатков браузеров в хранилище
	return this->_browsers.size();
}
/**
 * @brief Метод получения списка идентификаторов всех цифровых отпечатков браузеров, хранящихся в хранилище
 *
 * @return список идентификаторов цифровых отпечатков браузеров
 *
 */
vector <awh::tls::Fingerprint::id_t> awh::tls::Fingerprint::list() const noexcept {
	// Переменная результата
	vector <id_t> result;
	// Блокируем хранилище отпечатков для чтения
	const local::fgp_shared_lock_t lock(this->_mtx, local::fgp_shared);
	/**
	 * Выполняем перебор всех цифровых отпечатков браузеров в хранилище
	 */
	for(const auto & [id, browser] : this->_browsers)
		// Добавляем идентификатор цифрового отпечатка браузера в результат
		result.push_back(id);
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод установки режима потокобезопасности хранилища отпечатков
 *
 * @param mode флаг режима безопасности потоков
 *
 */
void awh::tls::Fingerprint::threadSafety(const bool mode) noexcept {
	// Активируем работу мьютекса блокировки хранилища отпечатков
	this->_mtx.enabled = mode;
}
/**
 * @brief Метод удаления цифрового отпечатка браузера из хранилища по идентификатору
 *
 * @param id идентификатор цифрового отпечатка
 * @return   результат выполнения удаления цифрового отпечатка
 *
 */
bool awh::tls::Fingerprint::remove(const id_t id) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Блокируем хранилище отпечатков для записи
		const local::fgp_exclusive_lock_t lock(this->_mtx, local::fgp_exclusive);
		// Ищем цифровой отпечаток браузера по идентификатору
		auto i = this->_browsers.find(id);
		// Если цифровой отпечаток браузера найден — удаляем его из хранилища
		if((result = (i != this->_browsers.end())))
			// Удаляем цифровой отпечаток браузера из хранилища
			this->_browsers.erase(i);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (id)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод получения данных цифрового отпечатка браузера по идентификатору
 *
 * @param id идентификатор цифрового отпечатка
 * @return   объект с цифровым отпечатком браузера, соответствующий указанному идентификатору
 *
 */
const awh::tls::Fingerprint::browser_t & awh::tls::Fingerprint::get(const id_t id) const noexcept {
	// Переменная результата
	const static browser_t result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Блокируем хранилище отпечатков для чтения
		const local::fgp_shared_lock_t lock(this->_mtx, local::fgp_shared);
		// Ищем цифровой отпечаток браузера по идентификатору
		auto i = this->_browsers.find(id);
		// Если цифровой отпечаток браузера найден
		if(i != this->_browsers.end())
			// Возвращаем цифровой отпечаток браузера, соответствующий указанному идентификатору
			return i->second;
		// Если идентификатор задан, но отпечаток не найден
		if(id != 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем предупреждение в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (id)), log_t::flag_t::WARNING, "Browser fingerprint id is not found");
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем предупреждение в лог
				this->_log->print("%s", log_t::flag_t::WARNING, "Browser fingerprint id is not found");
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (id)), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод добавления цифрового отпечатка браузера в хранилище
 *
 * @param browser объект с распарсенными данными ClientHello
 * @return        идентификатор добавленного цифрового отпечатка
 *
 */
awh::tls::Fingerprint::id_t awh::tls::Fingerprint::add(const browser_t & browser) noexcept {
	// Переменная результата
	id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Начинаем с 1 (0 можно оставить как "invalid")
		static atomic_uint8_t nextId{1};
		// Блокируем хранилище отпечатков для записи
		const local::fgp_exclusive_lock_t lock(this->_mtx, local::fgp_exclusive);
		/**
		 * Выполняем перебор идентификаторов до успешной вставки или исчерпания диапазона
		 */
		for(uint16_t attempt = 0; attempt < numeric_limits <id_t>::max(); ++attempt){
			// Получаем следующий идентификатор
			const id_t candidate = nextId.fetch_add(1, memory_order_relaxed);
			// Пропускаем зарезервированный идентификатор
			if(candidate == 0)
				// Переходим к следующей попытке
				continue;
			// Пробуем добавить цифровой отпечаток браузера в хранилище
			if(this->_browsers.emplace(candidate, browser).second){
				// Сохраняем идентификатор добавленного цифрового отпечатка
				result = candidate;
				// Выходим из цикла
				break;
			}
		}
		// Если свободный идентификатор не найден
		if(result == 0){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, "Browser fingerprint storage is full");
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, "Browser fingerprint storage is full");
			#endif
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод добавления цифрового отпечатка браузера в хранилище в бинарном виде (дамп цифрового отпечатка)
 *
 * @param buffer бинарный буфер с данными цифрового отпечатка
 * @param size   размер бинарного буфера в байтах
 * @return       идентификатор добавленного цифрового отпечатка
 *
 */
awh::tls::Fingerprint::id_t awh::tls::Fingerprint::add(const uint8_t * buffer, const size_t size) noexcept {
	// Переменная результата
	id_t result = 0;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Объект снимка цифрового отпечатка браузера для загрузки данных из бинарного буфера
		browser_t browser{};
		// Если данные снимка цифрового отпечатка браузера успешно загружены из бинарного буфера
		if(this->parse(buffer, size, browser))
			// Добавляем цифровой отпечаток браузера в хранилище и получаем его идентификатор
			result = this->add(browser);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer, size), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод формирования бинарного дампа всех цифровых отпечатков браузеров
 *
 * @return бинарный буфер, содержащий дамп всех цифровых отпечатков браузеров
 *
 */
vector <uint8_t> awh::tls::Fingerprint::dump() const noexcept {
	// Переменная результата
	vector <uint8_t> result;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Блокируем хранилище отпечатков для чтения
		const local::fgp_shared_lock_t lock(this->_mtx, local::fgp_shared);
		// Если список цифровых отпечатков браузеров заполнен
		if(!this->_browsers.empty()){
			// Бинарный буфер снимка цифрового отпечатка браузера
			vector <uint8_t> buffer;
			// Размер буфера снимка цифрового отпечатка браузера и количество снимков браузеров в результирующем дампе
			size_t size = 0, count = 0;
			/**
			 * Выполняем перебор всех снимков браузеров в хранилище
			 */
			for(const auto & [id, browser] : this->_browsers){
				// Очищаем буфер от предыдущих данных
				buffer.clear();
				// Если данные снимка цифрового отпечатка браузера успешно загружены в буфер
				if(this->dump(browser, buffer)){
					// Получаем размер буфера снимка цифрового отпечатка браузера
					size = buffer.size();
					// Добавляем в результирующий буфер идентификатор цифрового отпечатка браузера
					result.insert(result.end(), reinterpret_cast <const uint8_t *> (&id), reinterpret_cast <const uint8_t *> (&id) + sizeof(id));
					// Добавляем в результирующий буфер размер снимка цифрового отпечатка браузера
					result.insert(result.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Добавляем в результирующий буфер, полученный буфер снимка цифрового отпечатка браузера
					result.insert(result.end(), buffer.begin(), buffer.end());
					// Увеличиваем счётчик успешно добавленных снимков цифровых отпечатков браузеров
					count++;
				}
			}
			// Если удалось добавить хотя бы один снимок цифрового отпечатка браузера в результирующий дамп
			if(count > 0)
				// Добавляем в начало результирующего буфера количество добавленных снимков цифровых отпечатков браузеров
				result.insert(result.begin(), reinterpret_cast <const uint8_t *> (&count), reinterpret_cast <const uint8_t *> (&count) + sizeof(count));
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод загрузки бинарного дампа всех цифровых отпечатков браузеров
 *
 * @param buffer бинарный буфер для загрузки данных цифровых отпечатков
 * @return       результат загрузки бинарного дампа
 *
 */
bool awh::tls::Fingerprint::dump(const vector <uint8_t> & buffer) noexcept {
	// Переменная результата
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Блокируем хранилище отпечатков для записи
		const local::fgp_exclusive_lock_t lock(this->_mtx, local::fgp_exclusive);
		// Очищаем хранилище от предыдущих данных
		this->_browsers.clear();
		// Если буфер с данными цифровых отпечатков браузеров не пустой
		if(!buffer.empty() && (buffer.size() > sizeof(size_t))){
			// Текущая позиция чтения в буфере
			size_t offset = 0;
			// Количество снимков цифровых отпечатков браузеров
			size_t count = 0;
			// Извлекаем количество снимков цифровых отпечатков браузеров
			::memcpy(&count, &buffer[0], sizeof(count));
			// Смещаем позицию чтения из бинарного буфера
			offset += sizeof(count);
			// Если в буфере есть данные хотя бы для одного снимка цифрового отпечатка браузера
			if((count > 0) && (buffer.size() >= (offset + count * (sizeof(id_t) + sizeof(size_t))))){
				// Идентификатор снимка цифрового отпечатка браузера
				id_t id = 0;
				// Размер снимка цифрового отпечатка браузера
				size_t size = 0;
				// Объект для хранения данных цифрового отпечатка браузера
				browser_t browser{};
				/**
				 * Выполняем загрузку каждого снимка цифрового отпечатка браузера из буфера
				 */
				for(size_t i = 0; i < count; i++){
					// Извлекаем идентификатор снимка цифрового отпечатка браузера
					::memcpy(&id, &buffer[offset], sizeof(id));
					// Смещаем позицию чтения на размер идентификатора
					offset += sizeof(id);
					// Извлекаем размер снимка цифрового отпечатка браузера
					::memcpy(&size, &buffer[offset], sizeof(size));
					// Смещаем позицию чтения на размер поля
					offset += sizeof(size);
					// Если в буфере достаточно данных для загрузки снимка цифрового отпечатка браузера
					if(buffer.size() >= (offset + size)){
						// Загружаем снимок цифрового отпечатка браузера из буфера
						if(this->dump(vector <uint8_t> (&buffer[0] + offset, &buffer[0] + offset + size), browser))
							// Добавляем снимок цифрового отпечатка браузера в хранилище
							this->_browsers.emplace(id, ::move(browser));
						// Смещаем позицию чтения на размер снимка цифрового отпечатка браузера
						offset += size;
					// Если недостаточно данных в буфере для загрузки снимка цифрового отпечатка браузера — прерываем загрузку
					} else break;
				}
				// Если удалось загрузить хотя бы один снимок цифрового отпечатка браузера — сообщаем об успешном результате
				result = !this->_browsers.empty();
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(buffer.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем результат
	return result;
}
/**
 * @brief Метод загрузки бинарного дампа цифрового отпечатка
 *
 * @param input   бинарный буфер с данными цифрового отпечатка
 * @param browser объект для хранения данных цифрового отпечатка
 * @return        результат загрузки бинарного дампа цифрового отпечатка
 *
 */
bool awh::tls::Fingerprint::dump(const vector <uint8_t> & input, browser_t & browser) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Если входной буфер пустой, возвращаем ошибку
		if(input.empty())
			// Возвращаем значение по умолчанию
			return false;
		// Текущая позиция чтения в буфере
		size_t offset = 0;
		/**
		 * Вспомогательная функция для безопасного чтения N байт из входного буфера
		 *
		 * @param buffer бинарный буфер для записи прочитанных данных
		 * @param size   количество байт для чтения
		 * @return       результат чтения
		 *
		 */
		auto read = [&offset, &input](void * buffer, const size_t size) -> bool {
			// Если данных в буфере недостаточно, возвращаем ошибку
			if((offset + size) > input.size())
				// Недостаточно данных в буфере для чтения — возвращаем ошибку
				return false;
			// Копируем данные в целевой буфер
			::memcpy(buffer, &input[0] + offset, size);
			// Смещаем позицию чтения
			offset += size;
			// Сообщаем об успешном результате
			return true;
		};
		// Очищаем объект цифрового отпечатка браузера от предыдущих данных
		browser = browser_t{};
		// Читаем флаг использования GREASE
		if(!read(&browser.grease, sizeof(browser.grease)))
			// Возвращаем ошибку при чтении флага использования GREASE
			return false;
		// Читаем запись метаданных TLS рукопожатия
		if(!read(&browser.record, sizeof(browser.record)))
			// Возвращаем ошибку при чтении записи метаданных TLS рукопожатия
			return false;
		// Читаем объект рукопожатия TLS
		if(!read(&browser.handshake, sizeof(browser.handshake)))
			// Возвращаем ошибку при чтении объекта рукопожатия TLS
			return false;
		// Читаем объект ClientHello TLS
		if(!read(&browser.clientHello, sizeof(browser.clientHello)))
			// Возвращаем ошибку при чтении объекта ClientHello TLS
			return false;
		// Читаем размер куков DTLS
		size_t count = 0;
		// Читаем размер куков DTLS из буфера
		if(!read(&count, sizeof(count)))
			// Возвращаем ошибку при чтении размера куков DTLS
			return false;
		// Если куки DTLS присутствуют
		if(count > 0){
			// Выделяем память под куки DTLS
			browser.cookie.resize(count);
			// Читаем куки DTLS
			if(!read(&browser.cookie[0], count))
				// Возвращаем ошибку при чтении куков DTLS
				return false;
		}
		// Читаем размер идентификатора сессии TLS
		count = 0;
		// Читаем размер идентификатора сессии TLS из буфера
		if(!read(&count, sizeof(count)))
			// Возвращаем ошибку при чтении размера идентификатора сессии TLS
			return false;
		// Если идентификатор сессии TLS присутствует
		if(count > 0){
			// Выделяем память под идентификатор сессии TLS
			browser.session.resize(count);
			// Читаем идентификатор сессии TLS
			if(!read(&browser.session[0], count))
				// Возвращаем ошибку при чтении идентификатора сессии TLS
				return false;
		}
		// Читаем размер списка шифров TLS
		count = 0;
		// Читаем размер списка шифров TLS из буфера
		if(!read(&count, sizeof(count)))
			// Возвращаем ошибку при чтении размера списка шифров TLS
			return false;
		// Если список шифров TLS не пустой
		if(count > 0){
			// Выделяем память под список шифров TLS
			browser.ciphers.resize(count);
			// Читаем список шифров TLS
			if(!read(&browser.ciphers[0], count))
				// Возвращаем ошибку при чтении списка шифров TLS
				return false;
		}
		// Читаем размер списка методов компрессии сертификата TLS
		count = 0;
		// Читаем размер списка методов компрессии сертификата TLS из буфера
		if(!read(&count, sizeof(count)))
			// Возвращаем ошибку при чтении размера списка методов компрессии сертификата TLS
			return false;
		// Если список методов компрессии не пустой
		if(count > 0){
			// Выделяем память под список методов компрессии
			browser.compressors.resize(count);
			// Читаем список методов компрессии
			if(!read(&browser.compressors[0], count))
				// Возвращаем ошибку при чтении списка методов компрессии TLS
				return false;
		}
		/**
		 * Тип расширения TLS для чтения и восстановления объектов расширений из буфера
		 */
		extension_type_t type = extension_type_t::UNKNOWN;
		/**
		 * Читаем расширения TLS, пока не достигнем конца буфера
		 */
		while(offset < input.size()){
			// Читаем тип расширения TLS из буфера
			if(!read(&type, sizeof(type)))
				// Возвращаем ошибку при чтении типа расширения TLS
				return false;
			/**
			 * Восстанавливаем объект расширения в зависимости от типа
			 */
			switch(static_cast <uint8_t> (type)){
				// Если тип расширения соответствует grease
				case static_cast <uint8_t> (awh::tls::extension_type_t::GREASE):
					// Добавляем расширение в список
					browser.extensions.push_back(make_unique <extension_grease_t> ());
				break;
				// Если тип расширения соответствует channel_id
				case static_cast <uint8_t> (awh::tls::extension_type_t::CHANNEL_ID):
					// Добавляем расширение в список
					browser.extensions.push_back(make_unique <extension_channel_id_t> ());
				break;
				// Если тип расширения соответствует oid_filters
				case static_cast <uint8_t> (awh::tls::extension_type_t::OID_FILTERS):
					// Добавляем расширение в список
					browser.extensions.push_back(make_unique <extension_oid_filters_t> ());
				break;
				// Если тип расширения соответствует trust_anchors
				case static_cast <uint8_t> (awh::tls::extension_type_t::TRUST_ANCHORS):
					// Добавляем расширение в список
					browser.extensions.push_back(make_unique <extension_trust_anchors_t> ());
				break;
				// Если тип расширения соответствует encrypt_then_mac
				case static_cast <uint8_t> (awh::tls::extension_type_t::ENCRYPT_THEN_MAC):
					// Добавляем расширение в список
					browser.extensions.push_back(make_unique <extension_encrypt_then_mac_t> ());
				break;
				// Если тип расширения соответствует transparency_info
				case static_cast <uint8_t> (awh::tls::extension_type_t::TRANSPARENCY_INFO):
					// Добавляем расширение в список
					browser.extensions.push_back(make_unique <extension_transparency_info_t> ());
				break;
				// Если тип расширения соответствует post_handshake_auth
				case static_cast <uint8_t> (awh::tls::extension_type_t::POST_HANDSHAKE_AUTH):
					// Добавляем расширение в список
					browser.extensions.push_back(make_unique <extension_post_handshake_auth_t> ());
				break;
				// Если тип расширения соответствует client_certificate_type
				case static_cast <uint8_t> (awh::tls::extension_type_t::CLIENT_CERTIFICATE_TYPE):
					// Добавляем расширение в список
					browser.extensions.push_back(make_unique <extension_client_certificate_type_t> ());
				break;
				// Если тип расширения соответствует server_certificate_type
				case static_cast <uint8_t> (awh::tls::extension_type_t::SERVER_CERTIFICATE_TYPE):
					// Добавляем расширение в список
					browser.extensions.push_back(make_unique <extension_server_certificate_type_t> ());
				break;
				// Если тип расширения соответствует signed_certificate_timestamp
				case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNED_CERTIFICATE_TIMESTAMP):
					// Добавляем расширение в список
					browser.extensions.push_back(make_unique <extension_signed_certificate_timestamp_t> ());
				break;
				// Если тип расширения соответствует server_name
				case static_cast <uint8_t> (awh::tls::extension_type_t::SERVER_NAME): {
					// Создаём объект расширения
					auto extension = make_unique <extension_server_name_t> ();
					// Количество имён серверов в списке
					size_t count = 0;
					// Читаем количество имён серверов из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества имён серверов из буфера
						return false;
					// Если список имён серверов не пустой
					if(count > 0){
						// Размер имени сервера в байтах
						size_t size = 0;
						// Выделяем память под список имён серверов
						extension->names.resize(count);
						/**
						 * Перебираем имена серверов
						 */
						for(auto & name : extension->names){
							// Читаем размер имени сервера из буфера
							if(!read(&size, sizeof(size)))
								// Возвращаем ошибку при чтении размера имени сервера из буфера
								return false;
							// Если имя сервера не пустое
							if(size > 0){
								// Выделяем память под имя сервера
								name.resize(size);
								// Читаем имя сервера из буфера
								if(!read(&name[0], size))
									// Возвращаем ошибку при чтении имени сервера из буфера
									return false;
							}
						}
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует max_fragment_length
				case static_cast <uint8_t> (awh::tls::extension_type_t::MAX_FRAGMENT_LENGTH): {
					// Создаём объект расширения
					auto extension = make_unique <extension_max_fragment_length_t> ();
					// Читаем значение max_fragment_length
					if(!read(&extension->length, sizeof(extension->length)))
						// Возвращаем ошибку при чтении значения max_fragment_length из буфера
						return false;
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует status_request
				case static_cast <uint8_t> (awh::tls::extension_type_t::STATUS_REQUEST): {
					// Создаём объект расширения
					auto extension = make_unique <extension_status_request_t> ();
					// Размер типа статуса сертификата в байтах
					size_t size = 0;
					// Читаем размер типа статуса сертификата
					if(!read(&size, sizeof(size)))
						// Возвращаем ошибку при чтении размера типа статуса сертификата из буфера
						return false;
					// Если тип статуса сертификата не пустой
					if(size > 0){
						// Выделяем память под тип статуса сертификата
						extension->certificateStatusType.resize(size);
						// Читаем тип статуса сертификата
						if(!read(&extension->certificateStatusType[0], size))
							// Возвращаем ошибку при чтении типа статуса сертификата из буфера
							return false;
					}
					// Читаем длину списка идентификаторов ответчиков
					if(!read(&extension->responderIdListLength, sizeof(extension->responderIdListLength)))
						// Возвращаем ошибку при чтении длины списка идентификаторов ответчиков из буфера
						return false;
					// Читаем длину расширений запроса статуса сертификата
					if(!read(&extension->requestExtensionsLength, sizeof(extension->requestExtensionsLength)))
						// Возвращаем ошибку при чтении длины расширений запроса статуса сертификата из буфера
						return false;
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует supported_groups
				case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_GROUPS): {
					// Создаём объект расширения
					auto extension = make_unique <extension_supported_groups_t> ();
					// Количество поддерживаемых групп в списке
					size_t count = 0;
					// Читаем количество поддерживаемых групп из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества поддерживаемых групп из буфера
						return false;
					// Если список поддерживаемых групп не пустой
					if(count > 0){
						// Выделяем память под список поддерживаемых групп
						extension->supportedGroups.resize(count);
						// Читаем список поддерживаемых групп
						if(!read(&extension->supportedGroups[0], count))
							// Возвращаем ошибку при чтении списка поддерживаемых групп из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует ec_point_formats
				case static_cast <uint8_t> (awh::tls::extension_type_t::EC_POINT_FORMATS): {
					// Создаём объект расширения
					auto extension = make_unique <extension_ec_point_t> ();
					// Количество форматов точек эллиптической кривой в списке
					size_t count = 0;
					// Читаем количество форматов точек эллиптической кривой из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества форматов точек эллиптической кривой из буфера
						return false;
					// Если список форматов точек не пустой
					if(count > 0){
						// Выделяем память под список форматов точек
						extension->formats.resize(count);
						// Читаем список форматов точек
						if(!read(&extension->formats[0], count))
							// Возвращаем ошибку при чтении списка форматов точек из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует signature_algorithms
				case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS): {
					// Создаём объект расширения
					auto extension = make_unique <extension_signature_t> ();
					// Количество алгоритмов подписи в списке
					size_t count = 0;
					// Читаем количество алгоритмов подписи из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества алгоритмов подписи из буфера
						return false;
					// Если список алгоритмов подписи не пустой
					if(count > 0){
						// Выделяем память под список алгоритмов подписи
						extension->algorithms.resize(count);
						// Читаем список алгоритмов подписи
						if(!read(&extension->algorithms[0], count))
							// Возвращаем ошибку при чтении списка алгоритмов подписи из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует use_srtp
				case static_cast <uint8_t> (awh::tls::extension_type_t::USE_SRTP): {
					// Создаём объект расширения
					auto extension = make_unique <extension_use_srtp_t> ();
					// Читаем значение mki_length
					if(!read(&extension->mkiLength, sizeof(extension->mkiLength)))
						// Возвращаем ошибку при чтении значения mki_length из буфера
						return false;
					// Количество профилей SRTP в списке
					size_t count = 0;
					// Читаем количество профилей SRTP из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества профилей SRTP из буфера
						return false;
					// Если список профилей SRTP не пустой
					if(count > 0){
						// Выделяем память под список профилей SRTP
						extension->profiles.resize(count);
						// Читаем список профилей SRTP
						if(!read(&extension->profiles[0], count))
							// Возвращаем ошибку при чтении списка профилей SRTP из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует heartbeat
				case static_cast <uint8_t> (awh::tls::extension_type_t::HEARTBEAT): {
					// Создаём объект расширения
					auto extension = make_unique <extension_heartbeat_t> ();
					// Читаем режим heartbeat
					if(!read(&extension->mode, sizeof(extension->mode)))
						// Возвращаем ошибку при чтении режима heartbeat из буфера
						return false;
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует alpn
				case static_cast <uint8_t> (awh::tls::extension_type_t::ALPN): {
					// Создаём объект расширения
					auto extension = make_unique <extension_alpn_t> ();
					// Количество протоколов ALPN в списке
					size_t count = 0;
					// Читаем количество протоколов ALPN из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества протоколов ALPN из буфера
						return false;
					// Если список протоколов ALPN не пустой
					if(count > 0){
						// Размер протокола ALPN в байтах
						size_t size = 0;
						// Выделяем память под список протоколов ALPN
						extension->protocols.resize(count);
						/**
						 * Перебираем протоколы ALPN
						 */
						for(auto & proto : extension->protocols){
							// Читаем размер протокола ALPN из буфера
							if(!read(&size, sizeof(size)))
								// Возвращаем ошибку при чтении размера протокола ALPN из буфера
								return false;
							// Если протокол ALPN не пустой
							if(size > 0){
								// Выделяем память под протокол ALPN
								proto.resize(size);
								// Читаем протокол ALPN
								if(!read(&proto[0], size))
									// Возвращаем ошибку при чтении протокола ALPN из буфера
									return false;
							}
						}
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует padding
				case static_cast <uint8_t> (awh::tls::extension_type_t::PADDING): {
					// Создаём объект расширения
					auto extension = make_unique <extension_padding_t> ();
					// Читаем размер паддинга
					if(!read(&extension->size, sizeof(extension->size)))
						// Возвращаем ошибку при чтении размера паддинга из буфера
						return false;
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует extended_master_secret
				case static_cast <uint8_t> (awh::tls::extension_type_t::EXTENDED_MASTER_SECRET): {
					// Создаём объект расширения
					auto extension = make_unique <extension_extended_master_secret_t> ();
					// Размер данных master_secret в байтах
					size_t size = 0;
					// Читаем размер данных master_secret из буфера
					if(!read(&size, sizeof(size)))
						// Возвращаем ошибку при чтении размера данных master_secret из буфера
						return false;
					// Если данные master_secret не пустые
					if(size > 0){
						// Выделяем память под данные master_secret
						extension->masterSecretData.resize(size);
						// Читаем данные master_secret
						if(!read(&extension->masterSecretData[0], size))
							// Возвращаем ошибку при чтении данных master_secret из буфера
							return false;
					}
					// Читаем размер данных extended_master_secret
					if(!read(&size, sizeof(size)))
						// Возвращаем ошибку при чтении размера данных extended_master_secret из буфера
						return false;
					// Если данные extended_master_secret не пустые
					if(size > 0){
						// Выделяем память под данные extended_master_secret
						extension->extendedMasterSecretData.resize(size);
						// Читаем данные extended_master_secret
						if(!read(&extension->extendedMasterSecretData[0], size))
							// Возвращаем ошибку при чтении данных extended_master_secret из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует compress_certificate
				case static_cast <uint8_t> (awh::tls::extension_type_t::COMPRESS_CERTIFICATE): {
					// Создаём объект расширения
					auto extension = make_unique <extension_compress_certificate_t> ();
					// Количество алгоритмов сжатия сертификата в списке
					size_t count = 0;
					// Читаем количество алгоритмов сжатия сертификата из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества алгоритмов сжатия сертификата из буфера
						return false;
					// Если список алгоритмов сжатия не пустой
					if(count > 0){
						// Выделяем память под список алгоритмов сжатия
						extension->algorithms.resize(count);
						// Читаем список алгоритмов сжатия
						if(!read(&extension->algorithms[0], count))
							// Возвращаем ошибку при чтении списка алгоритмов сжатия из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует record_size_limit
				case static_cast <uint8_t> (awh::tls::extension_type_t::RECORD_SIZE_LIMIT): {
					// Создаём объект расширения
					auto extension = make_unique <extension_record_size_limit_t> ();
					// Читаем значение record_size_limit
					if(!read(&extension->data, sizeof(extension->data)))
						// Возвращаем ошибку при чтении значения record_size_limit из буфера
						return false;
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует delegated_credential
				case static_cast <uint8_t> (awh::tls::extension_type_t::DELEGATED_CREDENTIAL): {
					// Создаём объект расширения
					auto extension = make_unique <extension_delegated_credential_t> ();
					// Количество алгоритмов делегированных учётных данных в списке
					size_t count = 0;
					// Читаем количество алгоритмов делегированных учётных данных из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества алгоритмов делегированных учётных данных из буфера
						return false;
					// Если список алгоритмов не пустой
					if(count > 0){
						// Выделяем память под список алгоритмов
						extension->algorithms.resize(count);
						// Читаем список алгоритмов
						if(!read(&extension->algorithms[0], count))
							// Возвращаем ошибку при чтении списка алгоритмов делегированных учётных данных из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует session_ticket
				case static_cast <uint8_t> (awh::tls::extension_type_t::SESSION_TICKET): {
					// Создаём объект расширения
					auto extension = make_unique <extension_session_ticket_t> ();
					// Размер данных session_ticket в байтах
					size_t size = 0;
					// Читаем размер данных session_ticket из буфера
					if(!read(&size, sizeof(size)))
						// Возвращаем ошибку при чтении размера данных session_ticket из буфера
						return false;
					// Если данные session_ticket не пустые
					if(size > 0){
						// Выделяем память под данные session_ticket
						extension->data.resize(size);
						// Читаем данные session_ticket
						if(!read(&extension->data[0], size))
							// Возвращаем ошибку при чтении данных session_ticket из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует pre_shared_key
				case static_cast <uint8_t> (awh::tls::extension_type_t::PRE_SHARED_KEY): {
					// Создаём объект расширения
					auto extension = make_unique <extension_pre_shared_key_t> ();
					// Количество идентификаторов PSK в списке
					size_t count = 0;
					// Читаем количество идентификаторов PSK
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества идентификаторов PSK из буфера
						return false;
					// Если список идентификаторов PSK не пустой
					if(count > 0){
						// Размер данных идентификатора PSK в байтах
						size_t size = 0;
						// Выделяем память под список идентификаторов PSK
						extension->identities.resize(count);
						/**
						 * Перебираем идентификаторы PSK
						 */
						for(auto & identity : extension->identities){
							// Читаем значение ticket_age
							if(!read(&identity.ticketAge, sizeof(identity.ticketAge)))
								// Возвращаем ошибку при чтении значения ticket_age из буфера
								return false;
							// Читаем размер данных идентификатора PSK из буфера
							if(!read(&size, sizeof(size)))
								// Возвращаем ошибку при чтении размера данных идентификатора PSK из буфера
								return false;
							// Если данные идентификатора PSK не пустые
							if(size > 0){
								// Выделяем память под данные идентификатора PSK
								identity.data.resize(size);
								// Читаем данные идентификатора PSK из буфера
								if(!read(&identity.data[0], size))
									// Возвращаем ошибку при чтении данных идентификатора PSK из буфера
									return false;
							}
						}
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует early_data
				case static_cast <uint8_t> (awh::tls::extension_type_t::EARLY_DATA): {
					// Создаём объект расширения
					auto extension = make_unique <extension_early_data_t> ();
					// Читаем максимальный размер ранних данных
					if(!read(&extension->maxSize, sizeof(extension->maxSize)))
						// Возвращаем ошибку при чтении максимального размера ранних данных из буфера
						return false;
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует supported_versions
				case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_VERSIONS): {
					// Создаём объект расширения
					auto extension = make_unique <extension_supported_versions_t> ();
					// Количество поддерживаемых версий TLS в списке
					size_t count = 0;
					// Читаем количество поддерживаемых версий TLS из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества поддерживаемых версий TLS из буфера
						return false;
					// Если список поддерживаемых версий TLS не пустой
					if(count > 0){
						// Выделяем память под список поддерживаемых версий TLS
						extension->versions.resize(count);
						// Читаем список поддерживаемых версий TLS
						if(!read(&extension->versions[0], count))
							// Возвращаем ошибку при чтении списка поддерживаемых версий TLS из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует cookie
				case static_cast <uint8_t> (awh::tls::extension_type_t::COOKIE): {
					// Создаём объект расширения
					auto extension = make_unique <extension_cookie_t> ();
					// Размер данных расширения cookie в байтах
					size_t size = 0;
					// Читаем размер данных расширения cookie из буфера
					if(!read(&size, sizeof(size)))
						// Возвращаем ошибку при чтении размера данных расширения cookie из буфера
						return false;
					// Если данные расширения cookie не пустые
					if(size > 0){
						// Выделяем память под данные расширения cookie
						extension->data.resize(size);
						// Читаем данные расширения cookie
						if(!read(&extension->data[0], size))
							// Возвращаем ошибку при чтении данных расширения cookie из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует psk_key_exchange_modes
				case static_cast <uint8_t> (awh::tls::extension_type_t::PSK_KEY_EXCHANGE_MODES): {
					// Создаём объект расширения
					auto extension = make_unique <extension_psk_key_exchange_t> ();
					// Количество режимов обмена ключами PSK в списке
					size_t count = 0;
					// Читаем количество режимов обмена ключами PSK из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества режимов обмена ключами PSK из буфера
						return false;
					// Если список режимов обмена ключами PSK не пустой
					if(count > 0){
						// Выделяем память под список режимов обмена ключами PSK
						extension->modes.resize(count);
						// Читаем список режимов обмена ключами PSK
						if(!read(&extension->modes[0], count))
							// Возвращаем ошибку при чтении списка режимов обмена ключами PSK из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует certificate_authorities
				case static_cast <uint8_t> (awh::tls::extension_type_t::CERTIFICATE_AUTHORITIES): {
					// Создаём объект расширения
					auto extension = make_unique <extension_certificate_authorities_t> ();
					// Количество авторитетов сертификатов в списке
					size_t count = 0;
					// Читаем количество авторитетов сертификатов
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества авторитетов сертификатов из буфера
						return false;
					// Если список авторитетов сертификатов не пустой
					if(count > 0){
						// Размер авторитета сертификата в байтах
						size_t size = 0;
						// Выделяем память под список авторитетов сертификатов
						extension->authorities.resize(count);
						/**
						 * Перебираем авторитеты сертификатов
						 */
						for(auto & auth : extension->authorities){
							// Читаем размер авторитета сертификата из буфера
							if(!read(&size, sizeof(size)))
								// Возвращаем ошибку при чтении размера авторитета сертификата из буфера
								return false;
							// Если авторитет сертификата не пустой
							if(size > 0){
								// Выделяем память под авторитет сертификата
								auth.resize(size);
								// Читаем авторитет сертификата
								if(!read(&auth[0], size))
									// Возвращаем ошибку при чтении авторитета сертификата из буфера
									return false;
							}
						}
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует signature_algorithms_cert
				case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS_CERT): {
					// Создаём объект расширения
					auto extension = make_unique <extension_signature_algorithms_cert_t> ();
					// Количество алгоритмов подписи сертификатов в списке
					size_t count = 0;
					// Читаем количество алгоритмов подписи сертификатов из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества алгоритмов подписи сертификатов из буфера
						return false;
					// Если список алгоритмов подписи сертификатов не пустой
					if(count > 0){
						// Выделяем память под список алгоритмов подписи сертификатов
						extension->algorithms.resize(count);
						// Читаем список алгоритмов подписи сертификатов
						if(!read(&extension->algorithms[0], count))
							// Возвращаем ошибку при чтении списка алгоритмов подписи сертификатов из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует key_share
				case static_cast <uint8_t> (awh::tls::extension_type_t::KEY_SHARE): {
					// Создаём объект расширения
					auto extension = make_unique <extension_key_share_t> ();
					// Количество ключей для обмена в списке
					size_t count = 0;
					// Читаем количество ключей для обмена из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества ключей для обмена из буфера
						return false;
					// Если список ключей для обмена не пустой
					if(count > 0){
						// Размер данных ключа для обмена в байтах
						size_t size = 0;
						// Выделяем память под список ключей для обмена
						extension->shares.resize(count);
						/**
						 * Перебираем ключи для обмена
						 */
						for(auto & share : extension->shares){
							// Читаем группу ключа для обмена
							if(!read(&share.first, sizeof(share.first)))
								// Возвращаем ошибку при чтении группы ключа для обмена из буфера
								return false;
							// Читаем размер данных ключа для обмена из буфера
							if(!read(&size, sizeof(size)))
								// Возвращаем ошибку при чтении размера данных ключа для обмена из буфера
								return false;
							// Если данные ключа для обмена не пустые
							if(size > 0){
								// Выделяем память под данные ключа для обмена
								share.second.resize(size);
								// Читаем данные ключа для обмена
								if(!read(&share.second[0], size))
									// Возвращаем ошибку при чтении данных ключа для обмена из буфера
									return false;
							}
						}
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует quic_transport_parameters
				case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS): {
					// Создаём объект расширения
					auto extension = make_unique <extension_quic_transport_params_t> ();
					// Количество параметров транспортного уровня QUIC в списке
					size_t count = 0;
					// Читаем количество параметров транспортного уровня QUIC
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества параметров транспортного уровня QUIC из буфера
						return false;
					// Если список параметров не пустой
					if(count > 0){
						// Ключ и значение параметра транспортного уровня QUIC
						uint64_t key = 0, value = 0;
						/**
						 * Перебираем параметры транспортного уровня QUIC
						 */
						for(size_t i = 0; i < count; i++){
							// Обнуляем ключ параметра транспортного уровня QUIC
							key = 0;
							// Читаем ключ параметра транспортного уровня QUIC из буфера
							if(!read(&key, sizeof(key)))
								// Возвращаем ошибку при чтении ключа параметра транспортного уровня QUIC из буфера
								return false;
							// Обнуляем значение параметра транспортного уровня QUIC
							value = 0;
							// Читаем значение параметра транспортного уровня QUIC из буфера
							if(!read(&value, sizeof(value)))
								// Возвращаем ошибку при чтении значения параметра транспортного уровня QUIC из буфера
								return false;
							// Добавляем параметр в список
							extension->params.emplace(key, value);
						}
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует tls_flags
				case static_cast <uint8_t> (awh::tls::extension_type_t::TLS_FLAGS): {
					// Создаём объект расширения
					auto extension = make_unique <extension_tls_flags_t> ();
					// Количество флагов TLS в списке
					size_t count = 0;
					// Читаем количество флагов TLS
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества флагов TLS из буфера
						return false;
					// Если список флагов TLS не пустой
					if(count > 0){
						// Выделяем память под список флагов TLS
						extension->flags.resize(count);
						// Читаем список флагов TLS
						if(!read(&extension->flags[0], count))
							// Возвращаем ошибку при чтении списка флагов TLS из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует next_protocol_negotiation
				case static_cast <uint8_t> (awh::tls::extension_type_t::NEXT_PROTO_NEG): {
					// Создаём объект расширения
					auto extension = make_unique <extension_next_proto_neg_t> ();
					// Количество протоколов NPN в списке
					size_t count = 0;
					// Читаем количество протоколов NPN
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества протоколов NPN из буфера
						return false;
					// Если список протоколов NPN не пустой
					if(count > 0){
						// Размер протокола NPN в байтах
						size_t size = 0;
						// Выделяем память под список протоколов NPN
						extension->protocols.resize(count);
						/**
						 * Перебираем протоколы NPN
						 */
						for(auto & proto : extension->protocols){
							// Читаем размер протокола NPN
							if(!read(&size, sizeof(size)))
								// Возвращаем ошибку при чтении размера протокола NPN из буфера
								return false;
							// Если протокол NPN не пустой
							if(size > 0){
								// Выделяем память под протокол NPN
								proto.resize(size);
								// Читаем протокол NPN
								if(!read(&proto[0], size))
									// Возвращаем ошибку при чтении протокола NPN из буфера
									return false;
							}
						}
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует application_settings_old (устаревшее)
				case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS_OLD): {
					// Создаём объект расширения
					auto extension = make_unique <extension_application_settings_old_t> ();
					// Количество протоколов в списке
					size_t count = 0;
					// Читаем количество протоколов из буфера
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества протоколов из буфера
						return false;
					// Если список протоколов не пустой
					if(count > 0){
						// Размер протокола в байтах
						size_t size = 0;
						// Выделяем память под список протоколов
						extension->protocols.resize(count);
						/**
						 * Перебираем протоколы
						 */
						for(auto & proto : extension->protocols){
							// Читаем размер протокола
							if(!read(&size, sizeof(size)))
								// Возвращаем ошибку при чтении размера протокола из буфера
								return false;
							// Если протокол не пустой
							if(size > 0){
								// Выделяем память под протокол
								proto.resize(size);
								// Читаем протокол
								if(!read(&proto[0], size))
									// Возвращаем ошибку при чтении протокола из буфера
									return false;
							}
						}
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует application_settings
				case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS): {
					// Создаём объект расширения
					auto extension = make_unique <extension_application_settings_t> ();
					// Количество протоколов в списке
					size_t count = 0;
					// Читаем количество протоколов
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества протоколов из буфера
						return false;
					// Если список протоколов не пустой
					if(count > 0){
						// Размер протокола в байтах
						size_t size = 0;
						// Выделяем память под список протоколов
						extension->protocols.resize(count);
						/**
						 * Перебираем протоколы
						 */
						for(auto & proto : extension->protocols){
							// Читаем размер протокола
							if(!read(&size, sizeof(size)))
								// Возвращаем ошибку при чтении размера протокола из буфера
								return false;
							// Если протокол не пустой
							if(size > 0){
								// Выделяем память под протокол
								proto.resize(size);
								// Читаем протокол
								if(!read(&proto[0], size))
									// Возвращаем ошибку при чтении протокола из буфера
									return false;
							}
						}
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует ech_outer_extensions
				case static_cast <uint8_t> (awh::tls::extension_type_t::ECH_OUTER_EXTENSIONS): {
					// Создаём объект расширения
					auto extension = make_unique <extension_ech_outer_extensions_t> ();
					// Количество расширений в списке
					size_t count = 0;
					// Читаем количество расширений в расширении ech_outer_extensions
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества расширений в расширении ech_outer_extensions из буфера
						return false;
					// Если список расширений в расширении ech_outer_extensions не пустой
					if(count > 0){
						// Выделяем память под список расширений
						extension->extensions.resize(count);
						// Читаем список расширений
						if(!read(&extension->extensions[0], count))
							// Возвращаем ошибку при чтении списка расширений в расширении ech_outer_extensions из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует encrypted_client_hello
				case static_cast <uint8_t> (awh::tls::extension_type_t::ENCRYPTED_CLIENT_HELLO): {
					// Создаём объект расширения
					auto extension = make_unique <extension_encryption_client_hello_t> ();
					// Размер данных расширения encrypted_client_hello в байтах
					size_t size = 0;
					// Читаем размер данных расширения encrypted_client_hello
					if(!read(&size, sizeof(size)))
						// Возвращаем ошибку при чтении размера данных расширения encrypted_client_hello из буфера
						return false;
					// Если данные расширения encrypted_client_hello не пустые
					if(size > 0){
						// Выделяем память под данные расширения encrypted_client_hello
						extension->data.resize(size);
						// Читаем данные расширения encrypted_client_hello
						if(!read(&extension->data[0], size))
							// Возвращаем ошибку при чтении данных расширения encrypted_client_hello из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует renegotiation_info
				case static_cast <uint8_t> (awh::tls::extension_type_t::RENEGOTIATION_INFO): {
					// Создаём объект расширения
					auto extension = make_unique <extension_renegotiation_info_t> ();
					// Размер данных расширения renegotiation_info в байтах
					size_t size = 0;
					// Читаем размер данных расширения renegotiation_info
					if(!read(&size, sizeof(size)))
						// Возвращаем ошибку при чтении размера данных расширения renegotiation_info из буфера
						return false;
					// Если данные расширения renegotiation_info не пустые
					if(size > 0){
						// Выделяем память под данные расширения renegotiation_info
						extension->data.resize(size);
						// Читаем данные расширения renegotiation_info
						if(!read(&extension->data[0], size))
							// Возвращаем ошибку при чтении данных расширения renegotiation_info из буфера
							return false;
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Если тип расширения соответствует quic_transport_parameters_legacy
				case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS_LEGACY): {
					// Создаём объект расширения
					auto extension = make_unique <extension_quic_transport_params_legacy_t> ();
					// Количество параметров транспортного уровня QUIC в списке
					size_t count = 0;
					// Читаем количество параметров транспортного уровня QUIC (устаревшее расширение)
					if(!read(&count, sizeof(count)))
						// Возвращаем ошибку при чтении количества параметров транспортного уровня QUIC (устаревшее расширение) из буфера
						return false;
					// Если список параметров не пустой
					if(count > 0){
						// Ключ и значение параметра транспортного уровня QUIC (устаревшее расширение)
						uint64_t key = 0, value = 0;
						/**
						 * Перебираем параметры транспортного уровня QUIC (устаревшее расширение)
						 */
						for(size_t i = 0; i < count; i++){
							// Обнуляем ключ параметра транспортного уровня QUIC (устаревшее расширение)
							key = 0;
							// Читаем ключ параметра транспортного уровня QUIC (устаревшее расширение) из буфера
							if(!read(&key, sizeof(key)))
								// Возвращаем ошибку при чтении ключа параметра транспортного уровня QUIC (устаревшее расширение) из буфера
								return false;
							// Обнуляем значение параметра транспортного уровня QUIC (устаревшее расширение)
							value = 0;
							// Читаем значение параметра транспортного уровня QUIC (устаревшее расширение) из буфера
							if(!read(&value, sizeof(value)))
								// Возвращаем ошибку при чтении значения параметра транспортного уровня QUIC (устаревшее расширение) из буфера
								return false;
							// Добавляем параметр в список
							extension->params.emplace(key, value);
						}
					}
					// Добавляем расширение в список
					browser.extensions.push_back(::move(extension));
				} break;
				// Неизвестный тип расширения — нарушение формата данных
				default: return false;
			}
		}
		// Возвращаем результат
		return true;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод формирования бинарного дампа цифрового отпечатка браузера
 *
 * @param browser объект с распарсенными данными ClientHello
 * @param output  буфер для записи бинарного дампа цифрового отпечатка
 * @return        результат формирования бинарного дампа цифрового отпечатка
 *
 */
bool awh::tls::Fingerprint::dump(const browser_t & browser, vector <uint8_t> & output) const noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем очистку выходного буфера от предыдущих данных
		output.clear();
		// Добавляем флаг, указывающий на использование GREASE в результирующий буфер
		output.insert(output.end(), reinterpret_cast <const uint8_t *> (&browser.grease), reinterpret_cast <const uint8_t *> (&browser.grease) + sizeof(browser.grease));
		// Добавляем запись  метаданных TLS рукопожатия в результирующий буфер
		output.insert(output.end(), reinterpret_cast <const uint8_t *> (&browser.record), reinterpret_cast <const uint8_t *> (&browser.record) + sizeof(browser.record));
		// Добавляем объект TLS рукопожатия в результирующий буфер
		output.insert(output.end(), reinterpret_cast <const uint8_t *> (&browser.handshake), reinterpret_cast <const uint8_t *> (&browser.handshake) + sizeof(browser.handshake));
		// Добавляем объект ClientHello TLS рукопожатия в результирующий буфер
		output.insert(output.end(), reinterpret_cast <const uint8_t *> (&browser.clientHello), reinterpret_cast <const uint8_t *> (&browser.clientHello) + sizeof(browser.clientHello));
		// Получаем размер куков DTLS
		size_t size = browser.cookie.size();
		// Добавляем размер куков DTLS в результирующий буфер
		output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
		// Если куки DTLS присутствуют
		if(size > 0)
			// Добавляем куки DTLS в результирующий буфер
			output.insert(output.end(), browser.cookie.begin(), browser.cookie.end());
		// Получаем размер идентификатора сессии TLS
		size = browser.session.size();
		// Добавляем размер идентификатора сессии TLS в результирующий буфер
		output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
		// Если идентификатор сессии TLS присутствует
		if(size > 0)
			// Добавляем идентификатор сессии TLS в результирующий буфер
			output.insert(output.end(), browser.session.begin(), browser.session.end());
		// Получаем размер списка шифров TLS
		size = browser.ciphers.size();
		// Добавляем размер списка шифров TLS в результирующий буфер
		output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
		// Если список шифров TLS не пустой
		if(size > 0)
			// Добавляем список шифров TLS в результирующий буфер
			output.insert(output.end(), reinterpret_cast <const uint8_t *> (&browser.ciphers[0]), reinterpret_cast <const uint8_t *> (&browser.ciphers[0]) + size);
		// Получаем размер списка методов компрессии сертификата TLS
		size = browser.compressors.size();
		// Добавляем размер списка методов компрессии сертификата TLS в результирующий буфер
		output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
		// Если список методов компрессии сертификата TLS не пустой
		if(size > 0)
			// Добавляем список методов компрессии сертификата TLS в результирующий буфер
			output.insert(output.end(), reinterpret_cast <const uint8_t *> (&browser.compressors[0]), reinterpret_cast <const uint8_t *> (&browser.compressors[0]) + size);
		/**
		 * Выполняем перебор всех расширений TLS в цифровом отпечатке браузера
		 */
		for(const auto & ext : browser.extensions){
			// Добавляем тип расширения в результирующий буфер
			output.insert(output.end(), reinterpret_cast <const uint8_t *> (&ext->type), reinterpret_cast <const uint8_t *> (&ext->type) + sizeof(ext->type));
			/**
			 * Строим payload в зависимости от типа расширения
			 */
			switch(static_cast <uint8_t> (ext->type)){
				// Если тип расширения соответствует server_name
				case static_cast <uint8_t> (awh::tls::extension_type_t::SERVER_NAME): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_server_name_t *> (ext.get());
					// Получаем размер списка имён серверов в расширении server_name
					size = extension->names.size();
					// Добавляем размер списка имён серверов в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список имён серверов не пустой
					if(size > 0){
						/**
						 * Перебираем имена серверов в расширении server_name
						 */
						for(const auto & name : extension->names){
							// Получаем размер имени сервера
							size = name.size();
							// Добавляем размер имени сервера в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
							// Если имя сервера не пустое
							if(size > 0)
								// Добавляем имя сервера в результирующий буфер
								output.insert(output.end(), name.begin(), name.end());
						}
					}
				} break;
				// Если тип расширения соответствует max_fragment_length
				case static_cast <uint8_t> (awh::tls::extension_type_t::MAX_FRAGMENT_LENGTH): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_max_fragment_length_t *> (ext.get());
					// Добавляем значение max_fragment_length в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->length), reinterpret_cast <const uint8_t *> (&extension->length) + sizeof(extension->length));
				} break;
				// Если тип расширения соответствует status_request
				case static_cast <uint8_t> (awh::tls::extension_type_t::STATUS_REQUEST): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_status_request_t *> (ext.get());
					// Получаем размер типа статуса сертификата
					size = extension->certificateStatusType.size();
					// Добавляем размер типа статуса сертификата в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если тип статуса сертификата не пустой
					if(size > 0)
						// Добавляем тип статуса сертификата в результирующий буфер
						output.insert(output.end(), extension->certificateStatusType.begin(), extension->certificateStatusType.end());
					// Устанавливаем длину списка идентификаторов ответчиков
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->responderIdListLength), reinterpret_cast <const uint8_t *> (&extension->responderIdListLength) + sizeof(extension->responderIdListLength));
					// Устанавливаем длину расширений запроса статуса сертификата
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->requestExtensionsLength), reinterpret_cast <const uint8_t *> (&extension->requestExtensionsLength) + sizeof(extension->requestExtensionsLength));
				} break;
				// Если тип расширения соответствует supported_groups
				case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_GROUPS): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_supported_groups_t *> (ext.get());
					// Получаем размер списка поддерживаемых групп
					size = extension->supportedGroups.size();
					// Добавляем размер списка поддерживаемых групп в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список поддерживаемых групп не пустой
					if(size > 0)
						// Добавляем список поддерживаемых групп в результирующий буфер
						output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->supportedGroups[0]), reinterpret_cast <const uint8_t *> (&extension->supportedGroups[0]) + size);
				} break;
				// Если тип расширения соответствует ec_point_formats
				case static_cast <uint8_t> (awh::tls::extension_type_t::EC_POINT_FORMATS): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_ec_point_t *> (ext.get());
					// Получаем размер списка форматов точек эллиптической кривой
					size = extension->formats.size();
					// Добавляем размер списка форматов точек эллиптической кривой в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список форматов точек эллиптической кривой не пустой
					if(size > 0)
						// Добавляем список форматов точек эллиптической кривой в результирующий буфер
						output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->formats[0]), reinterpret_cast <const uint8_t *> (&extension->formats[0]) + size);
				} break;
				// Если тип расширения соответствует signature_algorithms
				case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_signature_t *> (ext.get());
					// Получаем размер списка алгоритмов подписи
					size = extension->algorithms.size();
					// Добавляем размер списка алгоритмов подписи в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список алгоритмов подписи не пустой
					if(size > 0)
						// Добавляем список алгоритмов подписи в результирующий буфер
						output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->algorithms[0]), reinterpret_cast <const uint8_t *> (&extension->algorithms[0]) + size);
				} break;
				// Если тип расширения соответствует use_srtp
				case static_cast <uint8_t> (awh::tls::extension_type_t::USE_SRTP): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_use_srtp_t *> (ext.get());
					// Устанавливаем значение mki_length в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->mkiLength), reinterpret_cast <const uint8_t *> (&extension->mkiLength) + sizeof(extension->mkiLength));
					// Получаем размер профилей SRTP
					size = extension->profiles.size();
					// Добавляем размер профилей SRTP в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список профилей SRTP не пустой
					if(size > 0)
						// Добавляем список профилей SRTP в результирующий буфер
						output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->profiles[0]), reinterpret_cast <const uint8_t *> (&extension->profiles[0]) + size);
				} break;
				// Если тип расширения соответствует heartbeat
				case static_cast <uint8_t> (awh::tls::extension_type_t::HEARTBEAT): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_heartbeat_t *> (ext.get());
					// Добавляем значение режима heartbeat в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->mode), reinterpret_cast <const uint8_t *> (&extension->mode) + sizeof(extension->mode));
				} break;
				// Если тип расширения соответствует alpn
				case static_cast <uint8_t> (awh::tls::extension_type_t::ALPN): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_alpn_t *> (ext.get());
					// Получаем размер списка протоколов ALPN
					size = extension->protocols.size();
					// Добавляем размер списка протоколов ALPN в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список протоколов ALPN не пустой
					if(size > 0){
						/**
						 * Перебираем протоколы в расширении ALPN
						 */
						for(const auto & protocol : extension->protocols){
							// Получаем размер протокола ALPN
							size = protocol.size();
							// Добавляем размер протокола ALPN в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
							// Если протокол ALPN не пустой
							if(size > 0)
								// Добавляем протокол ALPN в результирующий буфер
								output.insert(output.end(), protocol.begin(), protocol.end());
						}
					}
				} break;
				// Если тип расширения соответствует padding
				case static_cast <uint8_t> (awh::tls::extension_type_t::PADDING): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_padding_t *> (ext.get());
					// Добавляем размер паддинга в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->size), reinterpret_cast <const uint8_t *> (&extension->size) + sizeof(extension->size));
				} break;
				// Если тип расширения соответствует extended_master_secret
				case static_cast <uint8_t> (awh::tls::extension_type_t::EXTENDED_MASTER_SECRET): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_extended_master_secret_t *> (ext.get());
					// Получаем размер данных extended_master_secret
					size = extension->masterSecretData.size();
					// Добавляем размер данных extended_master_secret в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если данные extended_master_secret не пустые
					if(size > 0)
						// Добавляем данные extended_master_secret в результирующий буфер
						output.insert(output.end(), extension->masterSecretData.begin(), extension->masterSecretData.end());
					// Получаем размер данных extended_master_secret (устаревшее расширение)
					size = extension->extendedMasterSecretData.size();
					// Добавляем размер данных extended_master_secret в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если данные extended_master_secret не пустые
					if(size > 0)
						// Добавляем данные extended_master_secret в результирующий буфер
						output.insert(output.end(), extension->extendedMasterSecretData.begin(), extension->extendedMasterSecretData.end());
				} break;
				// Если тип расширения соответствует compress_certificate
				case static_cast <uint8_t> (awh::tls::extension_type_t::COMPRESS_CERTIFICATE): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_compress_certificate_t *> (ext.get());
					// Получаем размер списка алгоритмов сжатия сертификата
					size = extension->algorithms.size();
					// Добавляем размер списка алгоритмов сжатия сертификата в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список алгоритмов сжатия сертификата не пустой
					if(size > 0)
						// Добавляем список алгоритмов сжатия сертификата в результирующий буфер
						output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->algorithms[0]), reinterpret_cast <const uint8_t *> (&extension->algorithms[0]) + size);
				} break;
				// Если тип расширения соответствует record_size_limit
				case static_cast <uint8_t> (awh::tls::extension_type_t::RECORD_SIZE_LIMIT): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_record_size_limit_t *> (ext.get());
					// Добавляем значение record_size_limit в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->data), reinterpret_cast <const uint8_t *> (&extension->data) + sizeof(extension->data));
				} break;
				// Если тип расширения соответствует delegated_credential
				case static_cast <uint8_t> (awh::tls::extension_type_t::DELEGATED_CREDENTIAL): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_delegated_credential_t *> (ext.get());
					// Получаем размер списка алгоритмов делегированных учётных данных
					size = extension->algorithms.size();
					// Добавляем размер списка алгоритмов делегированных учётных данных в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список алгоритмов делегированных учётных данных не пустой
					if(size > 0)
						// Добавляем список алгоритмов делегированных учётных данных в результирующий буфер
						output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->algorithms[0]), reinterpret_cast <const uint8_t *> (&extension->algorithms[0]) + size);
				} break;
				// Если тип расширения соответствует session_ticket
				case static_cast <uint8_t> (awh::tls::extension_type_t::SESSION_TICKET): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_session_ticket_t *> (ext.get());
					// Получаем размер данных расширения session_ticket
					size = extension->data.size();
					// Добавляем размер данных расширения session_ticket в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если данные расширения session_ticket не пустые
					if(size > 0)
						// Добавляем данные расширения session_ticket в результирующий буфер
						output.insert(output.end(), extension->data.begin(), extension->data.end());
				} break;
				// Если тип расширения соответствует pre_shared_key
				case static_cast <uint8_t> (awh::tls::extension_type_t::PRE_SHARED_KEY): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_pre_shared_key_t *> (ext.get());
					// Получаем размер списка идентификаторов PSK
					size = extension->identities.size();
					// Добавляем размер списка идентификаторов PSK в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список идентификаторов PSK не пустой
					if(size > 0){
						/**
						 * Перебираем идентификаторы PSK в расширении pre_shared_key
						 */
						for(const auto & identity : extension->identities){
							// Добавляем значение ticket_age в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&identity.ticketAge), reinterpret_cast <const uint8_t *> (&identity.ticketAge) + sizeof(identity.ticketAge));
							// Получаем размер идентификатора PSK
							size = identity.data.size();
							// Добавляем размер идентификатора PSK в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
							// Если идентификатор PSK не пустой
							if(size > 0)
								// Добавляем идентификатор PSK в результирующий буфер
								output.insert(output.end(), identity.data.begin(), identity.data.end());
						}
					}
				} break;
				// Если тип расширения соответствует early_data
				case static_cast <uint8_t> (awh::tls::extension_type_t::EARLY_DATA): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_early_data_t *> (ext.get());
					// Добавляем значение max_early_data_size в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->maxSize), reinterpret_cast <const uint8_t *> (&extension->maxSize) + sizeof(extension->maxSize));
				} break;
				// Если тип расширения соответствует supported_versions
				case static_cast <uint8_t> (awh::tls::extension_type_t::SUPPORTED_VERSIONS): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_supported_versions_t *> (ext.get());
					// Получаем размер списка поддерживаемых версий TLS
					size = extension->versions.size();
					// Добавляем размер списка поддерживаемых версий TLS в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список поддерживаемых версий TLS не пустой
					if(size > 0)
						// Добавляем список поддерживаемых версий TLS в результирующий буфер
						output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->versions[0]), reinterpret_cast <const uint8_t *> (&extension->versions[0]) + size);
				} break;
				// Если тип расширения соответствует cookie
				case static_cast <uint8_t> (awh::tls::extension_type_t::COOKIE): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_cookie_t *> (ext.get());
					// Получаем размер данных расширения cookie
					size = extension->data.size();
					// Добавляем размер данных расширения cookie в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если данные расширения cookie не пустые
					if(size > 0)
						// Добавляем данные расширения cookie в результирующий буфер
						output.insert(output.end(), extension->data.begin(), extension->data.end());
				} break;
				// Если тип расширения соответствует psk_key_exchange_modes
				case static_cast <uint8_t> (awh::tls::extension_type_t::PSK_KEY_EXCHANGE_MODES): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_psk_key_exchange_t *> (ext.get());
					// Получаем размер списка режимов обмена ключами PSK
					size = extension->modes.size();
					// Добавляем размер списка режимов обмена ключами PSK в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список режимов обмена ключами PSK не пустой
					if(size > 0)
						// Добавляем список режимов обмена ключами PSK в результирующий буфер
						output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->modes[0]), reinterpret_cast <const uint8_t *> (&extension->modes[0]) + size);
				} break;
				// Если тип расширения соответствует certificate_authorities
				case static_cast <uint8_t> (awh::tls::extension_type_t::CERTIFICATE_AUTHORITIES): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_certificate_authorities_t *> (ext.get());
					// Получаем размер списка авторитетов сертификатов
					size = extension->authorities.size();
					// Добавляем размер списка авторитетов сертификатов в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список авторитетов сертификатов не пустой
					if(size > 0){
						/**
						 * Перебираем авторитеты сертификатов в расширении certificate_authorities
						 */
						for(const auto & authority : extension->authorities){
							// Получаем размер авторитета сертификата
							size = authority.size();
							// Добавляем размер авторитета сертификата в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
							// Если авторитет сертификата не пустой
							if(size > 0)
								// Добавляем авторитет сертификата в результирующий буфер
								output.insert(output.end(), authority.begin(), authority.end());
						}
					}
				} break;
				// Если тип расширения соответствует signature_algorithms_cert
				case static_cast <uint8_t> (awh::tls::extension_type_t::SIGNATURE_ALGORITHMS_CERT): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_signature_algorithms_cert_t *> (ext.get());
					// Получаем размер списка алгоритмов подписи сертификатов
					size = extension->algorithms.size();
					// Добавляем размер списка алгоритмов подписи сертификатов в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список алгоритмов подписи сертификатов не пустой
					if(size > 0)
						// Добавляем список алгоритмов подписи сертификатов в результирующий буфер
						output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->algorithms[0]), reinterpret_cast <const uint8_t *> (&extension->algorithms[0]) + size);
				} break;
				// Если тип расширения соответствует key_share
				case static_cast <uint8_t> (awh::tls::extension_type_t::KEY_SHARE): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_key_share_t *> (ext.get());
					// Получаем размер списка ключей для обмена
					size = extension->shares.size();
					// Добавляем размер списка ключей для обмена в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список ключей для обмена не пустой
					if(size > 0){
						/**
						 * Перебираем ключи для обмена в расширении key_share
						 */
						for(const auto & share : extension->shares){
							// Добавляем значение group в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&share.first), reinterpret_cast <const uint8_t *> (&share.first) + sizeof(share.first));
							// Получаем размер данных ключа для обмена
							size = share.second.size();
							// Добавляем размер данных ключа для обмена в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
							// Если данные ключа для обмена не пустые
							if(size > 0)
								// Добавляем данные ключа для обмена в результирующий буфер
								output.insert(output.end(), share.second.begin(), share.second.end());
						}
					}
				} break;
				// Если тип расширения соответствует quic_transport_parameters
				case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_quic_transport_params_t *> (ext.get());
					// Получаем размер параметров транспортного уровня QUIC
					size = extension->params.size();
					// Добавляем размер параметров транспортного уровня QUIC в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если параметры транспортного уровня QUIC не пустые
					if(size > 0){
						/**
						 * Перебираем параметры транспортного уровня QUIC в расширении quic_transport_parameters
						 */
						for(const auto & param : extension->params){
							// Добавляем значение идентификатора параметра транспортного уровня QUIC в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&param.first), reinterpret_cast <const uint8_t *> (&param.first) + sizeof(param.first));
							// Добавляем значение параметра транспортного уровня QUIC в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&param.second), reinterpret_cast <const uint8_t *> (&param.second) + sizeof(param.second));
						}
					}
				} break;
				// Если тип расширения соответствует tls_flags
				case static_cast <uint8_t> (awh::tls::extension_type_t::TLS_FLAGS): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_tls_flags_t *> (ext.get());
					// Получаем размер списка флагов TLS
					size = extension->flags.size();
					// Добавляем размер списка флагов TLS в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список флагов TLS не пустой
					if(size > 0)
						// Добавляем список флагов TLS в результирующий буфер
						output.insert(output.end(), extension->flags.begin(), extension->flags.end());
				} break;
				// Если тип расширения соответствует next_protocol_negotiation
				case static_cast <uint8_t> (awh::tls::extension_type_t::NEXT_PROTO_NEG): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_next_proto_neg_t *> (ext.get());
					// Получаем размер данных расширения next_protocol_negotiation
					size = extension->protocols.size();
					// Добавляем размер данных расширения next_protocol_negotiation в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если данные расширения next_protocol_negotiation не пустые
					if(size > 0){
						/**
						 * Перебираем протоколы в расширении next_protocol_negotiation
						 */
						for(const auto & protocol : extension->protocols){
							// Получаем размер протокола в расширении next_protocol_negotiation
							size = protocol.size();
							// Добавляем размер протокола в расширении next_protocol_negotiation в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
							// Если протокол в расширении next_protocol_negotiation не пустой
							if(size > 0)
								// Добавляем протокол в расширении next_protocol_negotiation в результирующий буфер
								output.insert(output.end(), protocol.begin(), protocol.end());
						}
					}
				} break;
				// Если тип расширения соответствует application_settings_old (устаревшее)
				case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS_OLD): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_application_settings_old_t *> (ext.get());
					// Получаем количество протоколов в расширении application_settings_old
					size = extension->protocols.size();
					// Добавляем количество протоколов в расширении application_settings_old в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если данные расширения application_settings_old не пустые
					if(size > 0){
						/**
						 * Перебираем протоколы в расширении application_settings_old
						 */
						for(const auto & protocol : extension->protocols){
							// Получаем размер протокола в расширении application_settings_old
							size = protocol.size();
							// Добавляем размер протокола в расширении application_settings_old в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
							// Если протокол в расширении application_settings_old не пустой
							if(size > 0)
								// Добавляем протокол в расширении application_settings_old в результирующий буфер
								output.insert(output.end(), protocol.begin(), protocol.end());
						}
					}
				} break;
				// Если тип расширения соответствует application_settings
				case static_cast <uint8_t> (awh::tls::extension_type_t::APPLICATION_SETTINGS): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_application_settings_t *> (ext.get());
					// Получаем количество протоколов в расширении application_settings
					size = extension->protocols.size();
					// Добавляем количество протоколов в расширении application_settings в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если данные расширения application_settings не пустые
					if(size > 0){
						/**
						 * Перебираем протоколы в расширении application_settings
						 */
						for(const auto & protocol : extension->protocols){
							// Получаем размер протокола в расширении application_settings
							size = protocol.size();
							// Добавляем размер протокола в расширении application_settings в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
							// Если протокол в расширении application_settings не пустой
							if(size > 0)
								// Добавляем протокол в расширении application_settings в результирующий буфер
								output.insert(output.end(), protocol.begin(), protocol.end());
						}
					}
				} break;
				// Если тип расширения соответствует ech_outer_extensions
				case static_cast <uint8_t> (awh::tls::extension_type_t::ECH_OUTER_EXTENSIONS): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_ech_outer_extensions_t *> (ext.get());
					// Получаем количество расширений в расширении ech_outer_extensions
					size = extension->extensions.size();
					// Добавляем количество расширений в расширении ech_outer_extensions в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если список расширений в расширении ech_outer_extensions не пустой
					if(size > 0)
						// Добавляем список расширений в результирующий буфер
						output.insert(output.end(), reinterpret_cast <const uint8_t *> (&extension->extensions[0]), reinterpret_cast <const uint8_t *> (&extension->extensions[0]) + size);
				} break;
				// Если тип расширения соответствует encrypted_client_hello
				case static_cast <uint8_t> (awh::tls::extension_type_t::ENCRYPTED_CLIENT_HELLO): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_encryption_client_hello_t *> (ext.get());
					// Получаем размер данных расширения encrypted_client_hello
					size = extension->data.size();
					// Добавляем размер данных расширения encrypted_client_hello в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если данные расширения encrypted_client_hello не пустые
					if(size > 0)
						// Добавляем данные расширения encrypted_client_hello в результирующий буфер
						output.insert(output.end(), extension->data.begin(), extension->data.end());
				} break;
				// Если тип расширения соответствует renegotiation_info
				case static_cast <uint8_t> (awh::tls::extension_type_t::RENEGOTIATION_INFO): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_renegotiation_info_t *> (ext.get());
					// Получаем размер данных расширения renegotiation_info
					size = extension->data.size();
					// Добавляем размер данных расширения renegotiation_info в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если данные расширения renegotiation_info не пустые
					if(size > 0)
						// Добавляем данные расширения renegotiation_info в результирующий буфер
						output.insert(output.end(), extension->data.begin(), extension->data.end());
				} break;
				// Если тип расширения соответствует quic_transport_parameters_legacy
				case static_cast <uint8_t> (awh::tls::extension_type_t::QUIC_TRANSPORT_PARAMETERS_LEGACY): {
					// Получаем объект расширения
					const auto * extension = static_cast <const extension_quic_transport_params_legacy_t *> (ext.get());
					// Получаем размер параметров транспортного уровня QUIC (устаревшее расширение)
					size = extension->params.size();
					// Добавляем размер параметров транспортного уровня QUIC (устаревшее расширение) в результирующий буфер
					output.insert(output.end(), reinterpret_cast <const uint8_t *> (&size), reinterpret_cast <const uint8_t *> (&size) + sizeof(size));
					// Если параметры транспортного уровня QUIC (устаревшее расширение) не пустые
					if(size > 0){
						/**
						 * Перебираем параметры транспортного уровня QUIC (устаревшее расширение) в расширении quic_transport_parameters_legacy
						 */
						for(const auto & param : extension->params){
							// Добавляем значение идентификатора параметра транспортного уровня QUIC (устаревшее расширение) в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&param.first), reinterpret_cast <const uint8_t *> (&param.first) + sizeof(param.first));
							// Добавляем значение параметра транспортного уровня QUIC (устаревшее расширение) в результирующий буфер
							output.insert(output.end(), reinterpret_cast <const uint8_t *> (&param.second), reinterpret_cast <const uint8_t *> (&param.second) + sizeof(param.second));
						}
					}
				} break;
			}
		}
		// Возвращаем результат
		return !output.empty();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод обмена заголовками
 *
 * @param fgp объект Fingerprint для обмена данными
 *
 */
void awh::tls::Fingerprint::swap(Fingerprint & fgp) noexcept {
	// Блокируем оба хранилища отпечатков для записи
	const local::fgp_exclusive_lock_t lock(this->_mtx, local::fgp_exclusive);
	const local::fgp_exclusive_lock_t lockFgp(fgp._mtx, local::fgp_exclusive);
	// Обмениваемся данными между объектами
	this->_browsers.swap(fgp._browsers);
}
/**
 * @brief Метод получения конечного итератора
 *
 * @return конечный итератор
 *
 */
awh::tls::Fingerprint::iterator_t awh::tls::Fingerprint::end() noexcept {
	// Блокируем хранилище отпечатков для чтения
	const local::fgp_shared_lock_t lock(this->_mtx, local::fgp_shared);
	// Возвращаем результат
	return iterator_t(this->_browsers.end(), this->_fmk, this->_log);
}
/**
 * @brief Метод получение начального итератора
 *
 * @return начальный итератор
 *
 */
awh::tls::Fingerprint::iterator_t awh::tls::Fingerprint::begin() noexcept {
	// Блокируем хранилище отпечатков для чтения
	const local::fgp_shared_lock_t lock(this->_mtx, local::fgp_shared);
	// Возвращаем результат
	return iterator_t(this->_browsers.begin(), this->_fmk, this->_log);
}
/**
 * @brief Метод поиска указанного заголовка
 *
 * @param id идентификатор заголовка для поиска
 * @return   итератор указанного заголовка
 *
 */
awh::tls::Fingerprint::iterator_t awh::tls::Fingerprint::find(const id_t id) noexcept {
	// Если идентификатор заголовка передан
	if(id != 0){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Блокируем хранилище отпечатков для чтения
			const local::fgp_shared_lock_t lock(this->_mtx, local::fgp_shared);
			// Извлекаем текущий итератор
			return iterator_t(this->_browsers.find(id), this->_fmk, this->_log);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(id), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Возвращаем результат
	return iterator_t(this->_browsers.end(), this->_fmk, this->_log);
}
/**
 * @brief Оператор извлечения цифрового отпечатка браузера
 *
 * @param id идентификатор цифрового отпечатка
 * @return   цифровой отпечаток браузера
 *
 */
const awh::tls::Fingerprint::browser_t & awh::tls::Fingerprint::operator[](const id_t id) const noexcept {
	// Возвращаем результат
	return this->get(id);
}
/**
 * @brief Проверка, пусто ли хранилище цифровых отпечатков браузеров
 *
 * @return результат проверки
 *
 */
awh::tls::Fingerprint::operator bool() const noexcept {
	// Блокируем хранилище отпечатков для чтения
	const local::fgp_shared_lock_t lock(this->_mtx, local::fgp_shared);
	// Возвращаем результат выполенной проверки
	return !this->_browsers.empty();
}
/**
 * @brief Получения количества цифровых отпечатков браузеров, хранящихся в хранилище
 *
 * @return количество цифровых отпечатков браузеров
 *
 */
awh::tls::Fingerprint::operator size_t() const noexcept {
	// Блокируем хранилище отпечатков для чтения
	const local::fgp_shared_lock_t lock(this->_mtx, local::fgp_shared);
	// Возвращаем результат количество цифровых отпечатков браузеров, хранящихся в хранилище
	return this->_browsers.size();
}
/**
 * @brief Получения бинарных данных дампа всех цифровых отпечатков браузеров
 *
 * @return бинарные данные буфера дампа всех цифровых отпечатков браузеров
 *
 */
awh::tls::Fingerprint::operator vector <uint8_t> () const noexcept {
	// Возвращаем результат бинарных данных дампа всех цифровых отпечатков браузеров
	return this->dump();
}
/**
 * @brief Оператор сравнения двух контейнеров отпечатков браузеров
 *
 * @param fgp отпечатки браузеров для сравнения
 * @return    результат сравнения
 *
 */
bool awh::tls::Fingerprint::operator == (const Fingerprint & fgp) const noexcept {
	// Блокируем оба хранилища отпечатков для чтения
	const local::fgp_shared_lock_t lock1(fgp._mtx, local::fgp_shared);
	const local::fgp_shared_lock_t lock2(this->_mtx, local::fgp_shared);
	/**
	 * Перебираем отпечатки браузеров в текущем контейнере отпечатков браузеров
	 */
	for(auto & [id, browser] : this->_browsers){
		// Ищем отпечаток браузера с данным идентификатором в другом контейнере отпечатков браузеров
		auto i = fgp._browsers.find(id);
		// Если отпечаток браузера с данным идентификатором не найден в другом контейнере отпечатков браузеров
		if(i == fgp._browsers.end())
			// Возвращаем результат сравнения
			return false;
		// Если отпечаток браузера не совпадает с отпечатком браузера с данным идентификатором в другом контейнере отпечатков браузеров
		else if(!(browser == i->second))
			// Возвращаем результат сравнения
			return false;
	}
	// Возвращаем результат сравнения двух отпечатков браузеров
	return true;
}
/**
 * @brief Оператор установки дампа цифрового отпечатка браузера
 *
 * @param buffer бинарный буфер для загрузки данных цифровых отпечатков
 * @return       текущий контейнер отпечатков браузеров
 *
 */
awh::tls::Fingerprint & awh::tls::Fingerprint::operator = (const vector <uint8_t> & buffer) noexcept {
	// Загружаем данные цифровых отпечатков из бинарного буфера
	this->dump(buffer);
	// Возвращаем текущий контейнер отпечатков браузеров
	return (* this);
}
/**
 * @brief Оператор перемещения
 *
 * @param fgp объект Fingerprint для перемещения
 * @return    текущий контейнер отпечатков браузеров
 *
 */
awh::tls::Fingerprint & awh::tls::Fingerprint::operator = (Fingerprint && fgp) noexcept {
	// Если объект для перемещения не является текущим объектом
	if(this != &fgp){
		// Блокируем оба хранилища отпечатков для записи
		const local::fgp_exclusive_lock_t lock1(fgp._mtx, local::fgp_exclusive);
		const local::fgp_exclusive_lock_t lock2(this->_mtx, local::fgp_exclusive);
		// Перемещаем данные из объекта для перемещения в текущий объект
		this->_browsers = ::move(fgp._browsers);
	}
	// Возвращаем текущий контейнер отпечатков браузеров
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param fgp объект Fingerprint для копирования
 * @return    текущий контейнер отпечатков браузеров
 *
 */
awh::tls::Fingerprint & awh::tls::Fingerprint::operator = (const Fingerprint & fgp) noexcept {
	// Если объект для копирования не является текущим объектом
	if(this != &fgp){
		// Блокируем оба хранилища отпечатков
		const local::fgp_shared_lock_t lock1(fgp._mtx, local::fgp_shared);
		const local::fgp_exclusive_lock_t lock2(this->_mtx, local::fgp_exclusive);
		// Копируем данные из объекта для копирования в текущий объект
		this->_browsers = fgp._browsers;
	}
	// Возвращаем текущий контейнер отпечатков браузеров
	return (* this);
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::tls::Fingerprint::Fingerprint(const fmk_t * fmk, const log_t * log) noexcept : _fmk(fmk), _log(log) {
	// Мьютекс выключен по умолчанию; включается через threadSafety(true)
	this->_mtx.enabled = false;
}
/**
 * @brief Деструктор
 *
 */
awh::tls::Fingerprint::~Fingerprint() noexcept {}
