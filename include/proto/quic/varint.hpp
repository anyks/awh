/**
 * @file varint.hpp
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
 * @brief Заголовочный файл кодирования целых переменной длины QUIC (RFC 9000 §16) —
 *        функции чтения и записи varint в сетевом порядке байт с длиной 1, 2, 4 или 8 октетов
 *
 * \~english
 * @brief Header file of the encoding of the QUIC variable-length integers (RFC 9000 §16) —
 *        the functions of the reading and of the writing of a varint in the network byte order with a length of 1, 2, 4 or 8 octets
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

#ifndef __AWH_PROTO_QUIC_VARINT__
#define __AWH_PROTO_QUIC_VARINT__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstddef>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/global.hpp"

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
		 * @brief Пространство имён кодека целых чисел переменной длины (RFC 9000 §16)
		 *
		 * @details Два старших бита первого октета кодируют длину числа:
		 *          00 - 1 октет (6 бит), 01 - 2 октета (14 бит),
		 *          10 - 4 октета (30 бит), 11 - 8 октетов (62 бита).
		 *          Значения кодируются в сетевом (big-endian) порядке байт.
		 *
		 * \~english
		 * @brief Namespace of the codec of the variable-length integers (RFC 9000 §16)
		 * @details The two high bits of the first octet encode the length of the number:
		 *          00 — 1 octet (6 bits), 01 — 2 octets (14 bits),
		 *          10 — 4 octets (30 bits), 11 — 8 octets (62 bits).
		 *          The values are encoded in the network (big-endian) byte order.
		 *
		 * \~
		 */
		namespace varint {
			/**
			 * \~russian
			 * @brief Функция определения размера закодированного числа в октетах
			 *
			 * @param value кодируемое число
			 * @return      размер в октетах (1/2/4/8) или 0, если число превышает 2^62-1
			 *
			 * \~english
			 * @brief Function of determining the size of an encoded number in octets
			 * @param value number being encoded
			 * @return      size in octets (1/2/4/8) or 0 if the number exceeds 2^62-1
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ size_t size(const uint64_t value) noexcept;
			/**
			 * \~russian
			 * @brief Функция определения размера закодированного числа по первому октету
			 *
			 * @param first первый октет закодированного числа
			 * @return      размер числа в октетах (1/2/4/8)
			 *
			 * \~english
			 * @brief Function of determining the size of an encoded number by the first octet
			 * @param first first octet of the encoded number
			 * @return      size of the number in octets (1/2/4/8)
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ size_t sizeAt(const uint8_t first) noexcept;
			/**
			 * \~russian
			 * @brief Функция чтения целого числа переменной длины
			 *
			 * @param data  входной буфер
			 * @param size  доступно байт
			 * @param value прочитанное число
			 * @return      количество прочитанных октетов или 0, если данных недостаточно
			 *
			 * \~english
			 * @brief Function of reading a variable-length integer
			 * @param data  input buffer
			 * @param size  bytes available
			 * @param value read number
			 * @return      number of the read octets or 0 if the data is insufficient
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ size_t read(const uint8_t * data, const size_t size, uint64_t & value) noexcept;
			/**
			 * \~russian
			 * @brief Функция записи целого числа переменной длины (минимальное кодирование)
			 *
			 * @param output выходной буфер
			 * @param value  записываемое число (не более 2^62-1)
			 * @return       количество записанных октетов или 0, если число превышает 2^62-1
			 *
			 * \~english
			 * @brief Function of writing a variable-length integer (the minimal encoding)
			 * @param output output buffer
			 * @param value  number being written (no more than 2^62-1)
			 * @return       number of the written octets or 0 if the number exceeds 2^62-1
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ size_t write(string & output, const uint64_t value) noexcept;
		};
	};
};

#endif // __AWH_PROTO_QUIC_VARINT__
