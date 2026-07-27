/**
 * @file: quic.cpp
 * @date: 2026-07-21
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация общих функций протокола QUIC — генерация и проверка идентификаторов соединения,
 *        расчёт токенов сброса и Retry-целостности, а также человекочитаемые названия типов пакетов,
 *        фреймов и кодов ошибок
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Подключаем заголовочные файлы криптографической библиотеки
 */
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/digest.h>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/quic2/quic.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Конструктор
 *
 */
awh::quic2::Cid::Cid() noexcept : size(0), data{0} {}

/**
 * @brief Оператор сравнения идентификаторов соединения
 *
 * @param a первый идентификатор соединения
 * @param b второй идентификатор соединения
 * @return  результат сравнения
 *
 */
bool awh::quic2::operator == (const cid_t & a, const cid_t & b) noexcept {
	// Идентификаторы равны при совпадении длины и содержимого
	return ((a.size == b.size) && (::memcmp(a.data, b.data, a.size) == 0));
}
/**
 * @brief Функция извлечения идентификатора соединения получателя из датаграммы (RFC 9000 §17.2)
 *
 * @param data   данные датаграммы
 * @param size   размер датаграммы
 * @param length длина идентификаторов, выдаваемых локальным эндпоинтом
 * @param key    извлечённый идентификатор соединения получателя
 * @return       результат извлечения (false - датаграмма не является пакетом QUIC)
 *
 */
bool awh::quic2::route(const uint8_t * data, const size_t size, const uint8_t length, net::origin_key_t & key) noexcept {
	// Если данные датаграммы не переданы либо она пустая
	if((data == nullptr) || (size == 0))
		// Выводим отрицательный результат
		return false;
	/**
	 * Если старший бит первого октета установлен, пакет несёт длинный заголовок:
	 * версия занимает четыре октета, за ней следует октет длины идентификатора
	 * соединения получателя и сам идентификатор
	 */
	if(data[0] & 0x80){
		// Если датаграмма короче фиксированной части длинного заголовка
		if(size < 6)
			// Выводим отрицательный результат
			return false;
		// Извлекаем длину идентификатора соединения получателя
		const uint8_t size_ = data[5];
		// Если длина идентификатора превышает допустимую либо не укладывается в датаграмму
		if((size_ > proto::MAX_CID_SIZE) || (size < (6u + static_cast <size_t> (size_))))
			// Выводим отрицательный результат
			return false;
		// Если идентификатор соединения получателя пустой
		if(size_ == 0)
			/**
			 * Выводим отрицательный результат: нулевой идентификатор допустим
			 * протоколом, но маршрутизировать по нему нечего - такой пакет
			 * адресован эндпоинту, который идентификаторов не выдаёт
			 */
			return false;
		// Формируем ключ маршрутизации из идентификатора соединения получателя
		key = net::origin_key_t(data + 6, size_);
		// Выводим положительный результат
		return true;
	}
	/**
	 * Пакет несёт короткий заголовок: длины идентификатора соединения получателя
	 * в нём нет, и известна она только выдавшему идентификатор эндпоинту
	 */
	if((length == 0) || (length > proto::MAX_CID_SIZE) || (size < (1u + static_cast <size_t> (length))))
		// Выводим отрицательный результат
		return false;
	// Формируем ключ маршрутизации из идентификатора соединения получателя
	key = net::origin_key_t(data + 1, length);
	// Выводим положительный результат
	return true;
}

/**
 * @brief Функция получения человекочитаемого названия типа пакета
 *
 * @param type тип пакета
 * @return     название типа пакета
 *
 */
