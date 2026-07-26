/**
 * @file: crypto.hpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл криптографического слоя QUIC (RFC 9001) — вывод начальных секретов,
 *        ключей и заголовочной защиты,
 *        шифрование и расшифровка пакетов на AEAD-примитивах BoringSSL без хранения состояния соединения
 *
 * @copyright: Copyright © 2026
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
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён транспортного протокола QUIC
	 *
	 */
	namespace quic {
		/**
		 * @brief Пространство имён слоя защиты пакетов QUIC (RFC 9001 §5)
		 *
		 * @details Вывод ключей (HKDF-Expand-Label с метками "quic key"/"quic iv"/"quic hp"),
		 *          пер-пакетное AEAD-шифрование, защита заголовка (header protection),
		 *          обновление ключей ("quic ku") и тег целостности пакета Retry.
		 *          Криптографические примитивы - BoringSSL. Слой не хранит состояния
		 *          соединения - ключи передаются явно в каждую функцию.
		 *
		 */
		namespace crypto {
			/**
			 * @brief Размер тега аутентификации AEAD (RFC 9001 §5.3)
			 *
			 */
			static constexpr size_t AEAD_TAG_SIZE = 16;
			/**
			 * @brief Размер выборки нагрузки для защиты заголовка (RFC 9001 §5.4.2)
			 *
			 */
			static constexpr size_t HP_SAMPLE_SIZE = 16;
			/**
			 * @brief Размер маски защиты заголовка (RFC 9001 §5.4.1)
			 *
			 */
			static constexpr size_t HP_MASK_SIZE = 5;

			/**
			 * @brief Криптографический набор защиты пакетов (RFC 9001 §5.1)
			 *
			 */
			enum class suite_t : uint8_t {
				AES_128_GCM_SHA256       = 0x00, // TLS_AES_128_GCM_SHA256 (обязательный, используется для Initial)
				AES_256_GCM_SHA384       = 0x01, // TLS_AES_256_GCM_SHA384
				CHACHA20_POLY1305_SHA256 = 0x02  // TLS_CHACHA20_POLY1305_SHA256
			};

			/**
			 * @brief Структура ключей защиты пакетов одного направления (RFC 9001 §5.1)
			 *
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
				 * @brief Конструктор
				 *
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
				 * @brief Деструктор
				 *
				 * @note Затирает секрет и выведенные ключи в памяти до освобождения,
				 *       чтобы ключевой материал не оставался в куче после разрыва
				 *       соединения либо смены фазы (RFC 9001 §6, defense-in-depth)
				 *
				 */
				~Keys() noexcept;
			} keys_t;

			/**
			 * @brief Функция вывода ключей HKDF-Expand-Label (RFC 8446 §7.1 / RFC 9001 §5.1)
			 *
			 * @param suite  криптографический набор (определяет хеш-функцию)
			 * @param secret секрет-источник вывода
			 * @param label  метка вывода (без префикса "tls13 ")
			 * @param length требуемая длина выводимого материала
			 * @param output выводимый ключевой материал
			 * @return       результат вывода (false - ошибка криптографической библиотеки)
			 *
			 */
			__AWH_SHARED_EXPORT__ bool hkdfExpandLabel(const suite_t suite, string_view secret, string_view label, const size_t length, string & output) noexcept;
			/**
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
			 */
			__AWH_SHARED_EXPORT__ bool initial(const cid_t & dcid, keys_t & client, keys_t & server) noexcept;
			/**
			 * @brief Функция вывода ключей защиты пакетов из секрета направления (RFC 9001 §5.1)
			 *
			 * @note Поля suite и secret структуры должны быть заполнены
			 *
			 * @param keys ключи защиты пакетов (заполняются key/iv/hp)
			 * @return     результат вывода (false - ошибка криптографической библиотеки)
			 *
			 */
			__AWH_SHARED_EXPORT__ bool derive(keys_t & keys) noexcept;
			/**
			 * @brief Функция обновления ключей на новую фазу (RFC 9001 §6)
			 *
			 * @note Новый секрет выводится меткой "quic ku", ключ защиты заголовка не меняется
			 *
			 * @param current текущие ключи защиты пакетов
			 * @param next    ключи защиты пакетов следующей фазы
			 * @return        результат обновления (false - ошибка криптографической библиотеки)
			 *
			 */
			__AWH_SHARED_EXPORT__ bool update(const keys_t & current, keys_t & next) noexcept;
			/**
			 * @brief Функция вычисления маски защиты заголовка (RFC 9001 §5.4)
			 *
			 * @param keys   ключи защиты пакетов
			 * @param sample выборка защищённой нагрузки (16 октетов)
			 * @param mask   вычисленная маска защиты заголовка (5 октетов)
			 * @return       результат вычисления (false - ошибка криптографической библиотеки)
			 *
			 */
			__AWH_SHARED_EXPORT__ bool hpMask(const keys_t & keys, const uint8_t sample[HP_SAMPLE_SIZE], uint8_t mask[HP_MASK_SIZE]) noexcept;
			/**
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
			 */
			__AWH_SHARED_EXPORT__ bool seal(string & output, const keys_t & keys, const uint64_t pn, string_view header, string_view payload) noexcept;
			/**
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
			 */
			__AWH_SHARED_EXPORT__ status_t open(uint8_t * packet, const size_t size, const size_t pnOffset, const uint64_t largestPn, const keys_t & keys, uint64_t & pn, string & output, error_t & error) noexcept;
			/**
			 * @brief Функция вычисления тега целостности пакета Retry (RFC 9001 §5.8)
			 *
			 * @param odcid  DCID первого пакета Initial клиента
			 * @param packet пакет Retry без тега целостности
			 * @param tag    вычисленный тег целостности (16 октетов)
			 * @return       результат вычисления (false - ошибка криптографической библиотеки)
			 *
			 */
			__AWH_SHARED_EXPORT__ bool retryTag(const cid_t & odcid, string_view packet, uint8_t tag[proto::RETRY_TAG_SIZE]) noexcept;
			/**
			 * @brief Функция проверки тега целостности пакета Retry (RFC 9001 §5.8)
			 *
			 * @param odcid  DCID первого пакета Initial клиента
			 * @param packet пакет Retry целиком (включая тег целостности)
			 * @return       результат проверки (true - тег целостности корректен)
			 *
			 */
			__AWH_SHARED_EXPORT__ bool retryVerify(const cid_t & odcid, string_view packet) noexcept;
		};
	};
};

#endif // __AWH_PROTO_QUIC_CRYPTO__
