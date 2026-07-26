/**
 * @file: packet.cpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация слоя пакетов QUIC (RFC 9000 §17) — разбор и сборка заголовков длинной и короткой формы,
 *        включая Initial, Handshake, 0-RTT, Retry и Version Negotiation,
 *        и извлечение номера пакета после снятия защиты
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
#include <proto/quic/packet.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние вспомогательные функции слоя пакетов
 *
 */
namespace {
	/**
	 * Используем пространство имён протокола QUIC
	 */
	using namespace awh::quic;

	/**
	 * @brief Функция чтения 32-битного числа в сетевом порядке байт
	 *
	 * @param buffer буфер с данными числа
	 * @return       прочитанное число
	 *
	 */
	inline uint32_t rd32(const uint8_t * buffer) noexcept {
		// Собираем число из четырёх байт в сетевом порядке
		return (static_cast <uint32_t> (buffer[0]) << 24) | (static_cast <uint32_t> (buffer[1]) << 16) |
		       (static_cast <uint32_t> (buffer[2]) << 8)  |  static_cast <uint32_t> (buffer[3]);
	}
	/**
	 * @brief Функция записи 32-битного числа в сетевом порядке байт
	 *
	 * @param output выходной буфер
	 * @param num    записываемое число
	 *
	 */
	inline void wr32(string & output, const uint32_t num) noexcept {
		// Дописываем старший байт числа
		output.push_back(static_cast <char> ((num >> 24) & 0xFF));
		// Дописываем второй байт числа
		output.push_back(static_cast <char> ((num >> 16) & 0xFF));
		// Дописываем третий байт числа
		output.push_back(static_cast <char> ((num >> 8) & 0xFF));
		// Дописываем младший байт числа
		output.push_back(static_cast <char> (num & 0xFF));
	}
	/**
	 * @brief Функция чтения идентификатора соединения с октетом длины
	 *
	 * @param data   входной буфер
	 * @param size   доступно байт
	 * @param output идентификатор соединения
	 * @param valid  флаг проверки длины по лимиту QUIC v1 (20 октетов)
	 * @return       количество прочитанных октетов или 0, если данных недостаточно или длина некорректна
	 *
	 */
	inline size_t rdCid(const uint8_t * data, const size_t size, cid_t & output, bool & valid) noexcept {
		// Считаем длину корректной по умолчанию
		valid = true;
		// Если в буфере нет даже октета длины
		if(size < 1)
			// Данных недостаточно
			return 0;
		// Извлекаем длину идентификатора соединения
		const size_t length = static_cast <size_t> (data[0]);
		// Если длина превышает лимит QUIC v1
		if(length > proto::MAX_CID_SIZE){
			// Длина идентификатора некорректна
			valid = false;
			// Разбор невозможен
			return 0;
		}
		// Если идентификатор целиком не помещается в буфер
		if(size < (1 + length))
			// Данных недостаточно
			return 0;
		// Устанавливаем длину идентификатора соединения
		output.size = length;
		// Если идентификатор не пустой
		if(length > 0)
			// Копируем данные идентификатора соединения
			::memcpy(output.data, data + 1, length);
		// Выводим количество прочитанных октетов
		return (1 + length);
	}
	/**
	 * @brief Функция записи идентификатора соединения с октетом длины
	 *
	 * @param output выходной буфер
	 * @param cid    идентификатор соединения
	 *
	 */
	inline void wrCid(string & output, const cid_t & cid) noexcept {
		// Дописываем октет длины идентификатора соединения
		output.push_back(static_cast <char> (cid.size));
		// Если идентификатор не пустой
		if(cid.size > 0)
			// Дописываем данные идентификатора соединения
			output.append(reinterpret_cast <const char *> (cid.data), cid.size);
	}
	/**
	 * @brief Функция записи усечённого номера пакета в сетевом порядке байт
	 *
	 * @param output выходной буфер
	 * @param pn     номер пакета
	 * @param pnSize размер кодирования номера пакета в октетах (1-4)
	 *
	 */
	inline void wrPacketNumber(string & output, const uint64_t pn, const size_t pnSize) noexcept {
		/**
		 * Перебираем октеты номера пакета от старшего к младшему
		 */
		for(size_t i = pnSize; i > 0; i--)
			// Дописываем очередной октет номера пакета
			output.push_back(static_cast <char> ((pn >> ((i - 1) * 8)) & 0xFF));
	}
};

/**
 * @brief Конструктор
 *
 */