string_view awh::quic2::packetName(const packet_t type) noexcept {
	/**
	 * В зависимости от типа пакета, выводим соответствующее название
	 */
	switch(static_cast <uint8_t> (type)){
		// Установка соединения
		case static_cast <uint8_t> (packet_t::INITIAL):
			// Выводим название типа пакета
			return "INITIAL";
		// Ранние данные клиента
		case static_cast <uint8_t> (packet_t::ZERO_RTT):
			// Выводим название типа пакета
			return "0-RTT";
		// Продолжение криптографического хендшейка
		case static_cast <uint8_t> (packet_t::HANDSHAKE):
			// Выводим название типа пакета
			return "HANDSHAKE";
		// Проверка адреса клиента сервером
		case static_cast <uint8_t> (packet_t::RETRY):
			// Выводим название типа пакета
			return "RETRY";
		// Согласование версии протокола
		case static_cast <uint8_t> (packet_t::VERSION_NEGOTIATION):
			// Выводим название типа пакета
			return "VERSION_NEGOTIATION";
		// Пакет с коротким заголовком
		case static_cast <uint8_t> (packet_t::ONE_RTT):
			// Выводим название типа пакета
			return "1-RTT";
	}
	// Тип пакета неизвестен
	return "UNKNOWN";
}
/**
 * @brief Функция получения человекочитаемого названия типа фрейма
 *
 * @param type тип фрейма
 * @return     название типа фрейма
 *
 */
string_view awh::quic2::frameName(const frame_t type) noexcept {
	// Значение типа фрейма
	const uint64_t value = static_cast <uint64_t> (type);
	// Если тип фрейма принадлежит диапазону STREAM (0x08-0x0F)
	if((value >= static_cast <uint64_t> (frame_t::STREAM)) && (value <= (static_cast <uint64_t> (frame_t::STREAM) | 0x07)))
		// Выводим название типа фрейма
		return "STREAM";
	// Если тип фрейма принадлежит диапазону DATAGRAM (0x30-0x31)
	if((value >= static_cast <uint64_t> (frame_t::DATAGRAM)) && (value <= (static_cast <uint64_t> (frame_t::DATAGRAM) | 0x01)))
		// Выводим название типа фрейма
		return "DATAGRAM";
	/**
	 * В зависимости от типа фрейма, выводим соответствующее название
	 */
	switch(value){
		// Заполнение без полезной нагрузки
		case static_cast <uint64_t> (frame_t::PADDING):
			// Выводим название типа фрейма
			return "PADDING";
		// Проверка живости пути
		case static_cast <uint64_t> (frame_t::PING):
			// Выводим название типа фрейма
			return "PING";
		// Подтверждение приёма пакетов
		case static_cast <uint64_t> (frame_t::ACK):
			// Выводим название типа фрейма
			return "ACK";
		// Подтверждение приёма пакетов со счётчиками ECN
		case static_cast <uint64_t> (frame_t::ACK_ECN):
			// Выводим название типа фрейма
			return "ACK_ECN";
		// Аварийное завершение потока отправителем
		case static_cast <uint64_t> (frame_t::RESET_STREAM):
			// Выводим название типа фрейма
			return "RESET_STREAM";
		// Запрос прекращения передачи потока
		case static_cast <uint64_t> (frame_t::STOP_SENDING):
			// Выводим название типа фрейма
			return "STOP_SENDING";
		// Данные криптографического хендшейка
		case static_cast <uint64_t> (frame_t::CRYPTO):
			// Выводим название типа фрейма
			return "CRYPTO";
		// Токен для будущих соединений клиента
		case static_cast <uint64_t> (frame_t::NEW_TOKEN):
			// Выводим название типа фрейма
			return "NEW_TOKEN";
		// Обновление лимита данных соединения
		case static_cast <uint64_t> (frame_t::MAX_DATA):
			// Выводим название типа фрейма
			return "MAX_DATA";
		// Обновление лимита данных потока
		case static_cast <uint64_t> (frame_t::MAX_STREAM_DATA):
			// Выводим название типа фрейма
			return "MAX_STREAM_DATA";
		// Обновление лимита двунаправленных потоков
		case static_cast <uint64_t> (frame_t::MAX_STREAMS_BIDI):
			// Выводим название типа фрейма
			return "MAX_STREAMS_BIDI";
		// Обновление лимита однонаправленных потоков
		case static_cast <uint64_t> (frame_t::MAX_STREAMS_UNI):
			// Выводим название типа фрейма
			return "MAX_STREAMS_UNI";
		// Блокировка лимитом данных соединения
		case static_cast <uint64_t> (frame_t::DATA_BLOCKED):
			// Выводим название типа фрейма
			return "DATA_BLOCKED";
		// Блокировка лимитом данных потока
		case static_cast <uint64_t> (frame_t::STREAM_DATA_BLOCKED):
			// Выводим название типа фрейма
			return "STREAM_DATA_BLOCKED";
		// Блокировка лимитом двунаправленных потоков
		case static_cast <uint64_t> (frame_t::STREAMS_BLOCKED_BIDI):
			// Выводим название типа фрейма
			return "STREAMS_BLOCKED_BIDI";
		// Блокировка лимитом однонаправленных потоков
		case static_cast <uint64_t> (frame_t::STREAMS_BLOCKED_UNI):
			// Выводим название типа фрейма
			return "STREAMS_BLOCKED_UNI";
		// Анонс нового идентификатора соединения
		case static_cast <uint64_t> (frame_t::NEW_CONNECTION_ID):
			// Выводим название типа фрейма
			return "NEW_CONNECTION_ID";
		// Вывод идентификатора соединения из обращения
		case static_cast <uint64_t> (frame_t::RETIRE_CONNECTION_ID):
			// Выводим название типа фрейма
			return "RETIRE_CONNECTION_ID";
		// Проверка достижимости пути
		case static_cast <uint64_t> (frame_t::PATH_CHALLENGE):
			// Выводим название типа фрейма
			return "PATH_CHALLENGE";
		// Ответ на проверку достижимости пути
		case static_cast <uint64_t> (frame_t::PATH_RESPONSE):
			// Выводим название типа фрейма
			return "PATH_RESPONSE";
		// Завершение соединения с ошибкой транспорта
		case static_cast <uint64_t> (frame_t::CONNECTION_CLOSE):
			// Выводим название типа фрейма
			return "CONNECTION_CLOSE";
		// Завершение соединения с ошибкой приложения
		case static_cast <uint64_t> (frame_t::CONNECTION_CLOSE_APP):
			// Выводим название типа фрейма
			return "CONNECTION_CLOSE_APP";
		// Подтверждение завершения хендшейка сервером
		case static_cast <uint64_t> (frame_t::HANDSHAKE_DONE):
			// Выводим название типа фрейма
			return "HANDSHAKE_DONE";
	}
	// Тип фрейма неизвестен
	return "UNKNOWN";
}
/**
 * @brief Функция получения человекочитаемого названия кода ошибки транспорта
 *
 * @param code код ошибки транспорта
 * @return     название кода ошибки
 *
 */
