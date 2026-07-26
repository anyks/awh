/**
 * @file: params.cpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация транспортных параметров QUIC (RFC 9000 §18) — разбор и сборка параметров соединения с проверкой
 *        ролевых ограничений и обработкой предпочтительного адреса сервера
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/quic/params.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние вспомогательные функции и константы кодека параметров транспорта
 *
 */
namespace {
	/**
	 * Используем пространство имён протокола QUIC
	 */
	using namespace awh::quic;

	/**
	 * @brief Значение max_udp_payload_size по умолчанию (RFC 9000 §18.2)
	 *
	 */
	static constexpr uint64_t DEFAULT_MAX_UDP_PAYLOAD_SIZE = 65527;
	/**
	 * @brief Нижняя граница max_udp_payload_size (RFC 9000 §18.2)
	 *
	 */
	static constexpr uint64_t MIN_MAX_UDP_PAYLOAD_SIZE = 1200;
	/**
	 * @brief Значение ack_delay_exponent по умолчанию (RFC 9000 §18.2)
	 *
	 */
	static constexpr uint64_t DEFAULT_ACK_DELAY_EXPONENT = 3;
	/**
	 * @brief Верхняя граница ack_delay_exponent (RFC 9000 §18.2)
	 *
	 */
	static constexpr uint64_t MAX_ACK_DELAY_EXPONENT = 20;
	/**
	 * @brief Значение max_ack_delay по умолчанию в миллисекундах (RFC 9000 §18.2)
	 *
	 */
	static constexpr uint64_t DEFAULT_MAX_ACK_DELAY = 25;
	/**
	 * @brief Верхняя граница max_ack_delay (2^14, RFC 9000 §18.2)
	 *
	 */
	static constexpr uint64_t MAX_MAX_ACK_DELAY = 16384;
	/**
	 * @brief Значение active_connection_id_limit по умолчанию (RFC 9000 §18.2)
	 *
	 */
	static constexpr uint64_t DEFAULT_ACTIVE_CID_LIMIT = 2;
	/**
	 * @brief Верхняя граница лимитов числа потоков (2^60, RFC 9000 §4.6)
	 *
	 */
	static constexpr uint64_t MAX_STREAMS_LIMIT = (static_cast <uint64_t> (1) << 60);
	/**
	 * @brief Размер фиксированной части значения preferred_address (RFC 9000 §18.2)
	 *
	 */
	static constexpr size_t PREFERRED_ADDRESS_FIXED_SIZE = (4 + 2 + 16 + 2 + 1);

	/**
	 * @brief Функция чтения 16-битного числа в сетевом порядке байт
	 *
	 * @param buffer буфер с данными числа
	 * @return       прочитанное число
	 *
	 */
	inline uint16_t rd16(const uint8_t * buffer) noexcept {
		// Собираем число из двух байт в сетевом порядке
		return static_cast <uint16_t> ((static_cast <uint16_t> (buffer[0]) << 8) | buffer[1]);
	}
	/**
	 * @brief Функция записи 16-битного числа в сетевом порядке байт
	 *
	 * @param output выходной буфер
	 * @param num    записываемое число
	 *
	 */
	inline void wr16(string & output, const uint16_t num) noexcept {
		// Дописываем старший байт числа
		output.push_back(static_cast <char> ((num >> 8) & 0xFF));
		// Дописываем младший байт числа
		output.push_back(static_cast <char> (num & 0xFF));
	}
	/**
	 * @brief Функция чтения целочисленного значения параметра (значение должно занимать длину целиком)
	 *
	 * @param data   буфер значения параметра
	 * @param length длина значения параметра
	 * @param value  прочитанное значение
	 * @return       результат чтения (false - значение закодировано некорректно)
	 *
	 */
	inline bool rdValue(const uint8_t * data, const size_t length, uint64_t & value) noexcept {
		// Если значение параметра пустое (целочисленный параметр обязан содержать число)
		if(length == 0)
			// Чтение невозможно
			return false;
		// Читаем целое число переменной длины и проверяем что оно занимает значение целиком
		return (varint::read(data, length, value) == length);
	}
	/**
	 * @brief Функция записи целочисленного параметра (идентификатор, длина, значение)
	 *
	 * @param output выходной буфер
	 * @param id     идентификатор параметра
	 * @param value  значение параметра
	 *
	 */
	inline void wrScalar(string & output, const params::id_t id, const uint64_t value) noexcept {
		// Дописываем идентификатор параметра
		varint::write(output, static_cast <uint64_t> (id));
		// Дописываем длину значения параметра
		varint::write(output, varint::size(value));
		// Дописываем значение параметра
		varint::write(output, value);
	}
	/**
	 * @brief Функция записи параметра с идентификатором соединения
	 *
	 * @param output выходной буфер
	 * @param id     идентификатор параметра
	 * @param cid    идентификатор соединения
	 *
	 */
	inline void wrCid(string & output, const params::id_t id, const cid_t & cid) noexcept {
		// Дописываем идентификатор параметра
		varint::write(output, static_cast <uint64_t> (id));
		// Дописываем длину значения параметра
		varint::write(output, cid.size);
		// Если идентификатор соединения не пустой
		if(cid.size > 0)
			// Дописываем данные идентификатора соединения
			output.append(reinterpret_cast <const char *> (cid.data), cid.size);
	}
};

