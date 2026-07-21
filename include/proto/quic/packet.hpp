/**
 * @file: packet.hpp
 * @date: 2026-07-21
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

#ifndef __AWH_PROTO_QUIC_PACKET__
#define __AWH_PROTO_QUIC_PACKET__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "quic.hpp"
#include "varint.hpp"
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
	namespace quic {
		/**
		 * @brief Пространство имён слоя пакетов QUIC (RFC 9000 §17): разбор и сборка заголовков
		 *
		 * @details Разбор zero-copy: полезная нагрузка отдаётся как string_view с указателем
		 *          во входную датаграмму. Слой не выполняет криптографию: разбор останавливается
		 *          на границе защиты заголовка (header protection), номер пакета читается
		 *          отдельной функцией после снятия защиты. Сборка дописывает байты в string.
		 *          Слой не хранит состояния соединения - это чистые функции над байтами.
		 */
		namespace packet {
			/**
			 * @brief Структура разобранного заголовка пакета (RFC 9000 §17.2/§17.3)
			 *
			 * @details Для пакетов Initial/0-RTT/Handshake поле payload указывает на защищённую
			 *          часть (Packet Number + нагрузка) длиной length. Для Version Negotiation
			 *          payload содержит список версий, для Retry - токен с тегом целостности,
			 *          для 1-RTT - остаток датаграммы начиная с Packet Number.
			 */
			typedef struct __AWH_SHARED_EXPORT__ Header {
				// Тип пакета
				packet_t type;
				// Первый октет пакета (биты номера пакета и key phase до снятия защиты заголовка)
				uint8_t first;
				// Версия протокола (для длинного заголовка)
				uint32_t version;
				// Идентификатор соединения получателя
				cid_t dcid;
				// Идентификатор соединения отправителя (для длинного заголовка)
				cid_t scid;
				// Токен пакета Initial (zero-copy во входную датаграмму)
				string_view token;
				// Значение поля Length: номер пакета + защищённая нагрузка (для Initial/0-RTT/Handshake)
				uint64_t length;
				// Смещение поля Packet Number от начала пакета (граница защиты заголовка)
				size_t pnOffset;
				// Полный размер пакета в датаграмме (датаграмма может содержать несколько пакетов)
				size_t size;
				// Защищённая или типоспецифичная нагрузка пакета (zero-copy во входную датаграмму)
				string_view payload;
				/**
				 * @brief Конструктор
				 *
				 */
				explicit Header() noexcept;
			} header_t;

			/**
			 * @brief Функция выбора размера кодирования номера пакета (RFC 9000 §A.2)
			 *
			 * @param pn           номер отправляемого пакета
			 * @param largestAcked наибольший подтверждённый пиром номер пакета (pn, если подтверждений ещё не было)
			 * @return             размер кодирования номера пакета в октетах (1-4)
			 */
			__AWH_SHARED_EXPORT__ size_t packetNumberSize(const uint64_t pn, const uint64_t largestAcked) noexcept;

			/**
			 * @brief Функция восстановления полного номера пакета из усечённого (RFC 9000 §A.3)
			 *
			 * @param largestPn наибольший номер пакета, принятый в данном пространстве номеров
			 * @param truncated усечённый номер пакета из заголовка
			 * @param pnSize    размер усечённого номера пакета в октетах (1-4)
			 * @return          восстановленный полный номер пакета
			 */
			__AWH_SHARED_EXPORT__ uint64_t decodePacketNumber(const uint64_t largestPn, const uint64_t truncated, const size_t pnSize) noexcept;

			/**
			 * @brief Пространство имён функций разбора заголовков пакетов QUIC (RFC 9000 §17)
			 *
			 */
			namespace parser {
				/**
				 * @brief Функция разбора заголовка пакета до границы защиты заголовка
				 *
				 * @note Для короткого заголовка (1-RTT) длина идентификатора соединения
				 *       в пакете не кодируется - её задаёт параметр dcidSize
				 *
				 * @param data     входная датаграмма (начало очередного пакета)
				 * @param size     доступно байт
				 * @param dcidSize длина идентификатора соединения локального эндпоинта (для короткого заголовка)
				 * @param output   разобранный заголовок пакета
				 * @param error    код ошибки транспорта
				 * @return         результат разбора (OK/INCOMPLETE/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t header(const uint8_t * data, const size_t size, const size_t dcidSize, header_t & output, error_t & error) noexcept;
				/**
				 * @brief Функция чтения усечённого номера пакета (после снятия защиты заголовка)
				 *
				 * @param data   буфер, указывающий на поле Packet Number
				 * @param size   доступно байт
				 * @param pnSize размер номера пакета в октетах (1-4, из битов первого октета)
				 * @param output прочитанный усечённый номер пакета
				 * @return       результат чтения (true - в буфере было достаточно байт)
				 */
				__AWH_SHARED_EXPORT__ bool packetNumber(const uint8_t * data, const size_t size, const size_t pnSize, uint64_t & output) noexcept;
				/**
				 * @brief Функция разбора списка версий пакета Version Negotiation (RFC 9000 §17.2.1)
				 *
				 * @param header заголовок разобранного пакета Version Negotiation
				 * @param output список поддерживаемых пиром версий
				 * @param error  код ошибки транспорта
				 * @return       результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t versions(const header_t & header, vector <uint32_t> & output, error_t & error) noexcept;
				/**
				 * @brief Функция разбора нагрузки пакета Retry (RFC 9000 §17.2.5)
				 *
				 * @param header заголовок разобранного пакета Retry
				 * @param token  токен для повторного пакета Initial (zero-copy во входную датаграмму)
				 * @param tag    тег целостности пакета Retry (16 октетов)
				 * @param error  код ошибки транспорта
				 * @return       результат разбора (OK/ERROR)
				 */
				__AWH_SHARED_EXPORT__ status_t retry(const header_t & header, string_view & token, uint8_t tag[proto::RETRY_TAG_SIZE], error_t & error) noexcept;
			};

			/**
			 * @brief Пространство имён функций сборки заголовков пакетов QUIC (RFC 9000 §17)
			 *
			 */
			namespace serialize {
				/**
				 * @brief Функция сборки длинного заголовка пакета с номером пакета (RFC 9000 §17.2)
				 *
				 * @note Поле Length должно включать размер номера пакета, нагрузку и тег AEAD.
				 *       Токен допустим только для пакетов Initial
				 *
				 * @param output  выходной буфер
				 * @param type    тип пакета (INITIAL/ZERO_RTT/HANDSHAKE)
				 * @param version версия протокола
				 * @param dcid    идентификатор соединения получателя
				 * @param scid    идентификатор соединения отправителя
				 * @param token   токен пакета Initial (пустой - токен отсутствует)
				 * @param length  значение поля Length (номер пакета + нагрузка + тег AEAD)
				 * @param pn      номер пакета
				 * @param pnSize  размер кодирования номера пакета в октетах (1-4)
				 * @return        результат сборки (false - некорректные параметры)
				 */
				__AWH_SHARED_EXPORT__ bool longHeader(string & output, const packet_t type, const uint32_t version, const cid_t & dcid, const cid_t & scid, string_view token, const uint64_t length, const uint64_t pn, const size_t pnSize) noexcept;
				/**
				 * @brief Функция сборки короткого заголовка пакета 1-RTT с номером пакета (RFC 9000 §17.3)
				 *
				 * @param output   выходной буфер
				 * @param dcid     идентификатор соединения получателя
				 * @param pn       номер пакета
				 * @param pnSize   размер кодирования номера пакета в октетах (1-4)
				 * @param keyPhase бит фазы ключей (RFC 9001 §6)
				 * @param spin     бит задержки (spin bit, RFC 9000 §17.4)
				 * @return         результат сборки (false - некорректные параметры)
				 */
				__AWH_SHARED_EXPORT__ bool shortHeader(string & output, const cid_t & dcid, const uint64_t pn, const size_t pnSize, const bool keyPhase, const bool spin) noexcept;
				/**
				 * @brief Функция сборки пакета Version Negotiation целиком (RFC 9000 §17.2.1)
				 *
				 * @param output   выходной буфер
				 * @param dcid     идентификатор соединения получателя (SCID пакета клиента)
				 * @param scid     идентификатор соединения отправителя (DCID пакета клиента)
				 * @param versions список поддерживаемых версий
				 * @param count    количество версий
				 * @return         результат сборки (false - некорректные параметры)
				 */
				__AWH_SHARED_EXPORT__ bool versionNegotiation(string & output, const cid_t & dcid, const cid_t & scid, const uint32_t * versions, const size_t count) noexcept;
				/**
				 * @brief Функция сборки пакета Retry целиком (RFC 9000 §17.2.5)
				 *
				 * @note Тег целостности вычисляется криптографическим слоем (RFC 9001 §5.8)
				 *       по псевдопакету с ODCID и передаётся сюда готовым
				 *
				 * @param output  выходной буфер
				 * @param version версия протокола
				 * @param dcid    идентификатор соединения получателя (SCID пакета клиента)
				 * @param scid    новый идентификатор соединения отправителя
				 * @param token   токен для повторного пакета Initial
				 * @param tag     тег целостности пакета Retry (16 октетов)
				 * @return        результат сборки (false - некорректные параметры)
				 */
				__AWH_SHARED_EXPORT__ bool retry(string & output, const uint32_t version, const cid_t & dcid, const cid_t & scid, string_view token, const uint8_t tag[proto::RETRY_TAG_SIZE]) noexcept;
			};
		};
	};
};

#endif // __AWH_PROTO_QUIC_PACKET__