string_view awh::quic2::errorName(const error_t code) noexcept {
	// Значение кода ошибки транспорта
	const uint64_t value = static_cast <uint64_t> (code);
	// Если код ошибки принадлежит диапазону ошибок TLS-хендшейка (0x0100-0x01FF)
	if((value >= static_cast <uint64_t> (error_t::CRYPTO_ERROR)) && (value <= (static_cast <uint64_t> (error_t::CRYPTO_ERROR) | 0xFF)))
		// Выводим название кода ошибки
		return "CRYPTO_ERROR";
	/**
	 * В зависимости от кода ошибки транспорта, выводим соответствующее название
	 */
	switch(value){
		// Штатное завершение
		case static_cast <uint64_t> (error_t::NO_ERROR):
			// Выводим название кода ошибки
			return "NO_ERROR";
		// Внутренняя ошибка реализации
		case static_cast <uint64_t> (error_t::INTERNAL_ERROR):
			// Выводим название кода ошибки
			return "INTERNAL_ERROR";
		// Сервер отказал в приёме соединения
		case static_cast <uint64_t> (error_t::CONNECTION_REFUSED):
			// Выводим название кода ошибки
			return "CONNECTION_REFUSED";
		// Нарушение flow control
		case static_cast <uint64_t> (error_t::FLOW_CONTROL_ERROR):
			// Выводим название кода ошибки
			return "FLOW_CONTROL_ERROR";
		// Превышен лимит числа потоков
		case static_cast <uint64_t> (error_t::STREAM_LIMIT_ERROR):
			// Выводим название кода ошибки
			return "STREAM_LIMIT_ERROR";
		// Фрейм недопустим в текущем состоянии потока
		case static_cast <uint64_t> (error_t::STREAM_STATE_ERROR):
			// Выводим название кода ошибки
			return "STREAM_STATE_ERROR";
		// Нарушение финального размера потока
		case static_cast <uint64_t> (error_t::FINAL_SIZE_ERROR):
			// Выводим название кода ошибки
			return "FINAL_SIZE_ERROR";
		// Некорректное кодирование фрейма
		case static_cast <uint64_t> (error_t::FRAME_ENCODING_ERROR):
			// Выводим название кода ошибки
			return "FRAME_ENCODING_ERROR";
		// Некорректные параметры транспорта
		case static_cast <uint64_t> (error_t::TRANSPORT_PARAMETER_ERROR):
			// Выводим название кода ошибки
			return "TRANSPORT_PARAMETER_ERROR";
		// Превышен лимит идентификаторов соединения
		case static_cast <uint64_t> (error_t::CONNECTION_ID_LIMIT_ERROR):
			// Выводим название кода ошибки
			return "CONNECTION_ID_LIMIT_ERROR";
		// Общее нарушение протокола
		case static_cast <uint64_t> (error_t::PROTOCOL_VIOLATION):
			// Выводим название кода ошибки
			return "PROTOCOL_VIOLATION";
		// Некорректный токен в пакете Initial
		case static_cast <uint64_t> (error_t::INVALID_TOKEN):
			// Выводим название кода ошибки
			return "INVALID_TOKEN";
		// Ошибка приложения
		case static_cast <uint64_t> (error_t::APPLICATION_ERROR):
			// Выводим название кода ошибки
			return "APPLICATION_ERROR";
		// Переполнен буфер CRYPTO-данных
		case static_cast <uint64_t> (error_t::CRYPTO_BUFFER_EXCEEDED):
			// Выводим название кода ошибки
			return "CRYPTO_BUFFER_EXCEEDED";
		// Ошибка обновления ключей
		case static_cast <uint64_t> (error_t::KEY_UPDATE_ERROR):
			// Выводим название кода ошибки
			return "KEY_UPDATE_ERROR";
		// Достигнут лимит использования AEAD-ключей
		case static_cast <uint64_t> (error_t::AEAD_LIMIT_REACHED):
			// Выводим название кода ошибки
			return "AEAD_LIMIT_REACHED";
		// Нет пригодного сетевого пути
		case static_cast <uint64_t> (error_t::NO_VIABLE_PATH):
			// Выводим название кода ошибки
			return "NO_VIABLE_PATH";
		// Согласование версии не дало общей версии
		case static_cast <uint64_t> (error_t::VERSION_NEGOTIATION_ERROR):
			// Выводим название кода ошибки
			return "VERSION_NEGOTIATION_ERROR";
	}
	// Код ошибки неизвестен
	return "UNKNOWN_ERROR";
}