/**
 * @brief Конструктор
 *
 */
awh::quic::params::Preferred_Address::Preferred_Address() noexcept :
 ipv4Port(0), ipv6Port(0), ipv4{0}, ipv6{0}, resetToken{0} {}

/**
 * @brief Конструктор
 *
 */
awh::quic::params::Params::Params() noexcept :
 hasOdcid(false), hasInitialScid(false), hasRetryScid(false),
 hasResetToken(false), hasPreferredAddress(false), disableActiveMigration(false),
 maxIdleTimeout(0), maxUdpPayloadSize(DEFAULT_MAX_UDP_PAYLOAD_SIZE),
 initialMaxData(0), initialMaxStreamDataBidiLocal(0),
 initialMaxStreamDataBidiRemote(0), initialMaxStreamDataUni(0),
 initialMaxStreamsBidi(0), initialMaxStreamsUni(0),
 ackDelayExponent(DEFAULT_ACK_DELAY_EXPONENT), maxAckDelay(DEFAULT_MAX_ACK_DELAY),
 activeConnectionIdLimit(DEFAULT_ACTIVE_CID_LIMIT), maxDatagramFrameSize(0), resetToken{0} {}

/**
 * @brief Функция разбора параметров транспорта из TLS-расширения
 *
 * @param data   буфер значения TLS-расширения quic_transport_parameters
 * @param size   доступно байт
 * @param sender роль эндпоинта, закодировавшего параметры
 * @param output разобранные параметры транспорта
 * @param error  код ошибки транспорта
 * @return       результат разбора (OK/ERROR)
 *
 */