awh::quic::packet::Header::Header() noexcept :
 type(packet_t::UNKNOWN), first(0), version(0),
 token{""}, length(0), pnOffset(0), size(0), payload{""} {}

/**
 * @brief Функция выбора размера кодирования номера пакета (RFC 9000 §A.2)
 *
 * @param pn           номер отправляемого пакета
 * @param largestAcked наибольший подтверждённый пиром номер пакета (pn, если подтверждений ещё не было)
 * @return             размер кодирования номера пакета в октетах (1-4)
 *
 */
size_t awh::quic::packet::packetNumberSize(const uint64_t pn, const uint64_t largestAcked) noexcept {
	// Количество неподтверждённых пакетов (если подтверждений не было - весь диапазон от нуля)
	const uint64_t unacked = ((pn >= largestAcked) ? ((pn - largestAcked) + 1) : (pn + 1));
	// Если диапазон представим семью битами
	if(unacked < (static_cast <uint64_t> (1) << 7))
		// Кодирование одним октетом
		return 1;
	// Если диапазон представим пятнадцатью битами
	if(unacked < (static_cast <uint64_t> (1) << 15))
		// Кодирование двумя октетами
		return 2;
	// Если диапазон представим двадцатью тремя битами
	if(unacked < (static_cast <uint64_t> (1) << 23))
		// Кодирование тремя октетами
		return 3;
	// Кодирование четырьмя октетами
	return 4;
}
/**
 * @brief Функция восстановления полного номера пакета из усечённого (RFC 9000 §A.3)
 *
 * @param largestPn наибольший номер пакета, принятый в данном пространстве номеров
 * @param truncated усечённый номер пакета из заголовка
 * @param pnSize    размер усечённого номера пакета в октетах (1-4)
 * @return          восстановленный полный номер пакета
 *
 */
uint64_t awh::quic::packet::decodePacketNumber(const uint64_t largestPn, const uint64_t truncated, const size_t pnSize) noexcept {
	/**
	 * Отсекаем недопустимую длину номера пакета: сдвиг на pnSize*8 при pnSize вне 1..4
	 * даёт неопределённое поведение (сдвиг на >= 64). Штатные вызывающие длину уже
	 * ограничивают, проверка защищает экспортируемую функцию от прочих (RFC 9000 §17.1)
	 */
	if((pnSize < 1) || (pnSize > proto::MAX_PKT_NUM_SIZE))
		// Восстановить номер невозможно - возвращаем усечённое значение как есть
		return truncated;
	// Ожидаемый номер следующего пакета
	const uint64_t expected = (largestPn + 1);
	// Окно значений усечённого номера пакета
	const uint64_t window = (static_cast <uint64_t> (1) << (pnSize * 8));
	// Половина окна значений
	const uint64_t half = (window / 2);
	// Маска битов усечённого номера пакета
	const uint64_t mask = (window - 1);
	// Кандидат - ожидаемый номер с заменёнными младшими битами
	const uint64_t candidate = ((expected & ~mask) | truncated);
	// Если кандидат отстаёт от ожидаемого более чем на половину окна и есть окно выше
	if(((expected >= half) && (candidate <= (expected - half))) && (candidate < ((static_cast <uint64_t> (1) << 62) - window)))
		// Выбираем значение из следующего окна
		return (candidate + window);
	// Если кандидат опережает ожидаемый более чем на половину окна и есть окно ниже
	if((candidate > (expected + half)) && (candidate >= window))
		// Выбираем значение из предыдущего окна
		return (candidate - window);
	// Кандидат находится в допустимом окне
	return candidate;
}
/**
 * @brief Функция разбора заголовка пакета до границы защиты заголовка
 *
 * @param data     входная датаграмма (начало очередного пакета)
 * @param size     доступно байт
 * @param dcidSize длина идентификатора соединения локального эндпоинта (для короткого заголовка)
 * @param output   разобранный заголовок пакета
 * @param error    код ошибки транспорта
 * @return         результат разбора (OK/INCOMPLETE/ERROR)
 *
 */
