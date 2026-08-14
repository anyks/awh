/**
 * @file crypto.hpp
 * @date 2026-07-21
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
 * @brief Заголовочный файл криптографического слоя QUIC (RFC 9001) — вывод начальных секретов,
 *        ключей и заголовочной защиты,
 *        шифрование и расшифровка пакетов на AEAD-примитивах BoringSSL без хранения состояния соединения
 *
 * \~english
 * @brief Header file of the QUIC cryptographic layer (RFC 9001) — the derivation of the initial secrets,
 *        of the keys and of the header protection,
 *        the encryption and the decryption of the packets on the AEAD primitives of BoringSSL without storing the state of a connection
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_PROTO_QUIC_CRYPTO__
#define __AWH_PROTO_QUIC_CRYPTO__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <memory>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "quic.hpp"
#include "../../sys/global.hpp"

/**
 * Предварительные объявления типов BoringSSL (реализация в crypto.cpp)
 */
struct aes_key_st;
struct evp_aead_ctx_st;

/**
 * \~russian
 * @brief основное пространство имён
 *
 *
 * \~english
 * @brief main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён транспортного протокола QUIC
	 *
	 *
	 * \~english
	 * @brief QUIC transport protocol namespace
	 *
	 * \~
	 */
	namespace quic {
		/**
		 * \~russian
		 * @brief Пространство имён слоя защиты пакетов QUIC (RFC 9001 §5)
		 *
		 * @details Вывод ключей (HKDF-Expand-Label с метками "quic key"/"quic iv"/"quic hp"),
		 *          пер-пакетное AEAD-шифрование, защита заголовка (header protection),
		 *          обновление ключей ("quic ku") и тег целостности пакета Retry.
		 *          Криптографические примитивы - BoringSSL. Слой не хранит состояния
		 *          соединения - ключи передаются явно в каждую функцию.
		 *
		 * \~english
		 * @brief Namespace of the layer of the protection of the QUIC packets (RFC 9001 §5)
		 * @details The derivation of the keys (HKDF-Expand-Label with the labels "quic key"/"quic iv"/"quic hp"),
		 *          the per-packet AEAD encryption, the header protection,
		 *          the key update ("quic ku") and the integrity tag of a Retry packet.
		 *          The cryptographic primitives are those of BoringSSL. The layer does not store the state of a
		 *          connection — the keys are passed explicitly into every function.
		 *
		 * \~
		 */
		namespace crypto {
			/**
			 * \~russian
			 * @brief Размер тега аутентификации AEAD (RFC 9001 §5.3)
			 *
			 * \~english
			 * @brief Size of the AEAD authentication tag (RFC 9001 §5.3)
			 *
			 * \~
			 */
			static constexpr size_t AEAD_TAG_SIZE = 16;
			/**
			 * \~russian
			 * @brief Размер выборки нагрузки для защиты заголовка (RFC 9001 §5.4.2)
			 *
			 * \~english
			 * @brief Size of the sample of the payload for the header protection (RFC 9001 §5.4.2)
			 *
			 * \~
			 */
			static constexpr size_t HP_SAMPLE_SIZE = 16;
			/**
			 * \~russian
			 * @brief Размер маски защиты заголовка (RFC 9001 §5.4.1)
			 *
			 * \~english
			 * @brief Size of the mask of the header protection (RFC 9001 §5.4.1)
			 *
			 * \~
			 */
			static constexpr size_t HP_MASK_SIZE = 5;

			/**
			 * \~russian
			 * @brief Криптографический набор защиты пакетов (RFC 9001 §5.1)
			 *
			 * \~english
			 * @brief Cryptographic suite of the protection of the packets (RFC 9001 §5.1)
			 *
			 * \~
			 */
			enum class suite_t : uint8_t {
				AES_128_GCM_SHA256       = 0x00, // TLS_AES_128_GCM_SHA256 (обязательный, используется для Initial)
				AES_256_GCM_SHA384       = 0x01, // TLS_AES_256_GCM_SHA384
				CHACHA20_POLY1305_SHA256 = 0x02  // TLS_CHACHA20_POLY1305_SHA256
			};

			/**
			 * \~russian
			 * @brief Структура ключей защиты пакетов одного направления (RFC 9001 §5.1)
			 *
			 * \~english
			 * @brief Structure of the keys of the protection of the packets of one direction (RFC 9001 §5.1)
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Keys {
				// Криптографический набор защиты пакетов
				suite_t suite;
				// Секрет направления (источник вывода ключей)
				string secret;
				// Ключ AEAD-шифрования нагрузки ("quic key")
				string key;
				// Вектор инициализации AEAD ("quic iv", 12 октетов)
				string iv;
				// Ключ защиты заголовка ("quic hp")
				string hp;
				/**
				 * Контекст AEAD-шифрования, развёрнутый один раз на время жизни ключей.
				 * Ключи копируются при установке фазы (install/update), поэтому владение
				 * разделяемое - контекст неизменяем после инициализации
				 */
				shared_ptr <evp_aead_ctx_st> aead;
				/**
				 * Развёрнутый ключ защиты заголовка для наборов на основе AES.
				 * Для ChaCha20 не используется - у потокового шифра нет развёртки ключа
				 */
				shared_ptr <aes_key_st> mask;
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
				explicit Keys() noexcept;
				/**
				 * Конструкторы копирования и перемещения стандартны - они создают новый
				 * экземпляр, затираемый в своём деструкторе. Операторы присваивания
				 * переопределены: перед перезаписью существующего экземпляра его прежний
				 * ключевой материал затирается, иначе освобождение старого буфера строки
				 * оставило бы ключи в куче (RFC 9001 §6, defense-in-depth)
				 */
				Keys(const Keys &) = default;
				Keys(Keys &&) noexcept = default;
				Keys & operator = (const Keys &) noexcept;
				Keys & operator = (Keys &&) noexcept;
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 * @note Затирает секрет и выведенные ключи в памяти до освобождения,
				 *       чтобы ключевой материал не оставался в куче после разрыва
				 *       соединения либо смены фазы (RFC 9001 §6, defense-in-depth)
				 *
				 * \~english
				 * @brief Destructor
				 * @note Wipes the secret and the derived keys in the memory before the release
				 *       so that the key material does not remain in the heap after a break of a
				 *       connection or after a change of the phase (RFC 9001 §6, defense-in-depth)
				 *
				 * \~
				 */
				~Keys() noexcept;
			} keys_t;

			/**
			 * \~russian
			 * @brief Функция вывода ключей HKDF-Expand-Label (RFC 8446 §7.1 / RFC 9001 §5.1)
			 *
			 * @param suite  криптографический набор (определяет хеш-функцию)
			 * @param secret секрет-источник вывода
			 * @param label  метка вывода (без префикса "tls13 ")
			 * @param length требуемая длина выводимого материала
			 * @param output выводимый ключевой материал
			 * @return       результат вывода (false - ошибка криптографической библиотеки)
			 *
			 * \~english
			 * @brief Function of the derivation of the keys by HKDF-Expand-Label (RFC 8446 §7.1 / RFC 9001 §5.1)
			 * @param suite  cryptographic suite (determines the hash function)
			 * @param secret secret being the source of the derivation
			 * @param label  label of the derivation (without the "tls13 " prefix)
			 * @param length required length of the material being derived
			 * @param output derived key material
			 * @return       result of the derivation (false — an error of the cryptographic library)
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool hkdfExpandLabel(const suite_t suite, string_view secret, string_view label, const size_t length, string & output) noexcept;
			/**
			 * \~russian
			 * @brief Функция вывода ключей Initial обоих направлений (RFC 9001 §5.2)
			 *
			 * @note Ключи выводятся из DCID первого пакета Initial клиента
			 *       с фиксированной солью QUIC v1 и набором AES_128_GCM_SHA256
			 *
			 * @param dcid   идентификатор соединения получателя первого пакета Initial клиента
			 * @param client ключи защиты пакетов клиента
			 * @param server ключи защиты пакетов сервера
			 * @return       результат вывода (false - ошибка криптографической библиотеки)
			 *
			 * \~english
			 * @brief Function of the derivation of the Initial keys of both directions (RFC 9001 §5.2)
			 * @note The keys are derived from the DCID of the first Initial packet of the client
			 *       with the fixed salt of QUIC v1 and with the AES_128_GCM_SHA256 suite
			 * @param dcid   connection identifier of the recipient of the first Initial packet of the client
			 * @param client keys of the protection of the packets of the client
			 * @param server keys of the protection of the packets of the server
			 * @return       result of the derivation (false — an error of the cryptographic library)
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool initial(const cid_t & dcid, keys_t & client, keys_t & server) noexcept;
			/**
			 * \~russian
			 * @brief Функция вывода ключей защиты пакетов из секрета направления (RFC 9001 §5.1)
			 *
			 * @note Поля suite и secret структуры должны быть заполнены
			 *
			 * @param keys ключи защиты пакетов (заполняются key/iv/hp)
			 * @return     результат вывода (false - ошибка криптографической библиотеки)
			 *
			 * \~english
			 * @brief Function of the derivation of the keys of the protection of the packets from the secret of a direction (RFC 9001 §5.1)
			 * @note The suite and secret fields of the structure must be filled in
			 * @param keys keys of the protection of the packets (the key/iv/hp are filled in)
			 * @return     result of the derivation (false — an error of the cryptographic library)
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool derive(keys_t & keys) noexcept;
			/**
			 * \~russian
			 * @brief Функция обновления ключей на новую фазу (RFC 9001 §6)
			 *
			 * @note Новый секрет выводится меткой "quic ku", ключ защиты заголовка не меняется
			 *
			 * @param current текущие ключи защиты пакетов
			 * @param next    ключи защиты пакетов следующей фазы
			 * @return        результат обновления (false - ошибка криптографической библиотеки)
			 *
			 * \~english
			 * @brief Function of the update of the keys to a new phase (RFC 9001 §6)
			 * @note The new secret is derived with the "quic ku" label, the key of the header protection does not change
			 * @param current current keys of the protection of the packets
			 * @param next    keys of the protection of the packets of the next phase
			 * @return        result of the update (false — an error of the cryptographic library)
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool update(const keys_t & current, keys_t & next) noexcept;
			/**
			 * \~russian
			 * @brief Функция вычисления маски защиты заголовка (RFC 9001 §5.4)
			 *
			 * @param keys   ключи защиты пакетов
			 * @param sample выборка защищённой нагрузки (16 октетов)
			 * @param mask   вычисленная маска защиты заголовка (5 октетов)
			 * @return       результат вычисления (false - ошибка криптографической библиотеки)
			 *
			 * \~english
			 * @brief Function of computing the mask of the header protection (RFC 9001 §5.4)
			 * @param keys   keys of the protection of the packets
			 * @param sample sample of the protected payload (16 octets)
			 * @param mask   computed mask of the header protection (5 octets)
			 * @return       result of the computation (false — an error of the cryptographic library)
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool hpMask(const keys_t & keys, const uint8_t sample[HP_SAMPLE_SIZE], uint8_t mask[HP_MASK_SIZE]) noexcept;
			/**
			 * \~russian
			 * @brief Функция полной защиты пакета: AEAD-шифрование нагрузки и защита заголовка (RFC 9001 §5.3/§5.4)
			 *
			 * @note Заголовок должен быть собран целиком и заканчиваться полем Packet Number,
			 *       размер номера пакета берётся из битов первого октета заголовка
			 *
			 * @param output  выходной буфер (дописывается защищённый пакет целиком)
			 * @param keys    ключи защиты пакетов отправителя
			 * @param pn      полный номер пакета (для нонса AEAD)
			 * @param header  собранный незащищённый заголовок пакета (включая Packet Number)
			 * @param payload незашифрованная нагрузка пакета (фреймы)
			 * @return        результат защиты (false - ошибка криптографической библиотеки)
			 *
			 * \~english
			 * @brief Function of the full protection of a packet: the AEAD encryption of the payload and the header protection (RFC 9001 §5.3/§5.4)
			 * @note The header must be assembled in full and must end with the Packet Number field,
			 *       the size of the packet number is taken from the bits of the first octet of the header
			 * @param output  output buffer (the protected packet is appended in full)
			 * @param keys    keys of the protection of the packets of the sender
			 * @param pn      full packet number (for the AEAD nonce)
			 * @param header  assembled unprotected packet header (including the Packet Number)
			 * @param payload unencrypted payload of the packet (the frames)
			 * @return        result of the protection (false — an error of the cryptographic library)
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool seal(string & output, const keys_t & keys, const uint64_t pn, string_view header, string_view payload) noexcept;
			/**
			 * \~russian
			 * @brief Функция полного снятия защиты пакета: защита заголовка и AEAD-расшифровка (RFC 9001 §5.3/§5.4)
			 *
			 * @note Буфер пакета модифицируется на месте: с заголовка снимается защита.
			 *       Параметры pnOffset и size соответствуют разобранному заголовку пакета
			 *
			 * @param packet    буфер пакета (модифицируется)
			 * @param size      полный размер пакета
			 * @param pnOffset  смещение поля Packet Number (граница защиты заголовка)
			 * @param largestPn наибольший принятый номер пакета в пространстве номеров
			 * @param keys      ключи защиты пакетов отправителя пакета
			 * @param pn        восстановленный полный номер пакета
			 * @param output    расшифрованная нагрузка пакета (фреймы)
			 * @param error     код ошибки транспорта
			 * @return          результат снятия защиты (OK/ERROR)
			 *
			 * \~english
			 * @brief Function of the full removal of the protection of a packet: the header protection and the AEAD decryption (RFC 9001 §5.3/§5.4)
			 * @note The buffer of the packet is modified in place: the protection is removed from the header.
			 *       The pnOffset and size parameters correspond to the parsed header of the packet
			 * @param packet    buffer of the packet (modified)
			 * @param size      full size of the packet
			 * @param pnOffset  offset of the Packet Number field (the boundary of the header protection)
			 * @param largestPn largest accepted packet number in the number space
			 * @param keys      keys of the protection of the packets of the sender of the packet
			 * @param pn        restored full packet number
			 * @param output    decrypted payload of the packet (the frames)
			 * @param error     transport error code
			 * @return          result of the removal of the protection (OK/ERROR)
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ status_t open(uint8_t * packet, const size_t size, const size_t pnOffset, const uint64_t largestPn, const keys_t & keys, uint64_t & pn, string & output, error_t & error) noexcept;
			/**
			 * \~russian
			 * @brief Функция вычисления тега целостности пакета Retry (RFC 9001 §5.8)
			 *
			 * @param odcid  DCID первого пакета Initial клиента
			 * @param packet пакет Retry без тега целостности
			 * @param tag    вычисленный тег целостности (16 октетов)
			 * @return       результат вычисления (false - ошибка криптографической библиотеки)
			 *
			 * \~english
			 * @brief Function of computing the integrity tag of a Retry packet (RFC 9001 §5.8)
			 * @param odcid  DCID of the first Initial packet of the client
			 * @param packet Retry packet without the integrity tag
			 * @param tag    computed integrity tag (16 octets)
			 * @return       result of the computation (false — an error of the cryptographic library)
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool retryTag(const cid_t & odcid, string_view packet, uint8_t tag[proto::RETRY_TAG_SIZE]) noexcept;
			/**
			 * \~russian
			 * @brief Функция проверки тега целостности пакета Retry (RFC 9001 §5.8)
			 *
			 * @param odcid  DCID первого пакета Initial клиента
			 * @param packet пакет Retry целиком (включая тег целостности)
			 * @return       результат проверки (true - тег целостности корректен)
			 *
			 * \~english
			 * @brief Function of checking the integrity tag of a Retry packet (RFC 9001 §5.8)
			 * @param odcid  DCID of the first Initial packet of the client
			 * @param packet Retry packet in full (including the integrity tag)
			 * @return       result of the check (true — the integrity tag is correct)
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ bool retryVerify(const cid_t & odcid, string_view packet) noexcept;
		};
	};
};

#endif // __AWH_PROTO_QUIC_CRYPTO__