awh::quic::status_t awh::quic::params::parser::decode(const uint8_t * data, const size_t size, const endpoint_t sender, params_t & output, error_t & error) noexcept {
	// Устанавливаем код ошибки транспорта на случай неудачи разбора
	error = error_t::TRANSPORT_PARAMETER_ERROR;
	/**
	 * Битовая маска встреченных известных параметров (для контроля дубликатов).
	 * Разрядность выбрана по наибольшему известному идентификатору: идентификаторы
	 * не сплошные, и маска покрывает диапазон целиком
	 */
	uint64_t seen = 0;
	// Смещение разбора
	size_t offset = 0;
	/**
	 *  Перебираем последовательность записей параметров
	 */
	while(offset < size){
		// Идентификатор параметра
		uint64_t id = 0;
		// Читаем идентификатор параметра
		size_t count = varint::read(data + offset, size - offset, id);
		// Если данных недостаточно
		if(count == 0)
			// Разбор невозможен
			return status_t::ERROR;
		// Сдвигаем смещение разбора
		offset += count;
		// Длина значения параметра
		uint64_t length = 0;
		// Читаем длину значения параметра
		count = varint::read(data + offset, size - offset, length);
		// Если данных недостаточно
		if(count == 0)
			// Разбор невозможен
			return status_t::ERROR;
		// Сдвигаем смещение разбора
		offset += count;
		// Если значение целиком не помещается в буфер
		if(length > (size - offset))
			// Разбор невозможен
			return status_t::ERROR;
		// Буфер значения параметра
		const uint8_t * value = (data + offset);
		// Сдвигаем смещение разбора за значением
		offset += static_cast <size_t> (length);
		/**
		 * Если параметр известен протоколу: идентификаторы сплошным диапазоном
		 * не идут, поэтому предельный размер фрейма DATAGRAM проверяется отдельно -
		 * иначе неизвестные параметры промежутка перестали бы игнорироваться
		 * (RFC 9000 §7.4.2)
		 */
		if((id <= static_cast <uint64_t> (id_t::RETRY_SOURCE_CONNECTION_ID)) || (id == static_cast <uint64_t> (id_t::MAX_DATAGRAM_FRAME_SIZE))){
			// Битовый флаг параметра
			const uint64_t flag = (static_cast <uint64_t> (1) << id);
			// Если параметр уже встречался (RFC 9000 §7.4)
			if((seen & flag) != 0)
				// Разбор невозможен
				return status_t::ERROR;
			// Отмечаем параметр как встреченный
			seen |= flag;
		// Неизвестные параметры игнорируются (RFC 9000 §7.4.2)
		} else continue;
		/**
		 * Определяем идентификатор параметра
		 */
		switch(id){
			// Исходный DCID первого пакета Initial клиента
			case static_cast <uint64_t> (id_t::ORIGINAL_DESTINATION_CONNECTION_ID): {
				// Если параметр прислал клиент или длина превышает лимит QUIC v1
				if((sender == endpoint_t::CLIENT) || (length > proto::MAX_CID_SIZE))
					// Разбор невозможен
					return status_t::ERROR;
				// Устанавливаем флаг наличия параметра
				output.hasOdcid = true;
				// Устанавливаем длину идентификатора соединения
				output.odcid.size = static_cast <size_t> (length);
				// Если идентификатор не пустой
				if(length > 0)
					// Копируем данные идентификатора соединения
					::memcpy(output.odcid.data, value, static_cast <size_t> (length));
			} break;
			// Таймаут простоя соединения
			case static_cast <uint64_t> (id_t::MAX_IDLE_TIMEOUT): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.maxIdleTimeout))
					// Разбор невозможен
					return status_t::ERROR;
			} break;
			// Токен сброса без сохранения состояния
			case static_cast <uint64_t> (id_t::STATELESS_RESET_TOKEN): {
				// Если параметр прислал клиент или длина не соответствует размеру токена
				if((sender == endpoint_t::CLIENT) || (length != proto::RESET_TOKEN_SIZE))
					// Разбор невозможен
					return status_t::ERROR;
				// Устанавливаем флаг наличия параметра
				output.hasResetToken = true;
				// Копируем токен сброса без сохранения состояния
				::memcpy(output.resetToken, value, proto::RESET_TOKEN_SIZE);
			} break;
			// Максимальный размер принимаемой UDP-нагрузки
			case static_cast <uint64_t> (id_t::MAX_UDP_PAYLOAD_SIZE): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.maxUdpPayloadSize))
					// Разбор невозможен
					return status_t::ERROR;
				// Если значение ниже минимально допустимого (RFC 9000 §18.2)
				if(output.maxUdpPayloadSize < MIN_MAX_UDP_PAYLOAD_SIZE)
					// Разбор невозможен
					return status_t::ERROR;
			} break;
			// Начальный лимит данных соединения
			case static_cast <uint64_t> (id_t::INITIAL_MAX_DATA): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.initialMaxData))
					// Разбор невозможен
					return status_t::ERROR;
			} break;
			// Начальный лимит данных локально инициируемых двунаправленных потоков
			case static_cast <uint64_t> (id_t::INITIAL_MAX_STREAM_DATA_BIDI_LOCAL): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.initialMaxStreamDataBidiLocal))
					// Разбор невозможен
					return status_t::ERROR;
			} break;
			// Начальный лимит данных удалённо инициируемых двунаправленных потоков
			case static_cast <uint64_t> (id_t::INITIAL_MAX_STREAM_DATA_BIDI_REMOTE): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.initialMaxStreamDataBidiRemote))
					// Разбор невозможен
					return status_t::ERROR;
			} break;
			// Начальный лимит данных однонаправленных потоков
			case static_cast <uint64_t> (id_t::INITIAL_MAX_STREAM_DATA_UNI): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.initialMaxStreamDataUni))
					// Разбор невозможен
					return status_t::ERROR;
			} break;
			// Начальный лимит числа двунаправленных потоков
			case static_cast <uint64_t> (id_t::INITIAL_MAX_STREAMS_BIDI): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.initialMaxStreamsBidi))
					// Разбор невозможен
					return status_t::ERROR;
				// Если значение превышает предел числа потоков в транспортном параметре (RFC 9000 §4.6)
				if(output.initialMaxStreamsBidi > MAX_STREAMS_LIMIT){
					// Устанавливаем код ошибки транспортного параметра (для фрейма был бы FRAME_ENCODING_ERROR)
					error = error_t::TRANSPORT_PARAMETER_ERROR;
					// Разбор невозможен
					return status_t::ERROR;
				}
			} break;
			// Начальный лимит числа однонаправленных потоков
			case static_cast <uint64_t> (id_t::INITIAL_MAX_STREAMS_UNI): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.initialMaxStreamsUni))
					// Разбор невозможен
					return status_t::ERROR;
				// Если значение превышает предел числа потоков в транспортном параметре (RFC 9000 §4.6)
				if(output.initialMaxStreamsUni > MAX_STREAMS_LIMIT){
					// Устанавливаем код ошибки транспортного параметра (для фрейма был бы FRAME_ENCODING_ERROR)
					error = error_t::TRANSPORT_PARAMETER_ERROR;
					// Разбор невозможен
					return status_t::ERROR;
				}
			} break;
			// Показатель степени задержки подтверждения
			case static_cast <uint64_t> (id_t::ACK_DELAY_EXPONENT): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.ackDelayExponent))
					// Разбор невозможен
					return status_t::ERROR;
				// Если значение превышает максимально допустимое (RFC 9000 §18.2)
				if(output.ackDelayExponent > MAX_ACK_DELAY_EXPONENT)
					// Разбор невозможен
					return status_t::ERROR;
			} break;
			// Максимальная задержка подтверждения
			case static_cast <uint64_t> (id_t::MAX_ACK_DELAY): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.maxAckDelay))
					// Разбор невозможен
					return status_t::ERROR;
				// Если значение достигает верхней границы (RFC 9000 §18.2)
				if(output.maxAckDelay >= MAX_MAX_ACK_DELAY)
					// Разбор невозможен
					return status_t::ERROR;
			} break;
			// Запрет активной миграции соединения
			case static_cast <uint64_t> (id_t::DISABLE_ACTIVE_MIGRATION): {
				// Если параметр содержит значение (RFC 9000 §18.2)
				if(length != 0)
					// Разбор невозможен
					return status_t::ERROR;
				// Устанавливаем флаг запрета активной миграции
				output.disableActiveMigration = true;
			} break;
			// Предпочтительный адрес сервера
			case static_cast <uint64_t> (id_t::PREFERRED_ADDRESS): {
				// Если параметр прислал клиент или значение короче фиксированной части
				if((sender == endpoint_t::CLIENT) || (length < (PREFERRED_ADDRESS_FIXED_SIZE + proto::RESET_TOKEN_SIZE)))
					// Разбор невозможен
					return status_t::ERROR;
				// Смещение разбора значения параметра
				size_t position = 0;
				// Копируем IPv4-адрес сервера
				::memcpy(output.preferredAddress.ipv4, value + position, 4);
				// Сдвигаем смещение за IPv4-адресом
				position += 4;
				// Читаем порт IPv4-адреса сервера
				output.preferredAddress.ipv4Port = rd16(value + position);
				// Сдвигаем смещение за портом
				position += 2;
				// Копируем IPv6-адрес сервера
				::memcpy(output.preferredAddress.ipv6, value + position, 16);
				// Сдвигаем смещение за IPv6-адресом
				position += 16;
				// Читаем порт IPv6-адреса сервера
				output.preferredAddress.ipv6Port = rd16(value + position);
				// Сдвигаем смещение за портом
				position += 2;
				// Извлекаем длину идентификатора соединения
				const size_t cidLength = static_cast <size_t> (value[position]);
				// Сдвигаем смещение за октетом длины
				position += 1;
				// Если длина идентификатора вне диапазона 1-20 (RFC 9000 §18.2)
				if((cidLength < 1) || (cidLength > proto::MAX_CID_SIZE))
					// Разбор невозможен
					return status_t::ERROR;
				// Если значение не соответствует объявленной длине идентификатора
				if(length != (position + cidLength + proto::RESET_TOKEN_SIZE))
					// Разбор невозможен
					return status_t::ERROR;
				// Устанавливаем длину идентификатора соединения
				output.preferredAddress.cid.size = cidLength;
				// Копируем данные идентификатора соединения
				::memcpy(output.preferredAddress.cid.data, value + position, cidLength);
				// Сдвигаем смещение за идентификатором
				position += cidLength;
				// Копируем токен сброса без сохранения состояния
				::memcpy(output.preferredAddress.resetToken, value + position, proto::RESET_TOKEN_SIZE);
				// Устанавливаем флаг наличия параметра
				output.hasPreferredAddress = true;
			} break;
			// Предельный размер принимаемого фрейма DATAGRAM (RFC 9221 §3)
			case static_cast <uint64_t> (id_t::MAX_DATAGRAM_FRAME_SIZE): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.maxDatagramFrameSize))
					// Разбор невозможен
					return status_t::ERROR;
			} break;
			// Лимит активных идентификаторов соединения
			case static_cast <uint64_t> (id_t::ACTIVE_CONNECTION_ID_LIMIT): {
				// Читаем значение параметра
				if(!rdValue(value, static_cast <size_t> (length), output.activeConnectionIdLimit))
					// Разбор невозможен
					return status_t::ERROR;
				// Если значение ниже минимально допустимого (RFC 9000 §18.2)
				if(output.activeConnectionIdLimit < DEFAULT_ACTIVE_CID_LIMIT)
					// Разбор невозможен
					return status_t::ERROR;
			} break;
			// SCID первого пакета эндпоинта
			case static_cast <uint64_t> (id_t::INITIAL_SOURCE_CONNECTION_ID): {
				// Если длина превышает лимит QUIC v1
				if(length > proto::MAX_CID_SIZE)
					// Разбор невозможен
					return status_t::ERROR;
				// Устанавливаем флаг наличия параметра
				output.hasInitialScid = true;
				// Устанавливаем длину идентификатора соединения
				output.initialScid.size = static_cast <size_t> (length);
				// Если идентификатор не пустой
				if(length > 0)
					// Копируем данные идентификатора соединения
					::memcpy(output.initialScid.data, value, static_cast <size_t> (length));
			} break;
			// SCID пакета Retry
			case static_cast <uint64_t> (id_t::RETRY_SOURCE_CONNECTION_ID): {
				// Если параметр прислал клиент или длина превышает лимит QUIC v1
				if((sender == endpoint_t::CLIENT) || (length > proto::MAX_CID_SIZE))
					// Разбор невозможен
					return status_t::ERROR;
				// Устанавливаем флаг наличия параметра
				output.hasRetryScid = true;
				// Устанавливаем длину идентификатора соединения
				output.retryScid.size = static_cast <size_t> (length);
				// Если идентификатор не пустой
				if(length > 0)
					// Копируем данные идентификатора соединения
					::memcpy(output.retryScid.data, value, static_cast <size_t> (length));
			} break;
		}
	}
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция сборки параметров транспорта для TLS-расширения
 *
 * @param output выходной буфер значения TLS-расширения
 * @param params параметры транспорта
 * @param sender роль эндпоинта, кодирующего параметры
 * @return       результат сборки (false - некорректные параметры)
 *
 */