/**
 * @brief Функция вывода токена сброса без сохранения состояния (RFC 9000 §10.3.2)
 *
 * @param key   общий ключ вывода токенов сброса
 * @param cid   идентификатор соединения локального эндпоинта
 * @param token выведенный токен сброса без сохранения состояния
 * @return      результат вывода (false - пустой ключ либо ошибка кода аутентичности)
 *
 */
bool awh::quic2::resetToken(string_view key, const cid_t & cid, uint8_t token[proto::RESET_TOKEN_SIZE]) noexcept {
	// Если общий ключ вывода токенов не задан
	if(key.empty())
		// Выводим отрицательный результат - вывод токена невозможен
		return false;
	// Буфер кода аутентичности
	uint8_t mac[EVP_MAX_MD_SIZE];
	// Длина кода аутентичности
	uint32_t length = 0;
	/**
	 * Выводим код аутентичности идентификатора соединения на общем ключе: токен
	 * обязан воспроизводиться по одному лишь идентификатору, поэтому состояние
	 * соединения в вывод не входит (RFC 9000 §10.3.2)
	 */
	if(::HMAC(::EVP_sha256(), key.data(), key.size(), cid.data, cid.size, mac, &length) == nullptr)
		// Выводим отрицательный результат
		return false;
	// Если длина кода аутентичности недостаточна
	if(length < proto::RESET_TOKEN_SIZE)
		// Выводим отрицательный результат
		return false;
	// Копируем усечённый код аутентичности в токен сброса
	::memcpy(token, mac, proto::RESET_TOKEN_SIZE);
	// Выводим положительный результат
	return true;
}
/**
 * @brief Функция сборки пакета сброса без сохранения состояния (RFC 9000 §10.3)
 *
 * @param output  выходной буфер пакета сброса
 * @param key     общий ключ вывода токенов сброса
 * @param cid     идентификатор соединения получателя из вызвавшей датаграммы
 * @param trigger размер вызвавшей сброс датаграммы
 * @return        результат сборки (false - датаграмма слишком мала либо ошибка вывода токена)
 *
 */