awh::quic::status_t awh::quic::packet::parser::header(const uint8_t * data, const size_t size, const size_t dcidSize, header_t & output, error_t & error) noexcept {
	// Если в буфере нет даже первого октета
	if(size < 1)
		// Данных недостаточно
		return status_t::INCOMPLETE;
	// Запоминаем первый октет пакета
	output.first = data[0];
	// Если установлен бит длинного заголовка
	if((data[0] & 0x80) != 0){
		// Если в буфере нет первого октета и версии
		if(size < 5)
			// Данных недостаточно
			return status_t::INCOMPLETE;
		// Извлекаем версию протокола
		output.version = rd32(data + 1);
		// Смещение разбора за первым октетом и версией
		size_t offset = 5;
		// Флаг корректности длины идентификатора соединения
		bool valid = true;
		// Читаем идентификатор соединения получателя
		size_t count = rdCid(data + offset, size - offset, output.dcid, valid);
		// Если длина идентификатора некорректна
		if(!valid){
			// Устанавливаем код ошибки транспорта
			error = error_t::PROTOCOL_VIOLATION;
			// Разбор невозможен
			return status_t::ERROR;
		}
		// Если данных недостаточно
		if(count == 0)
			// Данных недостаточно
			return status_t::INCOMPLETE;
		// Сдвигаем смещение разбора
		offset += count;
		// Читаем идентификатор соединения отправителя
		count = rdCid(data + offset, size - offset, output.scid, valid);
		// Если длина идентификатора некорректна
		if(!valid){
			// Устанавливаем код ошибки транспорта
			error = error_t::PROTOCOL_VIOLATION;
			// Разбор невозможен
			return status_t::ERROR;
		}
		// Если данных недостаточно
		if(count == 0)
			// Данных недостаточно
			return status_t::INCOMPLETE;
		// Сдвигаем смещение разбора
		offset += count;
		// Если версия пакета - Version Negotiation
		if(output.version == proto::VERSION_NEGOTIATION){
			// Устанавливаем тип пакета
			output.type = packet_t::VERSION_NEGOTIATION;
			// Пакет занимает датаграмму целиком
			output.size = size;
			// Нагрузка пакета - список поддерживаемых версий
			output.payload = string_view(reinterpret_cast <const char *> (data + offset), size - offset);
			// Разбор завершён
			return status_t::OK;
		}
		// Если сброшен обязательный fixed-бит (RFC 9000 §17.2)
		if((data[0] & 0x40) == 0){
			// Устанавливаем код ошибки транспорта
			error = error_t::PROTOCOL_VIOLATION;
			// Разбор невозможен
			return status_t::ERROR;
		}
		/**
		 * Определяем тип пакета по битам 4-5 первого октета
		 */
		switch((data[0] & 0x30) >> 4){
			// Пакет установки соединения
			case 0x00: {
				// Устанавливаем тип пакета
				output.type = packet_t::INITIAL;
				// Длина токена пакета Initial
				uint64_t tokenLength = 0;
				// Читаем длину токена
				count = varint::read(data + offset, size - offset, tokenLength);
				// Если данных недостаточно
				if(count == 0)
					// Данных недостаточно
					return status_t::INCOMPLETE;
				// Сдвигаем смещение разбора
				offset += count;
				// Если токен целиком не помещается в буфер
				if(tokenLength > (size - offset))
					// Данных недостаточно
					return status_t::INCOMPLETE;
				// Извлекаем токен пакета Initial
				output.token = string_view(reinterpret_cast <const char *> (data + offset), static_cast <size_t> (tokenLength));
				// Сдвигаем смещение разбора за токеном
				offset += static_cast <size_t> (tokenLength);
			} break;
			// Пакет ранних данных клиента
			case 0x01:
				// Устанавливаем тип пакета
				output.type = packet_t::ZERO_RTT;
			break;
			// Пакет продолжения криптографического хендшейка
			case 0x02:
				// Устанавливаем тип пакета
				output.type = packet_t::HANDSHAKE;
			break;
			// Пакет проверки адреса клиента
			case 0x03: {
				// Устанавливаем тип пакета
				output.type = packet_t::RETRY;
				// Пакет занимает датаграмму целиком
				output.size = size;
				// Нагрузка пакета - токен с тегом целостности
				output.payload = string_view(reinterpret_cast <const char *> (data + offset), size - offset);
				// Разбор завершён
				return status_t::OK;
			}
		}
		// Читаем значение поля Length (номер пакета + защищённая нагрузка)
		count = varint::read(data + offset, size - offset, output.length);
		// Если данных недостаточно
		if(count == 0)
			// Данных недостаточно
			return status_t::INCOMPLETE;
		// Сдвигаем смещение разбора
		offset += count;
		// Устанавливаем смещение поля Packet Number (граница защиты заголовка)
		output.pnOffset = offset;
		// Если защищённая часть не помещается в датаграмму или короче минимального номера пакета
		if((output.length > (size - offset)) || (output.length < 1)){
			// Устанавливаем код ошибки транспорта
			error = error_t::PROTOCOL_VIOLATION;
			// Разбор невозможен
			return status_t::ERROR;
		}
		// Вычисляем полный размер пакета в датаграмме
		output.size = (offset + static_cast <size_t> (output.length));
		// Нагрузка пакета - защищённая часть (номер пакета + нагрузка)
		output.payload = string_view(reinterpret_cast <const char *> (data + offset), static_cast <size_t> (output.length));
		// Разбор завершён
		return status_t::OK;
	}
	// Если сброшен обязательный fixed-бит (RFC 9000 §17.3)
	if((data[0] & 0x40) == 0){
		// Устанавливаем код ошибки транспорта
		error = error_t::PROTOCOL_VIOLATION;
		// Разбор невозможен
		return status_t::ERROR;
	}
	// Устанавливаем тип пакета с коротким заголовком
	output.type = packet_t::ONE_RTT;
	// Если длина идентификатора соединения превышает лимит QUIC v1
	if(dcidSize > proto::MAX_CID_SIZE){
		// Устанавливаем код ошибки транспорта
		error = error_t::PROTOCOL_VIOLATION;
		// Разбор невозможен
		return status_t::ERROR;
	}
	// Если идентификатор соединения не помещается в буфер
	if(size < (1 + dcidSize))
		// Данных недостаточно
		return status_t::INCOMPLETE;
	// Устанавливаем длину идентификатора соединения получателя
	output.dcid.size = dcidSize;
	// Если идентификатор не пустой
	if(dcidSize > 0)
		// Копируем данные идентификатора соединения получателя
		::memcpy(output.dcid.data, data + 1, dcidSize);
	// Устанавливаем смещение поля Packet Number (граница защиты заголовка)
	output.pnOffset = (1 + dcidSize);
	// Пакет с коротким заголовком занимает датаграмму до конца
	output.size = size;
	// Нагрузка пакета - защищённая часть (номер пакета + нагрузка)
	output.payload = string_view(reinterpret_cast <const char *> (data + output.pnOffset), size - output.pnOffset);
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция чтения усечённого номера пакета (после снятия защиты заголовка)
 *
 * @param data   буфер, указывающий на поле Packet Number
 * @param size   доступно байт
 * @param pnSize размер номера пакета в октетах (1-4, из битов первого октета)
 * @param output прочитанный усечённый номер пакета
 * @return       результат чтения (true - в буфере было достаточно байт)
 *
 */
bool awh::quic::packet::parser::packetNumber(const uint8_t * data, const size_t size, const size_t pnSize, uint64_t & output) noexcept {
	// Если размер номера пакета некорректен или номер не помещается в буфер
	if((pnSize < 1) || (pnSize > proto::MAX_PKT_NUM_SIZE) || (size < pnSize))
		// Чтение невозможно
		return false;
	// Обнуляем прочитанный номер пакета
	output = 0;
	/**
	 * Перебираем октеты номера пакета
	 */
	for(size_t i = 0; i < pnSize; i++)
		// Дописываем очередной октет номера пакета в сетевом порядке
		output = ((output << 8) | data[i]);
	// Чтение выполнено успешно
	return true;
}
/**
 * @brief Функция разбора списка версий пакета Version Negotiation (RFC 9000 §17.2.1)
 *
 * @param header заголовок разобранного пакета Version Negotiation
 * @param output список поддерживаемых пиром версий
 * @param error  код ошибки транспорта
 * @return       результат разбора (OK/ERROR)
 *
 */
awh::quic::status_t awh::quic::packet::parser::versions(const header_t & header, vector <uint32_t> & output, error_t & error) noexcept {
	// Если тип пакета не соответствует или список версий пуст или не кратен четырём октетам
	if((header.type != packet_t::VERSION_NEGOTIATION) || header.payload.empty() || ((header.payload.size() % 4) != 0)){
		// Устанавливаем код ошибки транспорта
		error = error_t::PROTOCOL_VIOLATION;
		// Разбор невозможен
		return status_t::ERROR;
	}
	// Буфер списка версий
	const uint8_t * data = reinterpret_cast <const uint8_t *> (header.payload.data());
	/**
	 * Перебираем список поддерживаемых версий
	 */
	for(size_t offset = 0; offset < header.payload.size(); offset += 4)
		// Дописываем очередную версию в список
		output.push_back(rd32(data + offset));
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция разбора нагрузки пакета Retry (RFC 9000 §17.2.5)
 *
 * @param header заголовок разобранного пакета Retry
 * @param token  токен для повторного пакета Initial (zero-copy во входную датаграмму)
 * @param tag    тег целостности пакета Retry (16 октетов)
 * @param error  код ошибки транспорта
 * @return       результат разбора (OK/ERROR)
 *
 */
awh::quic::status_t awh::quic::packet::parser::retry(const header_t & header, string_view & token, uint8_t tag[proto::RETRY_TAG_SIZE], error_t & error) noexcept {
	// Если тип пакета не соответствует или нагрузка не содержит тег целостности и хотя бы октет токена
	if((header.type != packet_t::RETRY) || (header.payload.size() <= proto::RETRY_TAG_SIZE)){
		// Устанавливаем код ошибки транспорта
		error = error_t::PROTOCOL_VIOLATION;
		// Разбор невозможен
		return status_t::ERROR;
	}
	// Извлекаем токен для повторного пакета Initial
	token = header.payload.substr(0, header.payload.size() - proto::RETRY_TAG_SIZE);
	// Копируем тег целостности пакета Retry
	::memcpy(tag, header.payload.data() + (header.payload.size() - proto::RETRY_TAG_SIZE), proto::RETRY_TAG_SIZE);
	// Разбор завершён
	return status_t::OK;
}
/**
 * @brief Функция сборки длинного заголовка пакета с номером пакета (RFC 9000 §17.2)
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
 */
bool awh::quic::packet::serialize::longHeader(string & output, const packet_t type, const uint32_t version, const cid_t & dcid, const cid_t & scid, string_view token, const uint64_t length, const uint64_t pn, const size_t pnSize) noexcept {
	// Если размер кодирования номера пакета некорректен
	if((pnSize < 1) || (pnSize > proto::MAX_PKT_NUM_SIZE))
		// Сборка невозможна
		return false;
	// Если длина идентификаторов соединения превышает лимит QUIC v1
	if((dcid.size > proto::MAX_CID_SIZE) || (scid.size > proto::MAX_CID_SIZE))
		// Сборка невозможна
		return false;
	// Если поле Length не включает даже номер пакета или превышает лимит кодирования
	if((length < pnSize) || (length > proto::VARINT_MAX))
		// Сборка невозможна
		return false;
	// Биты типа пакета в первом октете
	uint8_t typeBits = 0;
	/**
	 * Определяем тип собираемого пакета
	 */
	switch(static_cast <uint8_t> (type)){
		// Пакет установки соединения
		case static_cast <uint8_t> (packet_t::INITIAL):
			// Устанавливаем биты типа пакета
			typeBits = 0x00;
		break;
		// Пакет ранних данных клиента
		case static_cast <uint8_t> (packet_t::ZERO_RTT):
			// Устанавливаем биты типа пакета
			typeBits = 0x01;
		break;
		// Пакет продолжения криптографического хендшейка
		case static_cast <uint8_t> (packet_t::HANDSHAKE):
			// Устанавливаем биты типа пакета
			typeBits = 0x02;
		break;
		// Остальные типы пакетов собираются другими функциями
		default:
			// Сборка невозможна
			return false;
	}
	// Если токен передан для пакета, отличного от Initial
	if(!token.empty() && (type != packet_t::INITIAL))
		// Сборка невозможна
		return false;
	// Дописываем первый октет: форма, fixed-бит, тип и биты размера номера пакета
	output.push_back(static_cast <char> (0xC0 | (typeBits << 4) | static_cast <uint8_t> (pnSize - 1)));
	// Дописываем версию протокола
	wr32(output, version);
	// Дописываем идентификатор соединения получателя
	wrCid(output, dcid);
	// Дописываем идентификатор соединения отправителя
	wrCid(output, scid);
	// Если собирается пакет Initial
	if(type == packet_t::INITIAL){
		// Дописываем длину токена
		varint::write(output, token.size());
		// Если токен не пустой
		if(!token.empty())
			// Дописываем токен пакета Initial
			output.append(token);
	}
	// Дописываем значение поля Length
	varint::write(output, length);
	// Дописываем усечённый номер пакета
	wrPacketNumber(output, pn, pnSize);
	// Сборка выполнена успешно
	return true;
}
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
 *
 */
bool awh::quic::packet::serialize::shortHeader(string & output, const cid_t & dcid, const uint64_t pn, const size_t pnSize, const bool keyPhase, const bool spin) noexcept {
	// Если размер кодирования номера пакета или длина идентификатора некорректны
	if((pnSize < 1) || (pnSize > proto::MAX_PKT_NUM_SIZE) || (dcid.size > proto::MAX_CID_SIZE))
		// Сборка невозможна
		return false;
	// Первый октет: fixed-бит и биты размера номера пакета
	uint8_t first = (0x40 | static_cast <uint8_t> (pnSize - 1));
	// Если установлен бит задержки
	if(spin)
		// Устанавливаем spin-бит
		first |= 0x20;
	// Если установлен бит фазы ключей
	if(keyPhase)
		// Устанавливаем бит фазы ключей
		first |= 0x04;
	// Дописываем первый октет пакета
	output.push_back(static_cast <char> (first));
	// Если идентификатор соединения не пустой
	if(dcid.size > 0)
		// Дописываем идентификатор соединения получателя (без октета длины)
		output.append(reinterpret_cast <const char *> (dcid.data), dcid.size);
	// Дописываем усечённый номер пакета
	wrPacketNumber(output, pn, pnSize);
	// Сборка выполнена успешно
	return true;
}
/**
 * @brief Функция сборки пакета Version Negotiation целиком (RFC 9000 §17.2.1)
 *
 * @param output   выходной буфер
 * @param dcid     идентификатор соединения получателя (SCID пакета клиента)
 * @param scid     идентификатор соединения отправителя (DCID пакета клиента)
 * @param versions список поддерживаемых версий
 * @param count    количество версий
 * @return         результат сборки (false - некорректные параметры)
 *
 */
bool awh::quic::packet::serialize::versionNegotiation(string & output, const cid_t & dcid, const cid_t & scid, const uint32_t * versions, const size_t count) noexcept {
	// Если список версий пуст или длина идентификаторов превышает лимит QUIC v1
	if((count == 0) || (versions == nullptr) || (dcid.size > proto::MAX_CID_SIZE) || (scid.size > proto::MAX_CID_SIZE))
		// Сборка невозможна
		return false;
	// Дописываем первый октет пакета (форма и fixed-бит)
	output.push_back(static_cast <char> (0xC0));
	// Дописываем нулевую версию протокола
	wr32(output, proto::VERSION_NEGOTIATION);
	// Дописываем идентификатор соединения получателя
	wrCid(output, dcid);
	// Дописываем идентификатор соединения отправителя
	wrCid(output, scid);
	/**
	 * Перебираем список поддерживаемых версий
	 */
	for(size_t i = 0; i < count; i++)
		// Дописываем очередную версию
		wr32(output, versions[i]);
	// Сборка выполнена успешно
	return true;
}
/**
 * @brief Функция сборки пакета Retry целиком (RFC 9000 §17.2.5)
 *
 * @param output  выходной буфер
 * @param version версия протокола
 * @param dcid    идентификатор соединения получателя (SCID пакета клиента)
 * @param scid    новый идентификатор соединения отправителя
 * @param token   токен для повторного пакета Initial
 * @param tag     тег целостности пакета Retry (16 октетов)
 * @return        результат сборки (false - некорректные параметры)
 *
 */
bool awh::quic::packet::serialize::retry(string & output, const uint32_t version, const cid_t & dcid, const cid_t & scid, string_view token, const uint8_t tag[proto::RETRY_TAG_SIZE]) noexcept {
	// Если токен пуст или длина идентификаторов превышает лимит QUIC v1
	if(token.empty() || (tag == nullptr) || (dcid.size > proto::MAX_CID_SIZE) || (scid.size > proto::MAX_CID_SIZE))
		// Сборка невозможна
		return false;
	// Дописываем первый октет пакета (форма, fixed-бит и тип Retry)
	output.push_back(static_cast <char> (0xF0));
	// Дописываем версию протокола
	wr32(output, version);
	// Дописываем идентификатор соединения получателя
	wrCid(output, dcid);
	// Дописываем идентификатор соединения отправителя
	wrCid(output, scid);
	// Дописываем токен для повторного пакета Initial
	output.append(token);
	// Дописываем тег целостности пакета Retry
	output.append(reinterpret_cast <const char *> (tag), proto::RETRY_TAG_SIZE);
	// Сборка выполнена успешно
	return true;
}