bool awh::quic::params::serialize::encode(string & output, const params_t & params, const endpoint_t sender) noexcept {
	// Если клиент пытается закодировать параметры только для сервера (RFC 9000 §18.2)
	if((sender == endpoint_t::CLIENT) && (params.hasOdcid || params.hasResetToken || params.hasRetryScid || params.hasPreferredAddress))
		// Сборка невозможна
		return false;
	// Если значения скалярных параметров вне допустимых границ
	if((params.ackDelayExponent > MAX_ACK_DELAY_EXPONENT) || (params.maxAckDelay >= MAX_MAX_ACK_DELAY) ||
	   (params.maxUdpPayloadSize < MIN_MAX_UDP_PAYLOAD_SIZE) || (params.activeConnectionIdLimit < DEFAULT_ACTIVE_CID_LIMIT) ||
	   (params.initialMaxStreamsBidi > MAX_STREAMS_LIMIT) || (params.initialMaxStreamsUni > MAX_STREAMS_LIMIT))
		// Сборка невозможна
		return false;
	// Если длина идентификаторов соединения превышает лимит QUIC v1
	if((params.hasOdcid && (params.odcid.size > proto::MAX_CID_SIZE)) ||
	   (params.hasInitialScid && (params.initialScid.size > proto::MAX_CID_SIZE)) ||
	   (params.hasRetryScid && (params.retryScid.size > proto::MAX_CID_SIZE)))
		// Сборка невозможна
		return false;
	// Если предпочтительный адрес содержит некорректный идентификатор соединения
	if(params.hasPreferredAddress && ((params.preferredAddress.cid.size < 1) || (params.preferredAddress.cid.size > proto::MAX_CID_SIZE)))
		// Сборка невозможна
		return false;
	// Если задан исходный DCID первого пакета Initial клиента
	if(params.hasOdcid)
		// Дописываем параметр исходного DCID
		wrCid(output, id_t::ORIGINAL_DESTINATION_CONNECTION_ID, params.odcid);
	// Если задан таймаут простоя соединения
	if(params.maxIdleTimeout != 0)
		// Дописываем параметр таймаута простоя
		wrScalar(output, id_t::MAX_IDLE_TIMEOUT, params.maxIdleTimeout);
	// Если задан токен сброса без сохранения состояния
	if(params.hasResetToken){
		// Дописываем идентификатор параметра
		varint::write(output, static_cast <uint64_t> (id_t::STATELESS_RESET_TOKEN));
		// Дописываем длину токена сброса
		varint::write(output, proto::RESET_TOKEN_SIZE);
		// Дописываем токен сброса без сохранения состояния
		output.append(reinterpret_cast <const char *> (params.resetToken), proto::RESET_TOKEN_SIZE);
	}
	// Если максимальный размер UDP-нагрузки отличается от значения по умолчанию
	if(params.maxUdpPayloadSize != DEFAULT_MAX_UDP_PAYLOAD_SIZE)
		// Дописываем параметр максимального размера UDP-нагрузки
		wrScalar(output, id_t::MAX_UDP_PAYLOAD_SIZE, params.maxUdpPayloadSize);
	// Если задан начальный лимит данных соединения
	if(params.initialMaxData != 0)
		// Дописываем параметр начального лимита данных
		wrScalar(output, id_t::INITIAL_MAX_DATA, params.initialMaxData);
	// Если задан начальный лимит данных локально инициируемых двунаправленных потоков
	if(params.initialMaxStreamDataBidiLocal != 0)
		// Дописываем параметр лимита данных потоков
		wrScalar(output, id_t::INITIAL_MAX_STREAM_DATA_BIDI_LOCAL, params.initialMaxStreamDataBidiLocal);
	// Если задан начальный лимит данных удалённо инициируемых двунаправленных потоков
	if(params.initialMaxStreamDataBidiRemote != 0)
		// Дописываем параметр лимита данных потоков
		wrScalar(output, id_t::INITIAL_MAX_STREAM_DATA_BIDI_REMOTE, params.initialMaxStreamDataBidiRemote);
	// Если задан начальный лимит данных однонаправленных потоков
	if(params.initialMaxStreamDataUni != 0)
		// Дописываем параметр лимита данных потоков
		wrScalar(output, id_t::INITIAL_MAX_STREAM_DATA_UNI, params.initialMaxStreamDataUni);
	// Если задан начальный лимит числа двунаправленных потоков
	if(params.initialMaxStreamsBidi != 0)
		// Дописываем параметр лимита числа потоков
		wrScalar(output, id_t::INITIAL_MAX_STREAMS_BIDI, params.initialMaxStreamsBidi);
	// Если задан начальный лимит числа однонаправленных потоков
	if(params.initialMaxStreamsUni != 0)
		// Дописываем параметр лимита числа потоков
		wrScalar(output, id_t::INITIAL_MAX_STREAMS_UNI, params.initialMaxStreamsUni);
	// Если показатель степени задержки подтверждения отличается от значения по умолчанию
	if(params.ackDelayExponent != DEFAULT_ACK_DELAY_EXPONENT)
		// Дописываем параметр показателя степени задержки
		wrScalar(output, id_t::ACK_DELAY_EXPONENT, params.ackDelayExponent);
	// Если максимальная задержка подтверждения отличается от значения по умолчанию
	if(params.maxAckDelay != DEFAULT_MAX_ACK_DELAY)
		// Дописываем параметр максимальной задержки подтверждения
		wrScalar(output, id_t::MAX_ACK_DELAY, params.maxAckDelay);
	// Если установлен запрет активной миграции соединения
	if(params.disableActiveMigration){
		// Дописываем идентификатор параметра
		varint::write(output, static_cast <uint64_t> (id_t::DISABLE_ACTIVE_MIGRATION));
		// Дописываем нулевую длину значения
		varint::write(output, 0);
	}
	// Если задан предпочтительный адрес сервера
	if(params.hasPreferredAddress){
		// Дописываем идентификатор параметра
		varint::write(output, static_cast <uint64_t> (id_t::PREFERRED_ADDRESS));
		// Дописываем длину значения параметра
		varint::write(output, PREFERRED_ADDRESS_FIXED_SIZE + params.preferredAddress.cid.size + proto::RESET_TOKEN_SIZE);
		// Дописываем IPv4-адрес сервера
		output.append(reinterpret_cast <const char *> (params.preferredAddress.ipv4), 4);
		// Дописываем порт IPv4-адреса сервера
		wr16(output, params.preferredAddress.ipv4Port);
		// Дописываем IPv6-адрес сервера
		output.append(reinterpret_cast <const char *> (params.preferredAddress.ipv6), 16);
		// Дописываем порт IPv6-адреса сервера
		wr16(output, params.preferredAddress.ipv6Port);
		// Дописываем октет длины идентификатора соединения
		output.push_back(static_cast <char> (params.preferredAddress.cid.size));
		// Дописываем данные идентификатора соединения
		output.append(reinterpret_cast <const char *> (params.preferredAddress.cid.data), params.preferredAddress.cid.size);
		// Дописываем токен сброса без сохранения состояния
		output.append(reinterpret_cast <const char *> (params.preferredAddress.resetToken), proto::RESET_TOKEN_SIZE);
	}
	/**
	 * Если приём фреймов DATAGRAM поддерживается: нулевое значение равносильно
	 * отсутствию параметра, поэтому не кодируется (RFC 9221 §3)
	 */
	if(params.maxDatagramFrameSize > 0)
		// Дописываем параметр предельного размера принимаемого фрейма DATAGRAM
		wrScalar(output, id_t::MAX_DATAGRAM_FRAME_SIZE, params.maxDatagramFrameSize);
	// Если лимит активных идентификаторов отличается от значения по умолчанию
	if(params.activeConnectionIdLimit != DEFAULT_ACTIVE_CID_LIMIT)
		// Дописываем параметр лимита активных идентификаторов
		wrScalar(output, id_t::ACTIVE_CONNECTION_ID_LIMIT, params.activeConnectionIdLimit);
	// Если задан SCID первого пакета эндпоинта
	if(params.hasInitialScid)
		// Дописываем параметр SCID первого пакета
		wrCid(output, id_t::INITIAL_SOURCE_CONNECTION_ID, params.initialScid);
	// Если задан SCID пакета Retry
	if(params.hasRetryScid)
		// Дописываем параметр SCID пакета Retry
		wrCid(output, id_t::RETRY_SOURCE_CONNECTION_ID, params.retryScid);
	// Сборка выполнена успешно
	return true;
}
/**
 * @brief Функция сборки контекста ранних данных из параметров транспорта (RFC 9001 §4.6.1)
 *
 * @param output выходной буфер контекста ранних данных
 * @param params параметры транспорта
 * @return       результат сборки (false - некорректные параметры)
 *
 */
bool awh::quic::params::serialize::early(string & output, const params_t & params) noexcept {
	/**
	 * Список лимитов, запоминаемых клиентом вместе с билетом возобновления
	 * в порядке кодирования (RFC 9000 §7.4.1)
	 */
	const uint64_t values[] = {
		params.initialMaxData,
		params.initialMaxStreamDataBidiLocal,
		params.initialMaxStreamDataBidiRemote,
		params.initialMaxStreamDataUni,
		params.initialMaxStreamsBidi,
		params.initialMaxStreamsUni,
		params.activeConnectionIdLimit,
		/**
		 * Лимит размера фрейма датаграммы также запоминается: при 0-RTT клиент вправе
		 * отправлять DATAGRAM под запомненный лимит, и его снижение сервером при
		 * возобновлении обязано отклонить ранние данные несовпадением контекста (RFC 9221 §3)
		 */
		params.maxDatagramFrameSize
	};
	/**
	 * Перебираем список запоминаемых лимитов
	 */
	for(auto & value : values){
		// Если запись очередного лимита не выполнена
		if(varint::write(output, value) == 0)
			// Выводим отрицательный результат - лимит не кодируется
			return false;
	}
	// Сборка выполнена успешно
	return true;
}
