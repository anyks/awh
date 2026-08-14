/**
 * @file packet.hpp
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
 * @brief Заголовочный файл слоя пакетов QUIC (RFC 9000 §17) — структура разобранного заголовка пакета и чистые
 *        функции разбора и сборки заголовков длинной и короткой формы, включая Initial, Handshake, 0-RTT,
 *        Retry и Version Negotiation
 *
 * \~english
 * @brief Header file of the QUIC packet layer (RFC 9000 §17) — the structure of a parsed packet header and the pure
 *        functions of the parsing and of the assembly of the headers of the long and of the short form, including Initial, Handshake, 0-RTT,
 *        Retry and Version Negotiation
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
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
		 * @brief Пространство имён слоя пакетов QUIC (RFC 9000 §17): разбор и сборка заголовков
		 *
		 * @details Разбор zero-copy: полезная нагрузка отдаётся как string_view с указателем
		 *          во входную датаграмму. Слой не выполняет криптографию: разбор останавливается
		 *          на границе защиты заголовка (header protection), номер пакета читается
		 *          отдельной функцией после снятия защиты. Сборка дописывает байты в string.
		 *          Слой не хранит состояния соединения - это чистые функции над байтами.
		 *
		 * \~english
		 * @brief Namespace of the QUIC packet layer (RFC 9000 §17): the parsing and the assembly of the headers
		 * @details The parsing is zero-copy: the payload is given away as a string_view with a pointer
		 *          into the input datagram. The layer does not perform the cryptography: the parsing stops
		 *          at the boundary of the header protection, the packet number is read
		 *          by a separate function after the protection has been removed. The assembly appends the bytes into a string.
		 *          The layer does not store the state of a connection — these are pure functions over the bytes.
		 *
		 * \~
		 */
		namespace packet {
			/**
			 * \~russian
			 * @brief Структура разобранного заголовка пакета (RFC 9000 §17.2/§17.3)
			 *
			 * @details Для пакетов Initial/0-RTT/Handshake поле payload указывает на защищённую
			 *          часть (Packet Number + нагрузка) длиной length. Для Version Negotiation
			 *          payload содержит список версий, для Retry - токен с тегом целостности,
			 *          для 1-RTT - остаток датаграммы начиная с Packet Number.
			 *
			 * \~english
			 * @brief Structure of a parsed packet header (RFC 9000 §17.2/§17.3)
			 * @details For the Initial/0-RTT/Handshake packets the payload field points to the protected
			 *          part (the Packet Number and the payload) of the length length. For Version Negotiation
			 *          the payload contains the list of the versions, for Retry — the token with the integrity tag,
			 *          for 1-RTT — the remainder of the datagram beginning with the Packet Number.
			 *
			 * \~
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
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit Header() noexcept;
			} header_t;

			/**
			 * \~russian
			 * @brief Функция выбора размера кодирования номера пакета (RFC 9000 §A.2)
			 *
			 * @param pn           номер отправляемого пакета
			 * @param largestAcked наибольший подтверждённый пиром номер пакета (pn, если подтверждений ещё не было)
			 * @return             размер кодирования номера пакета в октетах (1-4)
			 *
			 * \~english
			 * @brief Function of choosing the size of the encoding of the packet number (RFC 9000 §A.2)
			 * @param pn           number of the packet being sent
			 * @param largestAcked largest packet number acknowledged by the peer (pn if there have been no acknowledgements yet)
			 * @return             size of the encoding of the packet number in octets (1-4)
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ size_t packetNumberSize(const uint64_t pn, const uint64_t largestAcked) noexcept;

			/**
			 * \~russian
			 * @brief Функция восстановления полного номера пакета из усечённого (RFC 9000 §A.3)
			 *
			 * @param largestPn наибольший номер пакета, принятый в данном пространстве номеров
			 * @param truncated усечённый номер пакета из заголовка
			 * @param pnSize    размер усечённого номера пакета в октетах (1-4)
			 * @return          восстановленный полный номер пакета
			 *
			 * \~english
			 * @brief Function of restoring the full packet number from a truncated one (RFC 9000 §A.3)
			 * @param largestPn largest packet number accepted in the given number space
			 * @param truncated truncated packet number from the header
			 * @param pnSize    size of the truncated packet number in octets (1-4)
			 * @return          restored full packet number
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ uint64_t decodePacketNumber(const uint64_t largestPn, const uint64_t truncated, const size_t pnSize) noexcept;

			/**
			 * \~russian
			 * @brief Пространство имён функций разбора заголовков пакетов QUIC (RFC 9000 §17)
			 *
			 * \~english
			 * @brief Namespace of the functions of the parsing of the QUIC packet headers (RFC 9000 §17)
			 *
			 * \~
			 */
			namespace parser {
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Function of parsing a packet header up to the boundary of the header protection
				 * @note For a short header (1-RTT) the length of the connection identifier
				 *       is not encoded in the packet — it is given by the dcidSize parameter
				 * @param data     input datagram (the beginning of the next packet)
				 * @param size     bytes available
				 * @param dcidSize length of the connection identifier of the local endpoint (for a short header)
				 * @param output   parsed packet header
				 * @param error    transport error code
				 * @return         result of the parsing (OK/INCOMPLETE/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t header(const uint8_t * data, const size_t size, const size_t dcidSize, header_t & output, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция чтения усечённого номера пакета (после снятия защиты заголовка)
				 *
				 * @param data   буфер, указывающий на поле Packet Number
				 * @param size   доступно байт
				 * @param pnSize размер номера пакета в октетах (1-4, из битов первого октета)
				 * @param output прочитанный усечённый номер пакета
				 * @return       результат чтения (true - в буфере было достаточно байт)
				 *
				 * \~english
				 * @brief Function of reading a truncated packet number (after the header protection has been removed)
				 * @param data   buffer pointing at the Packet Number field
				 * @param size   bytes available
				 * @param pnSize size of the packet number in octets (1-4, from the bits of the first octet)
				 * @param output read truncated packet number
				 * @return       result of the reading (true — there were enough bytes in the buffer)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool packetNumber(const uint8_t * data, const size_t size, const size_t pnSize, uint64_t & output) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора списка версий пакета Version Negotiation (RFC 9000 §17.2.1)
				 *
				 * @param header заголовок разобранного пакета Version Negotiation
				 * @param output список поддерживаемых пиром версий
				 * @param error  код ошибки транспорта
				 * @return       результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the list of the versions of a Version Negotiation packet (RFC 9000 §17.2.1)
				 * @param header header of the parsed Version Negotiation packet
				 * @param output list of the versions supported by the peer
				 * @param error  transport error code
				 * @return       result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t versions(const header_t & header, vector <uint32_t> & output, error_t & error) noexcept;
				/**
				 * \~russian
				 * @brief Функция разбора нагрузки пакета Retry (RFC 9000 §17.2.5)
				 *
				 * @param header заголовок разобранного пакета Retry
				 * @param token  токен для повторного пакета Initial (zero-copy во входную датаграмму)
				 * @param tag    тег целостности пакета Retry (16 октетов)
				 * @param error  код ошибки транспорта
				 * @return       результат разбора (OK/ERROR)
				 *
				 * \~english
				 * @brief Function of parsing the payload of a Retry packet (RFC 9000 §17.2.5)
				 * @param header header of the parsed Retry packet
				 * @param token  token for a repeated Initial packet (zero-copy into the input datagram)
				 * @param tag    integrity tag of the Retry packet (16 octets)
				 * @param error  transport error code
				 * @return       result of the parsing (OK/ERROR)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ status_t retry(const header_t & header, string_view & token, uint8_t tag[proto::RETRY_TAG_SIZE], error_t & error) noexcept;
			};

			/**
			 * \~russian
			 * @brief Пространство имён функций сборки заголовков пакетов QUIC (RFC 9000 §17)
			 *
			 * \~english
			 * @brief Namespace of the functions of the assembly of the QUIC packet headers (RFC 9000 §17)
			 *
			 * \~
			 */
			namespace serialize {
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Function of assembling a long packet header with the packet number (RFC 9000 §17.2)
				 * @note The Length field must include the size of the packet number, the payload and the AEAD tag.
				 *       A token is admissible only for the Initial packets
				 * @param output  output buffer
				 * @param type    type of the packet (INITIAL/ZERO_RTT/HANDSHAKE)
				 * @param version version of the protocol
				 * @param dcid    connection identifier of the recipient
				 * @param scid    connection identifier of the sender
				 * @param token   token of the Initial packet (empty — there is no token)
				 * @param length  value of the Length field (the packet number + the payload + the AEAD tag)
				 * @param pn      packet number
				 * @param pnSize  size of the encoding of the packet number in octets (1-4)
				 * @return        result of the assembly (false — the parameters are incorrect)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool longHeader(string & output, const packet_t type, const uint32_t version, const cid_t & dcid, const cid_t & scid, string_view token, const uint64_t length, const uint64_t pn, const size_t pnSize) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки короткого заголовка пакета 1-RTT с номером пакета (RFC 9000 §17.3)
				 *
				 * @param output   выходной буфер
				 * @param dcid     идентификатор соединения получателя
				 * @param pn       номер пакета
				 * @param pnSize   размер кодирования номера пакета в октетах (1-4)
				 * @param keyPhase бит фазы ключей (RFC 9001 §6)
				 * @param spin     бит задержки (spin bit, RFC 9000 §17.4)
				 * @return         результат сборки (false - некорректные параметры)
				 *
				 * \~english
				 * @brief Function of assembling a short header of a 1-RTT packet with the packet number (RFC 9000 §17.3)
				 * @param output   output buffer
				 * @param dcid     connection identifier of the recipient
				 * @param pn       packet number
				 * @param pnSize   size of the encoding of the packet number in octets (1-4)
				 * @param keyPhase bit of the key phase (RFC 9001 §6)
				 * @param spin     spin bit (RFC 9000 §17.4)
				 * @return         result of the assembly (false — the parameters are incorrect)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool shortHeader(string & output, const cid_t & dcid, const uint64_t pn, const size_t pnSize, const bool keyPhase, const bool spin) noexcept;
				/**
				 * \~russian
				 * @brief Функция сборки пакета Version Negotiation целиком (RFC 9000 §17.2.1)
				 *
				 * @param output   выходной буфер
				 * @param dcid     идентификатор соединения получателя (SCID пакета клиента)
				 * @param scid     идентификатор соединения отправителя (DCID пакета клиента)
				 * @param versions список поддерживаемых версий
				 * @param count    количество версий
				 * @return         результат сборки (false - некорректные параметры)
				 *
				 * \~english
				 * @brief Function of assembling a Version Negotiation packet in full (RFC 9000 §17.2.1)
				 * @param output   output buffer
				 * @param dcid     connection identifier of the recipient (the SCID of the packet of the client)
				 * @param scid     connection identifier of the sender (the DCID of the packet of the client)
				 * @param versions list of the supported versions
				 * @param count    number of the versions
				 * @return         result of the assembly (false — the parameters are incorrect)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool versionNegotiation(string & output, const cid_t & dcid, const cid_t & scid, const uint32_t * versions, const size_t count) noexcept;
				/**
				 * \~russian
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
				 *
				 * \~english
				 * @brief Function of assembling a Retry packet in full (RFC 9000 §17.2.5)
				 * @note The integrity tag is computed by the cryptographic layer (RFC 9001 §5.8)
				 *       over a pseudo-packet with the ODCID and is passed here ready
				 * @param output  output buffer
				 * @param version version of the protocol
				 * @param dcid    connection identifier of the recipient (the SCID of the packet of the client)
				 * @param scid    new connection identifier of the sender
				 * @param token   token for a repeated Initial packet
				 * @param tag     integrity tag of the Retry packet (16 octets)
				 * @return        result of the assembly (false — the parameters are incorrect)
				 *
				 * \~
				 */
				__AWH_SHARED_EXPORT__ bool retry(string & output, const uint32_t version, const cid_t & dcid, const cid_t & scid, string_view token, const uint8_t tag[proto::RETRY_TAG_SIZE]) noexcept;
			};
		};
	};
};

#endif // __AWH_PROTO_QUIC_PACKET__