bool awh::quic2::reset(string & output, string_view key, const cid_t & cid, const size_t trigger) noexcept {
	/**
	 * Минимальный размер пакета сброса: октет заголовка, непредсказуемые октеты
	 * и токен сброса. Более короткий пакет удалённый узел отличит от пакета 1-RTT
	 * по одному только размеру (RFC 9000 §10.3)
	 */
	static constexpr size_t MINIMUM = (1 + 4 + proto::RESET_TOKEN_SIZE);
	/**
	 * Предельный размер пакета сброса: наполнять его сверх меры незачем -
	 * от пакета 1-RTT он неотличим и в минимальном размере
	 */
	static constexpr size_t MAXIMUM = 64;
	/**
	 * Если вызвавшая датаграмма не превышает минимального размера: пакет сброса
	 * обязан быть меньше её, а короче минимума он быть не может (RFC 9000 §10.3.3)
	 */
	if(trigger <= MINIMUM)
		// Выводим отрицательный результат - отправка сброса невозможна
		return false;
	// Выведенный токен сброса без сохранения состояния
	uint8_t token[proto::RESET_TOKEN_SIZE];
	// Если вывод токена сброса не выполнен
	if(!resetToken(key, cid, token))
		// Выводим отрицательный результат
		return false;
	// Вычисляем размер пакета сброса на октет меньше вызвавшей его датаграммы
	const size_t size = ::min(trigger - 1, MAXIMUM);
	// Очищаем выходной буфер пакета сброса
	output.clear();
	// Резервируем память под пакет сброса
	output.resize(size);
	// Если генерация непредсказуемых октетов пакета не выполнена
	if(::RAND_bytes(reinterpret_cast <uint8_t *> (&output[0]), static_cast <int32_t> (size - proto::RESET_TOKEN_SIZE)) != 1){
		// Очищаем выходной буфер пакета сброса
		output.clear();
		// Выводим отрицательный результат
		return false;
	}
	/**
	 * Приводим первый октет к виду пакета с коротким заголовком: старший бит формы
	 * заголовка сбрасывается, следующий за ним фиксированный бит устанавливается,
	 * остальные остаются случайными (RFC 9000 §10.3)
	 */
	output[0] = static_cast <char> ((static_cast <uint8_t> (output[0]) & 0x3F) | 0x40);
	// Дописываем токен сброса в хвост пакета
	::memcpy(&output[size - proto::RESET_TOKEN_SIZE], token, proto::RESET_TOKEN_SIZE);
	// Выводим положительный результат
	return true;
}

/**
 * @brief Функция генерации общего ключа вывода токенов сброса (RFC 9000 §10.3.2)
 *
 * @param output сгенерированный общий ключ вывода токенов сброса
 * @return       результат генерации (false - ошибка генератора случайных чисел)
 *
 */
bool awh::quic2::resetKey(string & output) noexcept {
	// Размер общего ключа вывода токенов сброса
	static constexpr size_t SIZE = 32;
	// Резервируем память под общий ключ вывода токенов сброса
	output.resize(SIZE);
	// Если генерация общего ключа не выполнена
	if(::RAND_bytes(reinterpret_cast <uint8_t *> (&output[0]), static_cast <int32_t> (SIZE)) != 1){
		// Очищаем общий ключ вывода токенов сброса
		output.clear();
		// Выводим отрицательный результат
		return false;
	}
	// Выводим положительный результат
	return true;
}
