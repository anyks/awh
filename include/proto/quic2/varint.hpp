/**
 * @file: varint.hpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл кодирования целых переменной длины QUIC (RFC 9000 §16) —
 *        функции чтения и записи varint в сетевом порядке байт с длиной 1, 2, 4 или 8 октетов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_PROTO_QUIC2_VARINT__
#define __AWH_PROTO_QUIC2_VARINT__

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
	namespace quic2 {
		/**
		 * @brief Пространство имён кодека целых чисел переменной длины (RFC 9000 §16)
		 *
		 * @details Два старших бита первого октета кодируют длину числа:
		 *          00 - 1 октет (6 бит), 01 - 2 октета (14 бит),
		 *          10 - 4 октета (30 бит), 11 - 8 октетов (62 бита).
		 *          Значения кодируются в сетевом (big-endian) порядке байт.
		 *
		 */
		namespace varint {
			/**
			 * @brief Функция определения размера закодированного числа в октетах
			 *
			 * @param value кодируемое число
			 * @return      размер в октетах (1/2/4/8) или 0, если число превышает 2^62-1
			 *
			 */
			__AWH_SHARED_EXPORT__ size_t size(const uint64_t value) noexcept;
			/**
			 * @brief Функция определения размера закодированного числа по первому октету
			 *
			 * @param first первый октет закодированного числа
			 * @return      размер числа в октетах (1/2/4/8)
			 *
			 */
			__AWH_SHARED_EXPORT__ size_t sizeAt(const uint8_t first) noexcept;
			/**
			 * @brief Функция чтения целого числа переменной длины
			 *
			 * @param data  входной буфер
			 * @param size  доступно байт
			 * @param value прочитанное число
			 * @return      количество прочитанных октетов или 0, если данных недостаточно
			 *
			 */
			__AWH_SHARED_EXPORT__ size_t read(const uint8_t * data, const size_t size, uint64_t & value) noexcept;
			/**
			 * @brief Функция записи целого числа переменной длины (минимальное кодирование)
			 *
			 * @param output выходной буфер
			 * @param value  записываемое число (не более 2^62-1)
			 * @return       количество записанных октетов или 0, если число превышает 2^62-1
			 *
			 */
			__AWH_SHARED_EXPORT__ size_t write(string & output, const uint64_t value) noexcept;
		};
	};
};

#endif // __AWH_PROTO_QUIC2_VARINT__
